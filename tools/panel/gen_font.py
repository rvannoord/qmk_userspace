"""The panel's text font -> panel_font.c / .h

    python3 -I gen_font.py <qmk_firmware> <outdir> [--font <path/to/glcdfont.c>]
                           [--drashna <path/to/drashna_font.h> --face EIN]

Takes QMK's own 6x8 OLED font — the one `oled_write()` draws with — and re-emits the
printable ASCII range in the row-major form this panel's drawing code wants.

With `--drashna`, the glyphs come from one face of drashna's font file instead
(`docs/oled-v3.md` picked Ein), read through the preprocessor by
`tools/drashna/csource.py`. The QMK logo still comes from QMK's own font either way:
drashna's file keeps other glyphs in those rows, not the logo.

## Why QMK's font and not a nicer one

Because there is no nicer one to be had without vendoring. All 45 `glcdfont*.c` in the
QMK tree were compared over 0x20-0x7e: every one of them is within **0.6%** of the
default except `rubi` (5.3%) and `sendyyeah/pix` (12.6%), and the two `yosino58` files,
which use a different array layout and decode as noise. They are all the same Adafruit
GFX 5x7 face with a glyph or two changed. `--font` takes any of them if that changes.

Previously this drew DUNGEON.mode, an 8x8 pack that is not in this repo. QMK's font is
already in the toolchain, so this removes a vendored dependency rather than adding one.
It is also lighter: 5 columns of ink against 8, which the stats page has room for.

## The two formats

QMK's is **column-major**: 6 bytes a character, each byte one column, bit 0 the top row.
This panel's is **row-major**: 8 bytes a character, each byte one row, bit 7 the leftmost
column. So every glyph is transposed on the way through.

The 6 columns are placed at offset 0 in the 8 px cell. Flush left rather than centred,
which mattered while the Win/Mac badge was text: `docs/layout.md` rests its "no 1 px edge
frame" argument on x=0 being lit, and the OS logo carries that now. QMK's face and Ein are
both 5 columns of ink plus a spacing column, so the cell carries 3 px of tracking instead
of its native 1. Cell width stays 8 because the stats page is 16 characters across a
128 px panel.

One trap: the hex in `glcdfont.c`'s trailing comments (`// 0x41 A`) parses as font data
if the comments are not stripped first. That gives 1568 bytes instead of 1344, every
glyph shifted, and output that looks like a corrupt font rather than a bad regex.
"""

import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent / 'drashna'))

FIRST, LAST = 0x20, 0x7e
SRC_W, CELL_W, X_OFF = 6, 8, 0

# The QMK logo is not a bitmap: it is 63 characters of this same font, at 0x80,
# 0xa0 and 0xc0, three rows of 21, which the stock keymaps draw by writing a
# string of those bytes. Lifted here into a plain 126x24 image in page format,
# because this panel composes into a canvas rather than writing characters at a
# cursor - and because a splash that depends on the font's upper range still
# being where it is today is a trap for whoever next changes fonts.
LOGO_ROWS = (0x80, 0xa0, 0xc0)
LOGO_COLS = 21


def read_glcdfont(path):
    text = pathlib.Path(path).read_text(errors='ignore')
    body = text.split('PROGMEM = {', 1)[1].rsplit('};', 1)[0]
    body = re.sub(r'//[^\n]*', '', body)              # see the docstring
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    vals = [int(v, 16) for v in re.findall(r'0x[0-9a-fA-F]{2}', body)]
    if len(vals) < (LAST + 1) * SRC_W:
        raise SystemExit(f'gen_font: {path} holds {len(vals)} bytes, too few for ASCII')
    return vals


def transpose(cols):
    """Column-major, bit 0 top -> row-major, bit 7 leftmost."""
    rows = []
    for r in range(8):
        byte = 0
        for c in range(SRC_W):
            if (cols[c] >> r) & 1:
                byte |= 0x80 >> (c + X_OFF)
            if c + X_OFF >= CELL_W:
                raise SystemExit('gen_font: glyph does not fit the cell')
        rows.append(byte)
    return rows


def logo_pages(vals):
    """126x24 in page format: 3 pages of 126 columns, bit 0 at the top."""
    w = LOGO_COLS * SRC_W
    out = bytearray()
    for base in LOGO_ROWS:
        for i in range(LOGO_COLS):
            for c in range(SRC_W):
                out.append(vals[(base + i) * SRC_W + c])
    if len(out) != 3 * w:
        raise SystemExit('gen_font: logo is not 126x24')
    return bytes(out), w


