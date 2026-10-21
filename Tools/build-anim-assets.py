# Builds the road-film textures into Addons/DriveLink/gui/anim/ (Research/road-animation-research.md).
#   - DriveLink's own pixel art (drawn here, committed as source in Art/anim): one top-down ground tile per biome
#     (forest, field, desert, snow) with a 2-lane road, and a props atlas (trees, bushes, rocks, fence, sign, pole...)
#   - MiniDayZ sheets copied from the local pack (C:\DayZMods\_assets\minidayz): PROTOTYPE ONLY, never committed
#     (gitignored) until Bohemia OKs them or they are replaced
# Every texture is a power-of-two PNG named *_ca.png, converted to .paa with DayZ Tools ImageToPAA (DXT5 + alpha).
# Usage: python Tools/build-anim-assets.py
import os, shutil, subprocess, sys, random
import anim_boat   # Tools/anim_boat.py: the inflatable Boat_01
import anim_humvee # Tools/anim_humvee.py: the M1025 (Offroad_02)
import anim_station  # Tools/anim_station.py: the gas station lot + canopy roof (MiniDayZ buildings)
import anim_bikes  # Tools/anim_bikes.py: the dirt bike (Motorbike_02), the moped (Motorbike_01), the boat wake
from PIL import Image, ImageDraw

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(REPO, 'Addons', 'DriveLink', 'gui', 'anim')
OWN_SRC = os.path.join(REPO, 'Art', 'anim')
PACK = r'C:\DayZMods\_assets\minidayz\minidayz-assets'
IMAGE_TO_PAA = r'E:\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe'

