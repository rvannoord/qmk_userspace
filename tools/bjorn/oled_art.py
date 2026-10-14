#!/usr/bin/env python3
"""SSD1306 page-format art pipeline: ASCII grid <-> C header <-> PNG preview.

Page format (QMK oled_write_raw): the buffer is HEIGHT/8 pages of WIDTH bytes.
Byte at (page p, column x) holds 8 vertical pixels, bit 0 = topmost.

  grid2c   grid.txt name          -> C array on stdout
  c2png    header.h name out.png  -> preview PNG (parses `name[] = { ... }`)
  grid2png grid.txt out.png       -> preview PNG straight from a grid
  png-scale defaults to 4; pass --scale N to change.
"""
import re, sys, zlib, struct

ON, OFF = '#', '.'

def read_grid(path):
    rows = [l.rstrip('\n') for l in open(path) if l.strip() and not l.lstrip().startswith('//')]
    w = max(len(r) for r in rows)
    rows = [r.ljust(w, OFF) for r in rows]
    h = len(rows)
    if h % 8:
        raise SystemExit(f"height {h} is not a multiple of 8")
    return w, h, [[1 if c == ON else 0 for c in r] for r in rows]

def pixels_to_bytes(w, h, px):
    out = []
    for page in range(h // 8):
        for x in range(w):
            b = 0
            for bit in range(8):
                if px[page * 8 + bit][x]:
                    b |= 1 << bit
            out.append(b)
    return out

def bytes_to_pixels(w, h, data):
    px = [[0] * w for _ in range(h)]
    for page in range(h // 8):
        for x in range(w):
            b = data[page * w + x]
            for bit in range(8):
                px[page * 8 + bit][x] = (b >> bit) & 1
    return px

def parse_c(path, name):
    src = open(path).read()
    m = re.search(re.escape(name) + r'\s*\[\s*([0-9]*)\s*\]\s*=\s*\{(.*?)\}\s*;', src, re.S)
    if not m:
        raise SystemExit(f"array {name} not found in {path}")
    declared = int(m.group(1)) if m.group(1).strip() else None
    vals = [int(v, 0) for v in re.findall(r'0x[0-9a-fA-F]+|\b\d+\b', m.group(2))]
    if declared:
        vals = vals[:declared]
    return vals

def write_png(path, w, h, px, scale=4):
    W, H = w * scale, h * scale
    raw = bytearray()
    for y in range(H):
        raw.append(0)
        row = px[y // scale]
        for x in range(W):
            raw.append(255 if row[x // scale] else 0)
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    png = (b'\x89PNG\r\n\x1a\n'
           + chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 0, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(bytes(raw), 9))
           + chunk(b'IEND', b''))
    open(path, 'wb').write(png)

def emit_c(name, w, h, data):
    print(f"// {name}: {w}x{h} px, {h//8} page(s), page-major then column")
    print(f"static const char PROGMEM {name}[{len(data)}] = {{")
    for p in range(h // 8):
        print(f"    // page {p}")
        row = data[p * w:(p + 1) * w]
        for i in range(0, len(row), 16):
            print("    " + " ".join(f"0x{b:02x}," for b in row[i:i + 16]))
    print("};")

if __name__ == '__main__':
    a = sys.argv[1:]
    scale = 4
    if '--scale' in a:
        i = a.index('--scale'); scale = int(a[i + 1]); del a[i:i + 2]
    cmd = a[0]
    if cmd == 'grid2c':
        w, h, px = read_grid(a[1]); emit_c(a[2], w, h, pixels_to_bytes(w, h, px))
    elif cmd == 'c2png':
        data = parse_c(a[1], a[2])
        w = int(a[4]) if len(a) > 4 else 128
        h = len(data) // w * 8
        write_png(a[3], w, h, bytes_to_pixels(w, h, data), scale)
        print(f"{a[2]}: {len(data)} bytes -> {w}x{h} -> {a[3]}")
    elif cmd == 'grid2png':
        w, h, px = read_grid(a[1]); write_png(a[2], w, h, px, scale)
        print(f"{w}x{h} -> {a[2]}")
    else:
        raise SystemExit(__doc__)
