"""DriveLink's top-down motorbikes, front at the bottom (Tools/build-anim-assets.py uses them):
  dirt bike = Motorbike_02 (red, green, blue, yellow): white fenders, long brown seat, exhaust on the right, wide bars,
              a number plate over the round headlight (yellow on the red and green one, white on blue and yellow)
  moped     = Motorbike_01 (red, yellow, blue): long black seat on a painted body, a cream tank cowl with wide leg
              shields, a mirror on the right
draw_*(name) -> (body, front): front = the part drawn over the rider (bars, plate / cowl, front fender and wheel).
Run alone for a preview."""
from PIL import Image, ImageDraw

DIRT = {  # paint, number plates
    'red': ((190, 46, 40), (226, 200, 70)),
    'green': ((78, 140, 62), (226, 200, 70)),
    'blue': ((52, 84, 186), (214, 216, 220)),
    'yellow': ((226, 190, 50), (214, 216, 220)),
    'grey': ((130, 130, 130), (214, 216, 220)),
}
MOPED = {
    'red': (200, 60, 46),
    'yellow': (222, 186, 54),
    'blue': (62, 110, 204),
    'grey': (130, 130, 130),
}
WHITE, WHITE_D = (232, 232, 228), (176, 176, 172)
TYRE, TYRE_HI = (34, 34, 34), (64, 64, 64)
CHROME, CHROME_D = (196, 198, 202), (120, 122, 126)
CREAM, CREAM_D = (230, 222, 196), (178, 170, 146)


def shade(c, f, add=0):
    return tuple(max(0, min(255, int(v * f + add))) for v in c)


def draw_dirt(name):
    paint, plate = DIRT[name]
    W, H = 24, 54
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cx = 11
    # rear: tyre edges under the long white fender, the tail light at the very back
    d.rounded_rectangle((cx - 2, 2, cx + 2, 12), radius=2, fill=TYRE + (255,))
    d.rounded_rectangle((cx - 2, 1, cx + 2, 14), radius=2, fill=WHITE + (255,))
    d.line((cx + 2, 3, cx + 2, 13), fill=WHITE_D + (255,))
    d.rectangle((cx - 1, 0, cx + 1, 1), fill=(210, 40, 40, 255))
    # frame + side panels (paint) around the seat, the exhaust on the right
    d.rounded_rectangle((cx - 4, 12, cx + 4, 30), radius=2, fill=shade(paint, 0.5) + (255,))
    d.rounded_rectangle((cx - 3, 12, cx + 3, 30), radius=2, fill=paint + (255,))
    d.line((cx - 3, 14, cx - 3, 28), fill=shade(paint, 1.2, 14) + (255,))
    d.rectangle((cx - 4, 16, cx - 3, 22), fill=plate + (255,))          # side number plates
    d.rectangle((cx + 3, 16, cx + 4, 22), fill=plate + (255,))
    d.rounded_rectangle((cx + 4, 9, cx + 6, 26), radius=1, fill=(52, 50, 48, 255))  # exhaust
    d.line((cx + 5, 10, cx + 5, 25), fill=(96, 92, 88, 255))
    # the seat: long, brown-black, a little shine
    d.rounded_rectangle((cx - 2, 10, cx + 2, 28), radius=2, fill=(60, 50, 44, 255))
    d.line((cx - 1, 12, cx - 1, 26), fill=(94, 82, 72, 255))
    # tank (paint) in front of the seat
    d.rounded_rectangle((cx - 4, 26, cx + 4, 33), radius=3, fill=shade(paint, 0.5) + (255,))
    d.rounded_rectangle((cx - 3, 27, cx + 3, 32), radius=2, fill=paint + (255,))
    d.point((cx - 1, 28), fill=shade(paint, 1.25, 20) + (255,))
    front = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    f = ImageDraw.Draw(front)
    # handlebar: wide, chrome, grips and levers
    f.line((0, 33, W - 2, 33), fill=CHROME + (255,))
    f.line((1, 34, W - 3, 34), fill=CHROME_D + (255,))
    f.rectangle((0, 32, 2, 34), fill=(28, 28, 28, 255))
    f.rectangle((W - 4, 32, W - 2, 34), fill=(28, 28, 28, 255))
    f.line((2, 35, 5, 35), fill=CHROME_D + (255,))
    f.line((W - 6, 35, W - 3, 35), fill=CHROME_D + (255,))
    # number plate over the headlight, the red headlight ring
    f.rounded_rectangle((cx - 4, 34, cx + 4, 40), radius=1, fill=shade(plate, 0.7) + (255,))
    f.rounded_rectangle((cx - 3, 34, cx + 3, 39), radius=1, fill=plate + (255,))
    f.line((cx - 2, 36, cx + 2, 38), fill=shade(plate, 0.75) + (255,))
    f.rectangle((cx - 2, 40, cx + 2, 42), fill=(190, 40, 36, 255))
    f.point((cx, 41), fill=(240, 236, 210, 255))
    # front fender (white, long) over the tyre
    f.rounded_rectangle((cx - 2, 43, cx + 2, 53), radius=2, fill=TYRE + (255,))
    f.rounded_rectangle((cx - 2, 42, cx + 2, 51), radius=2, fill=WHITE + (255,))
    f.line((cx + 2, 43, cx + 2, 50), fill=WHITE_D + (255,))
    im.alpha_composite(front)
    return im, front


