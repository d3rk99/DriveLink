# Builds the road-film CHARACTER layers from the MiniDayZ APK (PROTOTYPE ONLY: Bohemia's art, gitignored output).
# MiniDayZ draws a survivor as stacked layers (skin, pants, jacket, vest, backpack, head, weapon), every layer with the
# same animations. Their frames sit at slightly different places per sheet (data.js has the table), so each layer is
# re-packed here into ONE uniform sheet the mod can address with the same math for every layer:
#   people/gear/weapons 128 x 1024, infected 128 x 512; 32 x 32 cells, 4 frames per row, directions down/up/right/left:
#     rows 0-3 idle  4-7 run  8-11 aim (firearm, MiniDayZ "pistol_")  12-15 swing (melee or a punch, "axe_")
#     16 using an item (crouched, working)
#     weapons only, the weapon SLUNG ON THE BACK (MiniDayZ "h_*"): 17-20 idle  21-24 run  25-28 while the hands fight
#     (a firearm on the back while swinging = "h_axe_", a melee on the back while aiming = "h_pistol_")
#     skins only: 17-18 dying (6 frames: 17 = frames 1-4, 18 = 5-6, then the last repeated) from player_skin_dead
#     infected: 0-7 as above (walk = run), 8-11 attack, 12-13 dying (die1, the same 2-row split)
#   (1-frame animations are repeated across the row; 30x30 frames are centred in the 32x32 cell; a pose a layer
#   doesn't have falls back to its idle frame of that direction)
# Clothing also gets a grey "...w" copy the mod tints with the item's colour from its DayZ class name (Hoodie_Blue).
# Also writes Addons/DriveLink/Scripts/3_Game/DriveLink/DL_FilmCatalog.c (generated: which layers exist).
# Usage: python Tools/build-anim-characters.py [path to the MiniDayZ .apk]
import io, json, os, subprocess, sys, zipfile
from PIL import Image

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APK = sys.argv[1] if len(sys.argv) > 1 else r'C:\DayZMods\Mini+DAYZ_+Zombie+Survival_1.4.1_APKPure.apk'
OUT = os.path.join(REPO, 'Addons', 'DriveLink', 'gui', 'anim', 'chr')
CATALOG = os.path.join(REPO, 'Addons', 'DriveLink', 'Scripts', '3_Game', 'DriveLink', 'DL_FilmCatalog.c')
IMAGE_TO_PAA = r'E:\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe'

DIRS = ['down', 'up', 'right', 'left']
ROWS = ['idle_' + d for d in DIRS] + ['run_' + d for d in DIRS]
ZED_ROWS = ROWS + ['attack_' + d for d in DIRS]
ZED_DEAD = {'zed_normal_skin1': 'zed_normal_skin1_dead', 'zed_normal_skin2': 'zed_normal_skin2_dead',
            'zed_normal_skin3': 'zed_normal_skin3_dead', 'zed_army_skin1': 'zed_army_dead_skin1',
            'zed_fast_skin1': 'zed_fast_skin1_dead'}   # infected -> its death type (first image name)
# one entry per row: candidate animation names (the first that exists), lower case
PEOPLE_ROWS = [[a] for a in ROWS] + [['pistol_' + d] for d in DIRS] + [['axe_' + d] for d in DIRS] + [['using_item']]
WEAPON_ROWS = PEOPLE_ROWS + [['h_twohand_' + d] for d in DIRS] + [['h_run_' + d] for d in DIRS] + \
    [['h_axe_' + d, 'h_pistol_' + d] for d in DIRS]
MELEE = {'axe_red', 'fireaxe', 'baseball_bat', 'crowbar', 'pickaxe', 'shovel', 'sledgehammer', 'pitchfork',
         'pipe_wrench', 'knife_any', 'sword'}
TINTED = {'body', 'legs', 'head', 'back', 'vest'}   # categories that get a grey copy for colour tinting

