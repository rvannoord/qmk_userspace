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

## `keyboards/splitkb/kyria/keymaps/rvannoord/panel_font.c`

Glyphs extracted from **DUNGEON.mode** by datagoblin ([datagoblin.itch.io/dungeonmode](https://datagoblin.itch.io/dungeonmode)), an 8x8 1-bit bitmap font released **CC0 1.0 Universal** — public domain, so no attribution is required and none is imposed on anything built with it. Credited by choice.

Taken from the pack's `bitmap/dungeon-mode.png`, a 16x16 grid of 8x8 cells, with `playscii/charsets/dungeon-mode.char` as the character map. Only the printable ASCII range and four symbol cells were extracted; the pack itself is not in this repo. The table in `panel_font.c` is generated and never hand-edited — to change which characters it carries, extract again from the pack.

## `keyboards/splitkb/kyria/keymaps/rvannoord/bjorn_art.c`

Bjorn, the Tamagotchi Viking, from **[infinition/Bjorn](https://github.com/infinition/Bjorn)**, MIT licensed, "Copyright (c) 2024 infinition".

Frames `IDLE1`, `IDLE3`, `IDLE8` and `IDLE9` from `resources/images/status/IDLE/`, 78x78, box-filtered to 64x64 and Floyd-Steinberg dithered in separate 64-column passes with region locking across the set, so the four postures share their dither pattern and a tier change does not shimmer. The blink, breath, doze, axe, crown and misfire frames are hand edits of those four.

The source commit was not recorded at the time. GitHub shows `main` unchanged since `61a4758e5ee3d37d43c10eb690340626139912c0` (2026-05-04), so the art generated on 2026-10-10 almost certainly came from it — an inference, to be confirmed when the sprites are fetched.

The art is **regenerable**: `tools/` holds the generator and the text grids it reads (`STAGE1.txt`), verified on 2026-10-14 to reproduce `bjorn_art.c` and `bjorn_art.h` byte for byte. Edit the grids, never the generated file.
