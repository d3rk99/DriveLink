//! Automatic helper policy from DayZ's advertised client mods (A2S_RULES / browser v2).
//! Protocol reference: https://woozymasta.github.io/a2s/pkg/a3sb/docs/
//! Compare mod identities and advertised hashes, never map/DLC masks or server key lists.
use crate::api::Shared;
use crate::config::ServerEntry;
use serde::Serialize;
use serde_json::{Value, json};
use std::collections::{BTreeMap, HashMap, HashSet};
use std::time::{Duration, Instant};
use tokio::net::UdpSocket;

#[derive(Clone, Debug, Serialize, PartialEq)]
pub struct ClientMod { pub id: u64, pub hash: u32, pub name: String }
impl ClientMod {
    fn identity(&self) -> String { if self.id != 0 { format!("steam:{}", self.id) } else { format!("local:{}", self.name.trim().to_lowercase()) } }
}
#[derive(Clone)]
pub struct Observation { mods: Result<Vec<ClientMod>, String>, checked: Instant, config_key: String }
#[derive(Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Policy { pub target_id: String, pub known: bool, pub restart: bool, pub reason: String, pub mode: String }

struct Reader<'a> { data: &'a [u8], at: usize }
impl<'a> Reader<'a> {
    fn take(&mut self, n: usize) -> Result<&'a [u8], String> { let end = self.at.checked_add(n).ok_or("oversized data")?; let b = self.data.get(self.at..end).ok_or("truncated DayZ rules")?; self.at = end; Ok(b) }
    fn byte(&mut self) -> Result<u8, String> { Ok(self.take(1)?[0]) }
    fn u16(&mut self) -> Result<u16, String> { Ok(u16::from_le_bytes(self.take(2)?.try_into().unwrap())) }
    fn u32(&mut self) -> Result<u32, String> { Ok(u32::from_le_bytes(self.take(4)?.try_into().unwrap())) }
    fn text(&mut self) -> Result<String, String> { let n = self.byte()? as usize; String::from_utf8(self.take(n)?.to_vec()).map_err(|_| "invalid mod text".into()) }
    fn cstr(&mut self) -> Result<&'a [u8], String> { let n = self.data.get(self.at..).ok_or("truncated rules")?.iter().position(|x| *x == 0).ok_or("unterminated rule")?; let b = self.take(n)?; self.take(1)?; Ok(b) }
}

pub fn parse_rules(packet: &[u8]) -> Result<Vec<ClientMod>, String> {
    if !packet.starts_with(b"\xff\xff\xff\xffE") { return Err("not an A2S_RULES response".into()); }
    let mut r = Reader { data: packet, at: 5 };
    let count = r.u16()?;
    let mut pages = BTreeMap::new(); let mut total = 0;
    for _ in 0..count {
        let k = r.cstr()?; let v = r.cstr()?;
        if k.len() == 2 {
            if k[0] == 0 || k[1] == 0 || k[0] > k[1] || (total != 0 && total != k[1]) || pages.insert(k[0], v).is_some() { return Err("invalid/duplicate DayZ rule page".into()); }
            total = k[1];
        }
    }
    if r.at != packet.len() || total == 0 || pages.len() != total as usize || !pages.contains_key(&1) { return Err("incomplete DayZ rule pages".into()); }
    let encoded: Vec<u8> = pages.values().flat_map(|p| p.iter().copied()).collect();
    let mut data = Vec::new(); let mut i = 0;
    while i < encoded.len() {
        let mut b = encoded[i]; i += 1;
        if b == 1 { b = match encoded.get(i) { Some(1) => 1, Some(2) => 0, Some(3) => 255, _ => return Err("invalid DayZ escape".into()) }; i += 1; }
        data.push(b);
    }
    let mut r = Reader { data: &data, at: 0 };
    if r.byte()? != 2 { return Err("unsupported DayZ browser protocol".into()); }
    r.byte()?; // flags
    let dlc = r.u16()?; r.take(dlc.count_ones() as usize * 4)?; // owned map/DLC differences do not require mod reloads
    let n = r.byte()?; let mut mods = Vec::new(); let mut identities = HashSet::new();
    for _ in 0..n {
        let hash = r.u32()?;
        let len = r.byte()?;
        let id = match len { 1 => r.byte()? as u64, 4 => r.u32()? as u64, 8 => u64::from_le_bytes(r.take(8)?.try_into().unwrap()), _ => return Err("unsupported mod identity encoding".into()) };
        let name = r.text()?;
        let item = ClientMod { id, hash, name };
        if hash == 0 || (id == 0 && item.name.trim().is_empty()) || !identities.insert(item.identity()) { return Err("ambiguous or incomplete client mod report".into()); }
        mods.push(item);
    }
    // Validate the rest rather than trusting a truncated prefix. These are signing keys, not loaded mods.
    let signatures = r.byte()?; for _ in 0..signatures { r.text()?; }
    if r.at < data.len() { r.text()?; }
    if r.at != data.len() || mods.is_empty() { return Err("incomplete/empty client mod report".into()); }
    Ok(mods)
}

