# Authoring provenance, not part of the build. Composes the planes from objects.txt and
# writes planes.txt-style grids plus previews under ./out. Needs Pillow; the generator
# that the firmware builds from is ../gen_space.py and needs nothing installed.
"""Spaceship background, parallax edition. Builds the three planes, composites the real
ship over them with the C arithmetic, writes GIFs, strips, ASCII data and the checks."""
import sys, pathlib, math, re
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))          # art.py sits beside this file
sys.path.insert(0, '/Users/roelvannoord/qmk_userspace/tools')
import art, preview
from PIL import Image

OUT = pathlib.Path(__file__).resolve().parent / 'out'
W, H = 128, 64
LOOP = 512                      # OLED_SCROLL_MODULO
PLANES = {'far': (128, 2), 'mid': (256, 1), 'near': (512, 0)}   # period, right-shift of state

# ---- object library ---------------------------------------------------------
def parse_objects(path):
    objs, name, rows = {}, None, []
    for line in pathlib.Path(path).read_text().splitlines() + ['']:
        if line.startswith('@'):
            name, rows = line[1:].strip().split()[0], []
            objs[name] = rows
        elif line.strip() == '':
            name = None
        elif name is not None:
            rows.append(line.rstrip())
    return {k: [[c == '#' for c in r] for r in v] for k, v in objs.items() if v}

def grid(w, h): return [[False] * w for _ in range(h)]

def disc(d, fill):
    r = (d - 1) / 2; g = grid(d, d)
    for y in range(d):
        for x in range(d):
            dd = math.hypot(x - r, y - r)
            if (dd <= r + 0.5) if fill else (r - 0.5 < dd <= r + 0.5): g[y][x] = True
    # thicken outline so no diagonal-only joins: midpoint style
    if not fill:
        for y in range(d):
            for x in range(d):
                if math.hypot(x - r, y - r) <= r + 0.5 and not g[y][x]:
                    inside = all(0 <= y+dy < d and 0 <= x+dx < d and math.hypot(x+dx-r, y+dy-r) <= r+0.5 for dy, dx in ((0,1),(0,-1),(1,0),(-1,0)))
                    if not inside: g[y][x] = True
    return g

def spans(g, rows):
    for y, segs in rows.items():
        for x0, x1 in segs:
            for x in range(x0, x1 + 1): g[y][x] = True

def earth():
    D = 43; g = disc(D, False)
    land = grid(D, D)
    spans(land, {
        4: [(22,26)], 5: [(21,27)], 6: [(9,18),(21,27)], 7: [(7,19),(22,26)], 8: [(6,20),(23,25)],
        9: [(5,21),(35,37)], 10: [(5,21),(35,38)], 11: [(5,20),(36,39)],
        12: [(6,13),(17,20),(37,40)], 13: [(6,13),(17,21),(36,40)], 14: [(7,21),(36,40)],
        15: [(8,21),(36,41)], 16: [(9,21),(36,41)], 17: [(10,20),(35,41)], 18: [(11,19),(34,41)],
        19: [(12,18),(34,41)], 20: [(13,16),(34,41)], 21: [(14,16),(34,42)], 22: [(15,17),(35,41)],
        23: [(16,24),(35,41)], 24: [(16,26),(36,41)], 25: [(16,28),(36,40)], 26: [(16,29),(37,40)],
        27: [(17,29),(37,39)], 28: [(17,29)], 29: [(18,28)], 30: [(18,27)], 31: [(18,26)], 32: [(19,25)],
        33: [(19,24)], 34: [(19,23)], 35: [(19,22)], 36: [(19,21)], 37: [(19,21)], 38: [(19,20)],
        40: [(14,28)],
    })
    inner = disc(D, True)
    for y in range(D):
        for x in range(D):
            if land[y][x] and inner[y][x]: g[y][x] = True
    return g, inner

def moon():
    D = 13; g = disc(D, True)
    for (cx, cy, s) in ((3, 4, 2), (7, 7, 3), (8, 2, 2), (3, 9, 2)):
        for y in range(cy, cy + s):
            for x in range(cx, cx + s): g[y][x] = False
    return g, disc(D, True)

