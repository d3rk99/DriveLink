"""DriveLink's road-film gas station: a lot on the right verge of the road tile (Tools/build-anim-assets.py
ground_tile), the MiniDayZ gas station building and fuelling canopy on it. Two sheets in the same frame:
  lot  = the asphalt (blending into the road through curved driveways), pump island, pumps, pillars, the shadows,
         the building                                   -> drawn on the ground layer
  roof = the canopy roof alone                          -> drawn over the people (they walk under it)
Frame: LOT_W x LOT_H film px, its left edge at film x LOT_X0 (the road's right edge line is at 180..181).
Used by Tools/build-anim-assets.py; run alone for a preview."""
import math
import os
import random
from PIL import Image, ImageDraw

LOT_X0 = 176          # film x of the frame's left edge
LOT_W, LOT_H = 80, 200
FLARE = 16            # the driveways: curved joins to the road at the top and the bottom
R = 12                # their radius
HOUSE_Y = 20          # the shop building at the back of the lot ...
HOUSE_K = 0.5
ROOF_Y = HOUSE_Y + 52 # ... and the canopy built onto its front (a little overlap: joined, not floating)
ROOF_K = 0.75         # canopy scale: deeper than a car is long (a Humvee is 84 px)
ROOF_X = 16           # its left edge (film 192): clear of the road, the rest runs off the screen
PUMP_X = 50           # where the filler stands at the pumps (frame px; facing up at them): see pump_y()
SHADOW = (6, 5)       # every shadow falls down-right, like the cars'
CANOPY_H = 16         # how high the canopy roof stands, film px in this front-on view

PACK = r'C:\DayZMods\_assets\minidayz\minidayz-assets\buildings'

ASPHALT = {'snow': (84, 88, 94)}
EDGE = (178, 174, 160)


def asphalt_of(biome):
    return ASPHALT.get(biome, (64, 66, 70))


def lot_mask():
    """where the lot's asphalt is (frame coords): the verge strip from the edge line out, with curved driveways"""
    m = Image.new('L', (LOT_W, LOT_H), 0)
    px = m.load()
    grass_x = 14                      # film 190: where the shoulder meets the grass
    for y in range(LOT_H):
        for x in range(LOT_W):
            yy = y if y < LOT_H / 2 else LOT_H - 1 - y   # mirror: the bottom driveway like the top one
            if x < 4:
                continue                                 # the road itself (the tile shows through)
            if yy >= FLARE:
                px[x, y] = 255
            elif yy >= FLARE - R and x < grass_x + R:
                # fillet between the shoulder (x < grass_x) and the lot's edge (yy = FLARE)
                cx, cy = grass_x + R, FLARE - R
                if x < grass_x or math.hypot(x - cx, yy - cy) >= R:
                    px[x, y] = 255
    return m


def texture(base, rnd):
    """asphalt like the road's (2x2 texels, +-8 wear), plus stains, cracks and tyre marks"""
    im = Image.new('RGBA', (LOT_W, LOT_H), base + (255,))
    d = ImageDraw.Draw(im)
    for _ in range(int(LOT_W * LOT_H / 14)):
        x, y = rnd.randrange(0, LOT_W, 2), rnd.randrange(0, LOT_H, 2)
        v = rnd.choice([-8, 8, -8, -4])
        d.rectangle((x, y, x + 1, y + 1), fill=tuple(max(0, min(255, c + v)) for c in base) + (255,))
    # oil stains by the pumps and the bays, soft dark blots
    for cx, cy, r in [(40, 140, 7), (58, 142, 5), (30, 150, 4), (60, 70, 6), (30, 96, 4), (50, 40, 5)]:
        for y in range(cy - r, cy + r + 1):
            for x in range(cx - r, cx + r + 1):
                k = math.hypot((x - cx) / r, (y - cy) / (r * 0.7))
                if 0 <= x < LOT_W and 0 <= y < LOT_H and k < 1 + rnd.uniform(-0.2, 0.1):
                    c = im.getpixel((x, y))
                    a = 0.55 + 0.25 * (1 - k)
                    im.putpixel((x, y), tuple(int(c[i] * (1 - a * 0.45)) for i in range(3)) + (255,))
    # cracks: thin dark zigzags
    for _ in range(6):
        x, y = rnd.randrange(8, LOT_W), rnd.randrange(FLARE, LOT_H - FLARE)
        for _ in range(rnd.randint(6, 14)):
            if 0 <= x < LOT_W and 0 <= y < LOT_H:
                c = im.getpixel((x, y))
                im.putpixel((x, y), tuple(int(v * 0.72) for v in c[:3]) + (255,))
            x += rnd.choice([-1, 0, 1])
            y += rnd.choice([1, 1, 0])
    # tyre marks curving in from the road
    for off in (0, 7):
        for t in range(40):
            x = 6 + t * 0.9
            y = FLARE + 4 + off + 22 * math.sin(t / 40 * math.pi / 2)
            xi, yi = int(x), int(y)
            if 0 <= xi < LOT_W and 0 <= yi < LOT_H and t % 3:
                c = im.getpixel((xi, yi))
                im.putpixel((xi, yi), tuple(int(v * 0.86) for v in c[:3]) + (255,))
    return im