async fn response(socket: &UdpSocket) -> Result<Vec<u8>, String> {
    let mut buf = vec![0; 65536]; let n = socket.recv(&mut buf).await.map_err(|e| e.to_string())?; buf.truncate(n);
    if buf.starts_with(b"\xff\xff\xff\xff") { return Ok(buf); }
    let mut parts = BTreeMap::new(); let mut message = None; let mut total = 0;
    for _ in 0..64 {
        if buf.len() < 12 || !buf.starts_with(b"\xfe\xff\xff\xff") { return Err("invalid split A2S packet".into()); }
        let id = u32::from_le_bytes(buf[4..8].try_into().unwrap());
        if id & 0x80000000 != 0 { return Err("compressed A2S rules are unsupported".into()); }
        if message.is_some_and(|v| v != id) || buf[8] == 0 || buf[8] > 64 || buf[9] >= buf[8] || (total != 0 && total != buf[8]) { return Err("inconsistent split A2S packet".into()); }
        message = Some(id); total = buf[8];
        if let Some(old) = parts.insert(buf[9], buf[12..].to_vec()) { if old != buf[12..] { return Err("conflicting split A2S packet".into()); } }
        if parts.len() == total as usize { return Ok(parts.values().flatten().copied().collect()); }
        buf.resize(65536, 0); let n = socket.recv(&mut buf).await.map_err(|e| e.to_string())?; buf.truncate(n);
    }
    Err("too many split A2S packets".into())
}
async fn query(host: &str, port: u16) -> Result<Vec<ClientMod>, String> {
    let addr = tokio::net::lookup_host((host, port)).await.map_err(|e| e.to_string())?.find(|a| a.is_ipv4()).ok_or("no IPv4 query address")?;
    let socket = UdpSocket::bind("0.0.0.0:0").await.map_err(|e| e.to_string())?;
    socket.connect(addr).await.map_err(|e| e.to_string())?;
    let mut request = b"\xff\xff\xff\xffV\xff\xff\xff\xff".to_vec();
    socket.send(&request).await.map_err(|e| e.to_string())?;
    let mut data = response(&socket).await?;
    if data.starts_with(b"\xff\xff\xff\xffA") && data.len() == 9 {
        request[5..9].copy_from_slice(&data[5..9]); socket.send(&request).await.map_err(|e| e.to_string())?; data = response(&socket).await?;
    }
    parse_rules(&data)
}
fn key(s: &ServerEntry) -> String { format!("{}|{}|{}", s.lan_host, s.public_host, s.query_port) }
fn current<'a>(all: &'a HashMap<String, Observation>, s: &ServerEntry) -> Result<&'a [ClientMod], String> {
    let item = all.get(&s.id).ok_or("Waiting for the server's client-mod query")?;
    if item.config_key != key(s) || item.checked.elapsed() > Duration::from_secs(120) { return Err("Client-mod report is stale; waiting for a fresh query".into()); }
    item.mods.as_deref().map_err(|e| format!("Client-mod query unavailable: {e}. Check the server's UDP Steam query port"))
}
fn compare(a: &[ClientMod], b: &[ClientMod]) -> Result<bool, String> {
    for item in a {
        if let Some(other) = b.iter().find(|x| x.identity() == item.identity()) { if other.hash != item.hash { return Err(format!("Shared mod {} advertises different builds; synchronize server mod versions", item.name)); } }
    }
    Ok(a.len() != b.len() || a.iter().zip(b).any(|(x,y)| x.identity() != y.identity() || x.hash != y.hash))
}
pub fn routes(h: &crate::api::Hub, source: &str) -> Vec<Policy> {
    let cfg = h.cfg(); let Some(from) = cfg.server(source) else { return vec![]; };
    let observations = h.client_mods.lock().unwrap();
    cfg.servers.iter().filter(|s| s.id != source).map(|to| {
        let mode = from.helper_routes.get(&to.id).map(String::as_str).unwrap_or("auto");
        let decision = match mode {
            "direct" => Ok(false), "restart" => Ok(true),
            _ => current(&observations, from).and_then(|a| current(&observations, to).and_then(|b| compare(a,b))),
        };
        let (known, restart, reason) = match decision {
            Ok(restart) => (true, restart, if mode != "auto" { "Administrator override".into() } else if restart { "Advertised client mod lists/order differ".into() } else { "Advertised client mod identities and builds match".into() }),
            Err(reason) => (false, true, reason),
        };
        Policy { target_id: to.id.clone(), known, restart, reason, mode: mode.into() }
    }).collect()
}
pub fn report(h: &crate::api::Hub, s: &ServerEntry) -> Value {
    let all = h.client_mods.lock().unwrap();
    match current(&all, s) { Ok(mods) => json!({"known":true,"mods":mods}), Err(reason) => json!({"known":false,"reason":reason}) }
}
pub async fn run(h: Shared) {
    let permits = std::sync::Arc::new(tokio::sync::Semaphore::new(16));
    let mut interval = tokio::time::interval(Duration::from_secs(30)); interval.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);
    loop {
        tokio::select! { _ = interval.tick() => {}, _ = h.mod_query_notify.notified() => {} }
        let servers = h.cfg().servers; let mut jobs = tokio::task::JoinSet::new();
        for s in servers {
            let hub = h.clone(); let permits = permits.clone();
            jobs.spawn(async move {
                let _permit = permits.acquire().await.unwrap();
                let mut hosts = Vec::new();
                if hub.is_local_server(&s.id) { hosts.push("127.0.0.1".to_string()); hosts.push(s.lan_host.clone()); }
                else { hosts.push(s.public_host.clone()); if let Some(ip) = hub.seen.lock().unwrap().get(&s.id).cloned() { hosts.push(ip); } hosts.push(s.lan_host.clone()); }
                hosts.retain(|a| !a.is_empty()); hosts.dedup();
                let mut result = Err("Steam query address/port is not known yet".to_string());
                if s.query_port != 0 { for host in hosts { result = tokio::time::timeout(Duration::from_secs(3), query(&host, s.query_port)).await.unwrap_or_else(|_| Err("query timed out".into())); if result.is_ok() { break; } } }
                hub.client_mods.lock().unwrap().insert(s.id.clone(), Observation { mods: result, checked: Instant::now(), config_key: key(&s) });
            });
        }
        while jobs.join_next().await.is_some() {}
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture(name: &str) -> Vec<u8> { std::fs::read(format!("tests/fixtures/{name}-rules.bin")).unwrap() }
    #[test] fn actual_three_server_reports() {
        let c = parse_rules(&fixture("chernarus")).unwrap(); let l = parse_rules(&fixture("livonia")).unwrap(); let n = parse_rules(&fixture("namalsk")).unwrap();
        assert_eq!(c.len(),3); assert_eq!(n.len(),5); assert!(!compare(&c,&l).unwrap()); assert!(compare(&c,&n).unwrap()); assert!(compare(&n,&l).unwrap());
        assert_eq!(c.iter().map(|m|m.id).collect::<Vec<_>>(),vec![3824238331,1828439124,1559212036]);
    }
    #[test] fn truncated_and_malformed_never_look_equal() {
        let data=fixture("chernarus"); for n in 0..data.len() { assert!(parse_rules(&data[..n]).is_err(),"accepted truncated length {n}"); }
        let mut bad=data.clone();bad[7]=0;assert!(parse_rules(&bad).is_err());
    }
    #[test] fn shared_mod_build_mismatch_blocks() { let a=parse_rules(&fixture("chernarus")).unwrap();let mut b=a.clone();b[0].hash^=1;assert!(compare(&a,&b).is_err()); }
    #[test] fn order_changes_require_restart() { let a=parse_rules(&fixture("chernarus")).unwrap();let mut b=a.clone();b.swap(0,1);assert!(compare(&a,&b).unwrap()); }
    #[test] fn workshop_display_names_are_not_identity() { let a=parse_rules(&fixture("chernarus")).unwrap();let mut b=a.clone();b[0].name="Another display name".into();assert!(!compare(&a,&b).unwrap()); }
    #[test] fn local_mods_need_name_and_build() { let a=vec![ClientMod{id:0,hash:9,name:"Private Pack".into()}];let mut b=a.clone();assert!(!compare(&a,&b).unwrap());b[0].hash=10;assert!(compare(&a,&b).is_err()); }
    fn single_mod_packet(dlc:u16)->Vec<u8> {
        let mut body=vec![2,0];body.extend(dlc.to_le_bytes());for _ in 0..dlc.count_ones(){body.extend(15u32.to_le_bytes());}
        body.push(1);body.extend(123u32.to_le_bytes());body.push(4);body.extend(3824238331u32.to_le_bytes());body.push(9);body.extend(b"DriveLink");body.extend([0,0]);
        let encoded:Vec<u8>=body.into_iter().flat_map(|b|match b{0=>vec![1,2],1=>vec![1,1],255=>vec![1,3],_=>vec![b]}).collect();
        let pages:Vec<_>=encoded.chunks(7).collect();let mut packet=b"\xff\xff\xff\xffE".to_vec();packet.extend((pages.len() as u16).to_le_bytes());
        for (i,p) in pages.iter().enumerate(){packet.extend([(i+1)as u8,pages.len()as u8,0]);packet.extend(*p);packet.push(0);}packet
    }
    #[test] fn official_dlc_differences_and_page_boundary_escapes_do_not_restart() {let a=parse_rules(&single_mod_packet(1)).unwrap();let b=parse_rules(&single_mod_packet(6)).unwrap();assert!(!compare(&a,&b).unwrap());}
    #[tokio::test] async fn challenged_query_reassembles_out_of_order_split_packets() {
        let server=UdpSocket::bind("127.0.0.1:0").await.unwrap();let port=server.local_addr().unwrap().port();
        let task=tokio::spawn(async move{
            let mut input=[0;64];let(_,peer)=server.recv_from(&mut input).await.unwrap();server.send_to(b"\xff\xff\xff\xffAabcd",peer).await.unwrap();
            let(n,peer)=server.recv_from(&mut input).await.unwrap();assert_eq!(&input[..n],b"\xff\xff\xff\xffVabcd");
            let data=fixture("chernarus");let chunks=[&data[..100],&data[100..]];
            for i in [1,0]{let mut packet=b"\xfe\xff\xff\xff".to_vec();packet.extend(45u32.to_le_bytes());packet.extend([2,i as u8]);packet.extend(1400u16.to_le_bytes());packet.extend(chunks[i]);server.send_to(&packet,peer).await.unwrap();}
        });
        let mods=tokio::time::timeout(Duration::from_secs(2),query("127.0.0.1",port)).await.unwrap().unwrap();assert_eq!(mods.len(),3);task.await.unwrap();
    }
}
