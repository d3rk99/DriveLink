"""DriveLink's interactive trail cards: the things the card scenes put on the road (DL_FilmPlayer card / act scenes).
  trail_ca = 4x4 cells of 64x64 (drawn at 32x32, x2 like the props atlas):
    row 0  deer facing left: standing, running 1, running 2, down
    row 1  the same facing right
    row 2  road barrier (striped beam on trestles), sandbags, campfire 1, campfire 2
    row 3  steam 1, steam 2, splash, meat
  river_ca = 256 x 1024: 4 frames of the river across the road with the broken bridge end, river_props = 256 x 64:
    sign, log, reeds, debris (both cut by Tools/anim_river_cut.py); river_snow_ca, river_desert_ca the same banks
    in snow and sand; splash_ca = 256 x 256: the ford's wake and side sprays, 4 frames each
  trail2_ca = 4x4 cells of 64x64 (drawn at 32x32, x2):
    row 0  signpost (rusted blank), "?" bubble, sick bubble, helicopter wreck
    row 1  fishing rod, fish, mud patch, headlight beam
    row 2  gas cloud 1, gas cloud 2, mud spray, "!" bubble
    row 3  compass bubble, broken signpost, waypoint ring (cells 0 and 1 + row 3 from Tools/anim_fork_cut.py)
  crossroad_ca = 256 x 128: a side road (the fork); lake_ca = 128 x 128: a pond; snow_ca = 256 x 256: blowing snow
  heli_ca = 128 x 128: the crashed helicopter with its shadow (the heli crash card)
  night_ca = 256 x 256: the darkness with the headlights cut out, and the headlights' glow (car, bike)
  barrier_ca = 256 x 256: the roadblock pieces (barriers, sandbags, junk pile, spike strip), cut by Tools/anim_roadblock_cut.py
Used by Tools/build-anim-assets.py; run alone to build just these two (PNG in Art/anim, .paa in gui/anim)."""
import os
import subprocess
from PIL import Image, ImageDraw

SH = (0, 0, 0, 70)
FUR = (150, 104, 62, 255)
FUR_D = (112, 76, 44, 255)
BELLY = (214, 190, 150, 255)
HOOF = (52, 40, 30, 255)
ANTLER = (196, 176, 140, 255)


def deer(frame):
    """side on, facing left: 0 standing (head up), 1 + 2 running, 3 down"""
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    if frame == 3:
        d.ellipse([6, 20, 28, 28], fill=SH)
        d.ellipse([7, 17, 25, 25], fill=FUR)
        d.ellipse([10, 21, 23, 25], fill=BELLY)
        d.ellipse([3, 19, 10, 24], fill=FUR)               # the head on the ground
        d.point((4, 21), fill=(20, 20, 20, 255))
        for x in (12, 20):
            d.line([(x, 25), (x + 5, 28)], fill=FUR_D)
        d.rectangle([14, 18, 16, 19], fill=(150, 30, 30, 255))
        return im
    d.ellipse([7, 25, 27, 29], fill=SH)
    # legs (front pair at x 10..12, back pair at x 21..23)
    if frame == 0:
        legs = [(10, 0), (12, 0), (21, 0), (23, 0)]
    elif frame == 1:
        legs = [(8, -2), (13, 2), (19, -2), (24, 2)]
    else:
        legs = [(10, 2), (11, -2), (22, 2), (22, -2)]
    for (x, lean) in legs:
        d.line([(x, 19), (x + lean, 26)], fill=FUR_D, width=1)
        d.point((x + lean, 27), fill=HOOF)
    body_y = 13 if frame == 0 else 12
    d.ellipse([7, body_y, 26, body_y + 8], fill=FUR)
    d.ellipse([10, body_y + 4, 23, body_y + 8], fill=BELLY)
    d.ellipse([23, body_y, 27, body_y + 3], fill=(236, 230, 220, 255))   # white tail
    # neck and head
    if frame == 0:
        d.polygon([(8, body_y + 2), (11, body_y + 1), (8, 6), (5, 7)], fill=FUR)
        d.ellipse([2, 4, 9, 9], fill=FUR)
        d.point((4, 6), fill=(20, 20, 20, 255))
        d.point((2, 7), fill=(30, 26, 22, 255))
        d.line([(7, 4), (6, 0)], fill=ANTLER)
        d.line([(6, 2), (4, 1)], fill=ANTLER)
        d.line([(8, 4), (10, 0)], fill=ANTLER)
        d.point((8, 5), fill=FUR_D)                         # ear
    else:
        d.polygon([(8, body_y + 2), (11, body_y + 1), (6, body_y - 3), (4, body_y - 1)], fill=FUR)
        d.ellipse([1, body_y - 5, 8, body_y], fill=FUR)
        d.point((3, body_y - 3), fill=(20, 20, 20, 255))
        d.line([(6, body_y - 5), (6, body_y - 9)], fill=ANTLER)
        d.line([(7, body_y - 5), (9, body_y - 9)], fill=ANTLER)
    return im


