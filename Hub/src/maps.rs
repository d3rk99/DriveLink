// Map tiles for the editor, made from the owner's OWN DayZ files (nothing is shipped; Research/R3-hub-map.md).
//
// Every map's satellite texture is in addons\worlds_<map>_data.pbo as layers\s_XXX_YYY_lco.paa (XXX = column from
// the west, YYY = row from the north). Each tile's material (p_XXX-YYY_*.rvmat) says where it lies: Stage1
// uvSource="worldPos", u = x*aside + pos[0], v-from-the-top = pos[1] - z*up -> x0 = -pos[0]/aside, zTop = pos[1]/up,
// tile = 1/aside metres; neighbours overlap, step = x0(col+1) - x0(col). DayZ Tools does the conversions:
// BankRev (unpack), CfgConvert (rvmat -> text), ImageToPAA (paa -> png).
//
// Output: maps/<map>/meta.json + a Leaflet tile pyramid maps/<map>/<z>/<col>_<row>.jpg with z = 0 at 1 m/px (step
// px tiles) and z = -1, -2, ... halving until one tile covers the map.
//
// Modded maps (Research/R3-hub-map.md, 2026-10-20): every PBO in the mod folders (client !Workshop\@*\Addons, server
// @*\addons, the folders in hub.json) is looked at by its HEADER only (the file list, nothing unpacked): the one with
// a layers\s_*_lco.paa set is a map's data PBO, whatever it is called; its path names the world (matched against the
// worlds the servers report). hub.json mapPbos pins a PBO by hand. A map whose PBO can't be read (packed / no tiles)
// can be made from one uploaded picture of the whole map (build_from_image).
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::sync::{Arc, Mutex};

#[derive(serde::Serialize, serde::Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Meta {
    pub map: String,
    pub size: f64,      // metres (= pixels at zoom 0)
    pub tile_size: u32, // px per tile at every zoom
    pub min_zoom: i32,
    pub max_zoom: i32,
}

pub type Status = Arc<Mutex<HashMap<String, String>>>;

/// Steam library folders (from libraryfolders.vdf), plus common defaults.
fn steam_libraries() -> Vec<PathBuf> {
    let mut libs: Vec<PathBuf> = vec![PathBuf::from(r"C:\Program Files (x86)\Steam"), PathBuf::from(r"C:\Program Files\Steam")];
    for root in libs.clone() {
        if let Ok(t) = std::fs::read_to_string(root.join(r"steamapps\libraryfolders.vdf")) {
            for line in t.lines() {
                let l = line.trim();
                if let Some(rest) = l.strip_prefix("\"path\"") {
                    let p = rest.trim().trim_matches('"').replace("\\\\", "\\");
                    if !p.is_empty() {
                        libs.push(PathBuf::from(p));
                    }
                }
            }
        }
    }
    for d in ["D", "E", "F", "G"] {
        libs.push(PathBuf::from(format!(r"{d}:\Steam")));
        libs.push(PathBuf::from(format!(r"{d}:\SteamLibrary")));
    }
    libs.dedup();
    libs
}

/// Main game addons followed by DLC addons (e.g. sakhal/Addons and nasdara/addons).
/// Scan only one level below the install root; Workshop mods are handled separately.
fn install_addon_dirs(path: &Path) -> Vec<PathBuf> {
    let is_addons = path.file_name().is_some_and(|n| n.to_string_lossy().eq_ignore_ascii_case("addons"));
    let root = if is_addons { path.parent().unwrap_or(path) } else { path };
    let main = if is_addons { path.to_path_buf() } else { root.join("addons") };
    let mut out = Vec::new();
    if main.is_dir() {
        out.push(main);
    }
    let mut dlc = Vec::new();
    if let Ok(entries) = std::fs::read_dir(root) {
        for entry in entries.flatten() {
            let name = entry.file_name();
            if name.to_string_lossy().starts_with('@') || name.to_string_lossy().eq_ignore_ascii_case("addons") {
                continue;
            }
            // Windows installs use both Addons and addons; match either spelling.
            if let Ok(children) = std::fs::read_dir(entry.path()) {
                for child in children.flatten() {
                    if child.file_name().to_string_lossy().eq_ignore_ascii_case("addons") && child.path().is_dir() {
                        dlc.push(child.path());
                    }
                }
            }
        }
    }
    dlc.sort();
    out.extend(dlc);
    out
}

