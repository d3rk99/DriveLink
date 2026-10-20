# Builds the road-film textures into Addons/DriveLink/gui/anim/ (Research/road-animation-research.md).
#   - DriveLink's own pixel art (drawn here, committed as source in Art/anim): one top-down ground tile per biome
#     (forest, field, desert, snow) with a 2-lane road, and a props atlas (trees, bushes, rocks, fence, sign, pole...)
#   - MiniDayZ sheets copied from the local pack (C:\DayZMods\_assets\minidayz): PROTOTYPE ONLY, never committed
#     (gitignored) until Bohemia OKs them or they are replaced
# Every texture is a power-of-two PNG named *_ca.png, converted to .paa with DayZ Tools ImageToPAA (DXT5 + alpha).
# Usage: python Tools/build-anim-assets.py
import os, shutil, subprocess, sys, random
from PIL import Image, ImageDraw

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(REPO, 'Addons', 'DriveLink', 'gui', 'anim')
OWN_SRC = os.path.join(REPO, 'Art', 'anim')
PACK = r'C:\DayZMods\_assets\minidayz\minidayz-assets'
IMAGE_TO_PAA = r'E:\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe'

MINIDAYZ = {  # texture name -> file in the pack
    'uaz_ca': r'vehicles\sprites\drivable\car_uaz_anims-sheet0.png',
    'volga_ca': r'vehicles\sprites\drivable\car_volga_anims-sheet0.png',
    'wreck_blue_ca': r'vehicles\sprites\wrecks\car_regular_blue_vert-sheet0.png',
    'wreck_gray_ca': r'vehicles\sprites\wrecks\car_regular_gray_vert-sheet0.png',
    'wreck_green_ca': r'vehicles\sprites\wrecks\car_regular_green_vert-sheet0.png',
    'wreck_red_ca': r'vehicles\sprites\wrecks\car_regular_red_vert-sheet0.png',
    'wreck_police_ca': r'vehicles\sprites\wrecks\car_police_vert-sheet0.png',
    'fueltank_ca': r'vehicles\related\fueltank-sheet0.png',
    'smoke_ca': r'vehicles\sprites\effects\eff_car_smoke_particles.png',
    'zed_ca': r'infected\normal\zed_normal_skin1-sheet0.png',
}
SKINS = ['aika', 'bandit', 'biker', 'boxer', 'def_1', 'def_2', 'def_3', 'doc', 'ganzo', 'guard', 'jager', 'jasmine',
         'kate', 'maniac', 'mike', 'pilot', 'redneck', 'scientist', 'survivalist', 'veteran']
for _s in SKINS:
    MINIDAYZ['skin_%s_ca' % _s.replace('_', '')] = r'people\player\skins\player_skin_%s-sheet0.png' % _s

# ---------------------------------------------------------------- ground tiles (one per biome)
BIOMES = {  # name -> (base, light, dark) ground colours
    'forest': ((84, 110, 54), (96, 124, 62), (72, 96, 46)),
    'field': ((128, 140, 70), (146, 154, 82), (112, 124, 60)),
    'desert': ((196, 170, 120), (210, 186, 136), (178, 152, 104)),
    'snow': ((222, 228, 232), (238, 242, 246), (200, 208, 216)),
}


def ground_tile(biome):
    # 256 x 256, loops top-to-bottom: a 2-lane road (x 72..183 of 256) with a dashed centre line and edge lines,
    # dirt shoulders, the biome's ground with speckle. Props (trees, fences...) are separate sprites.
    base, light, dark = BIOMES[biome]
    im = Image.new('RGBA', (128, 128), base + (255,))
    d = ImageDraw.Draw(im)
    rnd = random.Random(sum(map(ord, biome)))
    for _ in range(900):
        im.putpixel((rnd.randrange(128), rnd.randrange(128)), rnd.choice([light, dark]) + (255,))
    shoulder = {'snow': (150, 150, 152), 'desert': (170, 140, 96)}.get(biome, (132, 116, 86))
    d.rectangle([33, 0, 35, 127], fill=shoulder + (255,))
    d.rectangle([92, 0, 94, 127], fill=shoulder + (255,))
    asphalt = (84, 88, 94) if biome == 'snow' else (64, 66, 70)
    d.rectangle([36, 0, 91, 127], fill=asphalt + (255,))
    d.rectangle([37, 0, 37, 127], fill=(178, 174, 160, 255))    # edge lines
    d.rectangle([90, 0, 90, 127], fill=(178, 174, 160, 255))
    for y in range(0, 128, 16):                                 # dashed centre line, period 16 -> seamless at 128
        d.rectangle([63, y, 64, y + 7], fill=(205, 196, 150, 255))
    for _ in range(260):                                        # asphalt wear
        x, y = rnd.randrange(38, 90), rnd.randrange(128)
        if not (62 <= x <= 65):
            v = asphalt[0] + rnd.choice([-8, 8])
            im.putpixel((x, y), (v, v, v + 4, 255))
    if biome == 'snow':                                         # snow blown onto the road edges
        for _ in range(140):
            x = rnd.choice([rnd.randrange(36, 42), rnd.randrange(86, 92)])
            im.putpixel((x, rnd.randrange(128)), (226, 230, 234, 255))
    return im.resize((256, 256), Image.NEAREST)


