// Windows Firewall rules for the hub and the DayZ servers on the hub's machine, made from the Setup page.
// netsh advfirewall needs administrator rights: run as the Windows service it just works; run in a normal console
// the hub asks Windows for them (one UAC prompt on this PC). DayZ servers on OTHER machines get the PowerShell
// lines to run there. Rules are named "DriveLink ..." so they can be found, replaced and removed again.
use crate::config::Config;
use serde::Serialize;
use std::process::Command;

#[derive(Serialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Rule {
    pub name: String,
    pub proto: &'static str, // TCP | UDP
    pub ports: String,       // "7290,7291" / "2302-2305,27016"
    pub why: String,
    pub present: bool,
}

fn port_of(listen: &str) -> Option<u16> {
    listen.rsplit(':').next()?.parse().ok()
}

/// DayZ: the game port and the next three (UDP), plus the Steam query port (server browser).
pub fn dayz_ports(port: u16, query_port: u16) -> String {
    let mut p = format!("{}-{}", port, port.saturating_add(3));
    if query_port != 0 {
        p.push_str(&format!(",{query_port}"));
    }
    p
}

/// The rules this machine needs: the hub's own ports + every DayZ server that runs here.
pub fn wanted(cfg: &Config, local: &dyn Fn(&str) -> bool) -> Vec<Rule> {
    let mut out = Vec::new();
    let hub: Vec<String> = [&cfg.listen, &cfg.listen_tls].iter().filter_map(|l| port_of(l)).map(|p| p.to_string()).collect();
    if !hub.is_empty() {
        out.push(Rule { name: "DriveLink Hub".into(), proto: "TCP", ports: hub.join(","), why: "DayZ servers on other machines reach the hub".into(), present: false });
    }
    for s in &cfg.servers {
        if s.port != 0 && local(&s.id) {
            out.push(Rule {
                name: format!("DriveLink DayZ {}", s.id),
                proto: "UDP",
                ports: dayz_ports(s.port, s.query_port),
                why: format!("players join {} (game port{})", s.name, if s.query_port != 0 { " + Steam query port" } else { "s; query port not reported yet" }),
                present: false,
            });
        }
    }
    for r in &mut out {
        r.present = present(r);
    }
    out
}

fn netsh(args: &[&str]) -> std::io::Result<std::process::Output> {
    let mut c = Command::new("netsh");
    c.args(args);
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        c.creation_flags(0x0800_0000); // CREATE_NO_WINDOW
    }
    c.output()
}

/// The rule exists with these ports (netsh lists "LocalPort: ..." per rule).
fn present(r: &Rule) -> bool {
    let name = format!("name={}", r.name);
    match netsh(&["advfirewall", "firewall", "show", "rule", &name]) {
        Ok(o) if o.status.success() => {
            let text = String::from_utf8_lossy(&o.stdout).to_lowercase();
            text.contains(&r.ports.to_lowercase()) && text.contains(&r.proto.to_lowercase())
        }
        _ => false,
    }
}

fn lines(rules: &[Rule]) -> Vec<String> {
    let mut v = Vec::new();
    for r in rules {
        v.push(format!("netsh advfirewall firewall delete rule name=\"{}\"", r.name));
        v.push(format!("netsh advfirewall firewall add rule name=\"{}\" dir=in action=allow protocol={} localport={}", r.name, r.proto, r.ports));
    }
    v
}

/// Add (or replace) the missing rules. Ok(message) / Err(what went wrong).
pub fn apply(rules: &[Rule], dir: &std::path::Path) -> Result<String, String> {
    let todo: Vec<Rule> = rules.iter().filter(|r| !r.present).cloned().collect();
    if todo.is_empty() {
        return Ok("all rules are there already".into());
    }
    // first try directly (works when the hub runs as the service or in an administrator console)
    let mut need_admin = false;
    for r in &todo {
        let _ = netsh(&["advfirewall", "firewall", "delete", "rule", &format!("name={}", r.name)]);
        let o = netsh(&["advfirewall", "firewall", "add", "rule", &format!("name={}", r.name), "dir=in", "action=allow", &format!("protocol={}", r.proto), &format!("localport={}", r.ports)])
            .map_err(|e| format!("netsh: {e}"))?;
        if !o.status.success() {
            need_admin = true;
            break;
        }
    }
    if !need_admin {
        return Ok(format!("{} rule(s) added", todo.len()));
    }
    // ask Windows for administrator rights: one UAC prompt on this PC
    let script = dir.join("drivelink-firewall.cmd");
    std::fs::write(&script, format!("@echo off\r\n{}\r\n", lines(&todo).join("\r\n"))).map_err(|e| format!("write {}: {e}", script.display()))?;
    let ps = format!("Start-Process -FilePath cmd.exe -ArgumentList '/c','\"{}\"' -Verb RunAs -WindowStyle Hidden -Wait", script.display());
    let mut c = Command::new("powershell");
    c.args(["-NoProfile", "-NonInteractive", "-Command", &ps]);
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        c.creation_flags(0x0800_0000);
    }
    let o = c.output().map_err(|e| format!("powershell: {e}"))?;
    let _ = std::fs::remove_file(&script);
    if !o.status.success() {
        return Err("Windows did not give administrator rights (the prompt was declined, or no one is logged in at this PC). Run the commands below in an administrator terminal instead.".into());
    }
    Ok(format!("{} rule(s) added with administrator rights", todo.len()))
}

/// PowerShell for a DayZ server on another machine (run there as administrator).
pub fn remote_powershell(id: &str, port: u16, query_port: u16) -> String {
    let mut ports = vec![format!("{}-{}", port, port.saturating_add(3))];
    if query_port != 0 {
        ports.push(query_port.to_string());
    }
    let list = ports.iter().map(|p| format!("'{p}'")).collect::<Vec<_>>().join(",");
    format!("New-NetFirewallRule -DisplayName 'DriveLink DayZ {id}' -Direction Inbound -Action Allow -Protocol UDP -LocalPort {list}")
}

pub fn hub_commands(rules: &[Rule]) -> Vec<String> {
    lines(rules)
}
