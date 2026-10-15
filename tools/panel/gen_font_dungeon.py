# Cuts the printable ASCII range out of the DUNGEON.mode 8x8 sheet and emits the
# panel's font table. Row-major: one byte a row, bit 7 leftmost.
import sys, pathlib, zlib, struct

def read_png(p):
    d = pathlib.Path(p).read_bytes()
    assert d[:8] == b'\x89PNG\r\n\x1a\n'
    i, idat, w, h, bd, ct = 8, b'', 0, 0, 0, 0
    while i < len(d):
        ln = struct.unpack('>I', d[i:i+4])[0]; typ = d[i+4:i+8]; body = d[i+8:i+8+ln]
        if typ == b'IHDR': w, h, bd, ct = *struct.unpack('>II', body[:8]), body[8], body[9]
        elif typ == b'IDAT': idat += body
        i += 12 + ln
    assert (bd, ct) == (8, 6), (bd, ct)
    raw = zlib.decompress(idat); stride = w * 4; prev = bytearray(stride); out = []; pos = 0
    for _ in range(h):
        f = raw[pos]; pos += 1
        line = bytearray(raw[pos:pos+stride]); pos += stride
        for x in range(stride):
            a = line[x-4] if x >= 4 else 0; b = prev[x]; c = prev[x-4] if x >= 4 else 0
            if   f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        out.append(bytes(line)); prev = line
    return w, h, out

def charmap(p):
    """cell index -> character. Variation selectors are stripped: the sheet's own
    map has one emoji in it, and leaving U+FE0F in shifts every later cell."""
    lines = [l for l in pathlib.Path(p).read_text().splitlines() if not l.startswith('//')][2:]
    cells = {}
    for gy, line in enumerate(lines[:16]):
        line = line.replace('️', '')
        for gx, ch in enumerate(line[:16]):
            cells.setdefault(ch, (gx, gy))
    return cells

def glyph(rows, gx, gy):
    """8 bytes, row-major, bit 7 leftmost. Ink is opaque and bright: the sheet has an
    opaque dark background, so alpha alone does not separate figure from ground."""
    out = []
    for y in range(8):
        r = rows[gy * 8 + y]; byte = 0
        for x in range(8):
            o = (gx * 8 + x) * 4
            if r[o + 3] > 127 and r[o] > 127:
                byte |= 0x80 >> x
        out.append(byte)
    return out

FIRST, LAST = 0x20, 0x7e

# Sheet cells the panel draws as symbols rather than as text. Interim: these are the
# font's own cells, scaled 2x at draw time. Hand-drawn 16x16 art replaces them later.
SYMBOLS = [
    ('CAPS_LOCK', '\u25b2'),   # filled triangle: latched until you clear it
    ('CAPS_WORD', '\u2191'),   # arrow: ends itself at the next space
    ('ALTGR',     '\u25c9'),   # ring
    ('LINK',      '\u2620'),   # skull: the loudest thing on the sheet
]

if __name__ == '__main__':
    sheet, cmap, out_c, out_h = sys.argv[1:5]
    _, _, rows = read_png(sheet)
    cells = charmap(cmap)
    missing = []
    table = []
    for code in range(FIRST, LAST + 1):
        ch = chr(code)
        if ch in cells:
            gx, gy = cells[ch]
            table.append((ch, glyph(rows, gx, gy)))
        else:
            missing.append(ch)
            table.append((ch, [0] * 8))

    banner = ("// Generated from DUNGEON.mode by gen_font.py. Do not edit by hand.\n"
              "// Source sheet: dungeonmode/bitmap/dungeon-mode.png, a 16x16 grid of 8x8 cells.\n")
    with open(out_h, 'w') as f:
        f.write(banner)
        f.write("\n#pragma once\n\n#include <stdint.h>\n#include \"progmem.h\"\n\n")
        f.write(f"#define U_FONT_FIRST 0x{FIRST:02x}\n#define U_FONT_LAST  0x{LAST:02x}\n\n")
        f.write("// One byte a row, bit 7 leftmost. Characters the sheet does not carry are blank.\n")
        f.write(f"extern const uint8_t PROGMEM u_font8[{LAST - FIRST + 1}][8];\n")
        f.write("\n// Symbols, same format. Drawn at 2x, so each one covers 16x16.\n")
        f.write("enum u_sym {\n")
        for i, (name, _) in enumerate(SYMBOLS):
            f.write(f"    U_SYM_{name},\n")
        f.write("    U_SYM_COUNT,\n};\n\n")
        f.write("extern const uint8_t PROGMEM u_sym8[U_SYM_COUNT][8];\n")
    with open(out_c, 'w') as f:
        f.write(banner)
        f.write(f"\n#include \"{pathlib.Path(out_h).name}\"\n\n")
        f.write(f"const uint8_t PROGMEM u_font8[{LAST - FIRST + 1}][8] = {{\n")
        for ch, g in table:
            label = ch if ch not in '\\' else '\\\\'
            f.write("    {" + ", ".join(f"0x{b:02x}" for b in g) + "},  // '" + label + "'\n")
        f.write("};\n\n")
        f.write(f"const uint8_t PROGMEM u_sym8[U_SYM_COUNT][8] = {{\n")
        for name, ch in SYMBOLS:
            gx, gy = cells[ch]
            g = glyph(rows, gx, gy)
            f.write("    {" + ", ".join(f"0x{b:02x}" for b in g) + f"}},  // {name}, sheet row {gy} col {gx}\n")
        f.write("};\n")
    print("missing from the sheet:", " ".join(missing) or "(none)")