# ---------------------------------------------------------------- props atlas: 4x4 cells of 64x64 (drawn at 32x32, x2)
# Order = DL_Film PROP_* in the mod. Keep in sync.
PROPS = ['pine', 'broadleaf', 'bush', 'rock', 'fence', 'sign', 'pole', 'barrel',
         'haybale', 'drybush', 'sandrock', 'snowpine', 'drift', 'tires', 'wheel', 'crate']
SH = (0, 0, 0, 70)  # soft shadow, down-right


def prop(name):
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    if name in ('pine', 'snowpine'):
        d.ellipse([7, 8, 29, 30], fill=SH)
        for r, col in ((12, (30, 56, 34)), (9, (40, 72, 42)), (6, (52, 90, 52)), (3, (66, 108, 62))):
            d.ellipse([16 - r, 15 - r, 16 + r, 15 + r], fill=col + (255,))
        if name == 'snowpine':
            for (x, y) in ((10, 9), (19, 7), (22, 15), (13, 17), (16, 12), (8, 15)):
                d.ellipse([x, y, x + 3, y + 2], fill=(236, 240, 244, 255))
    elif name == 'broadleaf':
        d.ellipse([6, 8, 30, 30], fill=SH)
        for (x, y, r) in ((11, 12, 7), (20, 11, 7), (15, 19, 8), (22, 19, 6)):
            d.ellipse([x - r, y - r, x + r, y + r], fill=(58, 96, 44, 255))
        for (x, y, r) in ((11, 11, 4), (19, 10, 4), (15, 17, 4)):
            d.ellipse([x - r, y - r, x + r, y + r], fill=(84, 126, 58, 255))
    elif name == 'bush':
        d.ellipse([10, 14, 26, 26], fill=SH)
        d.ellipse([8, 11, 22, 23], fill=(54, 88, 44, 255))
        d.ellipse([14, 12, 24, 22], fill=(70, 108, 54, 255))
    elif name in ('rock', 'sandrock'):
        c1, c2 = ((112, 112, 110), (140, 140, 136)) if name == 'rock' else ((168, 140, 100), (192, 166, 124))
        d.polygon([(9, 22), (12, 13), (20, 10), (26, 16), (25, 24), (14, 26)], fill=SH)
        d.polygon([(7, 20), (10, 12), (18, 9), (24, 14), (23, 22), (12, 24)], fill=c1 + (255,))
        d.polygon([(10, 13), (18, 10), (21, 14), (13, 17)], fill=c2 + (255,))
    elif name == 'fence':                                        # runs along the road (vertical), loops top-bottom
        d.rectangle([17, 0, 18, 31], fill=SH)
        d.rectangle([13, 0, 13, 31], fill=(110, 82, 50, 255))
        d.rectangle([16, 0, 16, 31], fill=(110, 82, 50, 255))
        for y in (2, 12, 22):
            d.rectangle([12, y, 17, y + 2], fill=(84, 60, 36, 255))
    elif name == 'sign':
        d.line([(17, 28), (24, 30)], fill=SH, width=2)
        d.rectangle([15, 14, 16, 28], fill=(120, 120, 124, 255))
        d.polygon([(15, 3), (24, 17), (6, 17)], fill=(196, 40, 36, 255))
        d.polygon([(15, 7), (20, 15), (10, 15)], fill=(236, 232, 220, 255))
    elif name == 'pole':
        d.line([(16, 16), (30, 26)], fill=SH, width=2)
        d.ellipse([13, 13, 19, 19], fill=(96, 72, 46, 255))
        d.rectangle([6, 15, 26, 16], fill=(80, 60, 40, 255))
        for x in (7, 16, 25):
            d.rectangle([x, 14, x + 1, 14], fill=(200, 200, 200, 255))
    elif name == 'barrel':
        d.ellipse([11, 12, 25, 26], fill=SH)
        d.ellipse([9, 9, 23, 23], fill=(150, 62, 40, 255))
        d.ellipse([11, 11, 21, 21], fill=(120, 48, 32, 255))
        d.ellipse([14, 14, 18, 18], fill=(90, 36, 24, 255))
    elif name == 'haybale':
        d.ellipse([9, 11, 29, 29], fill=SH)
        d.ellipse([6, 7, 26, 27], fill=(196, 168, 84, 255))
        for r in (7, 4):
            d.ellipse([16 - r, 17 - r, 16 + r, 17 + r], outline=(160, 132, 60, 255))
    elif name == 'drybush':
        d.ellipse([10, 16, 26, 26], fill=SH)
        for (x1, y1) in ((9, 11), (22, 9), (25, 17), (8, 19), (15, 8)):
            d.line([(16, 20), (x1, y1)], fill=(120, 92, 60, 255))
            d.point((x1, y1), fill=(150, 130, 80, 255))
    elif name == 'drift':
        d.ellipse([5, 14, 29, 28], fill=(190, 198, 208, 255))
        d.ellipse([4, 11, 27, 25], fill=(244, 246, 250, 255))
    elif name == 'tires':
        for (x, y) in ((9, 10), (17, 12), (12, 18)):
            d.ellipse([x + 1, y + 2, x + 10, y + 11], fill=SH)
            d.ellipse([x, y, x + 9, y + 9], fill=(30, 30, 32, 255))
            d.ellipse([x + 3, y + 3, x + 6, y + 6], fill=(70, 70, 72, 255))
    elif name == 'wheel':                                        # the spare / patched wheel in the flat-tyre scene
        d.ellipse([10, 10, 26, 26], fill=SH)
        d.ellipse([7, 7, 23, 23], fill=(28, 28, 30, 255))
        d.ellipse([11, 11, 19, 19], fill=(150, 150, 154, 255))
        d.ellipse([14, 14, 16, 16], fill=(80, 80, 84, 255))
    elif name == 'crate':
        d.rectangle([11, 12, 27, 27], fill=SH)
        d.rectangle([8, 9, 24, 24], fill=(150, 112, 64, 255))
        d.rectangle([8, 9, 24, 24], outline=(110, 80, 44, 255))
        d.line([(8, 9), (24, 24)], fill=(110, 80, 44, 255))
    return im


