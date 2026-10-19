// HTTPS for the hub (test phase: optional, next to plain HTTP).
// hub.json tlsCert/tlsKey = PEM files of a real certificate (e.g. Let's Encrypt). Empty = the hub makes a
// self-signed one (hub-cert.pem / hub-key.pem next to hub.json) for localhost, 127.0.0.1 and every server's
// lanHost/publicHost. Whether DayZ's RestApi accepts a self-signed certificate is what the in-game test finds out.
use crate::config::Config;
use std::path::Path;

pub fn cert_files(cfg: &Config, dir: &Path) -> Result<(std::path::PathBuf, std::path::PathBuf), String> {
    if !cfg.tls_cert.is_empty() && !cfg.tls_key.is_empty() {
        let resolve = |p: &str| {
            let p = Path::new(p);
            if p.is_absolute() { p.to_path_buf() } else { dir.join(p) }
        };
        return Ok((resolve(&cfg.tls_cert), resolve(&cfg.tls_key)));
    }
    let cert = dir.join("hub-cert.pem");
    let key = dir.join("hub-key.pem");
    if cert.exists() && key.exists() {
        return Ok((cert, key));
    }
    let mut names: Vec<String> = vec!["localhost".into(), "127.0.0.1".into()];
    for s in &cfg.servers {
        for h in [&s.lan_host, &s.public_host] {
            if !h.is_empty() && !names.contains(h) {
                names.push(h.clone());
            }
        }
    }
    let ck = rcgen::generate_simple_self_signed(names.clone()).map_err(|e| format!("self-signed certificate: {e}"))?;
    std::fs::write(&cert, ck.cert.pem()).map_err(|e| e.to_string())?;
    std::fs::write(&key, ck.signing_key.serialize_pem()).map_err(|e| e.to_string())?;
    crate::logf!("made a self-signed certificate for {} -> {}", names.join(", "), cert.display());
    Ok((cert, key))
}
