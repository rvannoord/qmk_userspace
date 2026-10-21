# CLAUDE.md

My keyboard firmware: a Miryoku-based layout (Colemak-DH) for a splitkb Kyria rev3 with Liatris (RP2040) controllers, running QMK. Each half has an SSD1306 OLED, plus RGB matrix and two encoders. A Halcyon Kyria Wireless (ZMK) is on the way, see the last section.

Goal for the layout: keyboard first, mouse less and less, vim-style. Prefer suggestions that keep the hands on the home row.

## What this repo is

A QMK external userspace, started from `qmk/qmk_userspace`. It only holds my files. QMK itself is a separate, unmodified checkout, so updating QMK means pulling it and rebuilding, never merging.

| Path | Whose | Edit? |
|------|-------|-------|
| `keyboards/splitkb/kyria/keymaps/rvannoord/` | mine | Yes. Kyria mapping, OLED, encoders, RGB, tap-hold, host OS |
| `users/manna-harbour_miryoku/custom_config.h`, `custom_rules.mk` | mine | Yes. Layer overrides, tap-hold settings, keycode shims, QMK features |
| `users/manna-harbour_miryoku/miryoku_babel/` | Miryoku, generated | Never. Override a layer in `custom_config.h` instead |
| rest of `users/manna-harbour_miryoku/` | Miryoku, vendored | Only if there is no other way, and note it in `VENDORED.md` |

The human-facing map is `README.md` and the keymap's `readme.md`: a friend reads those, not this file, so keep both current when a file moves or a rule changes. Generated and vendored data lives in the keymap's `art/`.

The keymap folder pulls in the Miryoku userspace through `USER_NAME := manna-harbour_miryoku` in its `rules.mk`. The same `rules.mk` sets `CONVERT_TO = liatris` and the Miryoku build options, so a plain `qmk compile` always builds the right firmware. Never pass `-e MIRYOKU_...` on the command line: the old setup did, and afterwards nobody knew which options the flashed firmware had.

