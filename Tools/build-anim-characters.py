# Builds the road-film CHARACTER layers from the MiniDayZ APK (PROTOTYPE ONLY: Bohemia's art, gitignored output).
# MiniDayZ draws a survivor as stacked layers (skin, pants, jacket, vest, backpack, head, weapon), every layer with the
# same animations. Their frames sit at slightly different places per sheet (data.js has the table), so each layer is
# re-packed here into ONE uniform sheet the mod can address with the same math for every layer:
#   128 x 256 (people/gear/weapons) or 128 x 512 (infected), 32 x 32 cells, 4 frames per row:
#     row 0 idle_down  1 idle_up  2 idle_right  3 idle_left  4 run_down  5 run_up  6 run_right  7 run_left
#     infected also: 8 attack_down  9 attack_up  10 attack_right  11 attack_left
#   (1-frame animations are repeated across the row; 30x30 frames are centred in the 32x32 cell)
# Also writes Addons/DriveLink/Scripts/3_Game/DriveLink/DL_FilmCatalog.c (generated: which layers exist).
# Usage: python Tools/build-anim-characters.py [path to the MiniDayZ .apk]
import io, json, os, subprocess, sys, zipfile
from PIL import Image

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APK = sys.argv[1] if len(sys.argv) > 1 else r'C:\DayZMods\Mini+DAYZ_+Zombie+Survival_1.4.1_APKPure.apk'
OUT = os.path.join(REPO, 'Addons', 'DriveLink', 'gui', 'anim', 'chr')
CATALOG = os.path.join(REPO, 'Addons', 'DriveLink', 'Scripts', '3_Game', 'DriveLink', 'DL_FilmCatalog.c')
IMAGE_TO_PAA = r'E:\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe'

ROWS = ['idle_down', 'idle_up', 'idle_right', 'idle_left', 'run_down', 'run_up', 'run_right', 'run_left']
ZED_ROWS = ROWS + ['attack_down', 'attack_up', 'attack_right', 'attack_left']

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

    os.makedirs(OUT, exist_ok=True)
    made = {c: [] for c in CATEGORIES}
    for cat, names in CATEGORIES.items():
        rows = ZED_ROWS if cat == 'zed' else ROWS
        for name in names:
            t = by_first.get(name)
            if not t:
                print('not in data.js:', name)
                continue
            anims = {a[0]: a for a in t[7]}
            out = Image.new('RGBA', (128, 512 if cat == 'zed' else 256), (0, 0, 0, 0))
            missing = []
            for r, an in enumerate(rows):
                a = anims.get(an)
                if not a:
                    missing.append(an)
                    continue
                frames = a[7]
                for c in range(4):
                    f = frames[c % len(frames)]
                    img, x, y, w, h = f[0], f[2], f[3], f[4], f[5]
                    cell = sheet(img).crop((x, y, x + w, y + h))
                    out.paste(cell, (c * 32 + (32 - w) // 2, r * 32 + (32 - h) // 2), cell)
            if missing:
                print(name, 'missing', missing)
            png = os.path.join(OUT, 'chr_%s_ca.png' % short(name))
            out.save(png)
            paa = png[:-4] + '.paa'
            res = subprocess.run([IMAGE_TO_PAA, png, paa], capture_output=True, text=True)
            os.remove(png)
            if res.returncode != 0 or not os.path.exists(paa):
                print('FAILED', name, (res.stdout + res.stderr)[-200:])
                sys.exit(1)
            made[cat].append(short(name))
    # the catalog the mod reads (generated)
    lines = ['// GENERATED by Tools/build-anim-characters.py - do not edit. Which road-film character layers exist',
             '// (gui/anim/chr/chr_<name>_ca.paa, 32x32 cells, rows: idle down/up/right/left, run down/up/right/left;',
             '// infected also attack down/up/right/left).',
             'class DL_FilmCatalog', '{']
    for cat, names in made.items():
        lines.append('\tstatic ref array<string> %s = {%s};' % (cat.upper(), ', '.join('"%s"' % n for n in names)))
    lines += ['}', '']
    open(CATALOG, 'w', encoding='utf-8').write('\n'.join(lines))
    print({c: len(v) for c, v in made.items()})


if __name__ == '__main__':
    main()