def barrier():
    """a striped beam on two trestles, across the lane (seen from above and a little in front)"""
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rectangle([3, 18, 31, 21], fill=SH)
    for x in (4, 26):
        d.line([(x, 14), (x - 2, 22)], fill=(90, 64, 40, 255))
        d.line([(x + 1, 14), (x + 3, 22)], fill=(90, 64, 40, 255))
    for i in range(7):
        col = (206, 40, 34, 255) if i % 2 == 0 else (236, 232, 222, 255)
        d.rectangle([1 + i * 4, 12, 4 + i * 4, 15], fill=col)
    d.line([(1, 16), (28, 16)], fill=(70, 50, 40, 255))
    return im


def sandbags():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rectangle([4, 17, 31, 24], fill=SH)
    for row, y in enumerate((16, 11)):
        off = 0 if row == 0 else 4
        for i in range(4 - row):
            x = 2 + off + i * 7
            d.ellipse([x, y, x + 8, y + 6], fill=(150, 136, 96, 255))
            d.ellipse([x + 1, y + 1, x + 6, y + 3], fill=(176, 162, 118, 255))
            d.line([(x + 4, y + 1), (x + 4, y + 5)], fill=(120, 108, 74, 255))
    return im


def campfire(frame):
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse([7, 19, 25, 27], fill=(60, 56, 52, 255))       # the ring of stones
    for (x, y) in ((8, 21), (12, 25), (19, 25), (23, 21), (15, 19)):
        d.ellipse([x - 2, y - 1, x + 2, y + 2], fill=(130, 128, 124, 255))
    d.line([(10, 25), (22, 20)], fill=(96, 64, 36, 255), width=2)
    d.line([(10, 20), (22, 25)], fill=(96, 64, 36, 255), width=2)
    flames = [(16, 9, 5), (13, 13, 3), (19, 12, 3)] if frame == 0 else [(16, 11, 5), (12, 12, 3), (20, 14, 3)]
    for (x, y, r) in flames:
        d.polygon([(x - r, 23), (x, y), (x + r, 23)], fill=(240, 120, 30, 255))
    for (x, y, r) in flames[:1]:
        d.polygon([(x - r + 2, 23), (x, y + 4), (x + r - 2, 23)], fill=(252, 220, 90, 255))
    return im


def steam(frame):
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    blobs = [(16, 20, 6), (12, 13, 5), (19, 8, 4), (14, 4, 3)] if frame == 0 else [(15, 19, 6), (19, 12, 5), (13, 7, 4), (18, 3, 3)]
    for (x, y, r) in blobs:
        d.ellipse([x - r, y - r, x + r, y + r], fill=(236, 238, 240, 170))
        d.ellipse([x - r + 2, y - r + 1, x + r - 3, y + r - 3], fill=(250, 250, 250, 200))
    return im


def splash():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse([3, 10, 29, 24], outline=(226, 240, 248, 200), width=2)
    d.ellipse([9, 13, 23, 21], outline=(240, 248, 252, 230))
    for (x, y) in ((6, 8), (26, 9), (16, 5), (11, 6), (22, 5)):
        d.ellipse([x - 1, y - 1, x + 1, y + 1], fill=(240, 248, 252, 230))
    return im