def crescent():
    D = 11; g = disc(D, True); cut = disc(D, True); sil = grid(D, D)
    for y in range(D):
        for x in range(D):
            sil[y][x] = g[y][x]
            if x + 3 < D and cut[y][x + 3]: g[y][x] = False
    return g, sil

def saturn():
    Wd, Hd, cx, cy = 25, 13, 12, 6
    g = grid(Wd, Hd); sil = grid(Wd, Hd)
    body = disc(11, False); bfill = disc(11, True)
    for y in range(11):
        for x in range(11):
            if body[y][x]: g[cy - 5 + y][cx - 5 + x] = True
            if bfill[y][x]: sil[cy - 5 + y][cx - 5 + x] = True
    a, b = 12, 3.2
    for x in range(Wd):
        for y in range(Hd):
            dx, dy = (x - cx) / a, (y - cy) / b
            e = dx * dx + dy * dy
            if 0.72 <= e <= 1.0:
                inside_body = bfill[y - cy + 5][x - cx + 5] if 0 <= y - cy + 5 < 11 and 0 <= x - cx + 5 < 11 else False
                if not inside_body or y > cy:
                    g[y][x] = True
                sil[y][x] = True
    return g, sil

def dilate(g, n=1):
    h, w = len(g), len(g[0]); o = grid(w, h)
    for y in range(h):
        for x in range(w):
            if g[y][x]:
                for dy in range(-n, n + 1):
                    for dx in range(-n, n + 1):
                        if 0 <= y+dy < h and 0 <= x+dx < w: o[y+dy][x+dx] = True
    return o

def parse_artist(path):
    objs, name = {}, None
    for line in pathlib.Path(path).read_text().splitlines():
        m = re.match(r'\[(\w+)\]', line)
        if m:
            name = m.group(1).lower(); objs[name] = []
        elif name and line and set(line) <= {'#', '.'}:
            objs[name].append([c == '#' for c in line])
    return {k: v for k, v in objs.items() if v}

def fill_holes(g):
    """Silhouette: every dark pixel not reachable from outside the box becomes part of it."""
    h, w = len(g), len(g[0])
    seen = grid(w + 2, h + 2); stack = [(0, 0)]
    while stack:
        y, x = stack.pop()
        if not (0 <= y < h + 2 and 0 <= x < w + 2) or seen[y][x]: continue
        if 0 <= y - 1 < h and 0 <= x - 1 < w and g[y-1][x-1]: continue
        seen[y][x] = True
        stack += [(y+1, x), (y-1, x), (y, x+1), (y, x-1)]
    return [[not seen[y+1][x+1] for x in range(w)] for y in range(h)]

def library():
    lib = {k: (v, fill_holes(v)) for k, v in parse_objects(OUT / 'work/objects_placeholder.txt').items()}
    for k, v in parse_artist(OUT / 'objects/OBJECTS.txt').items():
        lib[k] = (v, fill_holes(v))
    return lib

# ---- placement --------------------------------------------------------------
FAR = [('star_2', x, y) for x, y in [(4,6),(18,33),(31,57),(43,15),(55,45),(66,2),(74,37),(88,22),(99,56),(111,9),(121,41),(9,48),(93,43),(126,26)]]
MID = [('saturn',180,0), ('moon_s',200,54), ('dipper',20,40)] + \
      [('star_7',x,y) for x,y in [(100,40),(230,8)]] + \
      [('star_5',x,y) for x,y in [(20,26),(150,58),(185,20)]] + \
      [('star_4',x,y) for x,y in [(12,50),(120,4),(175,44),(245,30)]] + \
      [('star_3a',x,y) for x,y in [(40,30),(90,14),(140,30),(220,52)]] + \
      [('star_3b',x,y) for x,y in [(75,56),(165,10),(250,48)]]
NEAR = [('earth',100,12), ('moon_l',150,2), ('comet_l',230,4), ('giant',300,32), ('crescent_l',400,38),
        ('asteroid',450,48), ('asteroid_s',468,58), ('probe',30,50)]

