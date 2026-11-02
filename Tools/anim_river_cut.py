"""Tools/anim_river_cut.py (run by hand): the river card's art from the drawn sheet Art/anim/river_reference.png
(a "river crossing / bridge out" asset sheet on a dark background):
  Art/anim/river_cut.png (-> river_ca) = 256 x 1024: 4 frames of 256 x 256 (the water's glints move with the current), each the
      shallow river across the film (its mud banks, the water band's middle on the cell's middle row 128, about
      25 film px either side), the grass cut away so the film's verges and road show, and on the near (upper) bank the
      broken end of the bridge: its crumbled asphalt and pillars, lined up with the film road's edge lines.
  Art/anim/river_snow_cut.png, river_desert_cut.png: the same with snowy or sandy banks.
  Art/anim/river_props.png = 256 x 64: 4 cells of 64 x 64: "BRIDGE OUT" sign, driftwood log, reeds, bridge debris.
One texel per film pixel. Usage: python Tools/anim_river_cut.py"""
import os
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(HERE, 'Art', 'anim')
a = np.asarray(Image.open(os.path.join(ART, 'river_reference.png')).convert('RGBA')).astype(np.int32)
r, g, b = a[..., 0], a[..., 1], a[..., 2]
lum = (r + g + b) // 3
BG = np.array([28, 30, 31])
grass = (g > b + 40) & (np.abs(r - g) < 18)

FORD = (566, 322, 917, 458)            # the shallow river with its banks (inside the sheet's frame)
TILE = (699, 93, 895, 265)             # the road tile with the broken bridge end (inside its frame)
TILE_LINES = (733, 859)                # its road edge lines; the film's are at 74.5 and 180.5
BROKEN_Y = 206                         # where the tile's asphalt starts to crumble
SIGN = (49, 644, 248, 752)
LOG = (424, 509, 600, 585)
REEDS = (1125, 656, 1184, 730)
DEBRIS = (972, 190, 1046, 263)


def background(box):
    """the sheet's dark background inside box, flood filled from the box's edge (the pieces' own dark outlines
    that touch it go too; at this scale they are less than a pixel)"""
    x0, y0, x1, y1 = box
    sub = np.abs(a[y0:y1, x0:x1, :3] - BG).sum(axis=2) < 30
    h, w = sub.shape
    seen = np.zeros_like(sub)
    stack = [(y, x) for y in range(h) for x in (0, w - 1)] + [(y, x) for x in range(w) for y in (0, h - 1)]
    while stack:
        y, x = stack.pop()
        if 0 <= y < h and 0 <= x < w and sub[y, x] and not seen[y, x]:
            seen[y, x] = True
            stack += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    full = np.zeros(a.shape[:2], bool)
    full[y0:y1, x0:x1] = seen
    return full


def shrink(box, k, keep):
    """box of the sheet -> pixels of k sheet pixels each: the kept majority's median colour, crisp alpha"""
    x0, y0, x1, y1 = box
    w, h = int(round((x1 - x0) / k)), int(round((y1 - y0) / k))
    out = np.zeros((h, w, 4), np.uint8)
    for j in range(h):
        for i in range(w):
            sy0, sy1 = int(y0 + j * k), max(int(y0 + (j + 1) * k), int(y0 + j * k) + 1)
            sx0, sx1 = int(x0 + i * k), max(int(x0 + (i + 1) * k), int(x0 + i * k) + 1)
            blk = a[sy0:sy1, sx0:sx1].reshape(-1, 4)
            ok = keep[sy0:sy1, sx0:sx1].reshape(-1)
            if ok.sum() >= len(ok) * 0.5:
                out[j, i, :3] = np.median(blk[ok][:, :3], axis=0)
                out[j, i, 3] = 255
    return out