def props_atlas():
    at = Image.new('RGBA', (128, 128), (0, 0, 0, 0))
    for i, n in enumerate(PROPS):
        at.paste(prop(n), ((i % 4) * 32, (i // 4) * 32))
    return at.resize((256, 256), Image.NEAREST)


def pow2(im):
    w, h = im.size
    W, H = 1, 1
    while W < w:
        W *= 2
    while H < h:
        H *= 2
    if (W, H) == (w, h):
        return im
    out = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    out.paste(im, (0, 0))
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(OWN_SRC, exist_ok=True)
    pngs = []
    own = [('ground_%s_ca' % b, (lambda b=b: ground_tile(b))) for b in BIOMES] + [('props_ca', props_atlas)]
    for name, fn in own:
        p = os.path.join(OWN_SRC, name + '.png')
        fn().save(p)
        pngs.append(p)
    tmp = os.path.join(OUT, '_minidayz_src')
    os.makedirs(tmp, exist_ok=True)
    for name, rel in MINIDAYZ.items():
        src = os.path.join(PACK, rel)
        if not os.path.exists(src):
            print('MISSING (skipped):', src)
            continue
        dst = os.path.join(tmp, name + '.png')
        pow2(Image.open(src).convert('RGBA')).save(dst)  # palette PNGs make ImageToPAA fail; pad to 2^n
        pngs.append(dst)
    for p in pngs:
        paa = os.path.join(OUT, os.path.basename(p)[:-4] + '.paa')
        r = subprocess.run([IMAGE_TO_PAA, p, paa], capture_output=True, text=True)
        ok = r.returncode == 0 and os.path.exists(paa)
        print(('ok      ' if ok else 'FAILED  ') + os.path.relpath(paa, REPO), '' if ok else (r.stdout + r.stderr)[-300:])
        if not ok:
            sys.exit(1)
    shutil.rmtree(tmp)


if __name__ == '__main__':
    main()