Miryoku options in use (everything else is Miryoku's default):

| Option | Value | Effect |
|--------|-------|--------|
| `MIRYOKU_MAPPING` | `EXTENDED_THUMBS` | thumb keys on arc positions 3–5 (Esc, Space, Tab on the left), see `docs/layout.md` |
| `MIRYOKU_CLIPBOARD` | `WIN` | Ctrl-based clipboard keys, turned into Cmd on the Mac by the Ctrl/GUI swap |

The other options (`MIRYOKU_ALPHAS`, `_EXTRA`, `_TAP`, `_NAV`, `_LAYERS`) and their values are listed in Miryoku's `readme.org` and `miryoku_babel/miryoku_layer_selection.h`. A change to any of them is a layout change: log it in `docs/layout.md`.

## Work in progress

`docs/oled-v2.md` is the staged plan for the current round of OLED work: the panels not sleeping, the link between the halves, the idle director, the goat, weather by season, and all of Bjorn's sprite sets. Work through it in order, stop at every checkpoint it names, and keep its Progress table current. It is removed when the last stage is done.

`docs/oled-v3.md` is the build order for the OLED ideas decided on 2026-10-21 from drashna's userspace: master pages behind a PAGE key, the live layer map, the OS logo and modifier icons, the Ein font, two more slave animations, a bootloader screen, encoder feedback, glitch bursts and a screensaver. The design is *The drashna round* in `docs/layout.md`. Same rules as v2: in order, stop at every checkpoint, keep its Progress table current. Its D12 says how it interleaves with v2. The repo has been GPL-3.0-or-later since its D6 (2026-10-21), because drashna's art is.

`hardware/kyria-oled-128/` is a custom 128×128 OLED board (1.12" SH1107) to replace the 128×64 modules, in design. Start with its `HANDOFF.md`, then `DESIGN.md`.

## Build and flash

QMK runs in WSL Ubuntu, with `qmk_firmware` in the WSL home and `user.overlay_dir` pointing at this repo under `/mnt/c/...`.

```bash
qmk compile -kb splitkb/kyria/rev3 -km rvannoord
```

Before flashing, read `docs/flash-checklist.md`: it lists what is waiting to be checked on the board and which commit each item came from, and it is where the result goes afterwards.

**Always flash both halves with the same build.** `NUM_TOTAL_TRANSACTIONS` is XORed into every split handshake, so a mismatched pair fails every split transaction, the matrix included — the other half just stops typing.

Both halves take the same `.uf2`: the Kyria rev3 reads left/right from a pin on the PCB. On the Mac `qmk flash` works directly, because macOS mounts the `RPI-RP2` volume; from Windows the file is copied in Explorer. Flash one half at a time:
1. Plug USB into that half.
2. Put it in the bootloader with Miryoku's Boot key (double-tap). If the firmware is broken, hold the SYS key (outer column, top row) while plugging in; that's the Bootmagic key.
3. Copy the `.uf2` to the `RPI-RP2` drive from Windows Explorer. `qmk flash` inside WSL can't see the drive.

Flash size is not a concern on the RP2040. Keep a known-good `.uf2` outside the repo to fall back on.

After changing `tap_hold.c`, `host_os.c`, `keymap.c` or a tap-hold value, run the scenario tests with `tests/run_scenarios.sh new` (see `tests/README.md`). They encode the current values, so a deliberate change updates the matching test too.

After changing any OLED or split file — `oled_canvas.c`, `oled_master_zones.c`, `oled_slave_bjorn.c`, `bjorn_art.c`, `panel_font.c`, `split_state.c`, `split_sync.c` — run `tests/run_unit.sh`. Those are host tests in plain C and need nothing but `cc`. `tests/run_unit.sh --dump` prints the panel's frames, which is how the zones get checked without a flash.

## Host OS

The keyboard plugs into Windows at work and an Apple Silicon Mac at home. One switch, `host_is_mac()` in `host_os.c`, decides everything OS-specific:

- QMK OS detection sets it on plug-in. `OS_DETECTION_SINGLE_REPORT` is on because Apple Silicon Macs re-trigger detection minutes later.
- The SYS key (`U_SYS`) flips it by hand when detection guesses wrong.
- On the Mac it swaps Ctrl and GUI, so Cmd shortcuts use the same fingers as Ctrl shortcuts on Windows. Redo is special-cased to Cmd+Shift+Z.
- The master OLED shows `Win` or `Mac`.

Never hardcode Ctrl or Cmd for one OS: anything OS-specific reads `host_is_mac()`. A possible later switch (keep the mods in place, adapt only the clipboard keys) should only need changes in `host_os.c`.

## Keys outside Miryoku

Miryoku uses 36 of the Kyria's 50 keys. The rest are `XXX` in the `LAYOUT_miryoku` wrapper in the keymap's `config.h`, which has two variants (Miryoku's default and `EXTENDED_THUMBS`). A key placed there exists on every layer. Change both variants.

Thumb arc, five positions per half, numbered from the outside edge toward the middle: in `LAYOUT()` the left thumb row is listed 1→5 and the right one 5→1. **Position 1 on both halves is a rotary encoder** (left: volume, right: screen brightness, in `encoder_update_user`). Its push-click is the key at that matrix position and is unassigned. Positions 3–5 carry Miryoku's thumb keys. Free keys: position 2 on each half, the two upper thumb keys per half, and the outer column. Custom keycodes are `QK_USER_*` aliases defined in that `config.h`, with a `U_` prefix: QMK already uses short names like `OS_TOGG`.

| Key | Position | Keycode |
|-----|----------|---------|
| SYS | left half, outer column, top row (also Bootmagic) | `U_SYS` = `QK_USER_0`, toggles host OS |

## Rules

- Never change QMK core (`quantum/`, `tmk_core/`, `platforms/`, `drivers/`). If something seems to need a core patch, stop and say so. Merging a core patch (sunaku's bilateral combinations in `quantum/action.c`) is what made the old fork impossible to sync.
- To change a Miryoku layer, redefine the whole layer in `custom_config.h` (`#define MIRYOKU_LAYER_SYM ...`), starting from the copy in `miryoku_babel/miryoku_layer_alternatives.h`. Keep the 36-key shape: `U_NA` on the side holding the layer key, `U_NP` for keys that don't exist.
- When QMK renames a keycode that Miryoku uses, add a shim (`#define OLD_NAME NEW_NAME`) to `custom_config.h`. Don't edit `miryoku_babel`.
- Use Miryoku's layer enum (`U_BASE`, `U_SYM`, ...) from `manna-harbour_miryoku.h`. Don't keep a second copy of the layer list.
- `keymap.c` is compiled inside QMK's `keymap_introspection.c`. It must `#include "manna-harbour_miryoku.h"` to see Miryoku's `U_*` names, and must never be added to `SRC`. Extra source files are added to `SRC` in the keymap's `rules.mk`.
- Files shared with tests (`tap_hold.c`, `host_os.c`) include `quantum.h`, not `QMK_KEYBOARD_H`.
- Home-row mods use stock QMK Chordal Hold and Flow Tap, tuned through the callbacks in `tap_hold.c`. Layer-tap thumb keys are exempt from both. Change one value per flash and log it in `docs/layout.md`.
- Read `docs/layout.md` before suggesting a layout change. A change that sticks gets written there before it goes into the firmware.
- Symbols depend on the OS keyboard layout (see `docs/layout.md`). Miryoku assumes plain US. US-International turns `' " ` ~ ^` into dead keys, which breaks both the symbol layer and vim.
- LF line endings everywhere (`.gitattributes`). A `rules.mk` with CRLF breaks make.
- Commits use my personal identity, `rvannoord <rvannoord@icloud.com>`, and are SSH-signed through 1Password. Check `git config user.email` before committing: the work PC's global identity is my Monitor one and must never end up in this repo.
- Never attribute anything to Claude: no `Co-Authored-By: Claude` trailer, no "Generated with Claude Code", and no mention of Claude or AI in commit messages, tags, PR or release text, or the content of files that get pushed. Everything here is published as my own work, whatever the default attribution setting says. The only exceptions are `CLAUDE.md` and `.claude/`.
- Commit and push only when I ask. Commits are conventional commits with an emoji, same as the existing history (`✨ feat(oled): ...`).

## Halcyon Kyria Wireless

Runs ZMK on an nRF52840, not QMK. It can't take an SSD1306: the display options are splitkb's e-paper module (best for battery) or the TFT. None of the OLED code here ports over. Its config lives in <its own ZMK repo / `zmk/`>. The layout stays the same on both boards, so a layout change goes into `docs/layout.md` first and then into both firmwares.
