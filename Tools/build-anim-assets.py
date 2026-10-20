# Builds the road-animation textures into Addons/DriveLink/gui/anim/ (Research/road-animation-research.md).
#   - DriveLink's own pixel-art strips (drawn here, committed as source): road, tree line
#   - MiniDayZ sprite sheets copied from the local pack (C:\DayZMods\_assets\minidayz): PROTOTYPE ONLY, never
#     committed (gitignored) until Bohemia OKs them or they are replaced
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
    'skin_biker_ca': r'people\player\skins\player_skin_biker-sheet0.png',
}


def road_strip():
    # 256 x 64, tiles left-right: grass verge, asphalt with a dashed centre line, grass verge (drawn at 128x32, x2)
    im = Image.new('RGBA', (128, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rnd = random.Random(7)
    d.rectangle([0, 0, 127, 7], fill=(86, 112, 52, 255))        # far verge
    d.rectangle([0, 8, 127, 9], fill=(120, 108, 84, 255))       # kerb / dirt edge
    d.rectangle([0, 10, 127, 25], fill=(62, 64, 68, 255))       # asphalt
    d.rectangle([0, 26, 127, 27], fill=(120, 108, 84, 255))
    d.rectangle([0, 28, 127, 31], fill=(78, 104, 46, 255))      # near verge
    for x in range(0, 128, 16):                                 # dashed line, period 16 -> seamless at 128
        d.rectangle([x, 17, x + 7, 18], fill=(205, 196, 150, 255))
    for _ in range(90):                                         # asphalt speckle + grass tufts
        x, y = rnd.randrange(128), rnd.randrange(10, 26)
        im.putpixel((x, y), (72, 74, 78, 255) if rnd.random() < 0.5 else (54, 56, 60, 255))
    for _ in range(60):
        x, y = rnd.randrange(128), rnd.choice(list(range(0, 8)) + list(range(28, 32)))
        im.putpixel((x, y), (104, 132, 62, 255))
    return im.resize((256, 64), Image.NEAREST)


def treeline_strip():
    # 256 x 64, tiles left-right: a band of pine/birch silhouettes over a pale sky edge (transparent above)
    im = Image.new('RGBA', (128, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rnd = random.Random(11)
    d.rectangle([0, 24, 127, 31], fill=(46, 70, 40, 255))       # ground under the trees
    x = 0
    while x < 128:
        h = rnd.randint(12, 22)
        w = rnd.randint(5, 8)
        col = rnd.choice([(34, 58, 34, 255), (40, 66, 38, 255), (30, 50, 32, 255)])
        top = 28 - h
        for i in range(h):                                     # pine: triangle
            half = max(1, int((i + 1) * w / (2 * h)))
            cx = x + w // 2
            d.line([((cx - half) % 128, top + i), ((cx + half) % 128, top + i)], fill=col)
        x += rnd.randint(4, 8)
    return im.resize((256, 64), Image.NEAREST)


def main():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(OWN_SRC, exist_ok=True)
    pngs = []
    for name, fn in (('road_ca', road_strip), ('trees_ca', treeline_strip)):
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
        Image.open(src).convert('RGBA').save(dst)  # palette PNGs (the skins) make ImageToPAA fail
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
