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

/// addons folders that have map data: hub.json dayzPath first, then the DayZ client / server installs found.
pub fn addon_dirs(dayz_path: &str) -> Vec<PathBuf> {
    let mut out = Vec::new();
    if !dayz_path.is_empty() {
        let p = PathBuf::from(dayz_path);
        out.push(if p.ends_with("addons") { p } else { p.join("addons") });
    }
    for lib in steam_libraries() {
        for game in ["DayZ", "DayZServer"] {
            let a = lib.join("steamapps").join("common").join(game).join("addons");
            if a.is_dir() && !out.contains(&a) {
                out.push(a);
            }
        }
    }
    out.into_iter().filter(|p| p.is_dir()).collect()
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

/// map name -> its worlds_<map>_data.pbo
pub fn available(dayz_path: &str) -> HashMap<String, PathBuf> {
    let mut m = HashMap::new();
    for dir in addon_dirs(dayz_path) {
        if let Ok(rd) = std::fs::read_dir(&dir) {
            for e in rd.flatten() {
                let n = e.file_name().to_string_lossy().to_lowercase();
                if let Some(map) = n.strip_prefix("worlds_").and_then(|r| r.strip_suffix("_data.pbo")) {
                    m.entry(map.to_string()).or_insert(e.path());
                }
            }
        }
    }
    m
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