def cluster():
    g = grid(12, 8)
    for x, y in [(0,2),(4,0),(8,3),(3,5),(9,6),(6,2)]:
        g[y][x] = g[y][x+1] = g[y+1][x] = g[y+1][x+1] = True
    return g, g

def build_plane(spec, period, lib):
    artg, mask = grid(period, H), grid(period, H)
    for name, ox, oy in spec:
        a, s = lib[name]
        sd = dilate(s)
        for y in range(len(a)):
            for x in range(len(a[0])):
                X, Y = (ox + x) % period, oy + y
                if 0 <= Y < H:
                    if a[y][x]: artg[Y][X] = True
                    if sd[y][x]: mask[Y][X] = True
    return artg, mask

def to_pages(g, width):
    return [bytes(sum((1 << b) for b in range(8) if g[p*8+b][x]) for x in range(width)) for p in range(8)]

# ---- compositing, exactly as the C will ------------------------------------
def frame(planes, state, wpm, ship, shipmask):
    split = wpm // 4
    out = []
    offs = {k: (state >> sh) for k, (P, sh) in PLANES.items()}
    for p in range(8):
        row = bytearray(W)
        for i in range(W):
            f = planes['far'][0][p][(offs['far'] + i) % 128]
            m, mm = planes['mid'][0][p][(offs['mid'] + i) % 256], planes['mid'][1][p][(offs['mid'] + i) % 256]
            n, nm = planes['near'][0][p][(offs['near'] + i) % 512], planes['near'][1][p][(offs['near'] + i) % 512]
            b = (((f & ~mm) | m) & ~nm) | n
            if 2 <= p <= 5 and i >= split:
                b = (b & shipmask[p-2][i-split]) | ship[p-2][i-split]
            row[i] = b & 0xFF
        out.append(bytes(row))
    return b''.join(out)

def dirty_blocks(a, b):
    return sum(1 for k in range(16) if a[k*64:(k+1)*64] != b[k*64:(k+1)*64])

def isolated(g, wrap=True, eight=False):
    h, w = len(g), len(g[0]); n = 0
    nb = [(0,1),(0,-1),(1,0),(-1,0)] + ([(1,1),(1,-1),(-1,1),(-1,-1)] if eight else [])
    for y in range(h):
        for x in range(w):
            if not g[y][x]: continue
            if not any(0 <= y+dy < h and g[y+dy][(x+dx) % w if wrap else x+dx] for dy, dx in nb if wrap or 0 <= x+dx < w):
                n += 1
    return n

