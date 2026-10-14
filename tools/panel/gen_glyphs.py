# The mod-row glyphs as the firmware wants them: page format, one byte a column,
# bit 0 at the top, two pages a glyph. Source is the designer's span lists.
import pathlib, sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from glyphs import WINDOW, OPTION, CARET, CMD, SHIFT
from statusbar import CAPSLOCK, CAPSWORD, SCROLLLOCK

SET = [('WINDOW', WINDOW, 'Windows GUI, the four-pane mark'),
       ('OPTION', OPTION, 'Alt on Windows, Option on the Mac: one key, two names'),
       ('CARET',  CARET,  'Ctrl. Moves cell with the OS swap, middle finger to pinky'),
       ('CMD',    CMD,    'Command, Mac only'),
       ('SHIFT',  SHIFT,  'Shift. One drawing, both columns')]

LOCKS = [('CAPS_LOCK',   CAPSLOCK,   'padlock. The only place Caps Lock appears at all'),
         ('CAPS_WORD',   CAPSWORD,   'latched shift, rule stopping short: it ends at the word'),
         ('SCROLL_LOCK', SCROLLLOCK, 'struck ring: something latched that you did not mean')]

def rowbytes(g):
    """8 bytes, row-major, bit 7 leftmost — the font's format, same 1x draw path."""
    out = []
    for r in range(8):
        row = g[r] if r < len(g) else ''
        byte = 0
        for c, ch in enumerate(row[:8]):
            if ch == '#':
                byte |= 0x80 >> c
        out.append(byte)
    return out

def pages(g):
    """32 bytes: page 0 is rows 0-7 across 16 columns, page 1 is rows 8-15."""
    out = []
    for q in range(2):
        for x in range(16):
            byte = 0
            for k in range(8):
                if g[q * 8 + k][x] == '#':
                    byte |= 1 << k
            out.append(byte)
    return out

banner = ("// Hand-drawn for this panel, in DUNGEON.mode's weight language. Page format:\n"
          "// one byte a column, bit 0 at the top, page 0 then page 1. Generated — the grids\n"
          "// they came from are span lists, not this array, so edit those and regenerate.\n")

with open(sys.argv[2], 'w') as f:
    f.write(banner)
    f.write("\n#pragma once\n\n#include <stdint.h>\n#include \"progmem.h\"\n\n")
    f.write("#define U_GLYPH_BYTES 32        // 16x16, two pages of 16 columns\n\n")
    f.write("enum u_glyph {\n")
    for name, _, note in SET:
        f.write(f"    U_GLYPH_{name},    // {note}\n")
    f.write("    U_GLYPH_COUNT,\n};\n\n")
    f.write("extern const uint8_t PROGMEM u_glyph16[U_GLYPH_COUNT][U_GLYPH_BYTES];\n")
    f.write("\n// The status bar's latched flags, 7x7 inside an 8x8 cell: column 7 is the\n"
            "// gutter that keeps the 1 px channel between neighbours from closing under\n"
            "// bloom, row 7 belongs to the SYS underline. Row 0 carries ink in all three\n"
            "// and in none of the six badge letters, so the cluster sits a pixel proud of\n"
            "// any text beside it. Drawn at 1 px, which the sheet's own symbol cells also\n"
            "// are — and which means they must never be inverted in place. Nothing does:\n"
            "// the alarm row is a different page.\n")
    f.write("enum u_lock {\n")
    for name, _, note in LOCKS:
        f.write(f"    U_LOCK_{name},    // {note}\n")
    f.write("    U_LOCK_COUNT,\n};\n\n")
    f.write("extern const uint8_t PROGMEM u_lock8[U_LOCK_COUNT][8];\n")

with open(sys.argv[1], 'w') as f:
    f.write(banner)
    f.write(f"\n#include \"{sys.argv[2].split('/')[-1]}\"\n\n")
    f.write("const uint8_t PROGMEM u_glyph16[U_GLYPH_COUNT][U_GLYPH_BYTES] = {\n")
    for name, g, _ in SET:
        b = pages(g)
        f.write(f"    {{   // {name}\n")
        f.write("        " + ", ".join(f"0x{v:02x}" for v in b[:16]) + ",\n")
        f.write("        " + ", ".join(f"0x{v:02x}" for v in b[16:]) + ",\n    },\n")
    f.write("};\n\n")
    f.write("const uint8_t PROGMEM u_lock8[U_LOCK_COUNT][8] = {\n")
    for name, g, _ in LOCKS:
        for r in g[:8]:
            assert len(r) < 8 or r[7] == '.', f'{name}: column 7 must stay clear'
        assert len(g) < 8 or '#' not in g[7], f'{name}: row 7 belongs to the underline'
        f.write("    {" + ", ".join(f"0x{b:02x}" for b in rowbytes(g)) + f"}},  // {name}\n")
    f.write("};\n")
print("wrote", sys.argv[1], sys.argv[2])