/// Map data folders: configured installation first, then discovered client/server installs, including DLCs.
pub fn addon_dirs(dayz_path: &str) -> Vec<PathBuf> {
    let mut out = Vec::new();
    if !dayz_path.is_empty() {
        out.extend(install_addon_dirs(Path::new(dayz_path)));
    }
    for lib in steam_libraries() {
        for game in ["DayZ", "DayZServer"] {
            let root = lib.join("steamapps").join("common").join(game);
            for a in install_addon_dirs(&root) {
                if !out.contains(&a) {
                    out.push(a);
                }
            }
        }
    }
    out
}

pub fn tools_dir(tools_path: &str) -> Option<PathBuf> {
    let mut c: Vec<PathBuf> = Vec::new();
    if !tools_path.is_empty() {
        let p = PathBuf::from(tools_path);
        c.push(if p.ends_with("Bin") { p } else { p.join("Bin") });
    }
    for lib in steam_libraries() {
        c.push(lib.join(r"steamapps\common\DayZ Tools\Bin"));
    }
    c.into_iter().find(|p| p.join(r"PboUtils\BankRev.exe").is_file() && p.join(r"ImageToPAA\ImageToPAA.exe").is_file())
}

/// map name -> the PBO with its satellite tiles: pinned by hand (hub.json mapPbos), the vanilla worlds_<map>_data.pbo,
/// then any PBO in a mod folder whose file list has a layers\s_*_lco.paa set (named after the world it is for)
pub fn available(cfg: &crate::config::Config) -> HashMap<String, PathBuf> {
    let mut m = HashMap::new();
    for (name, p) in &cfg.map_pbos {
        let pb = PathBuf::from(p);
        if pb.is_file() {
            m.insert(name.to_lowercase(), pb);
        }
    }
    for dir in addon_dirs(&cfg.dayz_path) {
        if let Ok(rd) = std::fs::read_dir(&dir) {
            for e in rd.flatten() {
                let n = e.file_name().to_string_lossy().to_lowercase();
                if let Some(map) = n.strip_prefix("worlds_").and_then(|r| r.strip_suffix("_data.pbo")) {
                    m.entry(map.to_string()).or_insert(e.path());
                }
            }
        }
    }
    let known: Vec<String> = cfg.servers.iter().map(|s| s.world.to_lowercase()).filter(|w| !w.is_empty()).collect();
    for dir in mod_addon_dirs(&cfg.dayz_path) {
        if let Ok(rd) = std::fs::read_dir(&dir) {
            for e in rd.flatten() {
                let p = e.path();
                if !p.extension().map(|x| x.eq_ignore_ascii_case("pbo")).unwrap_or(false) {
                    continue;
                }
                if let Some(cands) = probe_pbo(&p) {
                    let stem = p.file_stem().map(|s| s.to_string_lossy().to_lowercase()).unwrap_or_default();
                    let name = world_name(&cands, &stem, &known);
                    if !name.is_empty() {
                        m.entry(name).or_insert(p);
                    }
                }
            }
        }
    }
    m
}

