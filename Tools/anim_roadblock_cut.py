"""Tools/anim_roadblock_cut.py (run by hand): the roadblock pieces of the trail cards from the drawn sheet
Art/anim/roadblock_reference.png (6 pieces on a transparent background, 2 columns x 3 rows)
-> Art/anim/barrier_ca.png, 256 x 256: 2 x 3 cells of 128 x 64, in the sheet's order:
  0 barrier, 1 broken barrier, 2 sandbags, 3 junk pile, 4 spike strip, 5 bloody barrier.
Every piece gets the same scale and stands on the same ground line, with a see-through shadow down-right like the
film's other things. Usage: python Tools/anim_roadblock_cut.py [reference] [out]"""
import os
import sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, 'Art', 'anim', 'roadblock_reference.png')
out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, 'Art', 'anim', 'barrier_ca.png')
CW, CH = 128, 64
GROUND = 58            # the pieces' feet in a cell
SHADOW = 80            # the shadow's alpha

a = np.asarray(Image.open(src).convert('RGBA')).astype(np.int32)
H, W = a.shape[:2]
solid = a[..., 3] > 160

boxes = []
for row in range(3):
    for col in range(2):
        ry0, ry1 = H * row // 3, H * (row + 1) // 3
        rx0, rx1 = (0, 650) if col == 0 else (650, W)       # the junk pile reaches left of the middle
        ys, xs = np.nonzero(solid[ry0:ry1, rx0:rx1])
        boxes.append((rx0 + xs.min(), ry0 + ys.min(), rx0 + xs.max() + 1, ry0 + ys.max() + 1))
k = max(max((x1 - x0) / (CW - 6.0), (y1 - y0) / (CH - 10.0)) for (x0, y0, x1, y1) in boxes)

sheet = np.zeros((CH * 4, CW * 2, 4), np.uint8)       # 256 tall: a PAA wants powers of two; the last row is empty
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
    ox = n % 2 * CW + (CW - w) // 2
    oy = n // 2 * CH + GROUND - h
    sh = cell[..., 3] > 0                       # the shadow: the piece's shape, 2 down and 2 right, under it
    tgt = sheet[oy + 2:oy + 2 + h, ox + 2:ox + 2 + w]
    tgt[sh & (tgt[..., 3] == 0)] = (0, 0, 0, SHADOW)
    dst = sheet[oy:oy + h, ox:ox + w]
    dst[cell[..., 3] > 0] = cell[cell[..., 3] > 0]
res = Image.fromarray(sheet, 'RGBA')
pal = res.convert('RGB').quantize(40, method=Image.Quantize.MEDIANCUT).convert('RGB')
res = Image.merge('RGBA', pal.split() + (res.split()[3],))
res.save(out)
print('scale %.2f -> %s' % (k, out))