MINIDAYZ = {  # texture name -> file in the pack
    'waterpump_ca': r'buildings\waterpump-sheet0.png',               # the road film's well find
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


def water_tile():
    """256 x 256, loops top-to-bottom: open water for boat trips (PLACEHOLDER art) - a deep blue with lighter wave
    streaks and a few glints"""
    base = (44, 84, 118)
    im = Image.new('RGBA', (128, 128), base + (255,))
    d = ImageDraw.Draw(im)
    rnd = random.Random(42)
    for _ in range(700):
        v = rnd.choice([-6, 6])
        im.putpixel((rnd.randrange(128), rnd.randrange(128)), (base[0] + v, base[1] + v, base[2] + v, 255))
    for _ in range(46):                                         # wave streaks, wrapped so the tile loops
        x, y, ln = rnd.randrange(128), rnd.randrange(128), rnd.randint(4, 11)
        for i in range(ln):
            im.putpixel(((x + i) % 128, y), (70, 116, 150, 255))
            if i % 3 == 0:
                im.putpixel(((x + i) % 128, (y + 1) % 128), (58, 102, 136, 255))
    for _ in range(14):
        im.putpixel((rnd.randrange(128), rnd.randrange(128)), (150, 190, 214, 255))
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


def no_outline(im):
    """MiniDayZ item icons have a 1-px white outline (looks like a sticker on the road): the white pixels that touch
    transparency become transparent"""
    px = im.load()
    w, h = im.size
    edge = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a > 0 and min(r, g, b) > 235:
                near = [(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
                if any(not (0 <= nx < w and 0 <= ny < h) or px[nx, ny][3] == 0 for nx, ny in near):
                    edge.append((x, y))
    for x, y in edge:
        px[x, y] = (0, 0, 0, 0)
    return im


def grey_copy(im, ref_pct=0.75, white=225):
    """grey version brightened so the paint is near white: the mod tints it (SetColor) with the car's DayZ colour"""
    px = im.load()
    lum = sorted(0.3 * r + 0.59 * g + 0.11 * b for r, g, b, a in im.getdata() if a > 200)
    k = white / max(lum[int(len(lum) * ref_pct)], 1)
    out = im.copy()
    op = out.load()
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = px[x, y]
            if a > 0:
                v = min(255, int((0.3 * r + 0.59 * g + 0.11 * b) * k))
                op[x, y] = (v, v, v, a)
    return out


def clean_paint(im, box):
    """paints over the white graffiti inside box (x0, y0, x1, y1) with the surrounding body colour"""
    px = im.load()
    x0, y0, x1, y1 = box
    for _ in range(3):
        todo = []
        for y in range(y0, y1):
            for x in range(x0, x1):
                r, g, b, a = px[x, y]
                if a > 200 and min(r, g, b) > 150 and max(r, g, b) - min(r, g, b) < 60:
                    near = [px[x + dx, y + dy] for dx in range(-3, 4) for dy in range(-3, 4)
                            if 0 <= x + dx < im.width and 0 <= y + dy < im.height]
                    body = [c for c in near if c[3] > 200 and not (min(c[:3]) > 150 and max(c[:3]) - min(c[:3]) < 60)]
                    if body:
                        todo.append((x, y, tuple(sorted(c[i] for c in body)[len(body) // 2] for i in range(3))))
        for x, y, c in todo:
            px[x, y] = c + (255,)
    return im


def shade(rgb, f, add=0):
    return tuple(max(0, min(255, int(c * f + add))) for c in rgb)


def top_truck(rgb):
    """DriveLink's own top-down M3S (MiniDayZ has its truck side-on only), after the in-game truck seen from above:
    an off-white canvas bed (the same on every truck) over the rear twin axle, the painted cab with its round roof
    hatch, a two-pane windscreen and mirrors, a tapering bonnet with vents between wide front wings, headlights and
    a bumper. Tyres are drawn first, so they show only where they stick out from under the body. 52 x 108, front down."""
    W, H = 52, 108
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rnd = random.Random(11)
    tyre, tyre_hi = (34, 34, 34, 255), (58, 58, 58, 255)
    paint, dark, light, line = shade(rgb, 1), shade(rgb, 0.72), shade(rgb, 1.15, 12), shade(rgb, 0.42)

    def wheel(x0, y0, x1, y1):
        d.rounded_rectangle((x0, y0, x1, y1), radius=2, fill=tyre)
        for y in range(y0 + 1, y1, 3):
            d.line((x0, y, x1, y), fill=tyre_hi)               # tread

    # tyres: rear twin axle under the bed, front axle under the wings
    for y0 in (9, 24):                                        # twin rear axle: long tyres, only 1 px out at the side
        wheel(1, y0, 5, y0 + 13)
        wheel(W - 6, y0, W - 2, y0 + 13)
    wheel(1, 83, 5, 100)
    wheel(W - 6, 83, W - 2, 100)
    # chassis between bed and cab
    d.rectangle((7, 54, W - 8, 60), fill=(44, 44, 42, 255))

    # the canvas bed
    cv, cv_dark, cv_light, cv_line = (212, 204, 182), (178, 170, 148), (230, 224, 206), (96, 90, 76)
    x0, y0, x1, y1 = 4, 0, W - 5, 55
    d.rounded_rectangle((x0, y0, x1, y1), radius=6, fill=cv_line + (255,))
    d.rounded_rectangle((x0 + 1, y0 + 1, x1 - 1, y1 - 1), radius=5, fill=cv + (255,))
    for by in range(y0 + 4, y1 - 4, 11):                     # the canvas bulges out between the hoops
        d.ellipse((x0 - 1, by, x0 + 3, by + 8), fill=cv_line + (255,))
        d.ellipse((x0, by + 1, x0 + 4, by + 7), fill=cv + (255,))
        d.ellipse((x1 - 3, by, x1 + 1, by + 8), fill=cv_line + (255,))
        d.ellipse((x1 - 4, by + 1, x1, by + 7), fill=cv + (255,))
    for hy in range(y0 + 9, y1 - 3, 11):                     # hoops under the canvas, sagging between them
        d.line((x0 + 2, hy, x1 - 2, hy), fill=cv_light + (255,))
        d.line((x0 + 2, hy + 1, x1 - 2, hy + 1), fill=cv_dark + (255,))
        d.line((x0 + 6, hy + 5, x1 - 6, hy + 5), fill=shade(cv, 0.94) + (255,))
    d.line((x0 + 1, y0 + 5, x0 + 1, y1 - 6), fill=cv_dark + (255,))   # the canvas falls over the sides
    d.line((x1 - 1, y0 + 5, x1 - 1, y1 - 6), fill=cv_dark + (255,))
    d.rounded_rectangle((x0 + 3, y0 + 1, x1 - 3, y0 + 4), radius=2, fill=cv_dark + (255,))  # rear flap
    for _ in range(26):                                       # dirt
        px_, py_ = rnd.randint(x0 + 3, x1 - 3), rnd.randint(y0 + 4, y1 - 3)
        d.point((px_, py_), fill=shade(cv, 0.86) + (255,))
    d.line((x0 + 5, y1 - 1, x1 - 5, y1 - 1), fill=shade(cv, 0.8) + (255,))

    # the cab
    cx0, cx1 = 5, W - 6
    d.rounded_rectangle((cx0, 58, cx1, 80), radius=5, fill=line + (255,))
    d.rounded_rectangle((cx0 + 1, 59, cx1 - 1, 79), radius=4, fill=paint + (255,))
    d.line((cx0 + 5, 59, cx1 - 5, 59), fill=light + (255,))
    d.line((cx0 + 2, 75, cx1 - 2, 75), fill=dark + (255,))
    d.ellipse((11, 62, 21, 72), fill=dark + (255,))           # roof hatch
    d.ellipse((12, 63, 20, 71), fill=light + (255,))
    d.ellipse((13, 64, 19, 70), fill=paint + (255,))
    # windscreen (two panes) and mirrors
    glass, glass_hi = (46, 50, 58, 255), (84, 92, 104, 255)
    d.rounded_rectangle((cx0 + 2, 76, 24, 81), radius=2, fill=glass)
    d.rounded_rectangle((27, 76, cx1 - 2, 81), radius=2, fill=glass)
    d.line((cx0 + 3, 77, cx0 + 6, 77), fill=glass_hi)
    d.line((28, 77, 31, 77), fill=glass_hi)
    for mx in (1, W - 4):
        d.rectangle((mx, 77, mx + 2, 80), fill=(50, 50, 52, 255))
    d.line((3, 78, cx0, 78), fill=(70, 70, 72, 255))
    d.line((cx1, 78, W - 4, 78), fill=(70, 70, 72, 255))
    # front wings (wide) and the bonnet (tapering) between them
    for wx0, wx1 in ((2, 18), (W - 19, W - 3)):              # the wings, round over the wheels
        d.rounded_rectangle((wx0, 86, wx1, 103), radius=6, fill=line + (255,))
        d.rounded_rectangle((wx0 + 1, 87, wx1 - 1, 102), radius=5, fill=paint + (255,))
        d.line((wx0 + 5, 87, wx1 - 5, 87), fill=light + (255,))
    d.polygon([(cx0 + 1, 80), (cx1 - 1, 80), (W - 12, 99), (11, 99)], fill=line + (255,))
    d.rounded_rectangle((11, 94, W - 12, 102), radius=4, fill=line + (255,))
    d.polygon([(cx0 + 2, 80), (cx1 - 2, 80), (W - 13, 98), (12, 98)], fill=paint + (255,))
    d.rounded_rectangle((12, 94, W - 13, 101), radius=3, fill=paint + (255,))
    d.line((W // 2 - 1, 84, W // 2 - 1, 99), fill=dark + (255,))   # bonnet crease
    d.line((W // 2, 84, W // 2, 99), fill=light + (255,))
    for vy in range(90, 99, 2):                               # vents on the sides of the bonnet
        d.line((13, vy, 16, vy), fill=line + (255,))
        d.line((W - 17, vy, W - 14, vy), fill=line + (255,))
    d.rounded_rectangle((17, 97, W - 18, 101), radius=1, fill=(40, 40, 40, 255))   # grille
    for gx in range(18, W - 18, 2):
        d.point((gx, 99), fill=(90, 90, 90, 255))
    d.ellipse((5, 96, 9, 100), fill=(236, 226, 168, 255))    # headlights
    d.ellipse((W - 10, 96, W - 6, 100), fill=(236, 226, 168, 255))
    # bumper
    d.rounded_rectangle((3, 102, W - 4, 106), radius=2, fill=(58, 58, 58, 255))
    d.line((5, 102, W - 6, 102), fill=(92, 92, 92, 255))
    # a little wear, like the MiniDayZ cars (less is more): a few rust specks on the paint, one dent in the left wing
    rust, rust_dark, rust_light = (122, 70, 38, 255), (84, 48, 30, 255), (150, 96, 56, 255)
    px = im.load()
    # rust where it starts on a real truck: the wing edges, a cab corner, under the windscreen, round the hatch -
    # a few ragged patches (dark core, lighter rim, a speck or two around), only on paint
    rr = random.Random(5)
    for bx, by, rx, ry in ((44, 92, 2.6, 2.0), (7, 98, 2.4, 1.8), (41, 75, 2.8, 1.6), (13, 83, 1.4, 2.6), (22, 67, 1.6, 1.4)):
        for y in range(int(by - ry - 2), int(by + ry + 3)):
            for x in range(int(bx - rx - 2), int(bx + rx + 3)):
                if not (0 <= x < W and 0 <= y < H) or px[x, y][:3] not in (paint, dark, light):
                    continue
                e = ((x - bx) / rx) ** 2 + ((y - by) / ry) ** 2 + rr.uniform(-0.35, 0.35)
                if e < 0.35:
                    px[x, y] = rust_dark
                elif e < 0.8:
                    px[x, y] = rust
                elif e < 1.15:
                    px[x, y] = rust_light
                elif e < 1.9 and rr.random() < 0.18:
                    px[x, y] = rust                         # a stray speck near the patch
    d.arc((6, 90, 11, 95), 200, 340, fill=dark + (255,))     # the dent: a shadowed hollow with a lit lower lip
    d.point((8, 94), fill=light + (255,))
    return im


def top_bike(rgb):
    """PLACEHOLDER top-down motorbike, front at the bottom: rear wheel, seat, tank, handlebar, front wheel. 20 x 46"""
    W, H = 20, 46
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    paint, dark, light, line = shade(rgb, 1), shade(rgb, 0.7), shade(rgb, 1.15, 12), shade(rgb, 0.42)
    tyre = (30, 30, 30, 255)
    d.rounded_rectangle((7, 0, 12, 12), radius=2, fill=tyre)                # rear wheel
    d.rounded_rectangle((7, 34, 12, 45), radius=2, fill=tyre)               # front wheel
    d.rounded_rectangle((5, 9, 14, 19), radius=3, fill=(44, 40, 38, 255))   # seat
    d.rounded_rectangle((4, 18, 15, 30), radius=4, fill=line + (255,))      # tank
    d.rounded_rectangle((5, 19, 14, 29), radius=3, fill=paint + (255,))
    d.line((7, 20, 7, 27), fill=light + (255,))
    d.line((12, 20, 12, 27), fill=dark + (255,))
    d.rectangle((0, 30, 19, 31), fill=(70, 70, 74, 255))                    # handlebar
    d.rectangle((0, 30, 1, 32), fill=(30, 30, 30, 255))
    d.rectangle((18, 30, 19, 32), fill=(30, 30, 30, 255))
    d.ellipse((8, 32, 11, 35), fill=(236, 226, 168, 255))                   # headlight
    d.rectangle((13, 4, 15, 9), fill=(150, 150, 150, 255))                  # exhaust
    return im


BIKE_FRONT_ROW = 27   # rows from here down (tank front, handlebar, front wheel) are drawn over the rider


def bike_front(im):
    """the bike's front, to draw over its rider (the rider sits on the seat, the tank and bars are in front)"""
    out = Image.new('RGBA', im.size, (0, 0, 0, 0))
    px, op = im.load(), out.load()
    for y in range(BIKE_FRONT_ROW, im.height):
        for x in range(im.width):
            c = px[x, y]
            if c[3] == 255:                                     # the body only, not its shadow
                op[x, y] = c
    return out


def boat_rim(im):
    """the boat's rim (hull sides, bow, stern, engine) to draw over the crouching crew: they sit IN the boat"""
    out = Image.new('RGBA', im.size, (0, 0, 0, 0))
    px, op = im.load(), out.load()
    floor = {(168, 160, 142), (118, 96, 70), (146, 122, 92)}    # top_boat's floor and benches
    for y in range(im.height):
        for x in range(im.width):
            c = px[x, y]
            if c[3] == 255 and c[:3] not in floor:
                op[x, y] = c
    return out


def top_boat(rgb):
    """PLACEHOLDER top-down boat (Boat_01), bow at the bottom: a painted hull, a light floor with 3 benches, the
    outboard engine sticking out at the stern. 34 x 80"""
    W, H = 34, 80
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    paint, dark, light, line = shade(rgb, 1), shade(rgb, 0.7), shade(rgb, 1.15, 12), shade(rgb, 0.42)
    hull = [(2, 6), (31, 6), (31, 54), (17, 78), (16, 78), (2, 54)]
    d.polygon(hull, fill=line + (255,))
    d.polygon([(3, 7), (30, 7), (30, 54), (17, 76), (16, 76), (3, 54)], fill=paint + (255,))
    d.polygon([(6, 9), (27, 9), (27, 53), (17, 69), (16, 69), (6, 53)], fill=(168, 160, 142, 255))  # floor
    for y in (16, 32, 46):                                                  # benches
        d.rectangle((6, y, 27, y + 3), fill=(118, 96, 70, 255))
        d.line((6, y, 27, y), fill=(146, 122, 92, 255))
    d.line((3, 7, 3, 54), fill=light + (255,))
    d.line((30, 7, 30, 54), fill=dark + (255,))
    d.rounded_rectangle((12, 0, 21, 8), radius=2, fill=(36, 36, 38, 255))   # outboard engine
    d.rectangle((15, 0, 18, 2), fill=(90, 90, 92, 255))
    return im


# the paints each car comes in (DayZ class-name colours + the default), RGB of the paint's middle tone
CAR_PAINTS = {
    'sedan': {'white': (226, 224, 214), 'black': (52, 52, 58), 'wine': (118, 30, 44), 'yellow': (222, 180, 48),
              'grey': (138, 138, 138), 'red': (172, 44, 38), 'blue': (58, 88, 156)},
    '4x4': {'green': (90, 120, 72), 'blue': (60, 90, 152), 'white': (226, 226, 220), 'olive': (102, 110, 70),
            'beige': (188, 168, 124), 'grey': (138, 138, 138)},
    'truck': {'green': (92, 108, 70), 'blue': (68, 96, 162), 'orange': (206, 110, 42), 'grey': (138, 138, 138)},
    'bike': {'red': (0, 0, 0), 'green': (0, 0, 0), 'blue': (0, 0, 0), 'yellow': (0, 0, 0), 'grey': (0, 0, 0)},      # anim_bikes.DIRT
    'moped': {'red': (0, 0, 0), 'yellow': (0, 0, 0), 'blue': (0, 0, 0), 'grey': (0, 0, 0)},                         # anim_bikes.MOPED
    'humvee': {'camo': (0, 0, 0), 'beige': (0, 0, 0), 'grey': (0, 0, 0)},                                         # anim_humvee
    'boat': {'blue': (52, 86, 150), 'black': (52, 52, 58), 'orange': (210, 112, 40), 'camo': (96, 104, 70), 'grey': (138, 138, 138)},
}


def is_paint(c, base):
    """the body paint of the source sprite (not glass, tyres, chrome, rust or outline)"""
    r, g, b, a = c
    if a < 250:
        return False
    lum = 0.3 * r + 0.59 * g + 0.11 * b
    mx, mn = max(r, g, b), min(r, g, b)
    sat = (mx - mn) / mx if mx else 0
    if base == 'sedan':                 # the Volga is blue
        return b > r + 25 and b > g and sat > 0.25 and lum > 35
    # the UAZ (and the truck's cab) are beige; the truck canvas is greyish green
    return lum > 70 and sat > 0.12 and r >= b and g >= b


def paint(im, base, rgb):
    """repaints the body pixels: the target colour plus each pixel's difference from the paint's middle tone (so dents,
    panel lines, highlights stay), everything else (glass, trim, tyres, rust) untouched"""
    px = im.load()
    paint_l = sorted(0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2] for c in (px[x, y] for y in range(im.height)
                     for x in range(im.width)) if is_paint(c, base))
    ref = paint_l[len(paint_l) // 2] if paint_l else 128
    out = im.copy()
    op = out.load()
    for y in range(im.height):
        for x in range(im.width):
            c = px[x, y]
            if is_paint(c, base):
                d = (0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2]) - ref
                op[x, y] = tuple(max(0, min(255, int(t + d * 0.85))) for t in rgb) + (255,)
    return out


def reshadow(im, dx=6, dy=4, alpha=90):
    """drops the source's own shadow (uneven, with fringes on the left) and casts one even shadow from the body,
    down-right, the same for every car; the sheet grows by (dx, dy)"""
    body = Image.new('RGBA', im.size, (0, 0, 0, 0))
    bp, ip = body.load(), im.load()
    for y in range(im.height):
        for x in range(im.width):
            c = ip[x, y]
            if c[3] >= 250 and not (c[0] < 8 and c[1] < 8 and c[2] < 8):
                bp[x, y] = c
    box = body.getbbox()
    body = body.crop(box)
    out = Image.new('RGBA', (body.width + dx, body.height + dy), (0, 0, 0, 0))
    mask = body.split()[3].point(lambda v: alpha if v else 0)
    out.paste(Image.new('RGBA', body.size, (0, 0, 0, 255)), (dx, dy), mask)
    out.alpha_composite(body)
    return out


def car_sprites(tmp):
    """the player's car in the film, painted here in every colour it comes in (CAR_PAINTS):
         car_sedan_<paint>  the drivable Volga (graffiti painted out): Olga 24, Sarka 120, Gunter 2
         car_4x4_<paint>    the UAZ: Ada 4x4
         car_humvee_<paint> DriveLink's own M1025 (anim_humvee), camo / beige
         car_truck_<paint>  DriveLink's own top-down M3S (top_truck), white canvas on every paint
       each sprite at (0, 0) of its sheet with one even shadow down-right, front at the bottom; sizes in
       DL_FilmPlayer.BuildCar"""
    made = []
    volga = Image.open(os.path.join(PACK, MINIDAYZ['volga_ca'])).convert('RGBA').crop((178, 142, 240, 222))
    volga = clean_paint(volga, (4, 4, 56, 66))
    uaz = Image.open(os.path.join(PACK, MINIDAYZ['uaz_ca'])).convert('RGBA').crop((176, 80, 244, 154))
    drawn = {'truck': top_truck, 'bike': top_bike}
    for base, im in (('sedan', volga), ('4x4', uaz), ('truck', None), ('humvee', None), ('bike', None), ('moped', None), ('boat', None)):
        for name, rgb in CAR_PAINTS[base].items():
            if base in ('boat', 'bike', 'moped'):
                # drawn per variant with its own colours; the second picture is drawn over the crew (the boat's motor,
                # a bike's bars + front)
                if base == 'boat':
                    body, motor = anim_boat.draw(name)
                elif base == 'bike':
                    body, motor = anim_bikes.draw_dirt(name)
                else:
                    body, motor = anim_bikes.draw_moped(name)
                car = reshadow(body)
                over = Image.new('RGBA', car.size, (0, 0, 0, 0))
                over.alpha_composite(motor.crop(body.getbbox()), (0, 0))
                dst = os.path.join(tmp, 'car_%s_%s_ca.png' % (base, name))
                pow2(car).save(dst)
                made.append(dst)
                odst = os.path.join(tmp, 'car_%s%s_%s_ca.png' % (base, 'rim' if base == 'boat' else 'front', name))
                pow2(over).save(odst)
                made.append(odst)
                if name == next(iter(CAR_PAINTS[base])):
                    print('car', base, car.size)
                continue
            if base == 'humvee':
                car = reshadow(anim_humvee.draw(name))
            elif base in drawn:
                car = reshadow(drawn[base](rgb))
            else:
                car = paint(reshadow(im), base, rgb)
            if name == next(iter(CAR_PAINTS[base])):
                print('car', base, car.size)
            dst = os.path.join(tmp, 'car_%s_%s_ca.png' % (base, name))
            pow2(car).save(dst)
            made.append(dst)
            # open vehicles: the part drawn OVER the riders (same sheet size and place as the body)
            over = None
            if base == 'bike':
                over = bike_front(car)

            if over is not None:
                odst = os.path.join(tmp, 'car_%s%s_%s_ca.png' % (base, 'front' if base == 'bike' else 'rim', name))
                pow2(over).save(odst)
                made.append(odst)
    return made


def main():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(OWN_SRC, exist_ok=True)
    pngs = []
    own = [('ground_%s_ca' % b, (lambda b=b: ground_tile(b))) for b in BIOMES] + [('props_ca', props_atlas), ('ground_water_ca', water_tile)]
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
        im = Image.open(src).convert('RGBA')
        if name == 'fueltank_ca':
            im = no_outline(im)
        pow2(im).save(dst)  # palette PNGs make ImageToPAA fail; pad to 2^n
        pngs.append(dst)
    pngs += car_sprites(tmp)
    for name, im in (('gaslot_ca', anim_station.draw_lot('field')), ('gaslot_snow_ca', anim_station.draw_lot('snow')),
                     ('gasroof_ca', anim_station.draw_roof())):
        dst = os.path.join(tmp, name + '.png')     # MiniDayZ-derived: built locally, not committed
        pow2(im).save(dst)
        pngs.append(dst)
    wake = os.path.join(tmp, 'wake_ca.png')
    pow2(anim_bikes.wake_frames()).save(wake)   # 4 frames of 96 x 128 (512 x 128 sheet)
    pngs.append(wake)
    for p in pngs:
        paa = os.path.join(OUT, os.path.basename(p)[:-4] + '.paa')
        r = subprocess.run([IMAGE_TO_PAA, p, paa], capture_output=True, text=True)
        ok = r.returncode == 0 and os.path.exists(paa)
        print(('ok      ' if ok else 'FAILED  ') + os.path.relpath(paa, REPO), '' if ok else (r.stdout + r.stderr)[-300:])
        if not ok:
            sys.exit(1)
    shutil.rmtree(tmp) if not os.environ.get("KEEP") else None


if __name__ == '__main__':
    main()