/// mods' addon folders: the client's !Workshop\@*\Addons, @* beside DayZ / DayZServer, and under the hub.json folder
pub fn mod_addon_dirs(dayz_path: &str) -> Vec<PathBuf> {
    let mut roots: Vec<PathBuf> = Vec::new();
    if !dayz_path.is_empty() {
        let p = PathBuf::from(dayz_path);
        let base = if p.ends_with("addons") { p.parent().map(Path::to_path_buf).unwrap_or(p.clone()) } else { p };
        roots.push(base.clone());
        roots.push(base.join("!Workshop"));
    }
    for lib in steam_libraries() {
        for game in ["DayZ", "DayZServer"] {
            let g = lib.join("steamapps").join("common").join(game);
            roots.push(g.clone());
            roots.push(g.join("!Workshop"));
        }
    }
    let mut out: Vec<PathBuf> = Vec::new();
    for r in roots {
        let Ok(rd) = std::fs::read_dir(&r) else { continue };
        for e in rd.flatten() {
            let n = e.file_name().to_string_lossy().to_string();
            if !n.starts_with('@') || !e.path().is_dir() {
                continue;
            }
            if let Ok(sub) = std::fs::read_dir(e.path()) {
                for s in sub.flatten() {
                    if s.path().is_dir() && s.file_name().to_string_lossy().eq_ignore_ascii_case("addons") && !out.contains(&s.path()) {
                        out.push(s.path());
                    }
                }
            }
        }
    }
    out
}

/// PBO header looks, cached by file size + time (the Servers page asks every 5 s)
type ProbeCache = Mutex<HashMap<PathBuf, (u64, Option<std::time::SystemTime>, Option<Vec<String>>)>>;
fn probe_cache() -> &'static ProbeCache {
    static C: std::sync::OnceLock<ProbeCache> = std::sync::OnceLock::new();
    C.get_or_init(|| Mutex::new(HashMap::new()))
}

/// Does this PBO hold a map's satellite tiles (layers\s_XXX_YYY_lco.paa)? Then the path segments before "layers"
/// (prefix included, lower case), else None. Reads the header only: entries of name\0 + 5 x u32, the first one may be
/// the "Vers" header extension (key\0 value\0 ... \0, with the prefix), an empty name with method 0 ends the list.
pub fn probe_pbo(p: &Path) -> Option<Vec<String>> {
    let md = std::fs::metadata(p).ok()?;
    let (len, mtime) = (md.len(), md.modified().ok());
    if let Some((l, t, v)) = probe_cache().lock().unwrap().get(p) {
        if *l == len && *t == mtime {
            return v.clone();
        }
    }
    let found = read_pbo_header(p);
    probe_cache().lock().unwrap().insert(p.to_path_buf(), (len, mtime, found.clone()));
    found
}

fn read_pbo_header(p: &Path) -> Option<Vec<String>> {
    use std::io::Read;
    let f = std::fs::File::open(p).ok()?;
    let mut r = std::io::BufReader::new(f);
    let cstr = |r: &mut std::io::BufReader<std::fs::File>| -> Option<String> {
        let mut b = Vec::new();
        let mut one = [0u8; 1];
        loop {
            r.read_exact(&mut one).ok()?;
            if one[0] == 0 {
                break;
            }
            b.push(one[0]);
            if b.len() > 1024 {
                return None; // not a PBO header (or a packed one)
            }
        }
        Some(String::from_utf8_lossy(&b).to_string())
    };
    let mut prefix = String::new();
    let mut first = true;
    for _ in 0..500_000 {
        let name = cstr(&mut r)?;
        let mut nums = [0u8; 20];
        r.read_exact(&mut nums).ok()?;
        let method = u32::from_le_bytes([nums[0], nums[1], nums[2], nums[3]]);
        if name.is_empty() {
            if first && method == 0x5665_7273 {
                // header extension: key/value pairs
                loop {
                    let k = cstr(&mut r)?;
                    if k.is_empty() {
                        break;
                    }
                    let v = cstr(&mut r)?;
                    if k.eq_ignore_ascii_case("prefix") {
                        prefix = v;
                    }
                }
                first = false;
                continue;
            }
            return None; // end of the list: no tiles in it
        }
        first = false;
        let full = format!("{}\\{}", prefix, name).to_lowercase().replace('/', "\\");
        if let Some(i) = full.find("\\layers\\s_") {
            if full.ends_with("_lco.paa") {
                return Some(full[..i].split('\\').filter(|s| !s.is_empty()).map(str::to_string).collect());
            }
        }
    }
    None
}

