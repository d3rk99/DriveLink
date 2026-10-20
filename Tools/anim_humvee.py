"""DriveLink's top-down Offroad_02 (the M1025 Humvee), front at the bottom: rear hardtop, roof with the turret ring,
two-pane windscreen with mirrors on arms, the bonnet with its big grille vent, headlight pods, the brush guard.
Used by Tools/build-anim-assets.py; run alone for a preview."""
import random
from PIL import Image, ImageDraw

W, H = 56, 84

VARIANTS = {
    # base paint, highlight, shadow; camo: the woodland blotches over it
    'camo':  ((84, 96, 60), (118, 130, 88), (54, 62, 40)),
    'beige': ((182, 160, 116), (212, 192, 150), (134, 116, 82)),
    'grey':  ((128, 130, 126), (166, 168, 164), (88, 90, 88)),
}
WOODLAND = [(104, 82, 54), (40, 36, 38), (62, 76, 44)]   # brown, black, dark green (light green = the base)

TYRE, TYRE_HI = (30, 30, 32), (64, 64, 66)
GLASS, GLASS_HI = (46, 58, 70), (120, 140, 158)
METAL, METAL_HI = (58, 58, 60), (112, 112, 116)
LINE = (26, 28, 24)


def body_mask():
    """the hull outline: square, a little narrower at the hardtop's rear and over the bonnet's nose"""
    m = Image.new('L', (W, H), 0)
    d = ImageDraw.Draw(m)
    d.rounded_rectangle((4, 2, W - 5, 78), radius=3, fill=255)        # the body between the fenders
    d.rectangle((2, 8, W - 3, 26), fill=255)                           # rear fenders, over the wheels
    d.rectangle((2, 56, W - 3, 74), fill=255)                          # front fenders
    return m