def draw_moped(name):
    paint = MOPED[name]
    W, H = 28, 54
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cx = 12
    # the painted body under the seat (it shows round the seat and at the back), the black seat on top
    d.rounded_rectangle((cx - 4, 0, cx + 4, 24), radius=3, fill=shade(paint, 0.55) + (255,))
    d.rounded_rectangle((cx - 3, 1, cx + 3, 24), radius=3, fill=paint + (255,))
    d.rounded_rectangle((cx - 3, 2, cx + 3, 21), radius=3, fill=(36, 36, 38, 255))
    d.line((cx - 1, 4, cx - 1, 19), fill=(74, 74, 78, 255))
    d.point((cx - 4, 22), fill=(30, 30, 30, 255))                       # footpegs
    d.point((cx + 4, 22), fill=(30, 30, 30, 255))
    d.rectangle((cx - 2, 22, cx + 2, 26), fill=paint + (255,))
    front = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    f = ImageDraw.Draw(front)
    # leg shields (cream, wide) and the tank cowl with its dial
    f.rounded_rectangle((cx - 9, 31, cx - 3, 37), radius=1, fill=CREAM_D + (255,))
    f.rounded_rectangle((cx + 3, 31, cx + 9, 37), radius=1, fill=CREAM_D + (255,))
    f.line((cx - 9, 31, cx - 3, 31), fill=CREAM + (255,))
    f.line((cx + 3, 31, cx + 9, 31), fill=CREAM + (255,))
    f.rounded_rectangle((cx - 4, 27, cx + 4, 38), radius=3, fill=CREAM_D + (255,))
    f.rounded_rectangle((cx - 3, 27, cx + 3, 37), radius=3, fill=CREAM + (255,))
    f.point((cx, 30), fill=(150, 60, 50, 255))
    f.point((cx - 2, 32), fill=(80, 80, 80, 255))
    f.rectangle((cx - 1, 37, cx + 1, 39), fill=(70, 70, 72, 255))       # headlight
    # handlebar with grips and levers, the mirror on the right
    f.line((cx - 9, 27, cx + 9, 27), fill=CHROME + (255,))
    f.rectangle((cx - 10, 26, cx - 8, 28), fill=(28, 28, 28, 255))
    f.rectangle((cx + 8, 26, cx + 10, 28), fill=(28, 28, 28, 255))
    f.line((cx - 8, 29, cx - 5, 29), fill=CHROME_D + (255,))
    f.line((cx + 5, 29, cx + 8, 29), fill=CHROME_D + (255,))
    f.line((cx + 5, 26, cx + 10, 20), fill=(60, 60, 62, 255))           # mirror stalk
    f.ellipse((cx + 9, 17, cx + 13, 21), fill=CHROME_D + (255,))
    f.point((cx + 11, 19), fill=CHROME + (255,))
    # front fender (paint) and the small tyre
    f.rounded_rectangle((cx - 2, 42, cx + 2, 53), radius=2, fill=TYRE + (255,))
    f.rounded_rectangle((cx - 2, 39, cx + 2, 48), radius=2, fill=paint + (255,))
    f.line((cx - 1, 40, cx - 1, 47), fill=shade(paint, 1.2, 16) + (255,))
    for y in (49, 51):
        f.point((cx, y), fill=TYRE_HI + (255,))
    im.alpha_composite(front)
    return im, front


