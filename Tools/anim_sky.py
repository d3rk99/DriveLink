"""DriveLink road film, the sky (plane trips): the ground far below and the clouds. Own art.
  sky_<biome>_ca   256 x 512, loops top-to-bottom: fields / forest / desert / snow seen from high up - soft (drawn
                   small, scaled up smooth) and tinted light blue (the air in between)
  sky_sea_ca       the same for open water (a crossing)
  clouds_ca        256 x 256 atlas, 2 x 2 cells of 128 x 128: sharp pixel-art clouds, lit from the top left
  plane (draw_plane) a stand-in top-down light plane until the real art, nose at the bottom
Used by Tools/build-anim-assets.py; run alone for a preview."""
import math
import random
from PIL import Image, ImageDraw, ImageFilter

W, H = 256, 512
SMALL = 4                     # the ground is drawn at 1/4 size and scaled up smooth: softer than the clouds
HAZE = (176, 204, 228)        # the air between the plane and the ground
HAZE_K = 0.32

PALETTE = {
    # base, field colours, forest, road, water
    'field':  ((112, 136, 70), [(128, 150, 74), (150, 160, 84), (176, 166, 96), (104, 128, 64), (140, 120, 80), (160, 150, 70)], (58, 92, 52), (128, 124, 116), (70, 110, 140)),
    'forest': ((64, 98, 56), [(84, 120, 64), (112, 136, 70), (74, 110, 60)], (44, 76, 44), (120, 116, 108), (64, 100, 132)),
    'desert': ((196, 170, 120), [(206, 182, 132), (184, 154, 106), (170, 140, 96), (212, 192, 146)], (120, 116, 72), (150, 136, 112), (80, 120, 150)),
    'snow':   ((226, 230, 236), [(236, 238, 242), (214, 220, 228), (200, 206, 214)], (70, 90, 84), (160, 162, 166), (110, 140, 168)),
}


def haze(c):
    return tuple(int(c[i] * (1 - HAZE_K) + HAZE[i] * HAZE_K + 6) for i in range(3))