def spray_frame(f):
    """32 x 32 (x2): the water thrown up in front of a car fording a river, after the user's splash sheet: a crown of
    grey-blue water with white crests, darker at its foot with curved streaks, loose droplets around; f = 0..3"""
    import math
    import random
    shape = random.Random(40)                       # the crown's peaks: the same in every frame, rising and falling
    peaks = [(c, shape.uniform(4, 7), shape.uniform(1.4, 2.2), shape.uniform(0, 6.28)) for c in (-10, -5.5, -1, 3.5, 8, 11.5)]
    rnd = random.Random(70 + f)
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    px = im.load()
    DARK, MID, LIGHT, FOAM = (74, 102, 116, 255), (106, 136, 150, 255), (156, 180, 190, 255), (232, 238, 240, 255)
    foot = 26
    for x in range(32):
        dx = x - 15.5
        if abs(dx) > 14.5:
            continue
        h = 7 * (1 - (dx / 15) ** 2) + sum(a * (0.75 + 0.35 * math.sin(ph + f * 1.57)) * math.exp(-((dx - c) / w) ** 2)
                                           for (c, a, w, ph) in peaks)
        h = max(1.0, min(16.0, h + rnd.choice((-1, 0, 0, 1))))      # ragged, not smooth
        top = int(round(foot - h))
        for y in range(top, foot + 3):
            t = (y - top) / max(1.0, foot + 2 - top)
            if y > foot and abs(dx) > 11 - (y - foot) * 2:
                continue
            col = FOAM if t < 0.22 else LIGHT if t < 0.42 else MID if t < 0.7 else DARK
            if 0.22 <= t < 0.7 and rnd.random() < 0.18:
                col = FOAM if t < 0.42 else LIGHT       # mottled crests
            px[x, y] = col
    for (cx, cy, r) in ((15.5, foot + 1.5, 7 + f % 2), (15.5, foot + 0.5, 10)):    # curved streaks at the foot
        for k in range(-60, 61, 6):
            a = math.radians(k)
            x, y = int(round(cx + r * math.sin(a))), int(round(cy - 1.2 * math.cos(a) + 1))
            if 0 <= x < 32 and 0 <= y < 32 and px[x, y][3]:
                px[x, y] = LIGHT
    for _ in range(16 + f * 2):                                                  # droplets
        a = rnd.uniform(math.pi * 1.05, math.pi * 1.95)
        d = rnd.uniform(9, 16)
        x, y = int(15.5 + math.cos(a) * d * 1.1), int(foot - 4 + math.sin(a) * d * 0.9)
        if 0 <= x < 32 and 0 <= y < 32 and not px[x, y][3]:
            px[x, y] = MID if rnd.random() < 0.6 else LIGHT
    return im


def spray_sheet():
    """256 x 256: the ford's splashes, 4 frames of 64 x 64 each of the wake under the car (row 0) and the sprays thrown
    out to its left (row 1) and right (row 2), cut from Art/anim/ford_reference.png by Tools/anim_river_cut.py"""
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'splash_cut.png')).convert('RGBA')


def river_snow():
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'river_snow_cut.png')).convert('RGBA')


def river_desert():
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'river_desert_cut.png')).convert('RGBA')


def meat():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse([10, 15, 26, 25], fill=SH)
    d.ellipse([8, 11, 22, 22], fill=(170, 52, 48, 255))
    d.ellipse([10, 13, 18, 18], fill=(206, 96, 90, 255))
    d.line([(20, 19), (26, 23)], fill=(232, 226, 210, 255), width=2)
    return im


