---
paths:
  - "keyboards/**/oled*"
  - "keyboards/**/*logo*"
  - "keyboards/**/bjorn_*"
  - "keyboards/**/panel_*"
  - "keyboards/**/split_*"
  - "tests/unit/**"
  - "tools/**"
---

# OLED (SSD1306 on the Kyria rev3)

Checked against QMK `7a1bbf37c5` and this repo at `a7b6dfd` on 2026-10-14. The *design* rules
live in `docs/layout.md`, OLED section — read those before changing behaviour. This file is how
the hardware and the driver actually work.

## The panel

- 128×64 per half over I²C at 400 kHz, driven by a Liatris (RP2040). `OLED_ROTATION_180` keeps it
  128 wide.
- The buffer is 1024 bytes: 8 pages of 128 columns. **A byte is a vertical strip of 8 pixels, bit
  0 at the top.** Raw art has to be in this page format.
- 16 dirty blocks of 64 bytes. `block = buffer_index / 64`, which is `page * 2 + (x >= 64)`.
  Roughly 1.7 ms of I²C per block.
- `OLED_DRIVER = ssd1306` is lowercase. `oled_task_user` returns `bool`. Older tutorials get both
  wrong.

## The rule that matters most

**Any byte that changes marks its block dirty — even if it changes back to its old value within
the same frame.** `oled_write_raw_byte()` returns early only when the byte it is handed already
equals what is there (`oled_driver.c:667-672`). A dirty block makes `oled_render_dirty()` call
`oled_on()` (`:445`, `:453`), and `oled_on()` re-arms `oled_timeout` (`:731`, `:737`). So:

> **Never clear-then-redraw into the driver buffer.** Compose the frame in `u_canvas`
> (`oled_canvas.h`) and commit it once. An unchanged frame must dirty nothing.

This is not theoretical. Clearing each zone and redrawing it is exactly what kept both panels
awake for ever and burned the badge in — Bug A, `docs/oled-v2.md` §3.2. The unit tests guard it:
a state rendered twice 50 ms apart must produce a dirty mask of zero.

`oled_render()` runs on **every main loop**, not every 50 ms; only `oled_task_kb()` is gated by
`OLED_UPDATE_INTERVAL` (`oled_driver.c:922-940`). So one dirty block goes over the wire per loop,
and a permanently dirty frame means permanent I²C traffic on that half.

## Other driver facts

- Keep every buffer index below 1024. For `index >= 1024` the driver clamps to 1024 and then
  writes one byte past the end of the buffer (`oled_driver.c:668`). Host stubs mirror that with a
  canary byte.
- `oled_read_raw()` exists (`oled_driver.c:659`, `oled_driver.h:384`), so the buffer can be read
  back. That is how the canvas commit knows which bytes to skip.
- `oled_task_user` runs every 50 ms (20 Hz) and **must return false**, or `kyria.c`'s
  `oled_task_kb` paints its own screen over ours. It returns early while the panel is off.
- Rotation: the driver calls `oled_init_user(oled_init_kb(rotation))` (`oled_driver.c:295`), so a
  keymap `oled_init_user` *would* run. `kyria.c` returns 180; this keymap defines none.
- `oled_write` / `oled_write_P` write text at the cursor and do not clear what follows, so every
  field needs a fixed width and a blank of the same length. Only `oled_master_text.c` still uses
  them.
- Bitmaps live in `PROGMEM` and are read with `pgm_read_byte`. Both are no-ops on ARM; keep them
  so the code stays portable.

## Which half shows what

The master is the half with the USB cable (VBUS on GP19, decided once at boot). It draws the
three-band panel, `oled_master_zones.c` — status bar on page 0, a 128x40 layer picture on pages
1-5 (which is also the alarm), the mod row on pages 6-7; `oled_master_text.c` is the alternative.
The other half draws Bjorn, `oled_slave_bjorn.c`; the spaceship and the logos are the
alternatives. Chosen in
`users/manna-harbour_miryoku/custom_config.h` and enforced by `oled_panel.h`, which refuses a
build with none or more than one of each.

## Split

- State crosses on a custom 5-byte RPC, `U_SYNC_STATE` (`split_state.c`, `split_sync.c`). **Not**
  `SPLIT_LAYER_STATE_ENABLE` / `SPLIT_MODS_ENABLE` / `SPLIT_LED_STATE_ENABLE`: none of them is
  enabled.
- WPM crosses through `SPLIT_WPM_ENABLE`; panel on/off through `SPLIT_OLED_ENABLE` (from the
  board's `keyboard.json`), which the slave applies every loop from shared memory; the master's
  matrix is mirrored to the slave (`SPLIT_TRANSPORT_MIRROR`).
- The slave's RPC callback runs on a HIGHPRIO thread **holding the shared-memory lock**: check the
  length, `memcpy`, publish. Nothing else belongs there.
- **Both halves are flashed with the same build.** `NUM_TOTAL_TRANSACTIONS` is XORed into every
  split handshake (`platforms/chibios/drivers/serial_protocol.c:69`), so a mismatched pair fails
  every transaction, the matrix included — the other half simply stops typing.
- When the transport counts as disconnected the master zeroes the other half's matrix
  (`quantum/matrix_common.c:101`), so "the other half stopped typing" and "the link is down" are
  the same symptom.

## Where code goes

- Edges are latched in the per-loop tasks (`oled_master_task`, `oled_slave_task`, both called from
  `housekeeping_task_user` in `keymap.c`). `oled_render_*` only draws. A transient flag has to stay
  set for at least 100 ms or the 20 Hz render can miss it.
- New source files go into `SRC` in the keymap's `rules.mk`, never into `keymap.c`.
- Generated files — `bjorn_art.*`, `panel_font.*`, `panel_glyphs.*`, `panel_layer_icons.*`, and
  the art files added later — are never edited by hand. Their generators live under `tools/`.
  `panel_layer_icons.c` is the layer pictures: hand corrections to them go in
  `tools/panel/icon_overrides.txt`, which the generator applies after reducing, never into the
  `.c` itself.
- Run `tests/run_unit.sh` after touching any OLED or split file. `--dump` prints the frames as
  text; `--raw` emits the raw buffers for a renderer.

## Burn-in

`OLED_TIMEOUT 120000`, `OLED_BRIGHTNESS 128`. QMK's own timeout follows `oled_on()` calls, which
any dirty block makes, so the keymap also checks `last_input_activity_elapsed()` directly — a
clock no amount of drawing can re-arm — and switches both panels off.