/// the world a map PBO is for: a world a server reports that its path names, else the last non-generic part of the
/// path (worlds_X_data / X_data / X_terrain -> X), else the file name
pub fn world_name(cands: &[String], stem: &str, known: &[String]) -> String {
    let clean = |s: &str| -> String {
        let mut t = s.to_lowercase();
        for pre in ["worlds_", "world_"] {
            if let Some(r) = t.strip_prefix(pre) {
                t = r.to_string();
            }
        }
        for suf in ["_data", "_terrain", "_world", "_layers"] {
            if let Some(r) = t.strip_suffix(suf) {
                t = r.to_string();
            }
        }
        t
    };
    let generic = ["dz", "worlds", "world", "data", "terrain", "layers", "maps", "map", "addons"];
    let parts: Vec<String> = cands.iter().map(|c| clean(c)).filter(|c| !c.is_empty() && !generic.contains(&c.as_str())).collect();
    for k in known {
        if parts.iter().any(|p| p == k) || clean(stem) == *k {
            return k.clone();
        }
    }
    if let Some(last) = parts.last() {
        return last.clone();
    }
    clean(stem)
}

/// A map from ONE picture of the whole map (the owner uploads it: a map whose PBO can't be read). The picture covers
/// x 0..size, z 0..size (north up); tiles of 512 px from the zoom where the picture is at its own resolution (never
/// upscaled, at most 8192 px across) down to one tile.
pub fn build_from_image(dir: PathBuf, map: String, bytes: Vec<u8>, size: f64, status: Status) {
    let set = |s: String| {
        crate::logf!("map {map}: {s}");
        status.lock().unwrap().insert(map.clone(), s);
    };
    let res = (|| -> Result<Meta, String> {
        set(format!("reading the picture ({} kB)", bytes.len() / 1024));
        let img = image::load_from_memory(&bytes).map_err(|e| format!("not a PNG or JPG picture: {e}"))?.to_rgb8();
        drop(bytes);
        let w = img.width().max(img.height()) as f64;
        let ts: u32 = 512;
        let mut z = ((w / size).log2().floor() as i32).min(0).min(((8192.0 / size).log2().floor()) as i32);
        let max_zoom = z;
        let root = dir.join("maps").join(&map);
        let _ = std::fs::remove_dir_all(&root);
        let mut px = (size * 2f64.powi(z)).round().max(1.0) as u32;
        set(format!("{}x{} picture for {size} m: zoom {z} at {px} px", img.width(), img.height()));
        let mut level = image::imageops::resize(&img, px, px, image::imageops::FilterType::Lanczos3);
        drop(img);
        loop {
            let out = root.join(z.to_string());
            std::fs::create_dir_all(&out).map_err(|e| e.to_string())?;
            let n = px.div_ceil(ts);
            for y in 0..n {
                for x in 0..n {
                    let mut tile = image::RgbImage::new(ts, ts);
                    let (cx, cy) = (x * ts, y * ts);
                    let part = image::imageops::crop_imm(&level, cx, cy, ts.min(px - cx), ts.min(px - cy)).to_image();
                    image::imageops::replace(&mut tile, &part, 0, 0);
                    tile.save(out.join(format!("{x}_{y}.jpg"))).map_err(|e| e.to_string())?;
                }
            }
            set(format!("zoom level {z} done"));
            if n <= 1 {
                break;
            }
            px = px.div_ceil(2);
            level = image::imageops::resize(&level, px, px, image::imageops::FilterType::Triangle);
            z -= 1;
        }
        let meta = Meta { map: map.clone(), size, tile_size: ts, min_zoom: z, max_zoom };
        std::fs::write(root.join("meta.json"), serde_json::to_string_pretty(&meta).unwrap()).map_err(|e| e.to_string())?;
        Ok(meta)
    })();
    match res {
        Ok(m) => set(format!("ready ({} m, zoom {}..{}, from a picture)", m.size, m.min_zoom, m.max_zoom)),
        Err(e) => set(format!("FAILED: {e}")),
    }
}

