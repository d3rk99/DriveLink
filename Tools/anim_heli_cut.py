"""Tools/anim_heli_cut.py (run by hand): Cut the heli crash out of the user's reference picture: the heli (greys, black outline, green glass) stays as it
is; the shadow on the grass (a darker olive, a big connected area) becomes a see-through black like the film's
shadows; the grass goes. Then down to a 128 x 128 sprite with crisp pixels."""
import sys
import numpy as np
from PIL import Image, ImageFilter

src, out, preview = sys.argv[1], sys.argv[2], sys.argv[3]
im = np.asarray(Image.open(src).convert('RGB')).astype(np.int32)
r, g, b = im[..., 0], im[..., 1], im[..., 2]
mx = np.maximum(r, g)
lum = (r * 3 + g * 6 + b) // 10
yellow = (mx - b) / np.maximum(mx, 1)
ground_lum = np.median(lum[yellow > 0.3])

heli = (yellow < 0.27) | (lum < 42)


def clean(mask, close, open_, size):
    m = Image.fromarray((mask * 255).astype(np.uint8))
    for _ in range(close):
        m = m.filter(ImageFilter.MaxFilter(size))
    for _ in range(close):
        m = m.filter(ImageFilter.MinFilter(size))
    for _ in range(open_):
        m = m.filter(ImageFilter.MinFilter(size))
    for _ in range(open_):
        m = m.filter(ImageFilter.MaxFilter(size))
    return np.asarray(m) > 127


def biggest(mask, keep_min):
    """connected parts of the mask bigger than keep_min pixels (4-neighbour flood fill on a coarse grid)"""
    h, w = mask.shape
    seen = np.zeros_like(mask)
    keep = np.zeros_like(mask)
    for y0 in range(h):
        row = mask[y0] & ~seen[y0]
        xs = np.nonzero(row)[0]
        for x0 in xs:
            if seen[y0, x0]:
                continue
            stack = [(y0, x0)]
            seen[y0, x0] = True
            part = []
            while stack:
                y, x = stack.pop()
                part.append((y, x))
                for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
                    if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        stack.append((ny, nx))
            if len(part) >= keep_min:
                for (y, x) in part:
                    keep[y, x] = True
    return keep


# work on a quarter-size grid for the masks (fast flood fill), then scale the masks back up
small = (im.shape[1] // 4, im.shape[0] // 4)


def down(mask):
    return np.asarray(Image.fromarray((mask * 255).astype(np.uint8)).resize(small, Image.BOX)) > 127


def up(mask):
    return np.asarray(Image.fromarray((mask * 255).astype(np.uint8)).resize((im.shape[1], im.shape[0]), Image.NEAREST)) > 127


h_s = down(heli)
h_s = biggest(h_s, 400)                       # the heli, not grey specks in the grass
h_s = clean(h_s, 1, 0, 3)
heli = up(h_s) & (heli | up(clean(h_s, 0, 1, 3)))
shadow = (~heli) & (lum < ground_lum * 0.8)
s_s = biggest(down(shadow), 300)              # the shadow is one big area; grass tufts are small
s_s = clean(s_s, 1, 1, 3)
shadow = up(s_s) & ~heli

ys, xs = np.nonzero(heli | shadow)
y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
side = max(y1 - y0, x1 - x0)
cy, cx = (y0 + y1) // 2, (x0 + x1) // 2
y0, x0 = cy - side // 2, cx - side // 2
rgba = np.zeros((side, side, 4), np.uint8)


def crop(a):
    pad = np.zeros((side, side) + a.shape[2:], a.dtype)
    sy0, sx0 = max(y0, 0), max(x0, 0)
    sy1, sx1 = min(y0 + side, a.shape[0]), min(x0 + side, a.shape[1])
    pad[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0] = a[sy0:sy1, sx0:sx1]
    return pad


hc, sc, ic = crop(heli), crop(shadow), crop(im.astype(np.uint8))
rgba[hc, :3] = ic[hc]
rgba[hc, 3] = 255
rgba[sc, :3] = 0
rgba[sc, 3] = 105
big = Image.fromarray(rgba, 'RGBA')

# down to 128: colours by area average of the heli pixels only, alpha decided per cell (crisp, no soft edge)
N = 128
cell = side / N
outa = np.zeros((N, N, 4), np.uint8)
for j in range(N):
    for i in range(N):
        a0, a1 = int(j * cell), max(int((j + 1) * cell), int(j * cell) + 1)
        b0, b1 = int(i * cell), max(int((i + 1) * cell), int(i * cell) + 1)
        blk = rgba[a0:a1, b0:b1].reshape(-1, 4)
        hp = blk[blk[:, 3] == 255]
        if len(hp) >= len(blk) * 0.45:
            # the cell's most common dark-or-light tone: the median keeps outlines black, not grey-blurred
            outa[j, i, :3] = np.median(hp[:, :3], axis=0)
            outa[j, i, 3] = 255
        elif (blk[:, 3] > 0).sum() >= len(blk) * 0.45:
            outa[j, i] = (0, 0, 0, 105)
res = Image.fromarray(outa, 'RGBA')
# a small palette: pixel-art flat colours
pal = res.convert('RGB').quantize(40, method=Image.Quantize.MEDIANCUT).convert('RGB')
res = Image.merge('RGBA', pal.split() + (res.split()[3],))
res.save(out)
g = Image.new('RGBA', (N * 4 + 20, N * 4 + 20), (132, 124, 76, 255))
g.alpha_composite(res.resize((N * 4, N * 4), Image.NEAREST), (10, 10))
g.save(preview)
print('crop', side, 'cell', round(cell, 2))
