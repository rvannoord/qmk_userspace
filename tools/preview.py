"""Turn a preview frame dump into something you can look at.

    python3 -I preview.py <dir>            # strip.png and anim.gif
    python3 -I preview.py <dir> --invert   # as the panel looks with black and white swapped

Reads `frames.bin` (1024 bytes a tick, the panel buffer in page format) and
`events.csv` from a `tests/preview` run, and writes:

  strip.png   every frame that differs from the one before it, with its time
  anim.gif    the whole run at real timing, identical frames merged into longer delays

The GIF encoder is here rather than imported because this repo installs nothing: it is
about sixty lines of LZW and it only ever has to emit two colours.
"""

import pathlib
import struct
import sys
import zlib

W, H = 128, 64
FRAME = W * H // 8

LIT = (214, 232, 255)
DARK = (12, 14, 18)
BG = (24, 24, 28)
LABEL = (120, 150, 190)

# A 3x5 digit font, so frames can carry their timestamp without anything installed.
DIGITS = {
    '0': '111101101101111', '1': '010010010010010', '2': '111001111100111',
    '3': '111001111001111', '4': '101101111001001', '5': '111100111001111',
    '6': '111100111101111', '7': '111001001001001', '8': '111101111101111',
    '9': '111101111001111', '.': '000000000000010', 's': '011100010001110',
    ' ': '000000000000000',
}


def frames_of(path):
    d = pathlib.Path(path).read_bytes()
    return [d[i:i + FRAME] for i in range(0, len(d) - FRAME + 1, FRAME)]


def bits(frame, invert=False):
    """Page format to rows of bool: a byte is 8 vertical pixels, bit 0 at the top."""
    out = []
    for y in range(H):
        row = []
        for x in range(W):
            on = (frame[(y // 8) * W + x] >> (y % 8)) & 1
            row.append(bool(on) != invert)
        out.append(row)
    return out


# --- PNG ---------------------------------------------------------------------

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


def blit(canvas, frame_bits, ox, oy, scale):
    for y, row in enumerate(frame_bits):
        for x, on in enumerate(row):
            col = LIT if on else DARK
            for dy in range(scale):
                for dx in range(scale):
                    cy, cx = oy + y * scale + dy, ox + x * scale + dx
                    if 0 <= cy < len(canvas) and 0 <= cx < len(canvas[0]):
                        canvas[cy][cx] = col


def label(canvas, s, ox, oy):
    for i, ch in enumerate(s):
        g = DIGITS.get(ch)
        if not g:
            continue
        for y in range(5):
            for x in range(3):
                if g[y * 3 + x] == '1':
                    cy, cx = oy + y, ox + i * 4 + x
                    if 0 <= cy < len(canvas) and 0 <= cx < len(canvas[0]):
                        canvas[cy][cx] = LABEL


# --- GIF ---------------------------------------------------------------------

def lzw(data, min_bits):
    """GIF's variable-width LZW. Two colours in, so min_bits is 2."""
    clear, end = 1 << min_bits, (1 << min_bits) + 1
    width = min_bits + 1
    table = {bytes([i]): i for i in range(1 << min_bits)}
    nxt = end + 1
    out, acc, nacc = bytearray(), 0, 0

    def emit(code):
        nonlocal acc, nacc
        acc |= code << nacc
        nacc += width
        while nacc >= 8:
            out.append(acc & 0xFF)
            acc >>= 8
            nacc -= 8

    emit(clear)
    buf = b''
    for byte in data:
        nb = buf + bytes([byte])
        if nb in table:
            buf = nb
            continue
        emit(table[buf])
        table[nb] = nxt
        nxt += 1
        if nxt > (1 << width) and width < 12:
            width += 1
        elif nxt > 4095:
            emit(clear)
            table = {bytes([i]): i for i in range(1 << min_bits)}
            nxt = end + 1
            width = min_bits + 1
        buf = bytes([byte])
    if buf:
        emit(table[buf])
    emit(end)
    if nacc:
        out.append(acc & 0xFF)
    return bytes(out)


def gif(path, frames_bits, delays_cs, scale=2):
    gw, gh = W * scale, H * scale
    with open(path, 'wb') as f:
        f.write(b'GIF89a')
        f.write(struct.pack('<HHBBB', gw, gh, 0xF0, 0, 0))       # 2-colour global table
        f.write(bytes(DARK) + bytes(LIT))
        f.write(b'\x21\xFF\x0BNETSCAPE2.0\x03\x01\x00\x00\x00')  # loop for ever
        for fb, cs in zip(frames_bits, delays_cs):
            f.write(b'\x21\xF9\x04\x00' + struct.pack('<H', max(2, min(cs, 65535))) + b'\x00\x00')
            f.write(b'\x2C' + struct.pack('<HHHHB', 0, 0, gw, gh, 0))
            px = bytearray()
            for row in fb:
                line = bytearray()
                for on in row:
                    line += bytes([1 if on else 0]) * scale
                px += line * scale
            f.write(bytes([2]))
            data = lzw(bytes(px), 2)
            for i in range(0, len(data), 255):
                block = data[i:i + 255]
                f.write(bytes([len(block)]) + block)
            f.write(b'\x00')
        f.write(b'\x3B')


# --- main --------------------------------------------------------------------

def main(d, invert=False):
    d = pathlib.Path(d)
    fr = frames_of(d / 'frames.bin')
    if not fr:
        print('no frames'); return

    # Merge runs of identical frames: that is what the panel actually shows, and it is
    # also what makes a 130-second GIF a reasonable size.
    runs = []
    for i, f in enumerate(fr):
        if runs and runs[-1][0] == f:
            runs[-1][1] += 1
        else:
            runs.append([f, 1, i])
    print(f'{len(fr)} ticks, {len(runs)} distinct frames')

    gif(d / 'anim.gif', [bits(f, invert) for f, _, _ in runs],
        [max(2, n * 5) for _, n, _ in runs])

    cols = 10
    scale, pad, gap, lbl = 2, 10, 6, 7
    rows_n = (len(runs) + cols - 1) // cols
    cw = pad * 2 + cols * (W * scale + gap)
    ch = pad * 2 + rows_n * (H * scale + lbl + gap)
    canvas = [[BG] * cw for _ in range(ch)]
    for i, (f, n, first) in enumerate(runs):
        col, row = i % cols, i // cols
        x = pad + col * (W * scale + gap)
        y = pad + row * (H * scale + lbl + gap)
        label(canvas, f'{first * 0.05:.1f}s', x, y)
        blit(canvas, bits(f, invert), x, y + lbl, scale)
    png(d / 'strip.png', canvas)
    print(f'-> {d}/strip.png and {d}/anim.gif')


if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    main(args[0] if args else '.', '--invert' in sys.argv)