def atlas():
    at = Image.new('RGBA', (128, 128), (0, 0, 0, 0))
    for f in range(4):
        left = deer(f)
        at.paste(left, (f * 32, 0))
        at.paste(left.transpose(Image.FLIP_LEFT_RIGHT), (f * 32, 32))
    for i, im in enumerate((barrier(), sandbags(), campfire(0), campfire(1))):
        at.paste(im, (i * 32, 64))
    for i, im in enumerate((steam(0), steam(1), splash(), meat())):
        at.paste(im, (i * 32, 96))
    return at.resize((256, 256), Image.NEAREST)


def river():
    """256 x 1024: the river's 4 frames, cut from Art/anim/river_reference.png by Tools/anim_river_cut.py"""
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'river_cut.png')).convert('RGBA')


def river_props():
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'river_props.png')).convert('RGBA')


def river_drawn():
    """256 x 64: banks top and bottom, the road (66..190) runs into the water and out the other side"""
    W, H = 256, 64
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    px = im.load()
    import random
    rnd = random.Random(7)
    for y in range(H):
        for x in range(W):
            edge_t = 10 + 2.5 * __import__('math').sin(x / 17.0) + (1 if (x * 7) % 5 == 0 else 0)
            edge_b = H - 10 + 2.5 * __import__('math').sin(x / 13.0 + 2)
            if y < edge_t - 3 or y > edge_b + 3:
                continue
            if y < edge_t or y > edge_b:
                px[x, y] = (104, 84, 58, 255)            # mud bank
                continue
            depth = min(y - edge_t, edge_b - y)
            base = (70, 104, 118) if depth > 6 else (96, 126, 132)
            n = rnd.randint(-6, 6)
            band = ((x + y * 3) // 11) % 7 == 0
            c = tuple(max(0, min(255, v + n + (18 if band else 0))) for v in base)
            px[x, y] = c + (255,)
    d = ImageDraw.Draw(im)
    for i in range(14):                                     # foam streaks with the current
        x = rnd.randint(0, W - 20)
        y = rnd.randint(16, H - 16)
        d.line([(x, y), (x + rnd.randint(6, 16), y)], fill=(200, 222, 228, 200))
    for x in (66, 186):                                     # the broken road ends at both banks
        d.rectangle([x, 4, x + 4, 12], fill=(64, 66, 70, 255))
        d.rectangle([x, H - 13, x + 4, H - 5], fill=(64, 66, 70, 255))
    d.rectangle([70, 3, 186, 11], fill=(64, 66, 70, 255))
    d.rectangle([70, H - 12, 186, H - 4], fill=(64, 66, 70, 255))
    for x in range(74, 186, 9):                             # broken edge
        d.polygon([(x, 11), (x + 4, 14), (x + 8, 11)], fill=(64, 66, 70, 255))
        d.polygon([(x, H - 12), (x + 4, H - 15), (x + 8, H - 12)], fill=(64, 66, 70, 255))
    return im


def bubble(kind):
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse([5, 3, 27, 22], fill=(30, 30, 34, 255))
    d.ellipse([6, 4, 26, 21], fill=(246, 244, 236, 255))
    d.polygon([(11, 19), (14, 19), (9, 26)], fill=(246, 244, 236, 255))
    ink = (40, 40, 46, 255)
    if kind == 'q':
        d.arc([12, 6, 20, 14], 200, 20, fill=ink, width=2)
        d.line([(19, 12), (16, 15)], fill=ink, width=2)
        d.rectangle([15, 16, 16, 16], fill=ink)
        d.rectangle([15, 18, 16, 19], fill=ink)
    elif kind == 'alert':
        d.rectangle([15, 7, 17, 15], fill=(200, 40, 34, 255))
        d.rectangle([15, 17, 17, 19], fill=(200, 40, 34, 255))
    else:                                                   # sick: a green swirl
        g = (90, 160, 60, 255)
        d.arc([10, 7, 22, 19], 0, 300, fill=g, width=2)
        d.arc([13, 10, 19, 16], 90, 360, fill=g, width=1)
    return im


def signpost():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.line([(16, 29), (24, 31)], fill=SH, width=2)
    d.rectangle([15, 6, 17, 29], fill=(96, 70, 44, 255))
    rust = (150, 86, 50, 255)
    d.polygon([(4, 7), (14, 7), (14, 12), (4, 12), (1, 9)], fill=rust)
    d.polygon([(18, 13), (28, 13), (31, 16), (28, 19), (18, 19)], fill=rust)
    for (x, y) in ((6, 9), (10, 10), (22, 15), (26, 17)):
        d.point((x, y), fill=(110, 60, 34, 255))
    return im


def heli():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    """top down, lying on its side in a scorch mark: cabin left, tail boom right, one rotor blade bent"""
    d.ellipse([1, 9, 31, 29], fill=(30, 26, 22, 110))       # scorched ground
    out = (34, 38, 30, 255)
    body = (92, 104, 72, 255)
    hi = (118, 132, 92, 255)
    d.rectangle([4, 23, 17, 23], fill=out)                  # skids
    d.rectangle([4, 13, 17, 13], fill=out)
    d.ellipse([2, 14, 18, 23], fill=out)                    # cabin
    d.ellipse([3, 15, 17, 22], fill=body)
    d.ellipse([5, 16, 11, 19], fill=hi)
    d.ellipse([2, 16, 6, 21], fill=(130, 168, 180, 255))    # nose glass
    d.polygon([(16, 17), (29, 18), (29, 20), (16, 21)], fill=out)   # tail boom
    d.polygon([(16, 18), (28, 18), (28, 19), (16, 20)], fill=body)
    d.rectangle([27, 14, 29, 22], fill=out)                 # tail fin
    d.line([(10, 18), (3, 4)], fill=(56, 56, 56, 255), width=2)     # rotor blades from the hub
    d.line([(10, 18), (24, 9)], fill=(56, 56, 56, 255), width=2)
    d.line([(10, 18), (16, 27), (22, 28)], fill=(56, 56, 56, 255), width=2)   # the bent one
    d.ellipse([9, 17, 11, 19], fill=(20, 20, 20, 255))
    d.ellipse([12, 19, 16, 23], fill=(232, 120, 30, 255))   # fire in the cabin
    d.ellipse([13, 20, 15, 22], fill=(252, 214, 90, 255))
    return im


def heli_wreck():
    """128 x 128: the crashed helicopter with its shadow, cut from the reference picture Art/anim/heli_reference.png
    by Tools/anim_heli_cut.py (the heli in its own colours, the shadow a see-through black, the grass gone)"""
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'heli_wreck.png')).convert('RGBA')


