"""Layer bands for the master panel -> panel_layer_icons.c / .h

    python3 -I gen_layer_icons.py <halcyon-icons-dir> <kyria-default-keymap.c> <outdir>
                                  [--method threshold|fs]

One 128x40 image per Miryoku layer: the middle band of the master panel, rows 8..47,
five whole pages. Nine of the ten are splitkb's own Halcyon icons, lifted out of their
QGF files and reduced; the tenth, Base, is the Kyria logo out of QMK's own default
keymap for this board. Nothing here is drawn from scratch - see `VENDORED.md`.

**Whole bands rather than 40x40 sprites.** It costs 640 bytes a layer instead of 200,
which is nothing on an RP2040, and it buys two things: the generator owns every
placement decision, so moving the wordmark or centring an icon differently never
touches C; and the blit in `oled_master_zones.c` is one page-aligned memcpy with no
per-layer geometry at all. The band is page-aligned on both edges, so no bit shifting
is needed anywhere.

## How the reduction works

The Halcyon icons are 120x120 and **1 bit per pixel** - pure black and white, no
greyscale in the source at all. So the greys that Floyd-Steinberg works on are created
by the resampling, not read from the file: 120 -> 40 is an exact 3x box filter, which
turns each 3x3 patch of source pixels into one of ten grey levels, and FS turns those
back into one bit.

That means FS has only stroke *edges* to render, which it does as a stipple fringe.

`threshold` is the default, and the reason is worth keeping: Floyd-Steinberg was tried
first and rejected on sight - at 40px the fringe reads as dirt on the strokes rather
than as texture. That is not an argument against FS in general. It is right for Bjorn,
whose sources are greyscale photographs of a drawing and genuinely carry tone. It is
wrong here, where the source has no tone to resolve and the only thing FS can dither is
an edge it created itself. `--method fs` still works if a future source is greyscale.

## The alignment search

Before the box filter each source is shifted by 0..2 pixels in each axis and the offset
that produces the fewest mid-grey output pixels is kept. A stroke whose width is not a
multiple of 3 lands across box boundaries and greys its own edges; nudging the source so
strokes sit on box boundaries removes fringe that is an artefact of phase rather than of
stroke width. It cuts mid-grey pixels by 5-24% depending on the icon. Under `fs` that is 5-24% less
to stipple; under `threshold` it is a cleaner cut, because fewer pixels sit near the
128 boundary where one level decides the pixel.
"""

import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent / 'bjorn'))
import qgf                                              # noqa: E402
from sprites import box_filter, dither                  # noqa: E402

BAND_W, BAND_H, ICON = 128, 40, 40

# Miryoku's layer order, and the Halcyon icon each one borrows. `None` is Base, which is
# the Kyria logo instead. The mapping is by meaning, not by Halcyon's own array order:
# its `layer_icons[]` follows its keymap's layers, one of which Miryoku does not have.
# Layers whose icon is a glyph rendered straight from a font rather than the reduced
# Halcyon bitmap. Named here so the generated file credits what it actually carries - a
# comment claiming splitkb's art for a Font Awesome glyph would be a licensing error as
# much as a documentation one.
#
# Empty on purpose. Nav and Mouse were briefly taken from Nerd Fonts and put back: the
# master panel is meant to look like splitkb's Halcyon, and Font Awesome is a heavier,
# filled family that reads as borrowed next to nine outline icons. `nerd_grids.py` and
# this hook stay, because the weather work in docs/oled-v2.md wants exactly this path.
REPLACED = {}