def paste(dst, im, x, y):
    h, w = im.shape[:2]
    x0, y0 = max(x, 0), max(y, 0)
    x1, y1 = min(x + w, dst.shape[1]), min(y + h, dst.shape[0])
    sub = im[y0 - y:y1 - y, x0 - x:x1 - x]
    m = sub[..., 3] == 255
    dst[y0:y1, x0:x1][m] = sub[m]
    s = (sub[..., 3] > 0) & ~m                      # see-through pixels: over what is there (or alone)
    d = dst[y0:y1, x0:x1]
    under = d[s].astype(int)
    al = sub[s][:, 3:4].astype(int)
    mix = np.where(under[:, 3:4] > 0, (sub[s][:, :3] * al + under[:, :3] * (255 - al)) // 255, sub[s][:, :3])
    d[s] = np.concatenate([mix, np.maximum(al, under[:, 3:4])], axis=1).astype(np.uint8)


# ---- the river: the ford tile 256 wide
kf = (FORD[2] - FORD[0]) / 256.0
river = shrink(FORD, kf, ~grass)
rh = river.shape[0]
rr, rg, rb = river[..., 0].astype(int), river[..., 1].astype(int), river[..., 2].astype(int)
water = (river[..., 3] > 0) & (rb > rr + 4)
rows = np.nonzero(water.mean(axis=1) > 0.6)[0]
w_top, w_bot = rows[0], rows[-1]
mid = (w_top + w_bot) // 2
# the glints: light streaks on the water; taken out, then drawn back moving with the current
glint = water & ((rr + rg + rb) // 3 > 150)
base = river.copy()
ys, xs = np.nonzero(glint)
for y, x in zip(ys, xs):
    for dy in (2, -2, 3, -3, 1, -1):
        if 0 <= y + dy < rh and water[y + dy, x] and not glint[y + dy, x]:
            base[y, x] = river[y + dy, x]
            break

# ---- the broken bridge end (lower part of the road tile), on the near bank
kt = (TILE_LINES[1] - TILE_LINES[0]) / (180.5 - 74.5)
# the tile's plain asphalt goes (the film's own road shows, so the colours match); its broken concrete, pillars and
# rebar stay; the black gap under the broken edge becomes a dirt ramp with wheel ruts, sloping down into the water
# (the ford: the cars drive down it)
spread = a[..., :3].max(axis=2) - a[..., :3].min(axis=2)
asphalt = (spread < 14) & (lum >= 34) & (lum < 72)
void = lum < 34
end = shrink((TILE[0], BROKEN_Y, TILE[2], TILE[3]), kt, ~grass & ~asphalt & ~void)
gap = shrink((TILE[0], BROKEN_Y, TILE[2], TILE[3]), kt, void)
gap_only = (gap[..., 3] > 0) & (end[..., 3] == 0)
end_x = int(round(74.5 - (TILE_LINES[0] - TILE[0]) / kt))
fx = end_x + np.arange(end.shape[1])
gap_only &= ((fx >= 76) & (fx <= 179))[None, :]                 # only under the road, between the pillars
end[gap_only] = 0
# only the blocks at the sides stay: the deck's broken slabs and rebar across the road go, the ramp starts where the
# asphalt breaks off
mid_cols = (fx >= 84) & (fx <= 180)
deck_top = int(np.nonzero((end[:, mid_cols, 3] > 0).any(axis=1))[0][0])
# a broken face, not a straight cut: the cut wanders a pixel or two into the blocks row by row, and the blocks get
# a dark outline along it like their other sides
brk = np.random.RandomState(9)
cut = np.zeros(end.shape[:2], bool)
for j in range(end.shape[0]):
    l = 84 - brk.randint(0, 3)
    r = 180 + brk.randint(0, 3)
    cut[j] = (fx >= l) & (fx <= r)
end[cut] = 0
solid_e = end[..., 3] > 0
edge = np.zeros_like(solid_e)
edge[:, :-1] |= solid_e[:, :-1] & cut[:, 1:]
edge[:, 1:] |= solid_e[:, 1:] & cut[:, :-1]
edge[1:, :] |= solid_e[1:, :] & cut[:-1, :] & ~solid_e[:-1, :]
end[edge, :3] = (end[edge, :3] * 0.42).astype(np.uint8)

frames = np.zeros((1024, 256, 4), np.uint8)
bank = np.zeros((256, 256), bool)                  # the mud banks in a cell (recoloured for snow and desert)
oy = 128 - mid                                      # the water's middle on row 128
end_y = oy + w_top + 4 - end.shape[0]               # the pillars' feet just in the water
RAMP = (611, 418, 752, 496)                         # the concept sheet's sloped ford tile (Art/anim/ford_reference.png)
ford_src = np.asarray(Image.open(os.path.join(ART, 'ford_reference.png')).convert('RGBA')).astype(np.int32)
kr = (RAMP[2] - RAMP[0]) / 104.0
ramp = np.asarray(Image.fromarray(ford_src[RAMP[1]:RAMP[3], RAMP[0]:RAMP[2]].astype(np.uint8), 'RGBA').resize(
    (104, int(round((RAMP[3] - RAMP[1]) / kr))), Image.BOX)).astype(np.int32)
ramp_end = oy + w_top + 7                           # a little way into the water
for f in range(4):
    cell = frames[f * 256:(f + 1) * 256]
    paste(cell, base, 0, oy)
    if f == 0:
        bank[oy:oy + rh] = (base[..., 3] > 0) & ~water
    shift = f * 3
    for y, x in zip(ys, xs):
        nx = (x + shift) % 256
        if water[y, nx]:
            cell[oy + y, nx] = river[y, x]
    # the ramp: from the asphalt's broken edge (ragged, a dark lip, a few loose asphalt bits) down into the river,
    # wet at the bottom
    edge_rnd = np.random.RandomState(5)
    lip = end_y + deck_top + 2 + edge_rnd.randint(0, 4, 256)
    lip = np.maximum(lip, np.convolve(lip, np.ones(3) / 3, 'same').astype(int))
    for x in range(74, 182):
        top = lip[x]
        for y in range(top, ramp_end):
            dirt = ramp[(y - top) % ramp.shape[0], min(103, max(0, x - 76)), :3]
            c = dirt * min(1.0, 0.82 + 0.03 * (y - top))
            if y == top:
                c = np.array([46, 44, 46])                 # the broken asphalt's edge
            elif y < top + 4 and edge_rnd.rand() < 0.12:
                c = np.array([58, 60, 64])                 # bits of asphalt on the dirt
            wet = y - (oy + w_top)
            if wet >= 0:                                 # into the water: wet dirt under a film of water
                under = cell[y, x, :3].astype(int)
                c = c * 0.75 * (1 - wet / 8.0) + under * (wet / 8.0 + 0.25 * (1 - wet / 8.0))
            cell[y, x] = (*np.clip(c, 0, 255).astype(np.uint8), 255)
            if f == 0:
                bank[y, x] = False
    paste(cell, end, end_x, end_y)
    if f == 0:
        bank[end_y:end_y + end.shape[0], end_x:end_x + end.shape[1]] &= ~(end[..., 3] > 0)[:256 - end_y, :256 - end_x]
Image.fromarray(frames, 'RGBA').save(os.path.join(ART, 'river_cut.png'))


def recolour(frames, tone, keep_grey):
    """the banks in another land: the mud's light and dark kept, its colour the land's (stones stay grey)"""
    out = frames.copy()
    for f in range(4):
        cell = out[f * 256:(f + 1) * 256]
        px = cell[bank].astype(int)
        l = px[:, :3].mean(axis=1, keepdims=True)
        grey = (px[:, :3].max(axis=1) - px[:, :3].min(axis=1)) < 22
        new = np.clip(np.array(tone)[None, :] * (0.62 + 0.75 * l / 255.0), 0, 255)
        if keep_grey:
            new[grey] = np.clip(px[grey, :3] * 1.1, 0, 255)
        cell[bank, :3] = new.astype(np.uint8)
    return out


Image.fromarray(recolour(frames, (214, 222, 232), True), 'RGBA').save(os.path.join(ART, 'river_snow_cut.png'))
Image.fromarray(recolour(frames, (204, 176, 128), True), 'RGBA').save(os.path.join(ART, 'river_desert_cut.png'))

# ---- props
props = np.zeros((64, 256, 4), np.uint8)
for n, (box, size) in enumerate(((SIGN, 44), (LOG, 40), (REEDS, 26), (DEBRIS, 26))):
    keep = ~background(box)
    keep[:box[1]] = False
    k = max(box[2] - box[0], box[3] - box[1]) / float(size)
    im = shrink(box, k, keep)
    h, w = im.shape[:2]
    paste(props, im, n * 64 + (64 - w) // 2, 60 - h)
Image.fromarray(props, 'RGBA').save(os.path.join(ART, 'river_props.png'))
print('river %.2f (water rows %d..%d of %d -> %d..%d), bridge end %.2f at x %d' %
      (kf, w_top, w_bot, rh, oy + w_top, oy + w_bot, kt, end_x))

# ---- the ford's splashes (Art/anim/ford_reference.png): 4 frames each of the wake under the car and the sprays
# thrown out to its left and right -> Art/anim/splash_cut.png (-> splash_ca), 256 x 256: rows wake, left, right
FORD_BG = np.array([37, 40, 36])
WAKE_X, LEFT_X, RIGHT_X = (32, 222), (245, 354), (385, 502)
WAKE_Y = ((738, 768), (793, 838), (863, 922), (948, 1008))
SPRAY_Y = ((722, 778), (792, 856), (869, 941), (954, 1032))
fsrc = ford_src
flum = fsrc[..., :3].mean(axis=2)
fdist = np.abs(fsrc[..., :3] - FORD_BG).sum(axis=2)
KS = 3.5                                            # sheet pixels per film pixel: the wake ~54 px, a car is ~36


def spray_cut(x0, x1, y0, y1):
    w, h = int(round((x1 - x0) / KS)), int(round((y1 - y0) / KS))
    out = np.zeros((h, w, 4), np.uint8)
    for j in range(h):
        for i in range(w):
            sy0, sy1 = int(y0 + j * KS), int(y0 + (j + 1) * KS)
            sx0, sx1 = int(x0 + i * KS), int(x0 + (i + 1) * KS)
            blk = fsrc[sy0:sy1, sx0:sx1].reshape(-1, 4)
            on = (fdist[sy0:sy1, sx0:sx1] > 34).reshape(-1)
            if on.sum() < len(on) * 0.3:
                continue
            px = blk[on][:, :3]
            bright = px[np.argsort(px.mean(axis=1))[len(px) // 2:]]      # the lighter half: crisp foam, not mush
            col = np.median(bright, axis=0)
            l = col.mean()
            out[j, i, :3] = col
            out[j, i, 3] = 255 if l > 95 else 200 if l > 70 else 140    # the dark water parts see-through
    return out


splash = np.zeros((256, 256, 4), np.uint8)
for f in range(4):
    for row, (xs_, ys_) in enumerate(((WAKE_X, WAKE_Y[f]), (LEFT_X, SPRAY_Y[f]), (RIGHT_X, SPRAY_Y[f]))):
        im = spray_cut(xs_[0], xs_[1], ys_[0], ys_[1])
        h, w = im.shape[:2]
        oyy = row * 64 + (40 - h // 2 if row == 0 else 52 - h)           # the wake centred, the sprays standing
        sub = splash[oyy:oyy + h, f * 64 + (64 - w) // 2:f * 64 + (64 - w) // 2 + w]
        sub[im[..., 3] > 0] = im[im[..., 3] > 0]
Image.fromarray(splash, 'RGBA').save(os.path.join(ART, 'splash_cut.png'))