CATEGORIES = {  # category -> MiniDayZ type names (first image of the type, without -sheet0.png)
    'skin': ['player_skin_aika', 'player_skin_bandit', 'player_skin_biker', 'player_skin_boxer', 'player_skin_def_1',
             'player_skin_def_2', 'player_skin_def_3', 'player_skin_doc', 'player_skin_ganzo', 'player_skin_jasmine',
             'player_skin_kate', 'player_skin_maniac', 'player_skin_redneck', 'player_skin_survivalist',
             'player_skin_veteran'],
    'body': ['down_jacket', 'gorka_jacket', 'hunter_jacket', 'orel_jacket', 'paramedic_jacket', 'raider_jacket',
             'sport_jacket', 'shirt_gray', 'shirt_green', 'tshirt', 'hoodie_gray', 'hoodie_red', 'raincoat_blue',
             'sweater', 'cloak_brown', 'dress'],
    'legs': ['gorka_pants', 'hunter_pants', 'orel_pants', 'paramedic_pants', 'sportpants', 'workpants', 'jeans'],
    'head': ['gorka_helmet', 'helmet_army', 'helmet_hard', 'helmet_moto', 'helmet_moto_red', 'helmet_nvg', 'cowboy_hat',
             'police_hat', 'warm_hat', 'baseball_cap', 'ushanka', 'beret_red', 'bandana', 'balaklava', 'headlamp'],
    'mask': ['gasmask', 'welding_mask'],
    'back': ['hunter_backpack', 'improvised_backpack', 'mountain_backpack', 'school_backpack', 'taloon_backpack',
             'improvised_bag'],
    'vest': ['vest', 'vest_press', 'razgruz', 'razgruz_big'],
    'weapon': ['ak74_rifle', 'akm_rifle', 'aks74u_rifle', 'aug_rifle', 'm4_rifle', 'fncal_rifle', 'l85_rifle', 'rpk_rifle',
               'groza_rifle', 'svd_rifle', 'sv98_rifle', 'vss_rifle', 'sks_rifle', 'mosin_nagant', 'sawed_mosin',
               'repeater_rifle', 'sporter_rifle', 'bm16_shotgun', 'r670_shotgun', 'saiga_shotgun', 'sawed_izh',
               'mp5_smg', 'bizon_smg', 'mac10_pistol', 'glock_pistol', 'fnx_pistol', 'magnum_pistol', 'crossbow',
               'improvised_bow', 'axe_red', 'fireaxe', 'baseball_bat', 'crowbar', 'pickaxe', 'shovel', 'sledgehammer',
               'pitchfork', 'pipe_wrench', 'knife_any', 'sword'],
    'zed': ['zed_normal_skin1', 'zed_normal_skin2', 'zed_normal_skin3', 'zed_army_skin1', 'zed_fast_skin1'],
}


def short(name):
    return name.replace('player_skin_', '').replace('_', '')


def grey(img):
    """the layer in grey, brightened so its light parts are near white: the mod's SetColor multiplies it"""
    px = img.load()
    lum = []
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            if a > 0:
                lum.append(0.3 * r + 0.59 * g + 0.11 * b)
    if not lum:
        return img.copy()
    lum.sort()
    k = 225.0 / max(lum[int(len(lum) * 0.75)], 1)
    out = img.copy()
    op = out.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            if a > 0:
                v = min(255, int((0.3 * r + 0.59 * g + 0.11 * b) * k))
                op[x, y] = (v, v, v, a)
    return out


