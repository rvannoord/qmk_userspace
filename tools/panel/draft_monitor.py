"""Draft the Monitor ERP logos at panel size -> tools/panel/monitor/*.txt

    python3 -I tools/panel/draft_monitor.py [--sheet <png>]

Reads the brand files in tools/panel/monitor/src/, which are kept out of git (see
.gitignore): the horizontal and stacked "Monitor ERP" logos and the "Monitor your future"
wordmark.
Writes one text grid per bitmap, '#' lit and '.' dark, which is what gen_monitor.py turns
into art/ and what a hand pass edits. Re-running overwrites those edits, so a pass that
should survive belongs in GLYPH_FIX below, not in the grids.

Drawn rather than downscaled, because a downscale lands every letter wherever the scale
puts it and blurs it there:

- The ring is a filled disc peeled twice by one pixel, through straight neighbours only,
  so it is two pixels thick all the way round and steps evenly on the diagonals.
- The three inner shapes are sampled at pixel centres, which keeps their straight edges
  crisp, and the two diagonal gaps are cut exactly two pixels wide along lines fitted to
  the original's edges, so they read as two clean parallel slants.
- Each letter is drawn once and used wherever it repeats, at the original's position, its
  baseline and left edge snapped to whole pixels: rasterized from the logo itself, then
  replaced by the hand pass in GLYPH_FIX, which is what the grids carry. The wordmark is DM
  Sans at about 650; DM Sans as FreeType renders it was drawn too and compared on
  2026-10-22, and this own font was picked for its even weight at the small sizes
  (docs/layout.md, "The Monitor ERP screens").
"""

import argparse
import math
import pathlib

from PIL import Image, ImageDraw

HERE = pathlib.Path(__file__).resolve().parent
SRC = HERE / 'monitor' / 'src'
OUT = HERE / 'monitor'


def coverage(name, white=False):
    im = Image.open(SRC / name).convert('RGBA')
    w, h = im.size
    g = Image.new('L', (w, h), 0)
    ip, gp = im.load(), g.load()
    for y in range(h):
        for x in range(w):
            r, gg, b, a = ip[x, y]
            lum = (r + gg + b) / 765
            gp[x, y] = int(255 * a / 255 * (lum if white else 1 - lum))
    return g


def to_grid(img):
    p = img.load()
    return [[p[x, y] >= 128 for x in range(img.width)] for y in range(img.height)]


def write_grid(name, grid, note):
    with open(OUT / f'{name}.txt', 'w') as f:
        f.write(f'// {note}\n')
        for row in grid:
            f.write(''.join('#' if v else '.' for v in row) + '\n')


# --- the mark --------------------------------------------------------------------------

