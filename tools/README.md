# tools

The generators behind every generated file in the keymap. Python 3 standard library
only, nothing to install. Run them with `-I` so nothing is picked up from the working
directory.

**The generated files are never edited by hand.** Edit the source a generator reads,
then regenerate.

| Tool | Produces | Source it reads |
|------|----------|-----------------|
| `bjorn/gen_bjorn.py` | `bjorn_art.c` / `.h` | `bjorn/STAGE1.txt` |
| `panel/gen_glyphs.py` | `panel_glyphs.c` / `.h` | `panel/glyphs.py` (16x16 mod glyphs), `panel/statusbar.py` (7x7 lock glyphs) |
| `panel/gen_font.py` | `panel_font.c` / `.h` | the DUNGEON.mode pack, which is not in this repo — see `VENDORED.md` |
| `bjorn/sprites.py` | contact sheets, for looking at | Bjorn's upstream sprite sets, not in this repo |

```sh
cd tools/bjorn && python3 -I gen_bjorn.py STAGE1.txt ../../keyboards/splitkb/kyria/keymaps/rvannoord
cd tools/panel && python3 -I gen_glyphs.py ../../keyboards/splitkb/kyria/keymaps/rvannoord/panel_glyphs.{c,h}
```

Both were checked on 2026-10-14 to reproduce the committed files byte for byte.

## STAGE1.txt is drawn, not traced automatically

`STAGE1.txt` holds Bjorn's art as text grids, one per frame: `@ name width height`
followed by rows of `#` and `.`. They were drawn by hand from the upstream sprites,
and that is not incidental.

Dithering the upstream 78x78 frames down to 64x64 produces something about **76% lit**.
The art that ships is **23% lit**. The panel's whole premise is dark at rest, and a
three-quarters-lit figure would also burn in and spend the entire block budget on every
change. So the reduction path is: look at the source, draw the silhouette by hand at
64x64, and keep the line weight the panel can actually show.

`sprites.py` exists to make that looking possible, not to generate art. It renders the
upstream frames as contact sheets — the source in grey next to the automatic dither —
so the drawable ones can be picked out. Point it at a checkout of the upstream
resources:

```sh
cd tools/bjorn && python3 -I sprites.py ~/bjorn-src/resources/images /tmp/sheets
```

## The upstream sets are pose variants, not animations

`shared.py` upstream picks a **random** frame from a set each time it draws, from a list
built with `os.listdir`. So within a set the frames are interchangeable drawings of one
activity, in no particular order — the numbering carries no sequence. Anything that
wants motion has to establish the order itself.
