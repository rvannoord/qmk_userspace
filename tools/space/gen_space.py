"""The spaceship's space -> art/space_art.c / .h

    python3 -I gen_space.py <outdir>

Reads `planes.txt`, committed beside this file, and needs nothing installed.

## Three planes, one counter

The background is parallax: a far plane of stars, a mid plane, a near plane,
read at `state >> 2`, `state >> 1` and `state`. Their periods are 128, 256 and
512, so with the loop at 512 every plane is back at offset 0 exactly when the
loop wraps - the closure is arithmetic, not luck. Under the old 256 loop the
far plane would have been a 64-px tile shown twice across a 128-px screen and
nothing on the mid plane could ever leave the screen; that is why the loop
doubled.

## Why each strip is its period plus 128

A plane is authored exactly one period wide. The C reads `offset + i` for
i < 128, so the strip it indexes must be the period plus 128 columns with the
first 128 repeated at the end. That tail is appended HERE, not authored: an
authored tail can drift from its head, and then the seam shows once a loop.

## Masks

The mid and near planes carry occlusion masks so far stars do not shine through
a planet's dark continents. In `planes.txt` a mask `#` means "occludes". They
are stored COMPLEMENTED - 1 = background shows through - which is the ship
mask's own convention, so all four reads are the same AND-then-OR.

## The ship's mask is derived, not drawn

JBaguley's hand-drawn mask left a dark margin of up to 4 px around the ship. It
is not decoration: lit-on-black, a lit ship over a lit planet has no edge at
all without a dark rim - with no margin the ship vanishes into Earth, leaving
only its window. One pixel is enough to keep the edge and halves the halo, so
the mask is the ship's silhouette (enclosed holes filled, so the window stays
opaque) dilated by one, complemented. Generated from `ship_row_*` in
`art/oled_spaceship.h`, so the ship stays byte-identical and the mask can never
drift from it.

Page format throughout: a byte is eight vertical pixels, bit 0 at the top.
"""

import pathlib
import re
import sys

SCREEN = 128
PLANES = (('far', 'FAR'), ('mid', 'MID'), ('mid_mask', 'MID_MASK'),
          ('near', 'NEAR'), ('near_mask', 'NEAR_MASK'))


def read_planes(path):
    out, cur = {}, None
    for line in pathlib.Path(path).read_text().splitlines():
        if cur and len(line) == out[cur][0] and set(line) <= {'#', '.'}:
            out[cur][1].append(line)
            continue
        s = line.strip()
        if s.startswith('@'):
            name, size = s[1:].split()
            w, h = (int(v) for v in size.lower().split('x'))
            if h != 64:
                raise SystemExit(f'gen_space: {name} is {h} rows, the panel is 64')
            cur = name
            out[cur] = (w, [])
    for name, (w, rows) in out.items():
        if len(rows) != 64:
            raise SystemExit(f'gen_space: {name} has {len(rows)} rows, expected 64')
    return out


def strip(rows, period, invert):
    """8 pages x (period + SCREEN) bytes, the head repeated as the tail."""
    width = period + SCREEN
    out = []
    for page in range(8):
        row = bytearray()
        for x in range(width):
            byte = 0
            for k in range(8):
                if rows[page * 8 + k][x % period] == '#':
                    byte |= 1 << k
            row.append((~byte & 0xFF) if invert else byte)
        out.append(bytes(row))
    return out


SHIP_MARGIN = 1


