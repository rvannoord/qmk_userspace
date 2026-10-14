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
