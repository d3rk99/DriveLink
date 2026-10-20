"""DriveLink's top-down Boat_01 (an inflatable with an outboard), bow at the bottom. Returns (body, over) where over
is what is drawn over the crew (the motor). Used by Tools/build-anim-assets.py; run alone for a preview."""
import random
from PIL import Image, ImageDraw

W, H = 42, 88
TUBE = 9                     # tube width
STERN = 12                   # y where the tubes' stern caps end / the transom is
BOW_R = (W) / 2              # the bow is a half circle

VARIANTS = {
    # tube colour, highlight, shadow, trim (stripe / handles / seams), floor, floor dark, floor light
    'orange': ((214, 124, 52), (246, 176, 110), (150, 76, 28), (70, 52, 40), (150, 104, 70), (104, 70, 48), (190, 158, 132)),
    'blue':   ((226, 230, 234), (252, 252, 252), (164, 172, 182), (60, 122, 196), (92, 132, 178), (58, 92, 136), (170, 196, 222)),
    'black':  ((92, 94, 98), (150, 152, 156), (54, 56, 60), (40, 40, 42), (124, 122, 118), (86, 84, 82), (170, 168, 164)),
    'camo':   ((178, 182, 188), (236, 238, 240), (118, 122, 130), (64, 66, 72), (130, 128, 124), (92, 90, 88), (184, 182, 178)),
    'grey':   ((150, 150, 150), (200, 200, 200), (100, 100, 100), (60, 60, 60), (130, 126, 120), (90, 88, 84), (176, 172, 166)),
}


def inside_hull(x, y, inset):
    """the U-shaped hull outline (stern at the top, round bow at the bottom), shrunk by inset px"""
    cx = (W - 1) / 2
    r = BOW_R - inset
    by = H - 1 - BOW_R                       # centre of the bow circle
    if y < 2 + inset:
        return False
    if y <= by:
        if abs(x - cx) > r:
            return False
        # stern: two rounded tube caps; between them (the transom) the hull starts lower
        cap_r = TUBE / 2.0
        if inset == 0 and abs(x - cx) < BOW_R - TUBE:
            return y >= STERN - 3
        if inset == 0 and y < 2 + cap_r:
            ccx = cx - (BOW_R - cap_r) if x < cx else cx + (BOW_R - cap_r)
            return (x - ccx) ** 2 + (y - (2 + cap_r)) ** 2 <= cap_r * cap_r
        return True
    return (x - cx) ** 2 + (y - by) ** 2 <= r * r