def main():
    lib = library()
    planes = {}
    grids = {}
    for name, spec in (('far', FAR), ('mid', MID), ('near', NEAR)):
        P, sh = PLANES[name]
        a, m = build_plane(spec, P, lib)
        grids[name] = (a, m)
        planes[name] = (to_pages(a, P), to_pages(m, P))
    A = art.arrays()
    ship = [A['ship_row_%d' % i] for i in range(1, 5)]
    shipmask = [A['mask_row_%d' % i] for i in range(1, 5)]

    # loop closure: frame at state s equals frame at s+LOOP, by construction of the modulo; check the wrap step
    for name, (P, sh) in PLANES.items():
        for step in (1, 4, 7):
            d_in = ((LOOP - step) >> sh)   # offset at last state before wrap
            d_wrap = ((0 >> sh) + P - ((LOOP - step) >> sh))  # move across the wrap
            d_mid = ((100 + step) >> sh) - (100 >> sh)
            print(f'{name}: step {step}: move across wrap {d_wrap}, typical move {d_mid} ({step}/{1<<sh})')

    for wpm in (0, 50):
        step = 1 + wpm // 15
        states = list(range(0, LOOP, step))
        frames = [frame(planes, s, wpm, ship, shipmask) for s in states]
        assert frame(planes, (states[-1] + step) % LOOP, wpm, ship, shipmask) == frames[0], 'loop does not close'
        bits = [preview.bits(f) for f in frames]
        preview.gif(OUT / f'loop_wpm{wpm}.gif', bits, [5] * len(bits), scale=3)
        db = [dirty_blocks(frames[i-1], frames[i]) for i in range(1, len(frames))] + [dirty_blocks(frames[-1], frames[0])]
        iso4 = [isolated(b, wrap=False) for b in bits]; iso8 = [isolated(b, wrap=False, eight=True) for b in bits]
        lit = [sum(v for r in b for v in r) for b in bits]
        print(f'wpm {wpm}: {len(frames)} frames/loop, {len(frames)*0.05:.1f}s; dirty blocks/tick min {min(db)} mean {sum(db)/len(db):.2f} max {max(db)}; '
              f'isolated(4) max {max(iso4)} mean {sum(iso4)/len(iso4):.2f}; isolated(8) max {max(iso8)}; lit {min(lit)}-{max(lit)} px ({sum(lit)/len(lit)/8192*100:.1f}%)')
        if wpm == 0:
            # which frames have isolated pixels, and where
            worst = max(range(len(bits)), key=lambda i: iso4[i])
            print('  worst frame', worst, 'isolated', iso4[worst])

    # per-plane bloom check and ASCII data
    with open(OUT / 'LAYERS.txt', 'w') as f:
        f.write('# Spaceship background planes. # = lit. Each plane is one period wide; the C strip is period+128 columns\n'
                '# with columns 0..127 repeated after the period. 64 rows = 8 pages, bit 0 of a page byte is the top row.\n'
                '# Masks: # = occludes (background planes hidden there). Store in C as the complement (1 = show through),\n'
                '# the same convention as the ship mask, so the read is ((far & midmask) | mid) & nearmask | near.\n'
                f'# LOOP (OLED_SCROLL_MODULO) = {LOOP}.  far: period 128, offset = state >> 2.  mid: period 256, offset = state >> 1.\n'
                '# near: period 512, offset = state.\n\n')
        for name in ('far', 'mid', 'near'):
            a, m = grids[name]; P = PLANES[name][0]
            print(f'plane {name}: {P} cols, lit {sum(v for r in a for v in r)} px, isolated(4) {isolated(a)}, isolated(8) {isolated(a, eight=True)}')
            f.write(f'@ {name} {P}x{H}\n' + art.ascii(a) + '\n\n')
            if name != 'far':
                f.write(f'@ {name}_mask {P}x{H}\n' + art.ascii(m) + '\n\n')
    with open(OUT / 'objects_used.txt', 'w') as f:
        for k in sorted(lib):
            a, s = lib[k]
            f.write(f'@ {k} {len(a[0])}x{len(a)}\n' + art.ascii(a) + '\n\n')

    # strips as PNG, one per plane, art over a dim mask
    LIT, DARK, MSK = (214, 232, 255), (12, 14, 18), (40, 44, 60)
    sc = 2
    for name in ('far', 'mid', 'near'):
        a, m = grids[name]; P = PLANES[name][0]
        im = Image.new('RGB', (P * sc, H * sc), DARK); px = im.load()
        for y in range(H):
            for x in range(P):
                c = LIT if a[y][x] else (MSK if m[y][x] else DARK)
                for dy in range(sc):
                    for dx in range(sc): px[x*sc+dx, y*sc+dy] = c
        im.save(OUT / f'strip_{name}.png')
    # contact sheet: 16 frames across the rest loop
    sheet = Image.new('RGB', (4 * (W*2 + 6) + 6, 4 * (H*2 + 6) + 6), (30, 30, 34))
    for k in range(16):
        s = k * (LOOP // 16)
        b = preview.bits(frame(planes, s, 0, ship, shipmask))
        im = Image.new('RGB', (W, H), DARK); px = im.load()
        for y in range(H):
            for x in range(W):
                if b[y][x]: px[x, y] = LIT
        sheet.paste(im.resize((W*2, H*2), Image.NEAREST), (6 + (k % 4) * (W*2 + 6), 6 + (k // 4) * (H*2 + 6)))
    sheet.save(OUT / 'contact_rest.png')

if __name__ == '__main__':
    main()
