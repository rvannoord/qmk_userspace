# 16x16 1-bit glyph set for the Kyria master OLED. Rows are built from span lists.
def mk(spans, note=''):
    """spans: dict row -> list of (c0,c1) inclusive col ranges"""
    g = []
    for r in range(16):
        row = ['.'] * 16
        for (a, b) in spans.get(r, []):
            for c in range(a, b + 1):
                row[c] = '#'
        g.append(''.join(row))
    return g

def knock(g, pts):
    g = [list(r) for r in g]
    for (r, c) in pts:
        g[r][c] = '.'
    return [''.join(r) for r in g]

S = lambda *p: list(p)

# ---------- MOD GLYPHS (vertically centred in rows 1-14) ----------

WINDOW = knock(mk({
    1: S((1,5),(10,14)), 2: S((1,5),(10,14)), 3: S((1,5),(10,14)),
    4: S((1,5),(10,14)), 5: S((1,5),(10,14)),
    10: S((1,5),(10,14)), 11: S((1,5),(10,14)), 12: S((1,5),(10,14)),
    13: S((1,5),(10,14)), 14: S((1,5),(10,14)),
}), [(1,1),(1,14),(14,1),(14,14)])

# The Option mark, and its slope is not a free choice. Apple's U+2325 descends from the
# upper left to the lower right, with the short bar at top left and the long one at
# bottom right. This was drawn the other way round and read as the mirror image of the
# symbol on the keycap. Checked against splitkb's own keyboard_option_key.qgf, which is
# Apple's shape: it descends left to right, same as this.
OPTION = mk({
    3: S((0,4),(9,15)), 4: S((0,4),(9,15)),
    5: S((3,5)), 6: S((4,6)), 7: S((5,7)), 8: S((6,8)),
    9: S((7,9)), 10: S((8,10)), 11: S((9,11)),
    12: S((9,15)), 13: S((9,15)),
})

CARET = mk({
    4: S((6,9)),
    5: S((5,10)),
    6: S((4,6),(9,11)),
    7: S((3,5),(10,12)),
    8: S((2,4),(11,13)),
    9: S((1,3),(12,14)),
    10: S((0,2),(13,15)),
})

CMD = mk({
    1: S((1,5),(10,14)),
    2: S((1,1),(5,5),(10,10),(14,14)),
    3: S((1,1),(5,5),(10,10),(14,14)),
    4: S((1,1),(4,11),(14,14)),
    5: S((1,5),(10,14)),
    6: S((4,4),(11,11)),
    7: S((4,4),(11,11)),
    8: S((4,4),(11,11)),
    9: S((4,4),(11,11)),
    10: S((1,5),(10,14)),
    11: S((1,1),(4,11),(14,14)),
    12: S((1,1),(5,5),(10,10),(14,14)),
    13: S((1,1),(5,5),(10,10),(14,14)),
    14: S((1,5),(10,14)),
})

SHIFT = mk({
    1: S((7,8)), 2: S((6,9)),
    3: S((5,6),(9,10)), 4: S((4,5),(10,11)), 5: S((3,4),(11,12)),
    6: S((2,3),(12,13)), 7: S((1,2),(13,14)), 8: S((0,1),(14,15)),
    9: S((0,5),(10,15)), 10: S((0,5),(10,15)),
    11: S((4,5),(10,11)), 12: S((4,5),(10,11)),
    13: S((4,11)), 14: S((4,11)),
})

# ---------- EVENT ICONS (baseline: ink touches row 14; rows 0 and 15 clear) ----------

CAPSLOCK = knock(mk({
    1: S((6,9)), 2: S((5,6),(9,10)),
    3: S((4,5),(10,11)), 4: S((4,5),(10,11)),
    5: S((4,5),(10,11)), 6: S((4,5),(10,11)), 7: S((4,5),(10,11)),
    8: S((3,12)), 9: S((3,12)), 10: S((3,12)), 11: S((3,12)),
    12: S((3,12)), 13: S((3,12)), 14: S((3,12)),
}), [(10,6),(10,7),(10,8),(10,9),(11,6),(11,7),(11,8),(11,9),
     (12,7),(12,8),(13,7),(13,8),(14,3),(14,12)])

CAPSWORD = mk({
    2: S((7,8)), 3: S((6,9)),
    4: S((5,6),(9,10)), 5: S((4,5),(10,11)),
    6: S((3,4),(11,12)), 7: S((2,3),(12,13)),
    8: S((2,5),(10,13)), 9: S((4,5),(10,11)), 10: S((4,5),(10,11)),
    11: S((4,11)),
    13: S((2,13)), 14: S((2,13)),
})

ALTGR = knock(mk({
    3: S((10,12)), 4: S((9,11)), 5: S((8,10)),
    7: S((3,12)), 8: S((3,12)),
    9: S((3,4),(11,12)), 10: S((3,4),(11,12)),
    11: S((3,4),(11,12)), 12: S((3,4),(11,12)),
    13: S((3,12)), 14: S((3,12)),
}), [(7,3),(7,12),(14,3),(14,12)])

FAULT = mk({
    3: S((0,5),(10,15)),
    4: S((0,4),(11,15)),
    5: S((0,3),(12,15)),
    6: S((0,5),(10,15)),
    7: S((0,2),(13,15)),
    8: S((0,5),(10,15)),
    9: S((0,4),(11,15)),
    10: S((0,5),(10,15)),
    11: S((0,3),(12,15)),
    12: S((0,5),(10,15)),
    13: S((0,4),(11,15)),
    14: S((0,5),(10,15)),
})

SET = [
    ('WIN  pinky  GUI   (four-pane)', WINDOW),
    ('WIN  ring   Alt   (option mark)', OPTION),
    ('WIN  middle Ctrl  (caret)', CARET),
    ('WIN  index  Shift (hollow arrow)', SHIFT),
    ('MAC  pinky  Ctrl  (caret)', CARET),
    ('MAC  ring   Option(option mark)', OPTION),
    ('MAC  middle Cmd   (looped square)', CMD),
    ('MAC  index  Shift (hollow arrow)', SHIFT),
    ('EVT  Caps Lock    (padlock)', CAPSLOCK),
    ('EVT  Caps Word    (latched shift)', CAPSWORD),
    ('EVT  AltGr        (ring + acute)', ALTGR),
    ('EVT  Split fault  (torn halves)', FAULT),
]

def inv(g):
    return [''.join('#' if c == '.' else '.' for c in r) for r in g]

def check(name, g):
    errs = []
    if len(g) != 16: errs.append(f'height {len(g)}')
    for i, r in enumerate(g):
        if len(r) != 16: errs.append(f'row {i} width {len(r)}')
    if '#' in g[0]: errs.append('row 0 has ink')
    if '#' in g[15]: errs.append('row 15 has ink')
    ink = sum(r.count('#') for r in g)
    rows = [i for i, r in enumerate(g) if '#' in r]
    return ink, rows, errs

if __name__ == '__main__':
    for name, g in SET:
        ink, rows, errs = check(name, g)
        ig = inv(g)
        print(f'=== {name}  ink={ink}/256 ({100*ink//256}%)  rows {rows[0]}-{rows[-1]}' +
              (f'  !! {errs}' if errs else '') + ' ===')
        for a, b in zip(g, ig):
            print(a + '   ' + b)
        print()