pub fn meta(dir: &Path, map: &str) -> Option<Meta> {
    let t = std::fs::read_to_string(dir.join("maps").join(map).join("meta.json")).ok()?;
    serde_json::from_str(&t).ok()
}

fn run(exe: &Path, args: &[&std::ffi::OsStr]) -> Result<(), String> {
    let out = Command::new(exe).args(args).output().map_err(|e| format!("{}: {e}", exe.display()))?;
    if !out.status.success() {
        return Err(format!("{} failed: {}", exe.display(), String::from_utf8_lossy(&out.stdout).lines().last().unwrap_or("")));
    }
    Ok(())
}

// first Stage with uvSource="worldPos": (aside x, up z, pos u, pos v)
fn world_uv(rvmat_txt: &str) -> Option<(f64, f64, f64, f64)> {
    let i = rvmat_txt.find("uvSource=\"worldPos\"")?;
    let rest = &rvmat_txt[i..];
    let arr = |key: &str| -> Option<Vec<f64>> {
        let j = rest.find(key)?;
        let s = &rest[j + key.len()..];
        let end = s.find('}')?;
        Some(s[..end].split(',').filter_map(|x| x.trim().parse::<f64>().ok()).collect())
    };
    let a = arr("aside[]={")?;
    let u = arr("up[]={")?;
    let p = arr("pos[]={")?;
    Some((*a.first()?, *u.get(2)?, *p.first()?, *p.get(1)?))
}

