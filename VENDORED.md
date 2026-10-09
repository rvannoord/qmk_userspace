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