def ground(biome):
    base, fields, forest, road, water = PALETTE[biome]
    rnd = random.Random({'field': 11, 'forest': 12, 'desert': 13, 'snow': 14}[biome])
    w, h = W // SMALL, H // SMALL
    im = Image.new('RGB', (w, h), base)
    d = ImageDraw.Draw(im)
    # patchwork: fields in a loose grid, rows offset, a few split in strips (ploughed)
    y = 0
    while y < h:
        rh = rnd.randint(8, 18)
        x = -rnd.randint(0, 10)
        while x < w:
            cw = rnd.randint(8, 22)
            c = rnd.choice(fields)
            d.rectangle((x + 1, y + 1, x + cw - 1, y + rh - 1), fill=c)
            if rnd.random() < 0.3:
                for sx in range(x + 2, x + cw - 1, 2):
                    d.line((sx, y + 1, sx, y + rh - 1), fill=tuple(max(0, v - 10) for v in c))
            x += cw
        y += rh
    # woods: clumps of round blobs
    n = {'forest': 90, 'field': 26, 'desert': 6, 'snow': 30}[biome]
    for _ in range(n):
        cx, cy, r = rnd.randrange(w), rnd.randrange(h), rnd.randint(3, 9)
        for _ in range(r * 2):
            bx, by = cx + rnd.randint(-r, r), cy + rnd.randint(-r, r)
            br = rnd.randint(1, 3)
            for yy in (by, by - h, by + h):           # wrap vertically: the tile loops
                d.ellipse((bx - br, yy - br, bx + br, yy + br), fill=forest)
    # a river winding down, a road crossing it
    pts = []
    xr = rnd.randint(w // 4, w * 3 // 4)
    for yy in range(0, h + 1, 2):
        xr += rnd.choice([-1, 0, 0, 1])
        pts.append((xr, yy))
    d.line(pts, fill=water, width=2)
    rpts = []
    xr = rnd.randint(w // 5, w * 4 // 5)
    for yy in range(0, h + 1, 3):
        xr += rnd.choice([-1, 0, 1])
        rpts.append((xr, yy))
    d.line(rpts, fill=road, width=1)
    # loop seamlessly: the river and road meet themselves at the edges (end where they started)
    im = im.resize((W, H), Image.BILINEAR).filter(ImageFilter.GaussianBlur(0.6))
    px = im.load()
    for yy in range(H):
        for xx in range(W):
            px[xx, yy] = haze(px[xx, yy])
    return seam(im)


def seam(im):
    """blend the bottom rows into the top so the strip loops without a line"""
    out = im.copy()
    band = 24
    for y in range(band):
        k = y / band
        for x in range(W):
            a = im.getpixel((x, y))
            b = im.getpixel((x, H - band + y))
            out.putpixel((x, H - band + y), tuple(int(b[i] * (1 - k) + a[i] * k) for i in range(3)))
    return out


def sea():
    rnd = random.Random(21)
    w, h = W // SMALL, H // SMALL
    im = Image.new('RGB', (w, h), (44, 92, 136))
    d = ImageDraw.Draw(im)
    for _ in range(300):                          # swell: darker and lighter streaks
        x, y = rnd.randrange(w), rnd.randrange(h)
        d.line((x, y, x + rnd.randint(2, 5), y), fill=rnd.choice([(38, 82, 124), (56, 106, 150), (50, 98, 142)]))
    im = im.resize((W, H), Image.BILINEAR).filter(ImageFilter.GaussianBlur(0.6))
    d = ImageDraw.Draw(im)
    for _ in range(90):                           # white caps, small and soft
        x, y = rnd.randrange(W), rnd.randrange(H)
        d.line((x, y, x + rnd.randint(2, 4), y), fill=(150, 184, 206))
    px = im.load()
    for yy in range(H):
        for xx in range(W):
            px[xx, yy] = haze(px[xx, yy])
    return seam(im)


def cloud(rnd, cw, ch):
    """one sharp cumulus: puffs along a flat base (bigger in the middle), lit from above: white tops, grey belly,
    a darker rim on the shaded (bottom / right) side only"""
    im = Image.new('RGBA', (cw, ch), (0, 0, 0, 0))
    mask = Image.new('L', (cw, ch), 0)
    md = ImageDraw.Draw(mask)
    base = int(ch * 0.82)
    n = rnd.randint(7, 11)
    for k in range(n):
        u = (k + 0.5) / n
        bell = math.sin(u * math.pi) ** 0.7
        r = int(ch * (0.16 + 0.30 * bell) * rnd.uniform(0.8, 1.15))
        x = int(6 + u * (cw - 12) + rnd.randint(-3, 3))
        y = base - r + rnd.randint(-2, int(r * 0.35))
        md.ellipse((x - r, y - r, x + r, y + r), fill=255)
    md.rectangle((0, base + 1, cw, ch), fill=0)                 # the flat underside
    mp = mask.load()
    px = im.load()
    tops = []
    for x in range(cw):
        col = [y for y in range(ch) if mp[x, y]]
        tops.append((min(col), max(col)) if col else None)
    for x in range(cw):
        if not tops[x]:
            continue
        t0, t1 = tops[x]
        for y in range(t0, t1 + 1):
            if not mp[x, y]:
                continue
            t = (y - t0) / max(1, (base - t0))
            t += (x / cw - 0.5) * 0.18                          # a little darker to the right
            if t < 0.28:
                c = (255, 255, 255)
            elif t < 0.55:
                c = (243, 246, 250)
            elif t < 0.8:
                c = (222, 228, 238)
            else:
                c = (200, 208, 222)
            below = y + 1 >= ch or not mp[x, y + 1]
            right = x + 1 >= cw or not mp[x + 1, y]
            if below or (right and t > 0.3):
                c = (176, 188, 206)
            px[x, y] = c + (255,)
    return im


def clouds():
    rnd = random.Random(31)
    atlas = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    for i in range(4):
        cw, ch = [(126, 54), (104, 46), (122, 62), (86, 38)][i]
        c = cloud(rnd, cw, ch)
        atlas.alpha_composite(c, ((i % 2) * 128 + (128 - cw) // 2, (i // 2) * 128 + (128 - ch) // 2))
    return atlas


PAINT = {
    # wings / body, trim, cockpit glass
    'cessna':   ((236, 236, 232), (176, 40, 40), (70, 96, 120)),
    'tiger':    ((214, 176, 60), (60, 70, 50), (80, 90, 100)),
    'spitfire': ((96, 108, 72), (150, 140, 110), (60, 70, 80)),
}


def draw_plane(name):
    """stand-in light plane, top-down, nose down: high wing across, tail at the top. 64 x 56."""
    body, trim, glass = PAINT[name]
    lo = tuple(int(v * 0.72) for v in body)
    hi = tuple(min(255, int(v * 1.12)) for v in body)
    w, h = 64, 56
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cx = w // 2
    # tailplane, fin
    d.rounded_rectangle((cx - 13, 3, cx + 13, 9), radius=2, fill=body + (255,), outline=lo + (255,))
    d.line((cx, 1, cx, 10), fill=trim + (255,), width=2)
    # fuselage, tapering to the tail
    d.polygon([(cx - 2, 4), (cx + 2, 4), (cx + 5, 30), (cx + 5, 46), (cx - 5, 46), (cx - 5, 30)], fill=body + (255,), outline=lo + (255,))
    d.line((cx - 3, 12, cx - 4, 44), fill=hi + (255,))
    # wing straight across, a stripe near each tip
    d.rounded_rectangle((1, 26, w - 2, 35), radius=3, fill=body + (255,), outline=lo + (255,))
    d.line((2, 27, w - 3, 27), fill=hi + (255,))
    for tx in (7, w - 9):
        d.rectangle((tx, 27, tx + 2, 34), fill=trim + (255,))
    # cockpit, cowling, propeller disc
    d.rounded_rectangle((cx - 4, 31, cx + 4, 39), radius=2, fill=glass + (255,))
    d.line((cx - 3, 32, cx - 1, 32), fill=(170, 196, 214, 255))
    d.rounded_rectangle((cx - 5, 44, cx + 5, 50), radius=2, fill=trim + (255,))
    d.ellipse((cx - 11, 49, cx + 11, 54), outline=(40, 40, 40, 110))
    d.line((cx - 10, 52, cx + 10, 51), fill=(30, 30, 30, 200))
    return im


if __name__ == '__main__':
    import sys
    out = sys.argv[1] if len(sys.argv) > 1 else 'sky_preview.png'
    g = ground('field').convert('RGBA').crop((0, 0, 256, 256))
    s = sea().convert('RGBA').crop((0, 0, 256, 256))
    o = Image.new('RGBA', (512 + 8, 256), (0, 0, 0, 255))
    o.alpha_composite(g, (0, 0))
    o.alpha_composite(s, (264, 0))
    cl = clouds()
    o.alpha_composite(cl.crop((0, 0, 128, 128)), (30, 20))
    o.alpha_composite(cl.crop((128, 128, 256, 256)), (300, 120))
    p = draw_plane('cessna')
    sh = p.split()[3].point(lambda v: 60 if v > 100 else 0)
    o.paste(Image.new('RGBA', p.size, (0, 0, 0, 255)), (128 + 40, 120 + 50), sh)
    o.alpha_composite(p, (128 - 32, 120))
    o.resize((o.width * 2, o.height * 2), Image.NEAREST).save(out)
