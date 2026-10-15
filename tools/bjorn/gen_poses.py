"""Authored Bjorn poses -> bjorn_poses.c / .h

    python3 -I gen_poses.py <bjorn-src>/resources/images <outdir> [name ...]

Each pose is one of infinition's own frames, reduced the way the panel needs it — box
filter to 64x64, Floyd-Steinberg in two 64-column passes, inverted so the ink becomes
the lit figure — and emitted as a 512-byte left-column image in page format.

Taken whole and never recombined: a costume, a prop, a horn posture and an expression
that appear together in one source frame stay together. See docs/layout.md.

Full columns rather than patches, because a pose change is a cut to a different drawing
and touches everything. Eight blocks, which is the whole idle budget for one step — so
the director may only cut a pose when nothing else wants to move.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from sprites import read_bmp, box_filter, dither, unique_frames   # noqa: E402

# The poses this build carries. Names are the source frame's own, so a pose can always
# be traced back to the file it came from.
DEFAULT = [
    'IDLE2',              # smiling, helmet — the top posture tier
    'IDLE9',              # goat at his left, clear of him
    'IDLE10',             # goat at his right, head up toward him
    'IDLE11',             # smiling, horns drooping outward
    'NetworkScanner1',    # spectacles
    'SMBBruteforce2',     # hood, no prop
]


def find(src, name):
    for path, stem, _shared in unique_frames(pathlib.Path(src)):
        if stem == name:
            return path
    raise SystemExit(f'gen_poses: no frame called {name}')


# Where the goat stands, in the frames that have one.
#
# He is a separate animal on the same ground: when Bjorn breathes, his chest moves and
# the goat does not. Everything else in a frame — bottle, sword, shield, laptop — is
# something Bjorn is carrying, so it rides with him.
#
# Measured rather than guessed, and reproducibly: IDLE11 is the same pose family without
# a goat, so comparing ink per column and per row against it isolates him exactly. In
# both frames he occupies rows 45 to 62; above that those columns are Bjorn's horns,
# which is why this is a box and not a column range.
#
# Connected components were tried first and do not work: the source is a line drawing,
# so a component walk shatters Bjorn into horns, helmet and beard and finds companions
# everywhere.
GOAT = {
    'IDLE9':  (3, 15, 45),      # lower left, head low
    'IDLE10': (48, 61, 45),     # lower right, head up toward him
}


# Poses whose eyes the shipped lids must not be dropped onto.
#
# The lids are one pair of patches drawn at fixed coordinates, taken from infinition's
# own blink frames of the bare-faced poses. They land correctly on any pose whose eyes
# are where IDLE's are and uncovered. NetworkScanner1's are behind spectacles, so the
# lids close over the lenses instead of under them and he reads as blinking *through*
# his glasses.
#
# This is only safe for a pose that moves some other way. A pose that neither breathes
# nor blinks is frozen, and a figure that is drawn but never moves is this design's
# signal that the link is dead - see docs/layout.md. The goat poses already give up
# breathing, so they must keep their blink; NetworkScanner1 has no goat and breathes, so
# it can give up the blink. gen_poses asserts that below rather than trusting the lists.
NO_BLINK = {'NetworkScanner1'}


def still_box(name):
    return GOAT.get(name, (255, 255, 255))


def check_moves(names):
    """Every pose has to move somehow: stillness means a dead link on this panel."""
    for n in names:
        breathes = n not in GOAT
        blinks = n not in NO_BLINK
        if not breathes and not blinks:
            raise SystemExit(f'gen_poses: {n} would neither breathe nor blink, which is '
                             'this panel\'s signal for a dead link')


def seam_row(data):
    """The row the breath lifts from: his chin.

    Only the head and the helmet move. The shipped breath works that way — head and
    helmet up a pixel, beard swelling under them, body and feet still — and the reason
    is that a figure which translates as a whole reads as the picture being moved rather
    than as somebody breathing.

    Pinning only the feet is not enough; that was tried and it still bobs, because
    everything from the shoulders down goes with it. The seam belongs at the chin, which
    is reliably the narrowest row between the helmet brim and the beard: the helmet band
    is wide, the brim underside dips, the face is narrow and the beard widens again. The
    search starts below the brim so it finds the chin rather than the brim.

    The row at the seam is drawn twice when lifted, one pixel of thickening, which is
    another reason to put it where there is least of him.
    """
    # Row 41 is the documented top of the beard: docs/layout.md records the shipped
    # breath as the head and helmet lifting while the beard swells over rows 41-52. The
    # chin, at about row 31, is too high — it leaves the face and moustache behind with
    # the body, so only the helmet and horns move. Searching the beard for a narrow row
    # does not work either: it is dense all the way down and the minima are scattered
    # across rows 36 to 44 with nothing to choose between them. The framing is constant
    # across the corpus, so a constant is the honest answer.
    #
    # The search below still runs, over a narrow window, so a pose whose beard genuinely
    # starts a pixel or two off is followed rather than forced.
    best, best_lit = 41, 999
    for y in range(40, 43):
        page, bit = y // 8, y % 8
        lit = sum((data[page * 64 + x] >> bit) & 1 for x in range(64))
        if lit < best_lit:
            best, best_lit = y, lit
    return best


def column(path):
    """512 bytes: the left 64 columns, page format, bit 0 at the top, ink lit."""
    w, h, rows = read_bmp(path)
    bits = dither(box_filter(rows, w, h, 64, 64), 64, 64)
    out = bytearray()
    for page in range(8):
        for x in range(64):
            byte = 0
            for k in range(8):
                if bits[page * 8 + k][x] == 0:      # 0 is ink, and ink is lit
                    byte |= 1 << k
            out.append(byte)
    return bytes(out)


def main(src, outdir, names):
    outdir = pathlib.Path(outdir)
    paths = [find(src, n) for n in names]
    check_moves(names)
    poses = [(n, column(p)) for n, p in zip(names, paths)]
    seams = [seam_row(d) for _, d in poses]
    still = [still_box(n) for n in names]

    banner = ("// Generated by tools/bjorn/gen_poses.py from infinition's own frames.\n"
              "// Do not edit: change the list in the generator and regenerate.\n"
              "//\n"
              "// Each pose is one authored drawing, reduced whole. 512 bytes, the left\n"
              "// column in page format, ink lit.\n")

    with open(outdir / 'bjorn_poses.h', 'w') as f:
        f.write(banner)
        f.write("\n#pragma once\n\n#include <stdint.h>\n#include \"progmem.h\"\n\n")
        f.write("#define U_POSE_BYTES 512\n\nenum u_pose {\n")
        for n, _ in poses:
            f.write(f"    U_POSE_{n.upper()},\n")
        f.write("    U_POSE_COUNT,\n};\n\n")
        f.write("extern const uint8_t PROGMEM u_poses[U_POSE_COUNT][U_POSE_BYTES];\n")
        f.write("extern const char *const u_pose_names[U_POSE_COUNT];\n\n")
        f.write("// The row the breath lifts from. Rows above it move, rows below stay put, so\n"
                "// his feet and his shadow keep still while he breathes.\n")
        f.write("extern const uint8_t PROGMEM u_pose_seam[U_POSE_COUNT];\n\n")
        f.write("// Columns the breath leaves alone: a goat standing beside him is not part of\n"
                "// his chest. 255 means there is nothing but Bjorn in the frame.\n")
        f.write("extern const uint8_t PROGMEM u_pose_still_x0[U_POSE_COUNT];\n")
        f.write("extern const uint8_t PROGMEM u_pose_still_x1[U_POSE_COUNT];\n")
        f.write("extern const uint8_t PROGMEM u_pose_still_y0[U_POSE_COUNT];\n\n")
        f.write("// Whether the shipped lids may be dropped on this pose. False where the\n"
                "// eyes are covered - behind spectacles the lids close over the lenses.\n")
        f.write("extern const uint8_t PROGMEM u_pose_blinks[U_POSE_COUNT];\n")

    with open(outdir / 'bjorn_poses.c', 'w') as f:
        f.write(banner)
        f.write('\n#include "bjorn_poses.h"\n\n')
        f.write("const uint8_t PROGMEM u_poses[U_POSE_COUNT][U_POSE_BYTES] = {\n")
        for n, data in poses:
            lit = sum(bin(b).count('1') for b in data)
            f.write(f"    {{   // {n}, {lit * 100 // 4096}% lit\n")
            for i in range(0, len(data), 16):
                f.write("        " + ", ".join(f"0x{b:02x}" for b in data[i:i + 16]) + ",\n")
            f.write("    },\n")
        f.write("};\n\n")
        f.write("const char *const u_pose_names[U_POSE_COUNT] = {\n")
        for n, _ in poses:
            f.write(f'    "{n}",\n')
        f.write("};\n\n")
        f.write("const uint8_t PROGMEM u_pose_seam[U_POSE_COUNT] = {")
        f.write(", ".join(str(s) for s in seams))
        f.write("};\n")
        for i, field in enumerate(('x0', 'x1', 'y0')):
            f.write(f"const uint8_t PROGMEM u_pose_still_{field}[U_POSE_COUNT] = {{")
            f.write(", ".join(str(s[i]) for s in still))
            f.write("};\n")
        f.write("const uint8_t PROGMEM u_pose_blinks[U_POSE_COUNT] = {")
        f.write(", ".join('0' if n in NO_BLINK else '1' for n, _ in poses))
        f.write("};\n")

    for (n, data), s, (a, b, y) in zip(poses, seams, still):
        lit = sum(bin(c).count('1') for c in data)
        where = 'him only' if a == 255 else f'goat at x{a}-{b} from row {y}, held still'
        moves = 'breathes' if a == 255 else 'goat, so no breath'
        moves += ', no blink (spectacles)' if n in NO_BLINK else ', blinks'
        print(f'  {n:18} {lit * 100 // 4096}% lit, seam row {s}, {where}')
        print(f'  {"":18} {moves}')
    print(f'{len(poses)} poses, {len(poses) * 512} bytes -> {outdir}')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3:] or DEFAULT)