# where the rider sits (centre x of his 32 px cell, the bottom of what shows of him), in the body's pixels
DIRT_SEATS = [(11, 26), (11, 17)]
MOPED_SEATS = [(12, 22), (12, 13)]


if __name__ == '__main__':
    o = Image.new('RGBA', (8 * 32 + 8, 64), (110, 112, 116, 255))
    x = 6
    for n in ['red', 'green', 'blue', 'yellow']:
        b, _ = draw_dirt(n)
        o.alpha_composite(b, (x, 4))
        x += 32
    for n in ['red', 'yellow', 'blue']:
        b, _ = draw_moped(n)
        o.alpha_composite(b, (x, 4))
        x += 34
    o.resize((o.width * 4, o.height * 4), Image.NEAREST).save(
        r'C:/Users/13der/AppData/Local/Temp/claude/C--DayZMods/30f9d4be-06a8-49a9-9838-fdfa80aa14b1/scratchpad/spr/bikes2.png')


WAKE_W = 96   # one wake frame: 96 x 128, the boat's centre line at x 48


def wake_frames():
    """a boat's wake, 4 frames of 96 x 128 side by side (384 x 128), drawn UNDER the boat: the bow at y 118, the stern
    at y 30 (the boat sprite's 88 px), the V of foam spreading back from the hull sides, the propeller's wash behind
    the motor, spray at the bow; the frames move the foam back (the boat goes down the screen)"""
    import math
    import random
    sheet = Image.new('RGBA', (WAKE_W * 4, 128), (0, 0, 0, 0))
    rnd = random.Random(9)
    seeds = [(rnd.random(), rnd.random(), rnd.random()) for _ in range(150)]
    cx = WAKE_W // 2
    for fr in range(4):
        im = Image.new('RGBA', (WAKE_W, 128), (0, 0, 0, 0))
        px = im.load()
        phase = fr / 4.0

        def dot(x, y, a):
            x, y = int(round(x)), int(round(y))
            if 0 <= x < WAKE_W and 0 <= y < 128:
                old = px[x, y][3]
                px[x, y] = (236, 244, 250, min(255, max(old, int(a))))
        # the two arms of the V: from the hull sides near the bow out towards the edges, fading behind
        for side in (-1, 1):
            for i in range(240):
                t = i / 240.0                                  # 0 at the bow .. 1 far behind
                y = 112 - t * 112
                spread = 21 + t * 24
                x = cx + side * spread
                wave = math.sin((t * 9 - phase) * math.pi * 2)
                a = 235 * (1 - t) ** 0.6 * (0.55 + 0.45 * wave)
                dot(x, y, a)
                dot(x + side, y, a * 0.75)                    # 2 px wide
                if wave > 0.5:
                    dot(x - side, y, a * 0.55)                # crests
                    dot(x + side * 2, y, a * 0.4)
            # whitewater along the hull side
            for i in range(30):
                t = ((i / 30.0) + phase / 30.0) % 1.0
                dot(cx + side * (22 + (i % 3 == 0)), 112 - t * 80, 120 * (1 - t))
        # the propeller's wash: churned foam behind the motor, drifting back and spreading
        for (u, v, w) in seeds:
            t = (v + phase / 4.0) % 1.0
            y = 30 - t * 32
            x = cx + (u - 0.5) * (6 + t * 22)
            dot(x, y, 190 * (1 - t) * (0.5 + w * 0.5))
        # bow spray: a short bright arc at the front
        for k in range(-8, 9):
            dot(cx + k * 1.4, 119 - abs(k) * 0.45 + (fr % 2) * 0.5, 220 - abs(k) * 11)
        sheet.alpha_composite(im, (fr * WAKE_W, 0))
    return sheet