def deer_sheet():
    """512 x 128: the deer's 8 poses both ways, cut from Art/anim/deer_reference.png by Tools/anim_deer_cut.py"""
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'deer_ca.png')).convert('RGBA')


def barrier_sheet():
    """256 x 256: the roadblock pieces, cut from Art/anim/roadblock_reference.png by Tools/anim_roadblock_cut.py"""
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'barrier_ca.png')).convert('RGBA')


def rod():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.line([(26, 28), (6, 6)], fill=(110, 76, 40, 255), width=2)
    d.line([(6, 6), (4, 20)], fill=(220, 220, 220, 180))
    d.ellipse([2, 19, 6, 23], fill=(220, 40, 30, 255))
    d.rectangle([2, 21, 6, 23], fill=(240, 240, 240, 255))
    return im


def fish():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse([8, 12, 24, 20], fill=(120, 140, 150, 255))
    d.ellipse([10, 14, 22, 17], fill=(170, 186, 190, 255))
    d.polygon([(22, 16), (28, 11), (28, 21)], fill=(100, 120, 130, 255))
    d.point((11, 15), fill=(20, 20, 20, 255))
    return im


def mud():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for (x0, y0, x1, y1) in ((1, 8, 20, 26), (10, 3, 31, 22), (6, 14, 28, 31), (0, 2, 14, 16)):
        d.ellipse([x0, y0, x1, y1], fill=(92, 70, 46, 235))   # a lumpy patch, not a disc
    for (x0, y0, x1, y1) in ((5, 11, 17, 22), (13, 6, 27, 19), (10, 17, 24, 27)):
        d.ellipse([x0, y0, x1, y1], fill=(76, 56, 36, 245))
    for (x, y) in ((8, 14), (21, 10), (17, 23), (25, 18)):
        d.ellipse([x - 2, y - 1, x + 2, y + 1], fill=(120, 132, 134, 210))   # puddles
    return im


