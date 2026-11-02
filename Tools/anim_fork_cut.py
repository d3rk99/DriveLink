"""Tools/anim_fork_cut.py (run by hand): the fork card's art from the drawn sheet Art/anim/fork_reference.png
(signposts, a road junction tile, a "?" bubble, a compass and a waypoint ring on a transparent background):
  Art/anim/fork_parts.png  = 384 x 64: 6 cells of 64 x 64: signpost, broken signpost, "?" bubble, compass bubble,
                             waypoint ring, compass (alone). Tools/anim_trail.py puts them into trail2_ca.
  Art/anim/crossroad_cut.png = 256 x 128: the side road across the film's road (the junction's arms and middle, grass
                             cut away so the film's own verges show), one texel per film pixel.
Usage: python Tools/anim_fork_cut.py"""
import os
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(HERE, 'Art', 'anim')
a = np.asarray(Image.open(os.path.join(ART, 'fork_reference.png')).convert('RGBA')).astype(np.int32)
solid = a[..., 3] > 160

# the pieces' boxes in the reference (found by alpha)
SIGN = (40, 208, 376, 658)
SIGN2 = (407, 208, 717, 655)
BUBBLE = (89, 770, 263, 990)
COMPASS = (328, 746, 521, 1007)
RING = (580, 774, 807, 986)
# the tile's kerbs (their middles) land on the film road's edge lines (film x 74.5 and 180.5, the shoulders 66..72 and
# 184..189 then line up too), so the side road meets the road without a step
KERB_L, KERB_R = 982.5, 1185.5
LINE_L, LINE_R = 74.5, 180.5
BAND_Y0, BAND_Y1 = 257, 457         # the side road from shoulder to shoulder in the tile


def shrink(box, k, mask=None):
    """box of the reference -> pixels at k source pixels each: the majority's median colour, crisp alpha"""
    x0, y0, x1, y1 = box
    m = solid if mask is None else mask
    w, h = int(round((x1 - x0) / k)), int(round((y1 - y0) / k))
    out = np.zeros((h, w, 4), np.uint8)
    for j in range(h):
        for i in range(w):
            sy0, sy1 = int(y0 + j * k), max(int(y0 + (j + 1) * k), int(y0 + j * k) + 1)
            sx0, sx1 = int(x0 + i * k), max(int(x0 + (i + 1) * k), int(x0 + i * k) + 1)
            blk = a[sy0:sy1, sx0:sx1].reshape(-1, 4)
            ok = m[sy0:sy1, sx0:sx1].reshape(-1)
            if ok.sum() >= len(ok) * 0.5:
                out[j, i, :3] = np.median(blk[ok][:, :3], axis=0)
                out[j, i, 3] = 255
    return out


def put(cell, im, ground=None):
    """im centred in a 64 x 64 cell (standing on row `ground` when given)"""
    h, w = im.shape[:2]
    ox = (64 - w) // 2
    oy = (64 - h) // 2 if ground is None else ground - h
    sub = cell[oy:oy + h, ox:ox + w]
    sub[im[..., 3] > 0] = im[im[..., 3] > 0]


parts = np.zeros((64, 64 * 6, 4), np.uint8)
cells = [parts[:, i * 64:(i + 1) * 64] for i in range(6)]
ks = (SIGN[3] - SIGN[1]) / 60.0                                       # both signposts 60 px tall, the same scale
put(cells[0], shrink(SIGN, ks), 62)
put(cells[1], shrink(SIGN2, ks), 62)
bub = shrink(BUBBLE, (BUBBLE[3] - BUBBLE[1]) / 44.0)                  # ~22 film px at the film's 0.5
put(cells[2], bub)
# the compass bubble: the same bubble, its "?" painted over with the bubble's paper, the compass in it
cb = bub.copy()
h, w = cb.shape[:2]
lit = cb[(cb[..., 3] > 0) & (cb[..., :3].sum(axis=2) > 540)]
paper = np.median(lit, axis=0).astype(np.uint8)                       # the bubble's paper: its light colour
inner = np.zeros((h, w), bool)
for j in range(h):
    row = np.nonzero(cb[j, :, 3] > 0)[0]
    if len(row):
        inner[j, row[0] + 4:row[-1] - 3] = True
inner[:5] = False
inner[h - 13:] = False                                                 # the tail and the bottom edge stay
cb[inner] = paper
comp = shrink(COMPASS, (COMPASS[3] - COMPASS[1]) / 25.0)
ch, cw = comp.shape[:2]
oy, ox = (h - 8 - ch) // 2 + 1, (w - cw) // 2
sub = cb[oy:oy + ch, ox:ox + cw]
sub[comp[..., 3] > 0] = comp[comp[..., 3] > 0]
put(cells[3], cb)
put(cells[4], shrink(RING, (RING[2] - RING[0]) / 60.0))
put(cells[5], shrink(COMPASS, (COMPASS[3] - COMPASS[1]) / 44.0))
Image.fromarray(parts, 'RGBA').save(os.path.join(ART, 'fork_parts.png'))

# the side road: the tile scaled so its kerbs sit on the film road's edge lines, grass dropped, centred in 128 rows
k = (KERB_R - KERB_L) / (LINE_R - LINE_L)
r, g, b = a[..., 0], a[..., 1], a[..., 2]
grass = (g > r + 6) & (g > b + 18)
x0 = KERB_L - LINE_L * k
band = shrink((x0, BAND_Y0, x0 + 256 * k, BAND_Y1), k, solid & ~grass)[:, :256]
road = np.zeros((128, 256, 4), np.uint8)
top = (128 - band.shape[0]) // 2
road[top:top + band.shape[0], :band.shape[1]] = band
Image.fromarray(road, 'RGBA').save(os.path.join(ART, 'crossroad_cut.png'))
print('signs %.2f, road %.2f -> fork_parts.png, crossroad_cut.png' % (ks, k))
