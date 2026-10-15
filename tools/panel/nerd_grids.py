"""Rasterise Nerd Font glyphs into text grids for icon_overrides.txt

    python3 nerd_grids.py <HackNerdFont-Regular.ttf> <NAME:codepoint:size> ...
    python3 nerd_grids.py ~/Library/Fonts/HackNerdFont-Regular.ttf NAV:f047:34

**This is the one tool here that needs something installed** — Pillow, for the TTF
rasteriser. It is deliberately not part of the build: it is run by hand when a glyph is
chosen, its output is pasted into `icon_overrides.txt`, and that file is what the real
generator reads. So `tools/README.md`'s promise that the generators need nothing
installed still holds, and a build never depends on a font being present on the machine.

## Why a font at all, when the other icons are reduced bitmaps

Because a vector glyph is rasterised *at* the target size, where a bitmap has to be
filtered down to it. The Halcyon icons are 120x120 going to 40x40, and that 3x cut is
exactly what thinned strokes, chewed TAP's keycaps and broke NAV's and SYM's symmetry —
everything `icon_overrides.txt` was written to repair. A glyph rendered straight at 34 px
has none of those artefacts to repair, because no information was thrown away.

The catch is that it is a different drawing. Nerd Fonts aggregate Font Awesome, Octicons,
Weather Icons, Devicons and Seti, and those are a heavier, more filled family than
splitkb's outline-weight Material set. Mixing them is a judgement call per icon, not a
free upgrade — `--size` is the lever that keeps a borrowed glyph inside the family's ink
range rather than letting it shout.

Antialiasing is thresholded at 50%, not dithered: see `tools/README.md` — dither tone,
threshold line art.
"""

import pathlib
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    raise SystemExit('nerd_grids: needs Pillow (pip install pillow). '
                     'It is not needed to build the firmware - see the docstring.')

BAND_W, BAND_H = 128, 40


def raster(font_path, cp, size, thresh=128):
    """One glyph, rendered at 2x and fitted to `size` square, then cut to 1 bit."""
    font = ImageFont.truetype(str(font_path), size * 2)
    img = Image.new('L', (size * 4, size * 4), 0)
    ImageDraw.Draw(img).text((size * 2, size * 2), chr(cp), font=font, fill=255, anchor='mm')
    box = img.getbbox()
    if box is None:
        raise SystemExit(f'nerd_grids: U+{cp:04X} is blank in this font')
    img = img.crop(box)
    w, h = img.size
    scale = min(size / w, size / h)
    img = img.resize((max(1, round(w * scale)), max(1, round(h * scale))), Image.LANCZOS)
    out = Image.new('L', (size, size), 0)
    out.paste(img, ((size - img.size[0]) // 2, (size - img.size[1]) // 2))
    px = out.load()
    return [[1 if px[x, y] >= thresh else 0 for x in range(size)] for y in range(size)]


def main(font_path, specs):
    for spec in specs:
        name, cp_hex, size = spec.split(':')
        size = int(size)
        bits = raster(font_path, int(cp_hex, 16), size)
        ink = sum(map(sum, bits))
        x0, y0 = (BAND_W - size) // 2, (BAND_H - size) // 2
        print(f'# U+{cp_hex.upper()} at {size}px, {ink} ink')
        print(f'[{name}] x={x0} y={y0} w={size} h={size}')
        for row in bits:
            print(''.join('#' if v else '.' for v in row))
        print()


if __name__ == '__main__':
    if len(sys.argv) < 3:
        raise SystemExit(__doc__.split('\n\n')[0])
    main(pathlib.Path(sys.argv[1]).expanduser(), sys.argv[2:])