def scaled(path, k):
    im = Image.open(path).convert('RGBA')
    return im.resize((round(im.width * k), round(im.height * k)), Image.BOX)


def silhouette(im, alpha):
    """im's shape as a shadow"""
    a = im.split()[3].point(lambda v: alpha if v > 40 else 0)
    s = Image.new('RGBA', im.size, (0, 0, 0, 255))
    s.putalpha(a)
    return s


def roof_layer():
    """the canopy roof (MiniDayZ b_gas_station_fueling), centred on the screen edge: half of it shows"""
    roof = scaled(os.path.join(PACK, 'b_gas_station_fueling-sheet0.png'), ROOF_K)
    layer = Image.new('RGBA', (LOT_W, LOT_H), (0, 0, 0, 0))
    x0 = ROOF_X
    layer.paste(roof.crop((0, 0, LOT_W - x0, roof.height)), (x0, ROOF_Y))
    return layer, x0, roof.height


def draw_lot(biome):
    rnd = random.Random(4004)
    base = asphalt_of(biome)
    mask = lot_mask()
    tex = texture(base, rnd)
    lot = Image.new('RGBA', (LOT_W, LOT_H), (0, 0, 0, 0))
    lot.paste(tex, (0, 0), mask)
    mp = mask.load()
    px = lot.load()
    # the curb where asphalt meets the grass (light concrete), and the road's edge line following the driveways
    for y in range(LOT_H):
        for x in range(LOT_W):
            if not mp[x, y]:
                continue
            out = any(0 <= x + dx < LOT_W and 0 <= y + dy < LOT_H and not mp[x + dx, y + dy] and x + dx >= 4
                      for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if out:
                px[x, y] = (150, 146, 134, 255)
    # the shop building (MiniDayZ b_gas_station) at the back, its shadow; the canopy joins its front
    house = scaled(os.path.join(PACK, 'b_gas_station-sheet0.png'), HOUSE_K)
    hx = ROOF_X + 6
    lot.alpha_composite(silhouette(house, 100).crop((0, 0, LOT_W - hx - SHADOW[0], house.height)), (hx + SHADOW[0], HOUSE_Y + SHADOW[1]))
    lot.alpha_composite(house.crop((0, 0, LOT_W - hx, house.height)), (hx, HOUSE_Y))
    # the canopy (front-on like MiniDayZ's buildings: we look at it from the front and above). Its roof stands
    # CANOPY_H above the ground: the shade it throws lies that much lower, and its front row of columns runs from
    # the sign strip down to the pump island, the pumps standing on the island between them
    roof, rx0, rh = roof_layer()
    front = roof.getbbox()[3]                              # the roof's front edge (bottom of its sign strip)
    ground = front + CANOPY_H                              # the ground under that edge
    lot.alpha_composite(silhouette(roof, 105), (SHADOW[0], CANOPY_H))   # the shade under the canopy
    # shadows go on their own layer, blended over the asphalt (drawing them straight in would punch holes)
    sh = Image.new('RGBA', (LOT_W, LOT_H), (0, 0, 0, 0))
    ds = ImageDraw.Draw(sh)
    ob = Image.new('RGBA', (LOT_W, LOT_H), (0, 0, 0, 0))   # ... and the island, pumps, columns over them
    d = ImageDraw.Draw(ob)
    # the pump island: raised concrete (top face, then its yellow-and-black kerb facing us)
    isl = (ROOF_X + 1, ground - 7, LOT_W - 1, ground)
    ds.rectangle((isl[0] + 3, isl[3] + 1, isl[2], isl[3] + 3), fill=(0, 0, 0, 80))       # its shadow
    d.rectangle((isl[0], isl[1], isl[2], isl[3] - 2), fill=(158, 154, 144, 255))
    d.line((isl[0], isl[1], isl[2], isl[1]), fill=(190, 186, 174, 255))
    for x in range(isl[0], isl[2] + 1):
        c = (206, 170, 52, 255) if (x // 3) % 2 else (34, 34, 34, 255)
        d.line((x, isl[3] - 1, x, isl[3]), fill=c)
    # two pumps standing on the island: red body, white panel, dark display, a black hose
    for pxl in (ROOF_X + 15, ROOF_X + 33):
        base = isl[1] + 2
        top = base - 12
        ds.rectangle((pxl + 7, top + 4, pxl + 9, base + 1), fill=(0, 0, 0, 70))               # shadow
        d.rectangle((pxl, top, pxl + 6, base), fill=(176, 44, 38, 255), outline=(70, 24, 22, 255))
        d.line((pxl + 1, top + 1, pxl + 1, base - 1), fill=(214, 84, 70, 255))                 # lit side
        d.rectangle((pxl + 1, top + 1, pxl + 5, top + 5), fill=(226, 222, 210, 255))
        d.rectangle((pxl + 2, top + 2, pxl + 4, top + 3), fill=(40, 52, 48, 255))
        d.point((pxl + 3, top + 7), fill=(230, 200, 70, 255))
        d.line((pxl + 6, top + 6, pxl + 8, top + 8), fill=(24, 24, 24, 255))
        d.line((pxl + 8, top + 8, pxl + 8, base - 1), fill=(24, 24, 24, 255))
    # the columns carrying the roof: from the sign strip down onto the island, a base plate where they stand
    for pxl in (ROOF_X + 5, LOT_W - 9):
        ds.polygon([(pxl + 4, ground - 2), (pxl + 7, ground - 2), (pxl + 13, ground + 4), (pxl + 10, ground + 4)],
                   fill=(0, 0, 0, 90))                                                       # its shadow
        d.rectangle((pxl, front - 2, pxl + 3, ground - 3), fill=(176, 174, 168, 255))
        d.line((pxl, front - 2, pxl, ground - 3), fill=(222, 220, 214, 255))                # lit side
        d.line((pxl + 3, front - 2, pxl + 3, ground - 3), fill=(112, 110, 104, 255))        # shaded side
        d.line((pxl - 1, front - 1, pxl + 4, front - 1), fill=(92, 90, 86, 255))            # bracket under the strip
        d.rectangle((pxl - 1, ground - 3, pxl + 4, ground - 2), fill=(96, 94, 90, 255))     # base plate
    lot.alpha_composite(sh)
    lot.alpha_composite(ob)
    return lot


def pump_y():
    """frame y where the filler stands in front of the pumps"""
    roof, _, _ = roof_layer()
    return roof.getbbox()[3] + CANOPY_H + 6


def draw_roof():
    roof, _, _ = roof_layer()
    return roof


if __name__ == '__main__':
    import sys
    sys.path.insert(0, os.path.dirname(__file__))
    out = sys.argv[1] if len(sys.argv) > 1 else 'station_preview.png'
    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    g = Image.open(os.path.join(repo, 'Art', 'anim', 'ground_field_ca.png')).convert('RGBA')
    y0 = 20
    g.alpha_composite(draw_lot('field'), (LOT_X0, y0))
    import anim_humvee
    car = anim_humvee.draw('camo')
    g.alpha_composite(car, (128 - 28, y0 + pump_y() - 42))
    print("pump y", pump_y(), "front", draw_roof().getbbox()[3], "lot h", LOT_H)
    g.alpha_composite(draw_roof(), (LOT_X0, y0))
    g.resize((g.width * 3, g.height * 3), Image.NEAREST).save(out)
