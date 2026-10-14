# The three status-bar lock glyphs. Row-major like the font, because they are drawn by
# the same 1x path. 7x7 inside an 8x8 cell: col 7 is the gutter that keeps a 1px dark
# channel from closing up under bloom, row 7 belongs to the SYS underline.
import pathlib, sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from statusbar import CAPSLOCK, CAPSWORD, SCROLLLOCK

SET = [('CAPS_LOCK',   CAPSLOCK,   'padlock: the only place Caps Lock appears at all'),
       ('CAPS_WORD',   CAPSWORD,   'latched shift, rule stopping short — it ends at the word'),
       ('SCROLL_LOCK', SCROLLLOCK, 'struck ring: something latched that you did not intend')]

def rowbytes(g):
    out = []
    for r in range(8):
        row = g[r] if r < len(g) else '........'
        byte = 0
        for c in range(8):
            if c < len(row) and row[c] == '#':
                byte |= 0x80 >> c
        out.append(byte)
    return out

for name, g, _ in SET:
    assert all(len(r) <= 8 for r in g), name
    assert len(g) <= 8, name
    for r in g:
        assert len(r) < 8 or r[7] == '.', f'{name}: col 7 must stay clear'
    assert len(g) < 8 or '#' not in g[7], f'{name}: row 7 belongs to the underline'

print('\n'.join(
    f"    {{{', '.join(f'0x{b:02x}' for b in rowbytes(g))}}},  // {name}: {note}"
    for name, g, note in SET))
print('---enum---')
print('\n'.join(f"    U_LOCK_{name}," for name, _, _ in SET))