def read_drashna(path, face):
    import csource
    vals = csource.load(path, [f'OLED_FONT_{face}'])['font'].values
    if len(vals) != 256 * SRC_W:
        raise SystemExit(f'gen_font: {path} holds {len(vals)} bytes, not 256 glyphs')
    return vals, csource.header(path)


def main(qmk, outdir, font, drashna=None, face=None):
    font = pathlib.Path(font or pathlib.Path(qmk) / 'drivers/oled/glcdfont.c')
    outdir = pathlib.Path(outdir)
    vals = read_glcdfont(font)
    src = vals
    if drashna:
        drashna = pathlib.Path(drashna)
        src, notice = read_drashna(drashna, face)
    glyphs = [(ch, transpose(src[ch * SRC_W:(ch + 1) * SRC_W]))
              for ch in range(FIRST, LAST + 1)]

    if drashna:
        banner = (f"// Generated by tools/panel/gen_font.py from {drashna.name} (OLED_FONT_{face}) and {font.name}.\n"
                  "// Do not edit: regenerate instead.\n"
                  "//\n"
                  f"// The {face.capitalize()} face of drashna's OLED font, transposed into this panel's\n"
                  "// row-major form and placed flush left in an 8 px cell. The QMK logo is\n"
                  "// still QMK's own, out of glcdfont.c (GPL-2.0-or-later WITH BSD-2-Clause).\n"
                  "// See VENDORED.md, which also says what is known of the face's origin.\n"
                  "//\n" + ''.join(f'{line}\n' for line in notice))
    else:
        banner = (f"// Generated by tools/panel/gen_font.py from {font.name}.\n"
                  "// Do not edit: regenerate instead.\n"
                  "//\n"
                  "// QMK's own 6x8 OLED font, transposed into this panel's row-major form and\n"
                  "// placed at offset 1 in an 8 px cell. See VENDORED.md.\n")

    logo, logo_w = logo_pages(vals)

    with open(outdir / 'panel_font.h', 'w') as f:
        f.write(banner)
        f.write('\n#pragma once\n\n#include <stdint.h>\n#include "progmem.h"\n\n')
        f.write(f'#define U_FONT_FIRST 0x{FIRST:02x}\n#define U_FONT_LAST  0x{LAST:02x}\n\n')
        f.write('// One byte a row, bit 7 leftmost.\n')
        f.write(f'extern const uint8_t PROGMEM u_font8[{len(glyphs)}][8];\n\n')
        f.write('// The QMK logo, assembled from the font\'s own upper range. Page format,\n'
                '// bit 0 at the top, so it blits like any other art.\n')
        f.write(f'#define U_LOGO_W {logo_w}\n#define U_LOGO_PAGES 3\n')
        f.write('extern const uint8_t PROGMEM u_qmk_logo[U_LOGO_PAGES * U_LOGO_W];\n')

    with open(outdir / 'panel_font.c', 'w') as f:
        f.write(banner)
        f.write('\n#include "panel_font.h"\n\n')
        f.write(f'const uint8_t PROGMEM u_font8[{len(glyphs)}][8] = {{\n')
        for ch, rows in glyphs:
            # Quoted, because 0x5c is a backslash and a // comment ending in one is a
            # line continuation: it swallows the next glyph and the build fails on
            # -Wcomment rather than on anything that points here.
            label = 'space' if ch == 0x20 else f"'{chr(ch)}'" 
            f.write(f'    {{{", ".join(f"0x{b:02x}" for b in rows)}}},'
                    f'    // 0x{ch:02x} {label}\n')
        f.write('};\n')

    with open(outdir / 'panel_font.c', 'a') as f:
        f.write(f'\nconst uint8_t PROGMEM u_qmk_logo[U_LOGO_PAGES * U_LOGO_W] = {{\n')
        for i in range(0, len(logo), 16):
            f.write('    ' + ', '.join(f'0x{b:02x}' for b in logo[i:i + 16]) + ',\n')
        f.write('};\n')

    ink = sum(bin(b).count('1') for _, rows in glyphs for b in rows)
    print(f'  {len(glyphs)} glyphs, {ink} lit pixels, from {drashna or font}' + (f' ({face})' if drashna else ''))
    print(f'{len(glyphs) * 8} bytes -> {outdir}')


def take(argv, flag):
    if flag not in argv:
        return None
    i = argv.index(flag)
    value = argv[i + 1]
    del argv[i:i + 2]
    return value


if __name__ == '__main__':
    argv = sys.argv[1:]
    font, drashna, face = take(argv, '--font'), take(argv, '--drashna'), take(argv, '--face')
    if len(argv) != 2 or bool(drashna) != bool(face):
        raise SystemExit(__doc__.split('\n\n')[0])
    main(argv[0], argv[1], font, drashna, face)