def draw(name):
    tube, hi, lo, trim, floor, floor_d, floor_l = VARIANTS[name]
    rnd = random.Random(31)
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    px = im.load()
    cx = (W - 1) / 2
    by = H - 1 - BOW_R
    # tubes: everything inside the outline but outside the inner (floor) outline; shaded like a cylinder
    for y in range(H):
        for x in range(W):
            if not inside_hull(x, y, 0):
                continue
            if inside_hull(x, y, TUBE) and y >= STERN:
                continue
            # distance across the tube (0 outer .. 1 inner) for the shading
            if y <= by:
                d = min(abs(x - cx) - (BOW_R - TUBE), TUBE)
                t = 1 - max(0.0, min(1.0, d / TUBE)) if abs(x - cx) > BOW_R - TUBE else 0.5
            else:
                rr = ((x - cx) ** 2 + (y - by) ** 2) ** 0.5
                t = (BOW_R - rr) / TUBE
            t = max(0.0, min(1.0, t))
            # round tube: dark at both edges, lit a little inside of the middle (light from the top left)
            import math
            light = math.sin(t * math.pi) ** 0.8          # 0 at the edges, 1 in the middle
            if t > 0.55:
                light *= 0.82                             # the inner half turns away from the light
            if light < 0.5:
                k = light / 0.5
                c = tuple(int(lo[i] + (tube[i] - lo[i]) * k) for i in range(3))
            else:
                k = (light - 0.5) / 0.5
                c = tuple(int(tube[i] + (hi[i] - tube[i]) * k * 0.85) for i in range(3))
            px[x, y] = c + (255,)
    d = ImageDraw.Draw(im)
    # camo: grey-white digital blocks on the tubes
    if name == 'camo':
        for _ in range(170):
            x, y = rnd.randrange(W), rnd.randrange(H)
            s = rnd.choice([2, 2, 3])
            col = rnd.choice([(212, 216, 222), (130, 134, 142), (96, 100, 108)])
            for yy in range(y, min(H, y + s)):
                for xx in range(x, min(W, x + s)):
                    if px[xx, yy][3] == 255 and not (inside_hull(xx, yy, TUBE) and yy >= STERN):
                        px[xx, yy] = col + (255,)
    # the floor: planks with wear, 4 cross slats, the drain at the bow
    for y in range(STERN, H):
        for x in range(W):
            if inside_hull(x, y, TUBE):
                n = rnd.random()
                c = floor
                if n > 0.975:
                    c = floor_d
                elif n > 0.955:
                    c = floor_l
                if (x - TUBE) % 5 == 0:
                    c = tuple(int(v * 0.86) for v in floor)   # plank joints, lengthwise
                px[x, y] = c + (255,)
    for _ in range(9):                                       # worn, salt-bleached patches
        wx, wy = rnd.randrange(TUBE + 2, W - TUBE - 2), rnd.randrange(STERN + 4, H - TUBE - 6)
        for yy in range(wy, wy + rnd.randint(2, 5)):
            for xx in range(wx, wx + rnd.randint(2, 5)):
                if 0 <= xx < W and 0 <= yy < H and inside_hull(xx, yy, TUBE) and px[xx, yy][3] == 255:
                    c = px[xx, yy]
                    px[xx, yy] = tuple(min(255, int(c[i] * 0.55 + floor_l[i] * 0.45)) for i in range(3)) + (255,)
    for sy in (24, 38, 52, 66):
        for x in range(W):
            if inside_hull(x, sy, TUBE):
                px[x, sy] = (60, 58, 56, 255)
                if inside_hull(x, sy + 1, TUBE):
                    px[x, sy + 1] = (96, 92, 88, 255)
    d.ellipse((cx - 2, H - TUBE - 7, cx + 2, H - TUBE - 3), outline=(70, 70, 72, 255))
    # transom across the stern, between the tube caps
    d.rectangle((TUBE - 1, STERN - 3, W - TUBE, STERN), fill=lo + (255,))
    d.line((TUBE - 1, STERN - 3, W - TUBE, STERN - 3), fill=tube + (255,))
    # trim: seams across the tubes, handles on the outside, a rope line, the bow handle
    for sy in (20, 36, 52):
        for x in list(range(0, TUBE)) + list(range(W - TUBE, W)):
            if px[x, sy][3] == 255:
                px[x, sy] = lo + (255,)
    for hy in (14, 44, 66):
        d.rectangle((0, hy, 1, hy + 3), fill=trim + (255,))
        d.rectangle((W - 2, hy, W - 1, hy + 3), fill=trim + (255,))
    if name == 'blue':
        # the white boat's blue stripe along the outside of the tubes
        for y in range(STERN - 6, H):
            for x in range(W):
                if px[x, y][3] == 255 and inside_hull(x, y, 1) and not inside_hull(x, y, 3):
                    px[x, y] = trim + (255,)
    else:
        for y in range(STERN, int(by)):
            if y % 5 < 3:                                # the rope along the tubes
                px[2, y] = trim + (255,)
                px[W - 3, y] = trim + (255,)
    d.rectangle((cx - 3, H - 3, cx + 3, H - 1), fill=trim + (255,))  # bow handle
    # the outboard: a glossy cowling behind the transom, its shaft and tiller into the boat
    over = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    od = ImageDraw.Draw(over)
    eng, eng_hi = (40, 40, 44), (120, 122, 128)
    if name in ('blue', 'camo'):
        eng, eng_hi = (150, 150, 154), (226, 226, 230)
    od.rounded_rectangle((cx - 5, 0, cx + 5, 11), radius=4, fill=eng + (255,))
    od.line((cx - 1, 2, cx - 1, 8), fill=eng_hi + (255,))
    od.point((cx + 2, 9), fill=(200, 40, 40, 255))
    od.rectangle((cx - 2, 11, cx + 2, 13), fill=(30, 30, 32, 255))
    d.line((cx, 13, cx, 19), fill=(200, 200, 204, 255))  # tiller (under the crew)
    im.alpha_composite(over)
    return im, over


# where the crew sits, in the body's pixels: (centre x, bottom y, pose, shown part) - the driver on the left tube by
# the motor, one on the right tube, one on the left, one at the bow facing back (seats from the in-game boat).
# DL_FilmPlayer.SeatPos uses the same numbers.
SEATS = [(11, 24, 'crouch', 1.0), (31, 40, 'crouch', 1.0), (11, 54, 'crouch', 1.0), (21, 82, 'back', 0.62)]


if __name__ == '__main__':
    o = Image.new('RGBA', (4 * 52 + 8, 100), (70, 84, 96, 255))
    for i, n in enumerate(['orange', 'blue', 'black', 'camo']):
        b, _ = draw(n)
        o.alpha_composite(b, (8 + i * 52, 6))
    o.resize((o.width * 4, o.height * 4), Image.NEAREST).save(
        r'C:/Users/13der/AppData/Local/Temp/claude/C--DayZMods/30f9d4be-06a8-49a9-9838-fdfa80aa14b1/scratchpad/spr/rib.png')