LAYERS = [
    ('BASE',   None,             'the Kyria logo: splitkb\'s emblem and wordmark'),
    ('EXTRA',  'splitkb',        'the alternate alpha layout - Halcyon\'s own icon for exactly that'),
    ('TAP',    'keyboard',       'plain keys, the home-row mods switched off'),
    ('BUTTON', 'trackpad_input', 'a hand pressing: mouse buttons and the clipboard'),
    ('NAV',    'open_with',      'four arrows'),
    ('MOUSE',  'mouse',          'a mouse'),

    ('MEDIA',  'settings',       'RGB, transport and the boot keys - the repo has no music glyph left'),
    ('NUM',    'calculate',      'a calculator'),
    ('SYM',    'data_object',    'braces'),
    ('FUN',    'functions',      'a sigma'),
]


def kyria_logo(keymap_c):
    """The 128x64 logo out of QMK's splitkb/kyria default keymap, split into its parts.

    It was drawn for this exact panel, so the wordmark is already the right weight at
    its native size and is never scaled. The emblem is 65x64 and has to come down to 40.
    """
    body = pathlib.Path(keymap_c).read_text().split('kyria_logo[] = {')[1].split('};')[0]
    data = bytes(int(v) & 0xFF for v in re.findall(r'-?\d+', body))
    if len(data) != 1024:
        raise SystemExit(f'gen_layer_icons: kyria_logo is {len(data)} bytes, expected 1024')
    rows = [[(data[(y // 8) * 128 + x] >> (y % 8)) & 1 for x in range(128)] for y in range(64)]
    emblem = [[255 * v for v in r[3:68]] for r in rows]      # 65x64
    wordmark = [r[70:126] for r in rows[24:38]]              # 56x14, native
    return emblem, wordmark


def shift(rows, w, h, dx, dy):
    blank = [0] * w
    return [[(rows[y - dy][x - dx] if 0 <= x - dx < w else 0) if 0 <= y - dy < h else blank[0]
             for x in range(w)] for y in range(h)]


def reduce_to(rows, w, h, size, method):
    """Box filter to `size` square at the least-fringing phase, then to one bit."""
    best = None
    for dy in range(3):
        for dx in range(3):
            grey = box_filter(shift(rows, w, h, dx, dy), w, h, size, size)
            mid = sum(1 for r in grey for v in r if 24 < v < 231)
            if best is None or mid < best[0]:
                best = (mid, dx, dy, grey)
    _, dx, dy, grey = best
    if method == 'threshold':
        bits = [[1 if v >= 128 else 0 for v in r] for r in grey]
    else:
        bits = dither(grey, size, size)
    return bits, (dx, dy)


def overrides(path):
    """Hand-finished grids from icon_overrides.txt, keyed by layer name.

    The reduction is mechanical and honest but it is not a draughtsman - see that file
    for what each entry repairs. Applying them here rather than editing the generated
    .c is what keeps `panel_layer_icons.c` a generated file.
    """
    if not path.exists():
        return {}
    out, name = {}, None
    for line in path.read_text().splitlines():
        # Grid-ness is tested before comment-ness on purpose: a row of solid glyph is
        # all '#', which a leading-hash comment rule would silently eat. That bug
        # drops exactly the densest rows of each icon and leaves a plausible-looking
        # grid of the wrong height, so it fails late and reads as bad art.
        if set(line) <= {'#', '.'} and line:
            if name is not None:
                out[name]['rows'].append(line)
            continue
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        if line.startswith('['):
            head, _, rest = line[1:].partition(']')
            name = head
            spec = dict(kv.split('=') for kv in rest.split())
            out[name] = dict(x=int(spec['x']), y=int(spec['y']),
                             w=int(spec['w']), h=int(spec['h']), rows=[])
    for name, o in out.items():
        if len(o['rows']) != o['h'] or any(len(r) != o['w'] for r in o['rows']):
            raise SystemExit(f'gen_layer_icons: {name} override is not {o["w"]}x{o["h"]}')
    return out


def band_of_grid(o):
    """A 128x40 band straight from an override grid, placed at its own origin."""
    plane = [[0] * BAND_W for _ in range(BAND_H)]
    for y, row in enumerate(o['rows']):
        for x, ch in enumerate(row):
            if ch == '#' and 0 <= o['y'] + y < BAND_H and 0 <= o['x'] + x < BAND_W:
                plane[o['y'] + y][o['x'] + x] = 1
    return pack(plane)


def pack(plane):
    out = bytearray()
    for page in range(BAND_H // 8):
        for x in range(BAND_W):
            byte = 0
            for k in range(8):
                if plane[page * 8 + k][x]:
                    byte |= 1 << k
            out.append(byte)
    return bytes(out)


def band_of(bits, x0, extra=None):
    """A 128x40 band, page format, bit 0 at the top. A set bit is the glyph."""
    plane = [[0] * BAND_W for _ in range(BAND_H)]

    def put(src, ox, oy):
        for y, row in enumerate(src):
            for x, v in enumerate(row):
                if v and 0 <= oy + y < BAND_H and 0 <= ox + x < BAND_W:
                    plane[oy + y][ox + x] = 1

    put(bits, x0, (BAND_H - len(bits)) // 2)
    if extra is not None:
        art, ex, ey = extra
        put(art, ex, ey)
    return pack(plane)


CHAIN_W, CHAIN_PAGES = 40, 3


def chain_bytes(o):
    """The link-down chain: 40 columns, 3 whole pages, so the blit is page-aligned."""
    plane = [[0] * CHAIN_W for _ in range(CHAIN_PAGES * 8)]
    for y, row in enumerate(o['rows']):
        for x, ch in enumerate(row):
            if ch == '#':
                plane[o['y'] + y][o['x'] + x] = 1
    out = bytearray()
    for page in range(CHAIN_PAGES):
        for x in range(CHAIN_W):
            byte = 0
            for k in range(8):
                if plane[page * 8 + k][x]:
                    byte |= 1 << k
            out.append(byte)
    return bytes(out)


def ink_box(band):
    """The columns a band's ink actually occupies, for the default-layer rule."""
    cols = [x for x in range(BAND_W)
            if any(band[p * BAND_W + x] for p in range(BAND_H // 8))]
    return (cols[0], cols[-1]) if cols else (0, BAND_W - 1)


def main(icons_dir, keymap_c, outdir, method):
    icons_dir, outdir = pathlib.Path(icons_dir), pathlib.Path(outdir)
    emblem, wordmark = kyria_logo(keymap_c)
    hand = overrides(pathlib.Path(__file__).resolve().parent / 'icon_overrides.txt')
    chain = hand.pop('CHAIN', None)
    if chain is None:
        raise SystemExit('gen_layer_icons: icon_overrides.txt has no [CHAIN]')

    bands, report = [], []
    for name, src, why in LAYERS:
        if name in hand:
            band, off = band_of_grid(hand[name]), 'hand'
        elif src is None:
            bits, off = reduce_to(emblem, 65, 64, ICON, method)
            # Emblem and wordmark side by side, the pair centred: 40 + 4 + 56 = 100.
            x0 = (BAND_W - (ICON + 4 + len(wordmark[0]))) // 2
            band = band_of(bits, x0, (wordmark, x0 + ICON + 4,
                                      (BAND_H - len(wordmark)) // 2))
        else:
            w, h, rows = qgf.read(icons_dir / f'{src}.qgf.c')
            bits, off = reduce_to(rows, w, h, ICON, method)
            band = band_of(bits, (BAND_W - ICON) // 2)
        bands.append((name, band))
        lit = sum(bin(b).count('1') for b in band)
        if name in REPLACED:
            report.append((name, REPLACED[name], lit, 'replaced', why))
        else:
            report.append((name, src or 'kyria_logo', lit, off, why))

    banner = (f"// Generated by tools/panel/gen_layer_icons.py --method {method}.\n"
              "// Do not edit: change the mapping in the generator and regenerate.\n"
              "//\n"
              "// One 128x40 band per layer - the master panel's middle zone, rows 8..47.\n"
              "// Page format, bit 0 at the top; a set bit is the glyph, which the panel's\n"
              "// inversion then draws dark on a lit field. Sources in VENDORED.md.\n")

    with open(outdir / 'panel_layer_icons.h', 'w') as f:
        f.write(banner)
        f.write('\n#pragma once\n\n#include <stdint.h>\n#include "progmem.h"\n\n')
        f.write(f'#define U_BAND_W {BAND_W}\n#define U_BAND_H {BAND_H}\n')
        f.write(f'#define U_BAND_PAGES {BAND_H // 8}\n')
        f.write(f'#define U_BAND_BYTES {BAND_W * BAND_H // 8}\n\n')
        f.write(f'#define U_CHAIN_W {CHAIN_W}\n')
        f.write(f'#define U_CHAIN_PAGES {CHAIN_PAGES}\n')
        f.write(f'#define U_CHAIN_BYTES {CHAIN_W * CHAIN_PAGES}\n\n')
        f.write('// Indexed by Miryoku\'s own layer enum, so U_BASE..U_FUN index this directly.\n')
        f.write('extern const uint8_t PROGMEM u_layer_band[][U_BAND_BYTES];\n\n')
        f.write('// The columns each band\'s ink occupies. The default-layer rule is drawn to\n'
                '// this width so it sits under the icon rather than under the whole band.\n')
        f.write('extern const uint8_t PROGMEM u_band_x0[];\n')
        f.write('extern const uint8_t PROGMEM u_band_x1[];\n\n')
        f.write('// Knocked out lit over the inverted band while the split link is down.\n')
        f.write('extern const uint8_t PROGMEM u_chain[U_CHAIN_BYTES];\n')

    with open(outdir / 'panel_layer_icons.c', 'w') as f:
        f.write(banner)
        f.write('\n#include "panel_layer_icons.h"\n\n')
        f.write('const uint8_t PROGMEM u_layer_band[][U_BAND_BYTES] = {\n')
        for (name, band), (_, src, lit, off, why) in zip(bands, report):
            f.write(f'    {{   // {name}: {src}, {lit} ink - {why}\n')
            for i in range(0, len(band), 16):
                f.write('        ' + ', '.join(f'0x{b:02x}' for b in band[i:i + 16]) + ',\n')
            f.write('    },\n')
        f.write('};\n\n')
        boxes = [ink_box(b) for _, b in bands]
        f.write('const uint8_t PROGMEM u_band_x0[] = {'
                + ', '.join(str(a) for a, _ in boxes) + '};\n')
        f.write('const uint8_t PROGMEM u_band_x1[] = {'
                + ', '.join(str(b) for _, b in boxes) + '};\n\n')
        cb = chain_bytes(chain)
        f.write('const uint8_t PROGMEM u_chain[U_CHAIN_BYTES] = {\n')
        for i in range(0, len(cb), 16):
            f.write('    ' + ', '.join(f'0x{b:02x}' for b in cb[i:i + 16]) + ',\n')
        f.write('};\n')

    for name, src, lit, off, why in report:
        tag = {'hand': 'hand-finished', 'replaced': 'replaced'}.get(
            off, '' if src == 'kyria_logo' else f'phase {off}')
        print(f'  {name:7} {src:46} {lit:4} ink  {tag}')
    print(f'{len(bands)} bands, {len(bands) * BAND_W * BAND_H // 8} bytes, '
          f'method {method} -> {outdir}')


if __name__ == '__main__':
    argv, method = sys.argv[1:], 'threshold'
    if '--method' in argv:
        i = argv.index('--method')
        method = argv[i + 1]
        del argv[i:i + 2]
    if method not in ('fs', 'threshold'):
        raise SystemExit(f'gen_layer_icons: unknown method {method!r}')
    args = argv
    if len(args) != 3:
        raise SystemExit(__doc__.split('\n\n')[0])
    main(args[0], args[1], args[2], method)