def main():
    z = zipfile.ZipFile(APK)
    data = json.loads(z.read('assets/www/data.js').decode('utf-8-sig'))
    types = data['project'][3]
    by_first = {}
    for t in types:
        an = t[7]
        if isinstance(an, list) and an and isinstance(an[0], list) and an[0][7]:
            first = an[0][7][0][0].split('/')[-1].replace('-sheet0.png', '')
            by_first.setdefault(first, t)
    sheets = {}

    def sheet(path):
        if path not in sheets:
            sheets[path] = Image.open(io.BytesIO(z.read('assets/www/' + path))).convert('RGBA')
        return sheets[path]

    def to_paa(img, base):
        png = os.path.join(OUT, 'chr_%s_ca.png' % base)
        img.save(png)
        paa = png[:-4] + '.paa'
        res = subprocess.run([IMAGE_TO_PAA, png, paa], capture_output=True, text=True)
        os.remove(png)
        if res.returncode != 0 or not os.path.exists(paa):
            print('FAILED', base, (res.stdout + res.stderr)[-200:])
            sys.exit(1)

    def frames_of(t, anim):
        a = {x[0]: x for x in t[7]}.get(anim)
        return a[7] if a else []

    def paste_run(out, frames, row):
        """up to 8 frames over rows row and row + 1 (4 per row); the last frame repeats"""
        for i in range(8):
            f = frames[min(i, len(frames) - 1)]
            img, x, y, w, h = f[0], f[2], f[3], f[4], f[5]
            cell = sheet(img).crop((x, y, x + w, y + h))
            r, c = row + i // 4, i % 4
            out.paste(cell, (c * 32 + (32 - w) // 2, r * 32 + (32 - h) // 2), cell)

    # the player death animations are per base skin (id_1..15, bare skin): match each to our skins by its first frame
    dead_t = by_first.get('player_skin_dead')
    dead_ids = {}
    if dead_t:
        def first_frame(fr):
            f = fr[0]
            return sheet(f[0]).crop((f[2], f[3], f[2] + f[4], f[3] + f[5])).convert('RGB').resize((30, 30))
        dead = {a[0]: a[7] for a in dead_t[7]}
        for name in CATEGORIES['skin']:
            t = by_first.get(name)
            if not t:
                continue
            ref = first_frame(frames_of(t, 'idle_down'))
            best = min(dead, key=lambda k: sum(abs(p - q) for a, b in zip(first_frame(dead[k]).getdata(), ref.getdata())
                                               for p, q in zip(a, b)))
            dead_ids[name] = dead[best]
        print('dying skins matched:', len(dead_ids))

    os.makedirs(OUT, exist_ok=True)
    made = {c: [] for c in CATEGORIES}
    made['melee'] = []
    made['tint'] = []
    for cat, names in CATEGORIES.items():
        if cat == 'zed':
            rows, height = [[a] for a in ZED_ROWS], 512
        elif cat == 'weapon':
            rows, height = WEAPON_ROWS, 1024
        else:
            rows, height = PEOPLE_ROWS, 1024
        for name in names:
            t = by_first.get(name)
            if not t:
                print('not in data.js:', name)
                continue
            anims = {a[0].lower(): a for a in t[7]}
            out = Image.new('RGBA', (128, height), (0, 0, 0, 0))
            missing = []
            for r, cands in enumerate(rows):
                a = next((anims[c] for c in cands if c in anims), None)
                if not a:
                    missing.append(cands[0])
                    if cands[0].startswith('h_'):
                        continue                     # no back pose: nothing drawn
                    d = cands[0].rsplit('_', 1)[-1]
                    a = anims.get('idle_' + d) or anims.get('idle_down')
                    if not a:
                        continue
                frames = a[7]
                for c in range(4):
                    f = frames[c % len(frames)]
                    img, x, y, w, h = f[0], f[2], f[3], f[4], f[5]
                    cell = sheet(img).crop((x, y, x + w, y + h))
                    out.paste(cell, (c * 32 + (32 - w) // 2, r * 32 + (32 - h) // 2), cell)
            if missing:
                print(name, 'missing', missing)
            if cat == 'skin' and name in dead_ids:
                paste_run(out, dead_ids[name], 17)
            if cat == 'zed' and ZED_DEAD.get(name) in by_first:
                dt = by_first[ZED_DEAD[name]]
                fr = frames_of(dt, 'die1') or frames_of(dt, 'die2')
                if fr:
                    paste_run(out, fr, 12)
            to_paa(out, short(name))
            made[cat].append(short(name))
            if cat == 'weapon' and name in MELEE:
                made['melee'].append(short(name))
            if cat in TINTED:
                to_paa(grey(out), short(name) + 'w')
                made['tint'].append(short(name))
    # the catalog the mod reads (generated)
    lines = ['// GENERATED by Tools/build-anim-characters.py - do not edit. Which road-film character layers exist',
             '// (gui/anim/chr/chr_<name>_ca.paa, 32x32 cells; rows in Tools/build-anim-characters.py). MELEE: weapons',
             '// that swing (the rest aim and shoot); TINT: layers with a grey copy chr_<name>w_ca.paa for colouring.',
             'class DL_FilmCatalog', '{']
    for cat, names in made.items():
        lines.append('\tstatic const string %s = "%s";' % (cat.upper(), ','.join(names)))
    lines += ['',
              '\t// the names of one category (SKIN, BODY...) as an array',
              '\tstatic array<string> List(string csv)',
              '\t{',
              '\t\tarray<string> a = new array<string>;',
              '\t\tcsv.Split(",", a);',
              '\t\treturn a;',
              '\t}',
              '}', '']
    open(CATALOG, 'w', encoding='utf-8').write('\n'.join(lines))
    print({c: len(v) for c, v in made.items()})


if __name__ == '__main__':
    main()
