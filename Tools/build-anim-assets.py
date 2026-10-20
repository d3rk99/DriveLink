# Builds the road-animation textures into Addons/DriveLink/gui/anim/ (Research/road-animation-research.md).
#   - DriveLink's own pixel art (drawn here, committed as source): the top-down ground tile
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


def ground_tile():
    # 256 x 256, loops top-to-bottom (top-down view like MiniDayZ): a road down the middle with a dashed centre line,
    # dirt shoulders, grass with speckle, tree crowns seen from above (drawn at 128x128, x2)
    im = Image.new('RGBA', (128, 128), (88, 116, 56, 255))
    d = ImageDraw.Draw(im)
    rnd = random.Random(21)
    for _ in range(700):                                        # grass speckle
        x, y = rnd.randrange(128), rnd.randrange(128)
        im.putpixel((x, y), rnd.choice([(80, 106, 50, 255), (98, 126, 62, 255), (74, 98, 46, 255)]))
    d.rectangle([44, 0, 47, 127], fill=(122, 108, 80, 255))     # left shoulder
    d.rectangle([80, 0, 83, 127], fill=(122, 108, 80, 255))     # right shoulder
    d.rectangle([48, 0, 79, 127], fill=(64, 66, 70, 255))       # asphalt
    for y in range(0, 128, 16):                                 # dashed centre line, period 16 -> seamless at 128
        d.rectangle([63, y, 64, y + 7], fill=(205, 196, 150, 255))
    for _ in range(160):
        x, y = rnd.randrange(48, 80), rnd.randrange(128)
        if not (62 <= x <= 65):
            im.putpixel((x, y), rnd.choice([(56, 58, 62, 255), (72, 74, 78, 255)]))
    for side in ((2, 38), (90, 124)):                           # tree crowns, wrapped vertically so the tile loops
        for _ in range(7):
            cx, cy, r = rnd.randint(*side), rnd.randrange(128), rnd.randint(5, 9)
            for oy in (-128, 0, 128):
                d.ellipse([cx - r + 2, cy - r + 3 + oy, cx + r + 2, cy + r + 3 + oy], fill=(52, 72, 40, 255))   # shadow
                d.ellipse([cx - r, cy - r + oy, cx + r, cy + r + oy], fill=(42, 82, 44, 255))
                d.ellipse([cx - r + 2, cy - r + 1 + oy, cx + r - 3, cy + r - 4 + oy], fill=(56, 100, 52, 255))  # light
    return im.resize((256, 256), Image.NEAREST)


def main():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(OWN_SRC, exist_ok=True)
    pngs = []
    for name, fn in (('ground_ca', ground_tile),):
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
