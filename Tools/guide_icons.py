"""Tools/guide_icons.py: the travel guide's item pictures (DL_Guide.c) -> Addons/DriveLink/gui/anim/guide_items_ca.paa.
The pictures are the DayZ wiki's item images (dayz.fandom.com, game renders of DayZ's items), saved as
Art/guide/<item class>.png. Each is trimmed to the item, fitted into a 128 x 128 cell; the atlas is 1024 x 512, cells
in ITEMS order left to right, 8 to a row (DL_Guide.ICONS has the same order).
Usage: python Tools/guide_icons.py"""
import os
import subprocess
from PIL import Image

ITEMS = ['BakedBeansCan', 'WaterBottle', 'CanisterGasoline', 'TireRepairKit', 'BandageDressing', 'HuntingKnife',
         'Compass', 'GPSReceiver', 'Rope', 'EpoxyPutty', 'DuctTape', 'Shovel',
         'Mosin9130', 'FishingRod', 'Matchbox', 'PetrolLighter', 'Canteen', 'Headtorch_Grey',
         'CharcoalTablets', 'TetracyclineAntibiotics', 'GasMask', 'Heatpack', 'FirstAidKit', 'Flashlight']
CELL = 128
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(HERE, 'Art', 'guide')
TOOL = r'E:\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe'


def item(path):
    im = Image.open(path).convert('RGBA')
    px = im.load()
    w, h = im.size
    if im.getextrema()[3][0] == 255:                  # no transparency: a flat light background is cut away
        bg = px[0, 0]
        for y in range(h):
            for x in range(w):
                r, g, b, a = px[x, y]
                if abs(r - bg[0]) + abs(g - bg[1]) + abs(b - bg[2]) < 24:
                    px[x, y] = (r, g, b, 0)
    box = im.getbbox()
    if box:
        im = im.crop(box)
    im.thumbnail((CELL - 8, CELL - 8), Image.LANCZOS)
    cell = Image.new('RGBA', (CELL, CELL), (0, 0, 0, 0))
    cell.alpha_composite(im, ((CELL - im.width) // 2, (CELL - im.height) // 2))
    return cell


def main():
    at = Image.new('RGBA', (1024, 512), (0, 0, 0, 0))
    for i, t in enumerate(ITEMS):
        at.alpha_composite(item(os.path.join(SRC, t + '.png')), ((i % 8) * CELL, (i // 8) * CELL))
    png = os.path.join(SRC, 'guide_items_ca.png')
    at.save(png)
    paa = os.path.join(HERE, 'Addons', 'DriveLink', 'gui', 'anim', 'guide_items_ca.paa')
    r = subprocess.run([TOOL, png, paa], capture_output=True, text=True)
    print(('ok      ' if r.returncode == 0 and os.path.exists(paa) else 'FAILED  ') + paa)


if __name__ == '__main__':
    main()