def ship_mask(header_path):
    """4 pages x 128 of mask bytes, 1 = background shows through."""
    text = pathlib.Path(header_path).read_text()
    rows = []
    for n in range(1, 5):
        m = re.search(r'ship_row_%d\s*\[\s*\]\s*=\s*\{([^}]*)\}' % n, text)
        if not m:
            raise SystemExit(f'gen_space: ship_row_{n} not found in {header_path}')
        rows.append([int(v, 0) & 0xFF for v in re.findall(r'0x[0-9a-fA-F]+|\d+', m.group(1))])
    ink = [[(rows[y // 8][x] >> (y % 8)) & 1 for x in range(128)] for y in range(32)]
    # silhouette: everything not reachable from the border through dark pixels
    outside, stack = set(), [(y, x) for y in range(32) for x in (0, 127)] + [(y, x) for x in range(128) for y in (0, 31)]
    while stack:
        y, x = stack.pop()
        if (y, x) in outside or not (0 <= y < 32 and 0 <= x < 128) or ink[y][x]:
            continue
        outside.add((y, x))
        stack += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    sil = [[0 if (y, x) in outside else 1 for x in range(128)] for y in range(32)]
    r = SHIP_MARGIN
    opaque = [[1 if any(sil[yy][xx] for yy in range(max(0, y - r), min(32, y + r + 1))
                        for xx in range(max(0, x - r), min(128, x + r + 1))) else 0
               for x in range(128)] for y in range(32)]
    out = []
    for page in range(4):
        row = bytearray()
        for x in range(128):
            byte = 0
            for k in range(8):
                if not opaque[page * 8 + k][x]:
                    byte |= 1 << k            # 1 = show through
            row.append(byte)
        out.append(bytes(row))
    halo = sum(map(sum, opaque)) - sum(map(sum, ink))
    return out, halo


def main(outdir):
    here = pathlib.Path(__file__).resolve().parent
    planes = read_planes(here / 'planes.txt')
    outdir = pathlib.Path(outdir)
    banner = ("// Generated by tools/space/gen_space.py from tools/space/planes.txt.\n"
              "// Do not edit: change the planes file and regenerate.\n"
              "//\n"
              "// Parallax planes for the spaceship, page format, each its period plus 128\n"
              "// columns with the head repeated as the tail. Masks are stored complemented\n"
              "// (1 = background shows through), the ship mask's own convention.\n")
    with open(outdir / 'space_art.h', 'w') as h, open(outdir / 'space_art.c', 'w') as c:
        h.write(banner + '\n#pragma once\n\n#include <stdint.h>\n#include "progmem.h"\n\n')
        h.write('// The loop. Every plane period divides it, so the wrap is seamless.\n')
        h.write('#define U_SPACE_LOOP 512\n\n')
        c.write(banner + '\n#include "space_art.h"\n\n')
        total = 0
        for name, macro in PLANES:
            period, rows = planes[name]
            data = strip(rows, period, invert=name.endswith('_mask'))
            width = period + SCREEN
            total += 8 * width
            shift = {128: 2, 256: 1, 512: 0}[period]
            h.write(f'// period {period}, read at (state >> {shift}) + i\n')
            h.write(f'#define U_SPACE_{macro}_W {width}\n')
            h.write(f'extern const uint8_t PROGMEM u_space_{name}[8][U_SPACE_{macro}_W];\n\n')
            lit = sum(r.count('#') for r in rows)
            c.write(f'// {name}: period {period}, {lit} {"occluding" if name.endswith("_mask") else "lit"} px\n')
            c.write(f'const uint8_t PROGMEM u_space_{name}[8][U_SPACE_{macro}_W] = {{\n')
            for page in data:
                c.write('    {\n')
                for i in range(0, len(page), 16):
                    c.write('        ' + ', '.join(f'0x{b:02x}' for b in page[i:i + 16]) + ',\n')
                c.write('    },\n')
            c.write('};\n\n')
            print(f'  {name:10} period {period:3}  strip {width:3}  {lit:5} px')
        mask, halo = ship_mask(outdir / 'oled_spaceship.h')
        total += 4 * 128
        h.write(f'// The ship mask, derived from ship_row_* with a {SHIP_MARGIN} px margin.\n')
        h.write('// 1 = background shows through, the convention the render always used.\n')
        h.write('extern const uint8_t PROGMEM u_space_ship_mask[4][128];\n')
        c.write(f'// ship mask: silhouette + {SHIP_MARGIN} px, {halo} halo px\n')
        c.write('const uint8_t PROGMEM u_space_ship_mask[4][128] = {\n')
        for page in mask:
            c.write('    {\n')
            for i in range(0, 128, 16):
                c.write('        ' + ', '.join(f'0x{b:02x}' for b in page[i:i + 16]) + ',\n')
            c.write('    },\n')
        c.write('};\n')
        print(f'  ship mask  margin {SHIP_MARGIN}             {halo:5} halo px')
    print(f'{total} bytes -> {outdir}')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit(__doc__.split('\n\n')[0])
    main(sys.argv[1])
