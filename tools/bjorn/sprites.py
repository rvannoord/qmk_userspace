"""Bjorn's upstream sprite sets -> contact sheets, for deciding what is usable.

    python3 -I sprites.py <bjorn-src>/resources/images <out-dir>

Reads the 78x78 BMP frames and 28x28 icons from the upstream repo, reduces each one
the way the panel needs it — greyscale, area-weighted box filter to 64x64, then
Floyd-Steinberg in two separate 64-column passes — and writes PNG contact sheets to
look at. Nothing here writes C: this is the survey, and the art generator is
gen_bjorn.py, which takes hand-made grids.

Two passes rather than one across the whole width because the panel's two halves are
composed independently; error diffused across x=63 would make a change on one side
alter pixels on the other.

The source images are data from outside this repo. They are read, never executed, and
this script lives apart from them on purpose.
"""

import pathlib
import struct
import sys
import zlib


# --- BMP ---------------------------------------------------------------------

def read_bmp(path):
    """-> (w, h, rows of 0..255 grey), top row first."""
    d = pathlib.Path(path).read_bytes()
    if d[:2] != b'BM':
        raise ValueError(f'{path}: not a BMP')
    offset = struct.unpack('<I', d[10:14])[0]
    w, h = struct.unpack('<ii', d[18:26])
    planes, bpp = struct.unpack('<HH', d[26:30])
    compression = struct.unpack('<I', d[30:34])[0]
    if compression != 0 or planes != 1:
        raise ValueError(f'{path}: compressed or multi-plane BMPs are not handled')

    bottom_up = h > 0
    h = abs(h)
    stride = ((bpp * w + 31) // 32) * 4      # rows are padded to four bytes

    palette = []
    if bpp <= 8:
        n = struct.unpack('<I', d[46:50])[0] or (1 << bpp)
        for i in range(n):
            b, g, r, _ = d[54 + i * 4: 58 + i * 4]
            palette.append((r * 299 + g * 587 + b * 114) // 1000)

    rows = []
    for y in range(h):
        base = offset + y * stride
        row = []
        for x in range(w):
            if bpp == 24:
                b, g, r = d[base + x * 3: base + x * 3 + 3]
                row.append((r * 299 + g * 587 + b * 114) // 1000)
            elif bpp == 32:
                b, g, r = d[base + x * 4: base + x * 4 + 3]
                row.append((r * 299 + g * 587 + b * 114) // 1000)
            elif bpp == 8:
                row.append(palette[d[base + x]])
            elif bpp in (1, 4):
                per = 8 // bpp
                byte = d[base + x // per]
                shift = (per - 1 - (x % per)) * bpp
                row.append(palette[(byte >> shift) & ((1 << bpp) - 1)])
            else:
                raise ValueError(f'{path}: {bpp} bits per pixel is not handled')
        rows.append(row)
    if bottom_up:
        rows.reverse()
    return w, h, rows


# --- reduction ---------------------------------------------------------------

def box_filter(rows, w, h, out_w, out_h):
    """Area-weighted, so 78 -> 64 keeps the weight of every source pixel."""
    out = []
    for oy in range(out_h):
        y0, y1 = oy * h / out_h, (oy + 1) * h / out_h
        line = []
        for ox in range(out_w):
            x0, x1 = ox * w / out_w, (ox + 1) * w / out_w
            total = area = 0.0
            for sy in range(int(y0), min(int(y1) + 1, h)):
                wy = min(y1, sy + 1) - max(y0, sy)
                if wy <= 0:
                    continue
                for sx in range(int(x0), min(int(x1) + 1, w)):
                    wx = min(x1, sx + 1) - max(x0, sx)
                    if wx <= 0:
                        continue
                    total += rows[sy][sx] * wy * wx
                    area += wy * wx
            line.append(total / area if area else 0.0)
        out.append(line)
    return out


def dither(grey, w, h, split=64):
    """Floyd-Steinberg, in independent passes either side of `split`."""
    out = [[0] * w for _ in range(h)]
    for x0 in range(0, w, split):
        x1 = min(x0 + split, w)
        err = [row[x0:x1] for row in grey]
        for y in range(h):
            for x in range(x1 - x0):
                old = err[y][x]
                new = 255.0 if old >= 128 else 0.0
                out[y][x0 + x] = 1 if new else 0
                e = old - new
                if x + 1 < x1 - x0:
                    err[y][x + 1] += e * 7 / 16
                if y + 1 < h:
                    if x:
                        err[y + 1][x - 1] += e * 3 / 16
                    err[y + 1][x] += e * 5 / 16
                    if x + 1 < x1 - x0:
                        err[y + 1][x + 1] += e * 1 / 16
    return out


def ink(bits):
    """How much of the frame is the FIGURE, as a fraction of 4096.

    dither() returns 1 for a bright pixel, and the source is dark ink on a white page —
    so the ones are the background and the zeros are Bjorn. Counting the ones gives
    about 76% and looks like a catastrophically over-lit image; it is the paper.

    This was got wrong once and the cost was a day's work in the wrong direction, so it
    is spelled out rather than left to the reader. The figure is what lands on the
    panel, because the frame is inverted when it is drawn: ink becomes lit."""
    return sum(len(r) - sum(r) for r in bits)


# --- PNG ---------------------------------------------------------------------

LIT, DARK, BG, LABEL = (214, 232, 255), (12, 14, 18), (24, 24, 28), (120, 150, 190)


def png(path, rows):
    h, w = len(rows), len(rows[0])
    raw = b''.join(b'\x00' + bytes(v for px in r for v in px) for r in rows)

    def chunk(tag, data):
        c = tag + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c))

    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n')
        f.write(chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)))
        f.write(chunk(b'IDAT', zlib.compress(raw, 9)))
        f.write(chunk(b'IEND', b''))


def canvas(w, h, colour=BG):
    return [[colour] * w for _ in range(h)]


def blit_bits(c, bits, ox, oy, scale, lit=LIT, dark=DARK):
    for y, row in enumerate(bits):
        for x, on in enumerate(row):
            col = lit if on else dark
            for dy in range(scale):
                for dx in range(scale):
                    cy, cx = oy + y * scale + dy, ox + x * scale + dx
                    if 0 <= cy < len(c) and 0 <= cx < len(c[0]):
                        c[cy][cx] = col


def blit_grey(c, grey, ox, oy, scale):
    for y, row in enumerate(grey):
        for x, v in enumerate(row):
            g = max(0, min(255, int(v)))
            col = (g, g, g)
            for dy in range(scale):
                for dx in range(scale):
                    cy, cx = oy + y * scale + dy, ox + x * scale + dx
                    if 0 <= cy < len(c) and 0 <= cx < len(c[0]):
                        c[cy][cx] = col


# A 3x5 digit and capital font, so the sheets can label themselves without needing
# anything installed.
GLYPHS = {
    '0': '111101101101111', '1': '010010010010010', '2': '111001111100111',
    '3': '111001111001111', '4': '101101111001001', '5': '111100111001111',
    '6': '111100111101111', '7': '111001001001001', '8': '111101111101111',
    '9': '111101111001111', '-': '000000111000000', '.': '000000000000010',
    ' ': '000000000000000', '/': '001001010100100', ':': '000010000010000',
    'A': '111101111101101', 'B': '110101110101110', 'C': '111100100100111',
    'D': '110101101101110', 'E': '111100110100111', 'F': '111100110100100',
    'G': '111100101101111', 'H': '101101111101101', 'I': '111010010010111',
    'J': '001001001101111', 'K': '101101110101101', 'L': '100100100100111',
    'M': '101111111101101', 'N': '101111111111101', 'O': '111101101101111',
    'P': '111101111100100', 'Q': '111101101111011', 'R': '111101110101101',
    'S': '111100111001111', 'T': '111010010010010', 'U': '101101101101111',
    'V': '101101101101010', 'W': '101101111111101', 'X': '101101010101101',
    'Y': '101101010010010', 'Z': '111001010100111',
}


def label(c, s, ox, oy, scale=1, colour=LABEL):
    for i, ch in enumerate(s.upper()):
        g = GLYPHS.get(ch)
        if not g:
            continue
        for y in range(5):
            for x in range(3):
                if g[y * 3 + x] == '1':
                    for dy in range(scale):
                        for dx in range(scale):
                            cy, cx = oy + y * scale + dy, ox + (i * 4 + x) * scale + dx
                            if 0 <= cy < len(c) and 0 <= cx < len(c[0]):
                                c[cy][cx] = colour


# --- the sheets --------------------------------------------------------------

def frames_of(d):
    """Frames and the set's icon, told apart by size: frames are 78x78, icons 28x28.

    Upstream uses "the file name contains a digit" (shared.py), which miscounts
    LogStandalone2, whose icon is named LogStandalone2.bmp and so looks like a frame
    to that rule. Size is what actually distinguishes them."""
    frames, icons = [], []
    for p in sorted(d.glob('*.bmp'), key=lambda q: (len(q.stem), q.stem)):
        w, h, _ = read_bmp(p)
        (frames if (w, h) == (78, 78) else icons).append(p)
    return frames, icons


def reduce_frame(path):
    w, h, rows = read_bmp(path)
    grey = box_filter(rows, w, h, 64, 64)
    return rows, grey, dither(grey, 64, 64)


def main(src, out):
    src, out = pathlib.Path(src), pathlib.Path(out)
    out.mkdir(parents=True, exist_ok=True)
    sets = sorted(p for p in (src / 'status').iterdir() if p.is_dir())

    S, PAD, GAP, LBL = 3, 14, 8, 8          # 64 * 3 = 192 px a frame
    overview_cols = 8
    tiles = []

    for d in sets:
        frames, icons = frames_of(d)
        print(f'{d.name}: {len(frames)} frames, {len(icons)} icon(s)')

        # one sheet per set: source grey, reduced grey, and the 1-bit result
        cols = max(1, min(len(frames), 6))
        rows_n = (len(frames) + cols - 1) // cols
        cw = PAD * 2 + cols * (64 * S + GAP)
        ch = PAD * 2 + LBL + rows_n * (64 * S * 3 + LBL * 3 + GAP * 3)
        c = canvas(cw, ch)
        label(c, d.name, PAD, PAD, 2)
        y = PAD + LBL + 6
        for i, f in enumerate(frames):
            col, row = i % cols, i // cols
            x = PAD + col * (64 * S + GAP)
            yy = y + row * (64 * S * 3 + LBL * 3 + GAP * 3)
            srcrows, grey, bits = reduce_frame(f)
            label(c, f.stem[:18], x, yy, 1)
            blit_grey(c, box_filter(srcrows, len(srcrows[0]), len(srcrows), 64, 64), x, yy + LBL, S)
            label(c, f'{ink(bits)*100//4096}PC FIGURE', x, yy + LBL + 64 * S + 2, 1)
            # inverted, because that is how it reaches the panel: ink lit, paper dark
            blit_bits(c, [[v == 0 for v in r] for r in bits], x, yy + LBL * 2 + 64 * S + 4, S)
            tiles.append((f'{d.name}/{f.stem}', bits))
        png(out / f'{d.name}.png', c)

    # one overview of every frame at the size it would actually be drawn
    rows_n = (len(tiles) + overview_cols - 1) // overview_cols
    cw = PAD * 2 + overview_cols * (64 * 2 + GAP)
    ch = PAD * 2 + rows_n * (64 * 2 + LBL + GAP)
    c = canvas(cw, ch)
    for i, (name, bits) in enumerate(tiles):
        col, row = i % overview_cols, i // overview_cols
        x = PAD + col * (64 * 2 + GAP)
        y = PAD + row * (64 * 2 + LBL + GAP)
        label(c, name.split('/')[-1][:16], x, y, 1)
        blit_bits(c, [[v == 0 for v in r] for r in bits], x, y + LBL, 2)
    png(out / 'ALL.png', c)
    print(f'{len(tiles)} frames -> {out}')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
