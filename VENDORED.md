# Vendored code

## `users/manna-harbour_miryoku/`

Miryoku's QMK userspace by Manna Harbour ([manna-harbour/miryoku_qmk](https://github.com/manna-harbour/miryoku_qmk)), GPL-2.0-or-later.

Copied on 2026-10-09 from my old fork `rvannoord/miryoku_qmk`, branch `miryoku-with-latest-qmk`, commit `b12c2bb7c8` (tag `archive/miryoku-with-latest-qmk`). That copy is Miryoku upstream plus my own `custom_config.h` and `custom_rules.mk`. Everything else in the folder is Miryoku's, and `miryoku_babel/` is generated and never edited.

When a QMK update breaks Miryoku, fix it here and add a line below. Checking whether [drashna's port](https://github.com/drashna/qmk_userspace/tree/miryoku) already fixed the same thing is an optional shortcut.

## Changes since the copy

| Date | File | Change | Why |
|------|------|--------|-----|
| 2026-10-09 | `rules.mk` | `include users/manna-harbour_miryoku/...` → `include $(USER_PATH)/...` for `custom_rules.mk` and `post_rules.mk` | In an external userspace make runs from the `qmk_firmware` root, so the cwd-relative path doesn't exist, and `VPATH` doesn't apply to `include`. QMK sets `USER_PATH` to the userspace copy |
| 2026-10-09 | `manna-harbour_miryoku.c` | `key_overrides` from a NULL-terminated pointer to an array: `const key_override_t *key_overrides[] = { &capsword_key_override };` | Current QMK takes `ARRAY_SIZE(key_overrides)`, a hard error on a pointer |
| 2026-10-09 | `custom_config.h` (mine) | Keycode shims for the renamed mouse keys (`KC_MS_*`, `KC_WH_*`, `KC_BTN*` → `MS_*`) and RGB keys (`RGB_*` → `RM_*` / `UG_*`, or `KC_NO`) | QMK renamed them after Miryoku's layers were generated. Shimmed instead of editing `miryoku_babel/`. Also covers the `KC_BTN*` in the thumb combos in `manna-harbour_miryoku.c` |

## `keyboards/splitkb/kyria/keymaps/rvannoord/art/panel_font.c`

**QMK's own 6x8 OLED font**, `drivers/oled/glcdfont.c` — the face `oled_write()` draws with. GPL-2.0-or-later WITH BSD-2-Clause, derived from the Adafruit GFX library's font. It is already in the toolchain, so this is a vendored dependency the repo does **not** have to carry: `tools/panel/gen_font.py` reads it out of the QMK checkout at generation time.

The transform is a transpose. QMK's is column-major, 6 bytes a character, bit 0 the top row; this panel's drawing code is row-major, 8 bytes a character, bit 7 the leftmost column. The 6 columns sit flush left in the 8 px cell, because `docs/layout.md` rests its "no 1 px edge frame" argument on the badge lighting x=0.

Previously this was **DUNGEON.mode** by datagoblin ([datagoblin.itch.io/dungeonmode](https://datagoblin.itch.io/dungeonmode)), an 8x8 CC0 pack. Dropped on 2026-10-15 for QMK's own font, which is lighter and needs no pack. `tools/panel/gen_font_dungeon.py` still reads that pack if it is ever wanted back; the pack itself was never in this repo.

Worth recording, since the question will come up again: all 45 `glcdfont*.c` in the QMK tree were compared over 0x20-0x7e, and every one is within **0.6%** of the default except `rubi` (5.3%) and `sendyyeah/pix` (12.6%). They are the same Adafruit face with a glyph or two changed, so "pick a nicer one from QMK" is not an option that exists. `gen_font.py --font <path>` takes any of them.

## `keyboards/splitkb/kyria/keymaps/rvannoord/art/bjorn_art.c`

Bjorn, the Tamagotchi Viking, from **[infinition/Bjorn](https://github.com/infinition/Bjorn)**, MIT licensed, "Copyright (c) 2024 infinition".

Frames `IDLE1`, `IDLE3`, `IDLE8` and `IDLE9` from `resources/images/status/IDLE/`, 78x78, box-filtered to 64x64 and Floyd-Steinberg dithered in separate 64-column passes with region locking across the set, so the four postures share their dither pattern and a tier change does not shimmer. The blink, breath, doze, axe, crown and misfire frames are hand edits of those four.

Source commit **`61a4758e5ee3d37d43c10eb690340626139912c0`** (2026-05-04), confirmed on 2026-10-14 by fetching it: it is still the head of `main`, and its `resources/images/status/` holds exactly the 17 folders, 106 BMPs and 89 frames of 78x78 the art was taken from.

Nothing of infinition's is copied into this repo. The frames are reduced to 64x64 by box filter and Floyd-Steinberg, then hand-finished as text grids in `tools/bjorn/STAGE1.txt` — see `tools/README.md`. Fetch the originals with:

```sh
git clone --filter=blob:none --no-checkout https://github.com/infinition/Bjorn ~/bjorn-src
cd ~/bjorn-src && git sparse-checkout init --cone && git sparse-checkout set resources/images actions
git checkout 61a4758e5ee3d37d43c10eb690340626139912c0
```

The art is **regenerable**: `tools/` holds the generator and the text grids it reads (`STAGE1.txt`), verified on 2026-10-14 to reproduce `bjorn_art.c` and `bjorn_art.h` byte for byte. Edit the grids, never the generated file.

## `keyboards/splitkb/kyria/keymaps/rvannoord/art/oled_spaceship.h`

The spaceship animation — the ship, its mask, and the four-row space strip it flies through — is from **JBaguley's `retrograde` keymap for the Corne**, [`JBaguley/qmk_firmware`](https://github.com/JBaguley/qmk_firmware/tree/crkbd-retrograde-keymap/keyboards/crkbd/keymaps/retrograde), branch `crkbd-retrograde-keymap`, commit **`c653e0f22f`** (2022-10-20, "added retrograde keymap for corne"). GPL-2.0-or-later, under the keymap's foostan/drashna header.

Checked against that commit on 2026-10-16: `space_row_1..4`, `ship_row_1..4` and `mask_row_1..4` are **byte-identical** to the upstream arrays. The scroll mechanism in `oled_slave_spaceship.c` — the 256-column loop, `wpm/4` for the ship's position and `wpm/15` for speed — is also JBaguley's, carried over unchanged.

`space_top_row_1..2` and `space_bottom_row_1..2` were **not** upstream. They were my own attempt to extend the 32-row original to the full 64-row panel, and they never looked right — stray stars with the Earth and moon clipped at the old band's edge. All eight strip rows, his four and my four, were removed on 2026-10-16 and replaced by three parallax planes composed from my own objects (`tools/space/`). **The ship and the mechanism are still his**, byte-identical. His hand-drawn mask was retired the same day: the render now uses a mask derived from his ship rows by `tools/space/gen_space.py` — silhouette plus a 1 px rim — so it can never drift from the ship, and the margin went from up to 4 px to exactly one.

## `keyboards/splitkb/kyria/keymaps/rvannoord/art/panel_layer_icons.c`

The master panel's ten layer bands. Two separate sources, neither drawn here.

**Nine of the ten** are splitkb's Halcyon TFT icons, from **[antontanderup/qmk_userspace](https://github.com/antontanderup/qmk_userspace)**, pinned at commit **`fa9eaed52212232d10f49d3e0e608aa877ad547d`**, path `users/halcyon_modules/splitkb/hlc_tft_display/graphics/icons/`.

Licensing is in two layers, and the icon files say so themselves: *"generated source code only, image retains original copyright"*. The generated `.qgf.c` wrappers and that repo are **GPL-2.0** (`hlc_tft_display.c` itself is "Copyright 2024 splitkb.com", GPL-2.0-or-later); none of that code is in this repo, only pixels reduced from the images. The **images** inside them keep their own copyright — `calculate`, `data_object`, `functions`, `settings`, `mouse`, `open_with`, `keyboard` and `trackpad_input` are Google **Material Symbols** (Apache-2.0), and `splitkb` is splitkb's own mark, used here on splitkb hardware. Apache-2.0 is compatible with GPL-3.0, which this repo has been under since 2026-10-21, and was not with the GPL-2.0 it had before.

**The tenth**, Base, is the Kyria logo out of QMK's own `keyboards/splitkb/kyria/keymaps/default/keymap.c` — splitkb's art, drawn for this exact 128x64 panel. The 56x14 "kyria" wordmark is used at its native size and never scaled; only the 65x64 emblem is reduced.

Nothing of either source is copied into this repo. `tools/panel/qgf.py` reads the QGF files back to pixels and `tools/panel/gen_layer_icons.py` reduces them — box filter to 40x40 at the least-fringing sub-pixel phase, then a threshold cut, emitted as one 128x40 band per layer. Floyd-Steinberg (`--method fs`) was tried first and rejected: the sources are 1-bit, so the dither has no tone to resolve and only stipples the stroke edges it created itself — see `tools/README.md`. Fetch the originals with:

```sh
git clone --filter=blob:none --no-checkout https://github.com/antontanderup/qmk_userspace ~/halcyon-src
cd ~/halcyon-src && git sparse-checkout init --cone
git sparse-checkout set users/halcyon_modules/splitkb/hlc_tft_display
git checkout fa9eaed52212232d10f49d3e0e608aa877ad547d
```

One trap worth keeping. QMK's `docs/quantum_painter_qgf.md` says a non-repeating RLE run has `length = marker - 128`. The decoder that actually runs on the keyboard, `qp_drawimage_byte_rle_decoder` in `quantum/painter/qp_draw_codec.c`, uses `c - 127`. The doc is off by one; following it drops a byte per literal run and everything after the first run decodes as noise. `tools/panel/qgf.py` follows the decoder.
