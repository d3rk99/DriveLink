// Which addresses belong to this machine, and the LAN address other PCs on the network reach it by.
use std::net::{IpAddr, UdpSocket};

/// The address this PC uses on its network (the one a route to the internet goes out of). No packet is sent:
/// connect() on a UDP socket only picks the route.
pub fn lan_ip() -> Option<IpAddr> {
    let s = UdpSocket::bind("0.0.0.0:0").ok()?;
    s.connect("8.8.8.8:80").ok()?;
    let ip = s.local_addr().ok()?.ip();
    if ip.is_unspecified() { None } else { Some(ip) }
}

pub fn is_this_machine(addr: &str) -> bool {
    let Ok(ip) = addr.parse::<IpAddr>() else { return addr == "localhost" };
    ip.is_loopback() || Some(ip) == lan_ip()
}

/// RFC 1918 / loopback / link-local: an address only players on that network can use.
pub fn is_private(addr: &str) -> bool {
    match addr.parse::<IpAddr>() {
        Ok(IpAddr::V4(v)) => v.is_private() || v.is_loopback() || v.is_link_local(),
        Ok(IpAddr::V6(v)) => v.is_loopback(),
        Err(_) => addr == "localhost",
    }
}
