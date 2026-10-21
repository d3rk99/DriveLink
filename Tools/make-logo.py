"""DriveLink's logo for mod.cpp (the game's mod list / launcher): our own art only - the road film's road tile and
the M1025 (Tools/anim_humvee.py) on it, the name across the bottom. Writes Art/logo_ca.png (256 x 256); the asset
build turns it into Addons/DriveLink/gui/logo_ca.paa."""
import os
import sys
from PIL import Image, ImageDraw, ImageFont

TOOLS = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(TOOLS)
sys.path.insert(0, TOOLS)
import anim_humvee  # noqa: E402


def font(size):
    for f in ('segoeuib.ttf', 'arialbd.ttf', 'DejaVuSans-Bold.ttf'):
        try:
            return ImageFont.truetype(f, size)
        except OSError:
            pass
    return ImageFont.load_default()


def logo():
    road = Image.open(os.path.join(REPO, 'Art', 'anim', 'ground_field_ca.png')).convert('RGBA')
    im = road.copy()
    car = anim_humvee.draw('camo')
    car = car.resize((car.width * 2, car.height * 2), Image.NEAREST)
    sh = Image.new('RGBA', car.size, (0, 0, 0, 0))
    sh.paste(Image.new('RGBA', car.size, (0, 0, 0, 90)), (0, 0), car.split()[3])
    x, y = 128 - car.width // 2, 18
    im.alpha_composite(sh, (x + 8, y + 6))
    im.alpha_composite(car, (x, y))
    # the name on a dark band
    band = Image.new('RGBA', (256, 64), (12, 14, 16, 215))
    im.alpha_composite(band, (0, 192))
    d = ImageDraw.Draw(im)
    f = font(40)
    w = d.textlength('DriveLink', font=f)
    d.text(((256 - w) / 2 + 2, 200 + 2), 'DriveLink', font=f, fill=(0, 0, 0, 255))
    d.text(((256 - w) / 2, 200), 'DriveLink', font=f, fill=(240, 236, 226, 255))
    d.line((40, 248, 216, 248), fill=(214, 168, 64, 255), width=2)
    return im


if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, 'Art', 'logo_ca.png')
    logo().save(out)
    print(out)
