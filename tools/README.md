# tools

The generators behind every generated file in the keymap. Python 3 standard library
only, nothing to install. Run them with `-I` so nothing is picked up from the working
directory.

**The generated files are never edited by hand.** Edit the source a generator reads,
then regenerate.

| Tool | Produces | Source it reads |
|------|----------|-----------------|
| `bjorn/gen_bjorn.py` | `art/bjorn_art.c` / `.h` | `bjorn/STAGE1.txt`, the shipped frames as text grids |
| `bjorn/gen_poses.py` | `art/bjorn_poses.c` / `.h` | infinition's own frames, reduced the way the panel needs them; not in this repo, see `VENDORED.md` |
| `bjorn/sprites.py` | contact sheets, for looking at | Bjorn's upstream sprite sets, not in this repo |
| `bjorn/oled_art.py` | nothing on its own | a library: ASCII grid <-> page-format bytes <-> PNG, shared by the generators |
| `panel/gen_glyphs.py` | `art/panel_glyphs.c` / `.h` | with `--drashna <drashna_font.h>`, as committed: drashna's five 12x8 mod icons, centred at (2, 4) in their 16x16 cells, and his Caps Lock and Scroll Lock cells. Without it, the hand-drawn glyphs in `panel/glyphs.py` and `panel/statusbar.py`, Caps Word's among them. The iOS "i" comes from `statusbar.py` either way |
| `panel/gen_locks.py` | the lock glyphs on stdout, as C | `panel/statusbar.py`. A check tool: it prints what `gen_glyphs.py` emits, and asserts the gutter column and the underline row stay clear |
| `panel/gen_font.py` | `art/panel_font.c` / `.h` | with `--drashna <drashna_font.h> --face EIN`, as committed: the Ein face of drashna's font. Without it, QMK's own `drivers/oled/glcdfont.c`. The splash logo comes from QMK's checkout either way |
| `panel/gen_font_dungeon.py` | `art/panel_font.c` / `.h` | the DUNGEON.mode pack, which is not in this repo. Superseded 2026-10-15; kept in case that face is wanted back |
| `panel/gen_layer_icons.py` | `art/panel_layer_icons.c` / `.h` | splitkb's Halcyon icons and QMK's Kyria logo, neither in this repo (see `VENDORED.md`), then `panel/icon_overrides.txt` for the hand corrections |
| `panel/nerd_grids.py` | text grids for `icon_overrides.txt` | a Nerd Font TTF. **The one tool here that needs something installed** (Pillow) |
| `panel/qgf.py` | nothing on its own | a library: reads a QMK Graphics Format image back out of its generated `.qgf.c`, including RGB565 frames and files that hold several images |
| `world/gen_world.py` | `art/world_art.c` / `.h` | `world/sprites.txt`, the weather sprites as grids |
| `space/gen_space.py` | `art/space_art.c` / `.h` | `space/planes.txt`, the three parallax planes and two masks as grids, plus the ship rows in `art/oled_spaceship.h` for the derived ship mask |
| `space/authoring/` | `space/planes.txt`, previews | provenance: the objects and the placement script the planes were composed from. Needs Pillow, is not part of the build |
| `preview.py` | `strip.png`, `anim.gif` | a frame dump from `tests/preview/`; see `tests/README.md` |
| `drashna/gen_art.py` | `art/drashna_icons.c`, `kitty_art.c`, `status_frame.c`, `drashna_sprites.c`, `painter_art.c`, `painter_fonts.c`, `bongo_art.c`, `mario_art.c`, `matrix_art.c`, each with its `.h` | drashna's OLED and Quantum Painter sources and the bongocats module, as pinned clones that are not in this repo (see `VENDORED.md`). It names every lit cell of his font, checks the art it relies on, and stops if drashna has changed it |
| `drashna/painter.py` | nothing on its own | a library: drashna's Quantum Painter images and QMK Font Format fonts read back to 1-bit pixels, on top of `panel/qgf.py` |
| `drashna/csource.py` | nothing on its own | a library: C arrays out of a source run through `cc -E`, which is how the faces and panel sizes kept behind `#if` are read. It executes nothing from the source, and refuses an initialiser that is not plain integers |

```sh
cd tools/bjorn && python3 -I gen_bjorn.py STAGE1.txt ../../keyboards/splitkb/kyria/keymaps/rvannoord/art
cd tools/bjorn && python3 -I gen_poses.py ~/bjorn-src/resources/images ../../keyboards/splitkb/kyria/keymaps/rvannoord/art
cd tools/panel && python3 -I gen_glyphs.py ../../keyboards/splitkb/kyria/keymaps/rvannoord/art/panel_glyphs.c ../../keyboards/splitkb/kyria/keymaps/rvannoord/art/panel_glyphs.h --drashna ~/drashna-src/users/drashna/display/oled/drashna_font.h
python3 -I tools/panel/gen_font.py ~/qmk_firmware keyboards/splitkb/kyria/keymaps/rvannoord/art --drashna ~/drashna-src/users/drashna/display/oled/drashna_font.h --face EIN
python3 -I tools/panel/gen_layer_icons.py <halcyon-icons> ~/qmk_firmware/keyboards/splitkb/kyria/keymaps/default/keymap.c keyboards/splitkb/kyria/keymaps/rvannoord/art
python3 -I tools/world/gen_world.py keyboards/splitkb/kyria/keymaps/rvannoord/art
python3 -I tools/space/gen_space.py keyboards/splitkb/kyria/keymaps/rvannoord/art
python3 -I tools/drashna/gen_art.py ~/drashna-src ~/drashna-modules keyboards/splitkb/kyria/keymaps/rvannoord/art
```