def beam():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    px = im.load()
    for y in range(32):
        half = 2 + y * 0.45
        for x in range(32):
            if abs(x - 15.5) <= half:
                a = int(150 * (1 - y / 32.0) * (1 - abs(x - 15.5) / (half + 1)) + 30)
                px[x, y] = (255, 240, 170, max(0, min(255, a)))
    return im


def night():
    """256 x 256, 4 cells of 128 x 128 (shown 96 x 96 film px, the lamps on the top edge):
    top row = the night's darkness with the headlights' cones cut out (white, alpha = how dark; the film tints it the
    night's colour), car (2 lamps 11 film px either side of the middle) | bike (1 lamp in the middle);
    bottom row = the cones' light (white, alpha = a soft brightening over the scene), car | bike."""
    import math
    at = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    px = at.load()
    for cell, lamps in ((0, (-11, 11)), (1, (0,))):
        for ty in range(128):
            for tx in range(128):
                fx, fy = (tx + 0.5) * 0.75, (ty + 0.5) * 0.75          # film px from the cell's top-left
                light = 0.0
                for lx in lamps:
                    hw = 2.0 + fy * 0.32                                 # the cone's half width
                    dx = abs(fx - (48 + lx))
                    if dx >= hw + 2:
                        continue
                    side = max(0.0, min(1.0, (hw + 2 - dx) / 4.0))     # a soft 4 px edge
                    far = max(0.0, 1 - fy / 96.0) ** 0.7                 # fades out with distance
                    near = min(1.0, 0.55 + fy / 12.0)                    # the lamp itself is small
                    light = max(light, side * far * near)
                pool = max(0.0, 1 - math.hypot((fx - 48) / 30.0, (fy - 8) / 10.0)) * 0.35   # the road lit just ahead
                light = min(1.0, max(light, pool))
                px[cell * 128 + tx, ty] = (255, 255, 255, int(round(255 * (1 - 0.92 * light))))
                px[cell * 128 + tx, 128 + ty] = (255, 255, 255, int(round(255 * 0.30 * light)))
    return at


def gas(frame):
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    blobs = [(10, 18, 8), (20, 14, 9), (16, 22, 7), (24, 22, 6)] if frame == 0 else [(12, 15, 8), (21, 19, 9), (14, 23, 6), (8, 12, 5)]
    for (x, y, r) in blobs:
        d.ellipse([x - r, y - r, x + r, y + r], fill=(196, 200, 70, 120))
        d.ellipse([x - r + 3, y - r + 2, x + r - 3, y + r - 4], fill=(220, 224, 110, 110))
    return im


def spray():
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for (x, y, r) in ((8, 20, 3), (14, 12, 2), (20, 18, 3), (25, 10, 2), (11, 6, 2), (19, 26, 2), (5, 13, 2)):
        d.ellipse([x - r, y - r, x + r, y + r], fill=(96, 70, 44, 255))
    return im


