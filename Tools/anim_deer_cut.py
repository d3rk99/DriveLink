"""Tools/anim_deer_cut.py (run by hand): the deer of the trail cards from the drawn sheet Art/anim/deer_reference.png
(8 poses on a transparent background, facing right) -> Art/anim/deer_ca.png, 512 x 128: 8 cells of 64 x 64 a row,
row 0 facing right, row 1 the same mirrored (facing left).
Cells: 0 standing, 1 standing alert (head up), 2 + 3 walking, 4 + 5 running, 6 grazing, 7 down (shot).
The picture's soft, reddish edge fringe is dropped; every pose gets the same scale and stands on the same ground line.
Usage: python Tools/anim_deer_cut.py [reference] [out]"""
import os
import sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, 'Art', 'anim', 'deer_reference.png')
out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, 'Art', 'anim', 'deer_ca.png')
CELL = 64
GROUND = 58            # the hooves' line in a cell

a = np.asarray(Image.open(src).convert('RGBA')).astype(np.int32)
r, g, b, al = a[..., 0], a[..., 1], a[..., 2], a[..., 3]
red = (r > 140) & (g < 100) & (b < 100) & (r - g > 70)          # the fringe
solid = (al > 160) & ~red


def runs(idx, gap=20):
    res = []
    s = p = idx[0]
    for i in idx[1:]:
        if i > p + gap:
            res.append((s, p))
            s = i
        p = i
    res.append((s, p))
    return res


boxes = []
for (y0, y1) in runs(np.nonzero(solid.sum(axis=1) > 2)[0]):
    for (x0, x1) in runs(np.nonzero(solid[y0:y1 + 1].sum(axis=0) > 1)[0]):
        ys = np.nonzero(solid[y0:y1 + 1, x0:x1 + 1].any(axis=1))[0]
        boxes.append((x0, y0 + ys[0], x1 + 1, y0 + ys[-1] + 1))
assert len(boxes) == 8, boxes
biggest = max(max(x1 - x0, y1 - y0) for (x0, y0, x1, y1) in boxes)
k = biggest / (CELL - 4.0)        # source pixels per sprite pixel, the same for every pose

sheet = np.zeros((CELL * 2, CELL * 8, 4), np.uint8)
for n, (x0, y0, x1, y1) in enumerate(boxes):
    w = int(round((x1 - x0) / k))
    h = int(round((y1 - y0) / k))
    cell = np.zeros((h, w, 4), np.uint8)
    for j in range(h):
        for i in range(w):
            sy0, sy1 = int(y0 + j * k), max(int(y0 + (j + 1) * k), int(y0 + j * k) + 1)
            sx0, sx1 = int(x0 + i * k), max(int(x0 + (i + 1) * k), int(x0 + i * k) + 1)
            blk = a[sy0:sy1, sx0:sx1].reshape(-1, 4)
            ok = solid[sy0:sy1, sx0:sx1].reshape(-1)
            if ok.sum() >= len(ok) * 0.5:
                cell[j, i, :3] = np.median(blk[ok][:, :3], axis=0)
                cell[j, i, 3] = 255
    ox = (CELL - w) // 2
    oy = GROUND - h
    if n == 7:
        oy = GROUND - h + 2          # lying: a little lower, on its side
    sheet[max(oy, 0):oy + h, n * CELL + ox:n * CELL + ox + w] = cell[max(-oy, 0):]
res = Image.fromarray(sheet, 'RGBA')
pal = res.convert('RGB').quantize(32, method=Image.Quantize.MEDIANCUT).convert('RGB')
res = Image.merge('RGBA', pal.split() + (res.split()[3],))
right = res.crop((0, 0, CELL * 8, CELL))
left = Image.new('RGBA', (CELL * 8, CELL), (0, 0, 0, 0))
for n in range(8):
    left.paste(right.crop((n * CELL, 0, (n + 1) * CELL, CELL)).transpose(Image.FLIP_LEFT_RIGHT), (n * CELL, 0))
res.paste(left, (0, CELL))
res.save(out)
print('scale %.2f -> %s' % (k, out))