def draw(name):
    base, hi, lo = VARIANTS[name]
    rnd = random.Random(1025)
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # tyres stick out a little beside the fenders (wide track)
    for x0 in (0, W - 4):
        for y0 in (9, 57):
            d.rectangle((x0, y0, x0 + 3, y0 + 15), fill=TYRE + (255,))
            for ty in range(y0 + 1, y0 + 15, 3):                       # tread
                d.line((x0, ty, x0 + 3, ty), fill=TYRE_HI + (255,))
    mask = body_mask()
    paint = Image.new('RGBA', (W, H), base + (255,))
    pp = paint.load()
    if name == 'camo':
        # woodland: big soft-edged blotches, brown and black over the greens
        for col in WOODLAND:
            for _ in range(12):
                cx, cy = rnd.randrange(W), rnd.randrange(H)
                rx, ry = rnd.randint(5, 10), rnd.randint(3, 6)
                for y in range(max(0, cy - ry - 2), min(H, cy + ry + 3)):
                    for x in range(max(0, cx - rx - 2), min(W, cx + rx + 3)):
                        k = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
                        if k <= 1 + rnd.uniform(-0.15, 0.15):
                            pp[x, y] = col + (255,)
    im.paste(paint, (0, 0), mask)
    px = im.load()
    mp = mask.load()

    def shade(x0, y0, x1, y1, amount):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if 0 <= x < W and 0 <= y < H and mp[x, y]:
                    c = px[x, y]
                    px[x, y] = tuple(max(0, min(255, int(c[i] * amount))) for i in range(3)) + (255,)

    # light from the top left: the left edge and the fender tops lit, the right side darker
    shade(2, 0, 5, H - 1, 1.18)
    shade(W - 6, 0, W - 1, H - 1, 0.78)
    # outline
    for y in range(H):
        for x in range(W):
            if mp[x, y] and (x == 0 or y == 0 or x == W - 1 or y == H - 1 or not mp[x - 1, y] or not mp[x + 1, y]
                             or not mp[x, y - 1] or not mp[x, y + 1]):
                px[x, y] = LINE + (255,)
    # ---- rear: tailgate edge, tail lights, the spare-less flat back
    d.line((6, 3, W - 7, 3), fill=lo + (255,))
    d.rectangle((5, 3, 7, 5), fill=(170, 40, 34, 255))
    d.rectangle((W - 8, 3, W - 6, 5), fill=(170, 40, 34, 255))
    # ---- the hardtop over the cargo area (rear) and the cabin roof: one raised slab, a seam between them
    roof = (8, 6, W - 9, 49)
    d.rectangle(roof, outline=LINE + (255,))
    shade(roof[0] + 1, roof[1] + 1, roof[0] + 2, roof[3] - 1, 1.15)
    shade(roof[2] - 2, roof[1] + 1, roof[2] - 1, roof[3] - 1, 0.82)
    d.line((roof[0] + 1, 25, roof[2] - 1, 25), fill=lo + (255,))         # cargo top / cabin roof seam
    for ry in (12, 18):                                                   # hardtop ribs
        d.line((roof[0] + 3, ry, roof[2] - 3, ry), fill=lo + (255,))
    # rear side windows of the hardtop (small, dark)
    d.rectangle((6, 14, 7, 22), fill=GLASS + (255,))
    d.rectangle((W - 8, 14, W - 7, 22), fill=GLASS + (255,))
    # ---- the turret ring on the cabin roof, with its brackets
    cx, cy, r = (W - 1) / 2, 37, 8
    import math
    for deg in (45, 135, 225, 315):                                      # 4 small brackets on the diagonals
        ux, uy = math.cos(math.radians(deg)), math.sin(math.radians(deg))
        tip = (cx + ux * (r + 4), cy + uy * (r + 4))
        bl = (cx + ux * (r - 1) - uy * 3, cy + uy * (r - 1) + ux * 3)
        br = (cx + ux * (r - 1) + uy * 3, cy + uy * (r - 1) - ux * 3)
        d.polygon([tip, bl, br], fill=METAL + (255,), outline=LINE + (255,))
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=METAL + (255,), outline=LINE + (255,))
    d.ellipse((cx - r + 2, cy - r + 2, cx + r - 2, cy + r - 2), fill=(36, 36, 38, 255))   # the hatch opening
    d.ellipse((cx - r + 4, cy - r + 4, cx + r - 4, cy + r - 4), fill=lo + (255,))         # its closed lid
    d.arc((cx - r, cy - r, cx + r, cy + r), 160, 260, fill=METAL_HI + (255,))
    # ---- windscreen: two flat panes, a pillar between them; mirrors on arms out to the sides
    d.rectangle((9, 50, W - 10, 55), fill=LINE + (255,))
    d.rectangle((10, 51, int(cx) - 1, 54), fill=GLASS + (255,))
    d.rectangle((int(cx) + 1, 51, W - 11, 54), fill=GLASS + (255,))
    d.line((11, 51, 15, 51), fill=GLASS_HI + (255,))
    d.line((int(cx) + 2, 51, int(cx) + 5, 51), fill=GLASS_HI + (255,))
    for side in (-1, 1):
        ax = 4 if side < 0 else W - 5
        mx0 = 0 if side < 0 else W - 3
        d.line((ax, 52, mx0 + 1, 52), fill=METAL + (255,))               # the arm
        d.rectangle((mx0, 49, mx0 + 2, 53), fill=METAL + (255,), outline=LINE + (255,))
    # ---- the bonnet: raised centre panel with the big grille vent, the hinge line behind it
    d.line((6, 57, W - 7, 57), fill=lo + (255,))
    d.rectangle((14, 60, W - 15, 72), fill=lo + (255,), outline=LINE + (255,))
    for gy in range(62, 72, 2):                                          # vent slats
        d.line((16, gy, W - 17, gy), fill=(34, 34, 34, 255))
    shade(5, 58, 12, 75, 1.06)
    shade(W - 13, 58, W - 6, 75, 0.9)
    # ---- headlight pods on the front corners
    for hx in (5, W - 10):
        d.rectangle((hx, 73, hx + 4, 77), fill=METAL + (255,), outline=LINE + (255,))
        d.rectangle((hx + 1, 76, hx + 3, 77), fill=(236, 232, 196, 255))
    # ---- the brush guard: a tube frame across the front with uprights
    d.rectangle((8, 79, W - 9, 82), outline=METAL + (255,))
    d.line((9, 80, W - 10, 80), fill=METAL_HI + (255,))
    for ux in (12, 20, int(cx), W - 21, W - 13):
        d.line((ux, 78, ux, 82), fill=METAL + (255,))
    d.line((8, 77, 8, 82), fill=METAL + (255,))
    d.line((W - 9, 77, W - 9, 82), fill=METAL + (255,))
    return im


if __name__ == '__main__':
    import os, sys
    out = sys.argv[1] if len(sys.argv) > 1 else 'humvee_preview.png'
    o = Image.new('RGBA', (3 * 66 + 10, 96), (96, 104, 84, 255))
    for i, n in enumerate(['camo', 'beige', 'grey']):
        o.alpha_composite(draw(n), (10 + i * 66, 6))
    o.resize((o.width * 4, o.height * 4), Image.NEAREST).save(out)