def ring(d):
    """Two pixels thick everywhere: the disc, minus the disc peeled twice."""
    c = d / 2
    disc = [[math.hypot(x + 0.5 - c, y + 0.5 - c) <= c for x in range(d)] for y in range(d)]

    def peel(g):
        return [[g[y][x] and all(0 <= y + dy < d and 0 <= x + dx < d and g[y + dy][x + dx]
                                 for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                 for x in range(d)] for y in range(d)]

    inner = peel(peel(disc))
    return [[disc[y][x] and not inner[y][x] for x in range(d)] for y in range(d)]


def gap_lines(src, box):
    """The two gaps between the inner shapes, as centre lines x = a + k*y in source pixels."""
    x0, y0, x1, y1 = box
    p = src.load()
    edges = {'l1': [], 'm0': [], 'm1': [], 'r0': []}
    for y in range(y0, y1):
        xs = [x for x in range(x0, x1) if p[x, y] >= 128]
        runs = []
        if xs:
            st = pv = xs[0]
            for x in xs[1:]:
                if x != pv + 1:
                    runs.append((st, pv))
                    st = x
                pv = x
            runs.append((st, pv))
        if len(runs) == 3:
            edges['l1'].append((y, runs[0][1] + 1))
            edges['m0'].append((y, runs[1][0]))
            edges['m1'].append((y, runs[1][1] + 1))
            edges['r0'].append((y, runs[2][0]))

    def fit(pts):
        n = len(pts)
        my = sum(y for y, _ in pts) / n
        mx = sum(x for _, x in pts) / n
        k = sum((y - my) * (x - mx) for y, x in pts) / sum((y - my) ** 2 for y, _ in pts)
        return mx - k * my, k

    lines = []
    for a, b in (('l1', 'm0'), ('m1', 'r0')):
        (a1, k1), (a2, k2) = fit(edges[a]), fit(edges[b])
        lines.append(((a1 + a2) / 2, (k1 + k2) / 2))
    return lines


def mark(src, box, d, gap_px=2.0):
    """The mark at d pixels: the ring, and the inner shapes sampled at pixel centres."""
    x0, y0, x1, _ = box
    s = d / (x1 - x0)
    ring_px = round(34 * (x1 - x0) / 504 * 1)          # the ring, in source pixels
    cx, cy = x0 + (x1 - x0) / 2, y0 + (x1 - x0) / 2
    inside = (x1 - x0) / 2 - ring_px - 25               # past the ring and half its gap
    lines = gap_lines(src, (int(cx - inside), int(cy - inside), int(cx + inside), int(cy + inside)))
    p = src.load()
    g = ring(d)
    for py in range(d):
        for px in range(d):
            X = x0 + (px + 0.5) / s
            Y = y0 + (py + 0.5) / s
            if math.hypot(X - cx, Y - cy) >= inside:
                continue
            if any(abs(X - (a + k * Y)) * s < gap_px / 2 for a, k in lines):
                continue
            if p[int(X), int(Y)] >= 128:
                g[py][px] = True
    # The three shapes share one flat bottom in the original, which falls close to a pixel
    # edge here, so the anti-aliasing can give one of them an extra row. End all three on
    # the last row where the right one, the flattest, still has ink.
    bottom = max(py for py in range(d) for px in range(d * 2 // 3, d - 3)
                 if g[py][px] and not ring(d)[py][px])
    for py in range(bottom + 1, d):
        for px in range(d):
            if not ring(d)[py][px]:
                g[py][px] = False
    return g


# --- letters ---------------------------------------------------------------------------

def letter_runs(src, x_from, y0, y1, gap=4):
    """Left to right, the ink's column runs from x_from on: the letters."""
    p = src.load()
    cols = [any(p[x, y] >= 128 for y in range(y0, y1)) for x in range(x_from, src.width)]
    runs, st = [], None
    for i, c in enumerate(cols + [False]):
        if c and st is None:
            st = i
        if not c and st is not None:
            runs.append((x_from + st, x_from + i - 1))
            st = None
    return runs


def raster_letter(src, x0, x1, base, cap_src, cap_px):
    """One letter, its baseline on a pixel edge and its left edge at the best sub-pixel
    phase: the one that leaves the fewest undecided pixels."""
    st = cap_px / cap_src
    w = int((x1 - x0 + 1) * st) + 2
    h = cap_px + 4
    best = None
    for fx in range(4):
        sx = x0 - fx / 4 / st
        sy = base - (cap_px + 2) / st
        big = src.transform((w * 8, h * 8), Image.EXTENT, (sx, sy, sx + w / st, sy + h / st), Image.BILINEAR)
        bp = big.load()
        for yy in range(h * 8):                       # only this letter's own columns
            for xx in range(w * 8):
                if not (x0 <= sx + xx / 8 / st <= x1):
                    bp[xx, yy] = 0
        small = big.resize((w, h), Image.BOX)
        vals = small.get_flattened_data() if hasattr(small, 'get_flattened_data') else small.getdata()
        grey = sum(1 for v in vals if 60 < v < 195)
        if best is None or grey < best[0]:
            best = (grey, small)
    g = to_grid(best[1])
    cols = [x for x in range(w) if any(g[y][x] for y in range(h))]
    return [row[cols[0]:cols[-1] + 1] for row in g], cap_px + 2


# The hand pass, by capital height and letter: the "own font". A grid here replaces the
# drafted glyph outright, and its last row sits on the baseline. One weight throughout,
# two-pixel stems and two-pixel bars, round letters stepped like the ring; the P and R
# share a bowl, and each size is the same design as the other. Drawn against the logo's
# letters rasterized on their own (what this file drafts without them).
GLYPH_FIX = {
    (11, 'M'): ['###......###', '###......###', '####....####', '####....####', '##.##..##.##', '##.##..##.##',
                '##..####..##', '##..####..##', '##...##...##', '##........##', '##........##'],
    (11, 'o'): ['..####..', '.######.', '###..###', '##....##', '##....##', '###..###', '.######.', '..####..'],
    (11, 'n'): ['##.###..', '#######.', '##...###', '##....##', '##....##', '##....##', '##....##', '##....##'],
    (11, 'i'): ['##', '##', '..', '##', '##', '##', '##', '##', '##', '##', '##'],
    (11, 't'): ['.##..', '.##..', '#####', '#####', '.##..', '.##..', '.##..', '.##..', '.####', '..###'],
    (11, 'r'): ['##.##', '#####', '###..', '##...', '##...', '##...', '##...', '##...'],
    (11, 'E'): ['#######', '#######', '##.....', '##.....', '##.....', '######.', '######.', '##.....', '##.....',
                '#######', '#######'],
    (11, 'R'): ['######..', '#######.', '##....##', '##....##', '##....##', '#######.', '######..', '##..##..',
                '##...##.', '##...##.', '##....##'],
    (11, 'P'): ['######..', '#######.', '##....##', '##....##', '##....##', '#######.', '######..', '##......',
                '##......', '##......', '##......'],

    (10, 'M'): ['###.....###', '####...####', '####...####', '##.##.##.##', '##.##.##.##', '##..###..##',
                '##..###..##', '##...#...##', '##.......##', '##.......##'],
    (10, 'o'): ['..###..', '.#####.', '##...##', '##...##', '##...##', '.#####.', '..###..'],
    (10, 'n'): ['##.##..', '######.', '##..###', '##...##', '##...##', '##...##', '##...##'],
    (10, 'i'): ['##', '##', '..', '##', '##', '##', '##', '##', '##', '##'],
    (10, 't'): ['.##..', '.##..', '#####', '#####', '.##..', '.##..', '.##..', '.###.', '..###'],
    (10, 'r'): ['##.##', '#####', '###..', '##...', '##...', '##...', '##...'],
    (10, 'E'): ['######', '######', '##....', '##....', '#####.', '#####.', '##....', '##....', '######', '######'],
    (10, 'R'): ['#####..', '######.', '##...##', '##...##', '######.', '#####..', '##.##..', '##..##.', '##..##.',
                '##...##'],
    (10, 'P'): ['#####..', '######.', '##...##', '##...##', '######.', '#####..', '##.....', '##.....', '##.....',
                '##.....'],

    # "Monitor your future" is hand lettering, not DM Sans: one-pixel strokes, as light as
    # the original's marker line. The O tapers to the bottom and the U's right stem runs
    # past its curve, as in the original; the M keeps its deep V.
    (13, 'M'): ['#.....#', '##...##', '##...##', '#.#.#.#', '#.#.#.#', '#.#.#.#', '#..#..#', '#..#..#',
                '#.....#', '#.....#', '#.....#', '#.....#', '#.....#'],
    (13, 'O'): ['.####.', '#....#', '#....#', '#....#', '#....#', '#....#', '#....#', '#....#', '#....#',
                '#....#', '#....#', '.#..#.', '..##..'],
    (13, 'N'): ['#....#', '##...#', '##...#', '#.#..#', '#.#..#', '#.#..#', '#..#.#', '#..#.#', '#..#.#',
                '#...##', '#...##', '#....#', '#....#'],
    (13, 'I'): ['#'] * 13,
    (13, 'T'): ['#####'] + ['..#..'] * 12,
    (13, 'R'): ['###..', '#..#.', '#...#', '#...#', '#...#', '#..#.', '###..', '#.#..', '#..#.', '#..#.', '#...#',
                '#...#', '#...#'],
    (13, 'Y'): ['#...#', '#...#', '#...#', '.#.#.', '.#.#.'] + ['..#..'] * 8,
    (13, 'U'): ['#...#'] * 11 + ['.#.##', '.###.'],
    (13, 'F'): ['#####'] + ['#....'] * 5 + ['####.'] + ['#....'] * 6,
    (13, 'E'): ['#####'] + ['#....'] * 5 + ['####.'] + ['#....'] * 5 + ['#####'],
}


def fixed(cap_px, ch, grid, base):
    rows = GLYPH_FIX.get((cap_px, ch))
    if not rows:
        return grid, base
    return [[c == '#' for c in r] for r in rows], len(rows)


def compose_line(src, text_runs, text, base, cap_src, cap_px, s, x_origin, width, height, top):
    """Lay one line of letters into a grid, at the original's positions."""
    canvas = [[False] * width for _ in range(height)]
    cache = {}
    base_row = top
    st = cap_px / cap_src
    first = round((text_runs[0][0] - x_origin) * s)
    right, prev_x1 = -2, None
    for ch, (x0, x1) in zip(text, text_runs):
        if ch not in cache:
            g, b = raster_letter(src, x0, x1, base, cap_src, cap_px)
            cache[ch] = fixed(cap_px, ch, g, b)
        g, b = cache[ch]
        # Spaced at the letters' own scale from the first one, which the mark's scale
        # places. Every gap is at least its own width in the original, rounded and never
        # under a pixel, so rounding can neither close a gap nor eat the word space.
        gap = 1 if prev_x1 is None else max(1, round((x0 - prev_x1 - 1) * st))
        left = max(first + round((x0 - text_runs[0][0]) * st), right + 1 + gap)
        right, prev_x1 = left + len(g[0]) - 1, x1
        for y, row in enumerate(g):
            for x, v in enumerate(row):
                Y, X = base_row - b + y, left + x
                if v and 0 <= Y < height and 0 <= X < width:
                    canvas[Y][X] = True
    return canvas


# --- the logos -------------------------------------------------------------------------

H_FILE, S_FILE, F_FILE = 'Monitor-product-logo-black.png', 'Monitor-product-logo-square-black.png', \
    'monitoryourfuture-logo-white.png'


def horizontal(src, d, cap_px):
    """The mark and 'Monitor ERP' beside it, d pixels tall."""
    box = (40, 42, 544, 546)                       # the mark in the original: 504 across
    s = d / 504
    runs = [r for r in letter_runs(src, 600, 150, 420)]
    base = 370
    width = 128
    g = [[False] * width for _ in range(d)]
    m = mark(src, box, d)
    for y in range(d):
        for x in range(d):
            g[y][x] = m[y][x]
    line = compose_line(src, runs, 'MonitorERP', base, 171, cap_px, s, 40, width, d,
                        round((base - 42) * s))
    for y in range(d):
        for x in range(width):
            g[y][x] = g[y][x] or line[y][x]
    return crop_right(g)


def stacked(src, d, cap_px, gap_rows):
    """The mark above 'Monitor ERP', as the stacked logo has it."""
    sq = coverage(S_FILE)
    runs = letter_runs(sq, 0, 1460, 1690)
    base, cap_src = 1684, 213
    text = compose_line(sq, runs, 'MonitorERP', base, cap_src, cap_px, cap_px / cap_src, runs[0][0],
                        200, cap_px + 4, cap_px + 1)
    tx = [x for row in text for x, v in enumerate(row) if v]
    tw = max(tx) - min(tx) + 1
    width = max(tw, d)
    height = d + gap_rows + cap_px + 1
    g = [[False] * width for _ in range(height)]
    m = mark(src, (40, 42, 544, 546), d)
    # The mark centred over the text, as in the original (it sits within a pixel of it).
    mx = (width - d) // 2
    for y in range(d):
        for x in range(d):
            g[y][mx + x] = m[y][x]
    ty = d + gap_rows
    for y, row in enumerate(text):
        for x, v in enumerate(row):
            if v and min(tx) <= x <= max(tx):
                X, Y = (width - tw) // 2 + x - min(tx), ty + y - 1
                if 0 <= Y < height:
                    g[Y][X] = True
    return trim(g)


def future(cap_px):
    """'Monitor your future', hand lettering, in one line."""
    fu = coverage(F_FILE, white=True)
    runs = letter_runs(fu, 0, 40, 290)
    line = compose_line(fu, runs, 'MONITORYOURFUTURE', 267, 204, cap_px, cap_px / 204, runs[0][0],
                        128, cap_px + 2, cap_px)
    return trim(line)


def trim(g):
    """Down to the ink, all four sides."""
    ys = [y for y, row in enumerate(g) if any(row)]
    xs = [x for row in g for x, v in enumerate(row) if v]
    return [row[min(xs):max(xs) + 1] for row in g[min(ys):max(ys) + 1]]


def crop_right(g):
    last = max(x for row in g for x, v in enumerate(row) if v)
    return [row[:last + 1] for row in g]


def sheet(path, panels):
    S = 6
    w = max(len(g[0]) for _, g in panels) * S + 40
    h = sum(len(g) * S + 40 for _, g in panels) + 20
    im = Image.new('RGB', (w, h), (24, 24, 28))
    d = ImageDraw.Draw(im)
    oy = 30
    for label, g in panels:
        d.text((20, oy - 18), label, fill=(220, 220, 230))
        for y, row in enumerate(g):
            for x, v in enumerate(row):
                if v:
                    d.rectangle([20 + x * S, oy + y * S, 20 + x * S + S - 2, oy + y * S + S - 2], fill=(140, 200, 255))
        oy += len(g) * S + 40
    im.save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sheet', help='also draw the drafts into this PNG')
    a = ap.parse_args()
    src = coverage(H_FILE)
    drafts = [('horizontal', horizontal(src, 33, 11)), ('band', horizontal(src, 28, 10)),
              ('stacked', stacked(src, 38, 10, 7)), ('future', future(13))]
    for name, g in drafts:
        write_grid(name, g, f'drafted by tools/panel/draft_monitor.py from the brand files; {len(g[0])}x{len(g)}')
    if a.sheet:
        sheet(a.sheet, [(f'{n} ({len(g[0])}x{len(g)})', g) for n, g in drafts])


if __name__ == '__main__':
    main()