/// Builds maps/<map>/ (runs in its own thread; progress in status[map]).
pub fn build(dir: PathBuf, map: String, pbo: PathBuf, tools: PathBuf, status: Status) {
    let set = |s: String| {
        crate::logf!("map {map}: {s}");
        status.lock().unwrap().insert(map.clone(), s);
    };
    let res = (|| -> Result<Meta, String> {
        let work = dir.join("maps").join("_work").join(&map);
        let _ = std::fs::remove_dir_all(&work);
        std::fs::create_dir_all(&work).map_err(|e| e.to_string())?;
        set(format!("unpacking {}", pbo.display()));
        run(&tools.join(r"PboUtils\BankRev.exe"), &["-f".as_ref(), work.as_os_str(), pbo.as_os_str()])?;
        let layers = walk_find_dir(&work, "layers").ok_or("no layers folder in the PBO")?;
        // tiles + their materials
        let mut tiles: Vec<(u32, u32, PathBuf)> = Vec::new();
        let mut mats: HashMap<(u32, u32), PathBuf> = HashMap::new();
        for e in std::fs::read_dir(&layers).map_err(|e| e.to_string())?.flatten() {
            let n = e.file_name().to_string_lossy().to_lowercase();
            if let Some(r) = n.strip_prefix("s_").and_then(|r| r.strip_suffix("_lco.paa")) {
                let p: Vec<&str> = r.split('_').collect();
                if let (Some(c), Some(rw)) = (p.first().and_then(|x| x.parse().ok()), p.get(1).and_then(|x| x.parse().ok())) {
                    tiles.push((c, rw, e.path()));
                }
            } else if let Some(r) = n.strip_prefix("p_").and_then(|r| r.strip_suffix(".rvmat")) {
                let cr: Vec<&str> = r.split('_').next().unwrap_or("").split('-').collect();
                if let (Some(c), Some(rw)) = (cr.first().and_then(|x| x.parse().ok()), cr.get(1).and_then(|x| x.parse().ok())) {
                    mats.entry((c, rw)).or_insert(e.path());
                }
            }
        }
        if tiles.is_empty() {
            return Err("no satellite tiles (layers\\s_*_lco.paa) in the PBO".into());
        }
        let cols = tiles.iter().map(|t| t.0).max().unwrap() + 1;
        let rows = tiles.iter().map(|t| t.1).max().unwrap() + 1;
        // geometry from two neighbouring materials
        let read_mat = |c: u32, r: u32| -> Result<(f64, f64, f64, f64), String> {
            let m = mats.get(&(c, r)).ok_or(format!("no material for tile {c},{r}"))?;
            let out = work.join(format!("m_{c}_{r}.txt"));
            run(&tools.join(r"CfgConvert\CfgConvert.exe"), &["-txt".as_ref(), "-dst".as_ref(), out.as_os_str(), m.as_os_str()])?;
            let t = std::fs::read_to_string(&out).map_err(|e| e.to_string())?;
            world_uv(&t).ok_or(format!("no worldPos stage in {}", m.display()))
        };
        let (a, up, pu0, pv0) = read_mat(1, 1)?;
        let (_, _, pu1, _) = read_mat(2, 1)?;
        let tile_m = 1.0 / a;
        let x0_1 = -pu0 / a;
        let step = (-pu1 / a) - x0_1;
        let ztop_1 = pv0 / up;
        let size = step * cols as f64;
        let overlap_m = (tile_m - step) / 2.0;
        set(format!("{cols}x{rows} tiles, {step} m step, {tile_m} m tiles, {overlap_m} m overlap, map {size} m (tile 1,1 x0 {x0_1} zTop {ztop_1})"));
        // paa -> png (8 at a time), crop the overlap, write zoom 0 at 1 m/px
        let core = step.round() as u32;
        let z0 = dir.join("maps").join(&map).join("0");
        std::fs::create_dir_all(&z0).map_err(|e| e.to_string())?;
        let pngs = work.join("png");
        std::fs::create_dir_all(&pngs).map_err(|e| e.to_string())?;
        let img2paa = tools.join(r"ImageToPAA\ImageToPAA.exe");
        let total = tiles.len();
        let done = Arc::new(Mutex::new(0usize));
        let errors = Arc::new(Mutex::new(Vec::<String>::new()));
        let queue = Arc::new(Mutex::new(tiles));
        let mut handles = Vec::new();
        for _ in 0..8 {
            let (queue, done, errors, pngs, z0, img2paa, status, map) =
                (queue.clone(), done.clone(), errors.clone(), pngs.clone(), z0.clone(), img2paa.clone(), status.clone(), map.clone());
            handles.push(std::thread::spawn(move || loop {
                let Some((c, r, paa)) = queue.lock().unwrap().pop() else { break };
                let png = pngs.join(format!("{c}_{r}.png"));
                let res = run(&img2paa, &[paa.as_os_str(), png.as_os_str()]).and_then(|_| {
                    let img = image::open(&png).map_err(|e| e.to_string())?.to_rgb8();
                    let px_per_m = img.width() as f64 / tile_m;
                    let ov = (overlap_m * px_per_m).round() as u32;
                    let cw = (step * px_per_m).round() as u32;
                    let crop = image::imageops::crop_imm(&img, ov, ov, cw.min(img.width() - ov), cw.min(img.height() - ov)).to_image();
                    let out = if crop.width() != core { image::imageops::resize(&crop, core, core, image::imageops::FilterType::Triangle) } else { crop };
                    out.save(z0.join(format!("{c}_{r}.jpg"))).map_err(|e| e.to_string())?;
                    let _ = std::fs::remove_file(&png);
                    Ok(())
                });
                if let Err(e) = res {
                    errors.lock().unwrap().push(format!("{c},{r}: {e}"));
                }
                let mut d = done.lock().unwrap();
                *d += 1;
                if *d % 32 == 0 || *d == total {
                    status.lock().unwrap().insert(map.clone(), format!("converting tiles {}/{}", *d, total));
                }
            }));
        }
        for h in handles {
            let _ = h.join();
        }
        let errs = errors.lock().unwrap().clone();
        if !errs.is_empty() {
            crate::logf!("map {map}: {} tiles failed, first: {}", errs.len(), errs[0]);
        }
        // pyramid
        let mut z = 0i32;
        let mut n = cols.max(rows);
        while n > 1 {
            let (src, dst) = (dir.join("maps").join(&map).join(z.to_string()), dir.join("maps").join(&map).join((z - 1).to_string()));
            std::fs::create_dir_all(&dst).map_err(|e| e.to_string())?;
            let half = n.div_ceil(2);
            for y in 0..half {
                for x in 0..half {
                    let mut canvas = image::RgbImage::new(core * 2, core * 2);
                    for (dx, dy) in [(0, 0), (1, 0), (0, 1), (1, 1)] {
                        if let Ok(t) = image::open(src.join(format!("{}_{}.jpg", x * 2 + dx, y * 2 + dy))) {
                            image::imageops::replace(&mut canvas, &t.to_rgb8(), (dx * core) as i64, (dy * core) as i64);
                        }
                    }
                    image::imageops::resize(&canvas, core, core, image::imageops::FilterType::Triangle)
                        .save(dst.join(format!("{x}_{y}.jpg")))
                        .map_err(|e| e.to_string())?;
                }
            }
            z -= 1;
            n = half;
            set(format!("zoom level {z} done"));
        }
        let meta = Meta { map: map.clone(), size, tile_size: core, min_zoom: z, max_zoom: 0 };
        std::fs::write(dir.join("maps").join(&map).join("meta.json"), serde_json::to_string_pretty(&meta).unwrap()).map_err(|e| e.to_string())?;
        let _ = std::fs::remove_dir_all(&work);
        Ok(meta)
    })();
    match res {
        Ok(m) => set(format!("ready ({} m, zoom {}..0)", m.size, m.min_zoom)),
        Err(e) => set(format!("FAILED: {e}")),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Install(PathBuf);
    impl Install {
        fn new() -> Self {
            let root = std::env::temp_dir().join(format!("drivelink-map-discovery-{}", rand::random::<u64>()));
            for dir in ["addons", "sakhal/Addons", "nasdara/addons", "@Example/Addons", "unrelated/nested/addons"] {
                std::fs::create_dir_all(root.join(dir)).unwrap();
            }
            Self(root)
        }
    }
    impl Drop for Install {
        fn drop(&mut self) {
            let _ = std::fs::remove_dir_all(&self.0);
        }
    }

    #[test]
    fn finds_dlc_folders_without_scanning_mods_or_nested_directories() {
        let install = Install::new();
        let dirs = install_addon_dirs(&install.0);
        assert_eq!(dirs, vec![install.0.join("addons"), install.0.join("nasdara/addons"), install.0.join("sakhal/Addons")]);
    }

    #[test]
    fn configured_addons_folder_also_finds_sibling_dlc() {
        let install = Install::new();
        assert_eq!(install_addon_dirs(&install.0.join("addons")), install_addon_dirs(&install.0));
        assert!(install_addon_dirs(&install.0.join("missing")).is_empty());
    }

    #[test]
    fn available_maps_include_dlc_and_keep_pinned_pbo_priority() {
        let install = Install::new();
        let dlc = install.0.join("sakhal/Addons/worlds_test_dlc_discovery_data.pbo");
        std::fs::write(&dlc, []).unwrap();
        let mut cfg: crate::config::Config = serde_json::from_value(serde_json::json!({
            "serverKey": "test-only-map-discovery-key", "dayzPath": install.0
        })).unwrap();
        assert_eq!(available(&cfg).get("test_dlc_discovery"), Some(&dlc));
        let pinned = install.0.join("pinned.pbo");
        std::fs::write(&pinned, []).unwrap();
        cfg.map_pbos.insert("test_dlc_discovery".into(), pinned.to_string_lossy().into_owned());
        assert_eq!(available(&cfg).get("test_dlc_discovery"), Some(&pinned));
    }
}

fn walk_find_dir(root: &Path, name: &str) -> Option<PathBuf> {
    let mut stack = vec![root.to_path_buf()];
    while let Some(d) = stack.pop() {
        for e in std::fs::read_dir(&d).ok()?.flatten() {
            if e.path().is_dir() {
                if e.file_name().to_string_lossy().eq_ignore_ascii_case(name) {
                    return Some(e.path());
                }
                stack.push(e.path());
            }
        }
    }
    None
}