def atlas2():
    at = Image.new('RGBA', (128, 128), (0, 0, 0, 0))
    cells = [signpost(), bubble('q'), bubble('sick'), heli(), rod(), fish(), mud(), beam(), gas(0), gas(1), spray(), bubble('alert')]
    for i, im in enumerate(cells):
        at.paste(im, ((i % 4) * 32, (i // 4) * 32))
    at = at.resize((256, 256), Image.NEAREST)
    # the fork's drawn pieces (Tools/anim_fork_cut.py), full size: signpost over cell 0, "?" over cell 1;
    # row 3: compass bubble, broken signpost, waypoint ring
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    parts = Image.open(os.path.join(here, 'Art', 'anim', 'fork_parts.png')).convert('RGBA')
    for src, cell in ((0, 0), (2, 1), (3, 12), (1, 13), (4, 14)):
        x, y = (cell % 4) * 64, (cell // 4) * 64
        at.paste(Image.new('RGBA', (64, 64), (0, 0, 0, 0)), (x, y))
        at.paste(parts.crop((src * 64, 0, src * 64 + 64, 64)), (x, y))
    return at


def crossroad():
    """256 x 128: the side road across the film's road (the fork), cut from Art/anim/fork_reference.png by
    Tools/anim_fork_cut.py"""
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return Image.open(os.path.join(here, 'Art', 'anim', 'crossroad_cut.png')).convert('RGBA')


def crossroad_drawn():
    """256 x 64: the old drawn side road (unused)"""
    import random
    W, H = 256, 64
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    top, bot = 18, 46
    d.rectangle([0, top, W, bot], fill=(64, 66, 70, 255))
    for x in list(range(0, 66)) + list(range(191, W)):
        d.point((x, top - 1), fill=(178, 174, 160, 255))
        d.point((x, bot + 1), fill=(178, 174, 160, 255))
    for x in list(range(0, 60, 12)) + list(range(196, W, 12)):
        d.rectangle([x, 31, x + 6, 32], fill=(220, 200, 120, 255))
    rnd = random.Random(3)
    for i in range(40):                                     # cracks
        d.point((rnd.randint(0, W - 1), rnd.randint(top + 1, bot - 1)), fill=(52, 54, 58, 255))
    return im


def lake():
    """128 x 128: a pond with a muddy shore"""
    import math
    import random
    im = Image.new('RGBA', (128, 128), (0, 0, 0, 0))
    px = im.load()
    rnd = random.Random(11)
    for y in range(128):
        for x in range(128):
            a = math.atan2(y - 64, x - 64)
            r = 54 + 5 * math.sin(a * 3) + 3 * math.sin(a * 7 + 1)
            dd = math.hypot(x - 64, y - 64)
            if dd > r + 3:
                continue
            if dd > r:
                px[x, y] = (110, 96, 62, 255)
                continue
            deep = (52, 84, 104) if dd < r - 12 else (78, 112, 124)
            n = rnd.randint(-5, 5)
            px[x, y] = tuple(max(0, min(255, v + n)) for v in deep) + (255,)
    d = ImageDraw.Draw(im)
    for (x, y) in ((50, 40), (74, 70), (40, 80), (86, 48)):
        d.line([(x, y), (x + 8, y)], fill=(170, 200, 210, 200))
    return im


def snow():
    """256 x 256, tiles: snow blowing sideways (the blizzard, scrolled fast over the film)"""
    import random
    im = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    rnd = random.Random(5)
    for i in range(170):
        x, y = rnd.randint(0, 255), rnd.randint(0, 255)
        l = rnd.randint(2, 7)
        d.line([(x, y), (x - l, y + l // 2)], fill=(250, 252, 255, rnd.randint(150, 255)), width=1 if l < 5 else 2)
    return im


def main():
    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    src = os.path.join(repo, 'Art', 'anim')
    out = os.path.join(repo, 'Addons', 'DriveLink', 'gui', 'anim')
    tool = r'E:\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe'
    for name, fn in (('trail_ca', atlas), ('river_ca', river), ('trail2_ca', atlas2), ('crossroad_ca', crossroad),
                     ('lake_ca', lake), ('snow_ca', snow), ('heli_ca', heli_wreck), ('deer_ca', deer_sheet),
                     ('barrier_ca', barrier_sheet), ('night_ca', night),
                     ('river_props', river_props), ('river_snow_ca', river_snow), ('river_desert_ca', river_desert),
                     ('splash_ca', spray_sheet)):
        p = os.path.join(src, name + '.png')
        fn().save(p)
        paa = os.path.join(out, name + '.paa')
        r = subprocess.run([tool, p, paa], capture_output=True, text=True)
        print(('ok      ' if r.returncode == 0 and os.path.exists(paa) else 'FAILED  ') + paa)


if __name__ == '__main__':
    main()
