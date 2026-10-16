# qmk_userspace

Firmware for my splitkb **Kyria rev3** (Liatris RP2040 controllers, an SSD1306 OLED on each
half, RGB matrix, two encoders), running [QMK](https://qmk.fm) with a
[Miryoku](https://github.com/manna-harbour/miryoku) layout on Colemak-DH. Keyboard first, mouse
less and less, vim-style: the layout keeps the hands on the home row.

Two things make this more than a keymap. The host-OS switch: `Win` or `Mac` is detected on
plug-in and swaps Ctrl and GUI, so shortcuts use the same fingers on both. And the OLEDs: the
master half shows the layer as a picture with the held modifiers under it; the other half shows
Bjorn, a Tamagotchi Viking who reacts to typing speed and idles in the weather, or a spaceship,
or a logo, cycled from a key.

## How it is built

This is a QMK **external userspace**, started from
[`qmk/qmk_userspace`](https://github.com/qmk/qmk_userspace). It holds only my files. QMK itself
is a separate, unmodified checkout that the `qmk` CLI is pointed at, so updating QMK is a
`git pull` there and a rebuild here: nothing is merged, and nothing in QMK core is patched. (The
previous setup was a fork carrying a core patch, and it could not be synced any more.)

| Path | Whose | What |
|------|-------|------|
| `keyboards/splitkb/kyria/keymaps/rvannoord/` | mine | the keymap: key positions, host OS, tap-hold tuning, OLEDs, split sync. **Start at its `readme.md`** |
| `users/manna-harbour_miryoku/custom_config.h`, `custom_rules.mk` | mine | the features and tuning that Miryoku's userspace includes |
| rest of `users/manna-harbour_miryoku/` | Miryoku, vendored | the layers. `miryoku_babel/` is generated upstream; a layer is overridden in `custom_config.h`, never edited there |
| `tools/` | mine | the Python generators behind every file in the keymap's `art/` |
| `tests/` | mine | host tests: plain-C unit tests that need only `cc`, GoogleTest scenarios that run through a QMK tree, and a host preview of the idle show |
| `docs/` | mine | `layout.md` is the spec and the tuning log; `flash-checklist.md` what is on the board and what to look at; `environment.md` the two machines; `oled-v2.md` the work plan for the OLEDs |
| `VENDORED.md` | mine | where everything that is not my own work came from, pinned to commits |

## Build, flash, test

```sh
qmk config user.overlay_dir="$(realpath .)"      # once, with a stock qmk_firmware set up
qmk compile -kb splitkb/kyria/rev3 -km rvannoord
```

The `.uf2` lands in this folder. Both halves take the same file and **must run the same
build**: the split handshake XORs the transaction count, so a mismatched pair stops talking.
Put a half in the bootloader (double-tap Miryoku's Boot key, or hold that half's outer-column
top-row key while plugging in), then `picotool load -v -x <file>.uf2` or `qmk flash`, and
repeat for the other half. `docs/flash-checklist.md` has the procedure, including how to read
back an unknown build first.

```sh
tests/run_unit.sh            # split transport, panel, spaceship, Bjorn: needs only cc
tests/run_scenarios.sh new   # tap-hold and host OS through real QMK: needs ~/qmk_firmware
```

GitHub Actions builds the firmware on push, through the template's own workflow in
`.github/workflows/build_binaries.yaml`.

## Also coming

A splitkb Halcyon Kyria Wireless. It runs ZMK, not QMK, and gets its own repo; the layout
stays the same on both boards, so a layout change goes into `docs/layout.md` first and then
into both firmwares.