Without `--drashna`, `gen_font` and `gen_glyphs` give QMK's own font and the hand-drawn 16x16
mod glyphs from `panel/glyphs.py`. Those were the committed files until stage 1 of
`docs/oled-v3.md` replaced them on 2026-10-21.

Every generator reproduces its committed files byte for byte. `gen_bjorn`, `gen_world` and
`gen_space` were re-run and compared on 2026-10-16. `gen_layer_icons` and `gen_poses` were
compared on 2026-10-15 and 2026-10-14, when their sources were last fetched. `gen_art`, and
`gen_font` and `gen_glyphs` with `--drashna`, were compared on 2026-10-21.

## Floyd-Steinberg has nothing to work with on 1-bit sources

Worth knowing before reaching for the dither again. Bjorn's frames are 78x78 **greyscale**
BMPs, so FS is resolving real tone. The Halcyon icons are 120x120 and **1 bit per pixel** —
there is no tone in the file at all. Every grey FS sees there was manufactured by the 3x
box filter out of stroke edges, so all FS can render is a stipple fringe along each stroke.

So `gen_layer_icons.py` defaults to **`--method threshold`**, not to the dither. FS was
tried on these first and rejected on sight: at 40px its fringe reads as dirt on the
strokes rather than as texture. `--method fs` is still there for a source that has real
tone. Either way the generator searches sub-pixel phase before filtering — nudging the
source so strokes land on box boundaries cuts mid-grey pixels by 5-24%, which is less to
stipple under `fs` and a cleaner cut under `threshold`.

The rule of thumb: **dither tone, threshold line art.**

## How the art is reduced

Floyd-Steinberg, and it works. Box-filter the 78x78 source to 64x64, dither in two
separate 64-column passes, and invert — the source is dark ink on a white page, so the
*ink* becomes the lit figure. That lands at **22-24% lit**, which is where the shipped
art sits (`bjorn_tier0` is 23%).

> A correction worth keeping, because it cost a day. This was first measured as "the
> dither gives 76% lit, the shipped art is 23%, so the reduction is unusable and every
> frame must be hand-drawn". That was wrong: 76% was the **white page**, counted as ink.
> The figure is 23% and always was. The conclusion drawn from it — hand-draw everything
> — produced clean outline art with none of Bjorn's mass, which is not what he looks
> like. Measure the ink, not the background.

`STAGE1.txt` holds the shipped frames as text grids, and they are hand-finished: the
blink, breath, doze, axe, crown and misfire frames are edits on top of the four dithered
IDLE poses. So the pipeline is dither first, then hand-work the frames that need to move,
not hand-draw from nothing.

One thing the dither genuinely cannot fix: the hooded poses. The cowl is a solid dark
mass in the source, so it reduces to about **33% lit** and reads as a blob rather than as
a face inside a hood. Six images are affected (SSH1, 2, 3, 7, 8, 13). That is a polarity
problem, not a resolution one.

## If the panel ever gets bigger

A 128x128 OLED is a possibility. When that happens, **re-dither from the source BMPs at
the new size. Never scale the 64x64 output.**

Doubling 1-bit art doubles its pixels: every dither dot becomes a 2x2 block, the texture
that gives Bjorn his mass turns into visible chequerboard, and the result looks worse
than the small version rather than better. Re-dithering puts the error diffusion back to
work at the new resolution — the source is 78x78 greyscale, so 128 is an upscale that
adds no detail, but it gives the dither four times as many pixels to spread its error
into, and the gradients come out smoother.

`sprites.py` takes the target size as an argument to `box_filter`; the dither's 64-column
split is the panel's own half-width and would become 64 still, since that is about the
two halves' independent compositing rather than about the frame size.

**The layer icons are the same rule and a bigger win.** They are 120x120 in the source and
are thrown away down to 40x40 — a 3x reduction, which is where the thin strokes, the lost
keycaps and the broken symmetry in `icon_overrides.txt` all come from. Re-reduce from the
`.qgf.c` files at the new size; never scale the 40px output. At a 120px band they need no
reduction at all, and **`icon_overrides.txt` should then be deleted rather than ported** —
every grid in it repairs a specific artefact of the 3x cut, so at native size each one is
a hand edit undoing damage that no longer exists.

Worth saying plainly, because it is the whole shape of this work: the SSD1306 is the only
reason any of the reduction exists. The Halcyon Kyria Wireless's TFT is 135x240 and these
icons came *from* a 135x240 panel, so on that board they are simply used as they are.

What else would have to change: `OLED_MATRIX_SIZE` and `OLED_BLOCK_SIZE` are different on
a 128x128 panel, so every block index in `bjorn_art.c` and every patch origin is wrong,
and `gen_bjorn.py` would need its `block = index / 64` arithmetic revisited.
`oled_canvas.c` already carries `_Static_assert`s on both constants, so such a build
fails loudly at compile time rather than drawing nonsense — that is deliberate.

## The upstream sets are pose variants, not animations

`shared.py` upstream picks a **random** frame from a set each time it draws, from a list
built with `os.listdir`. So within a set the frames are interchangeable drawings of one
activity, in no particular order — the numbering carries no sequence. Anything that
wants motion has to establish the order itself.
