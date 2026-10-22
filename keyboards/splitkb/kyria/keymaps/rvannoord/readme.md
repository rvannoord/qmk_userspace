# rvannoord: Kyria rev3, Miryoku, two OLEDs

There is no `keymaps[]` in this folder. The layers are Miryoku's: `rules.mk` sets
`USER_NAME := manna-harbour_miryoku`, which pulls in `users/manna-harbour_miryoku/`, and its
`manna-harbour_miryoku.c` defines the keymap through the `LAYOUT_miryoku` wrapper in `config.h`
here. The wrapper is where Miryoku's 36 keys meet the Kyria's 50: the other fourteen are `XXX`
except for the two system keys. Everything else in this folder is what Miryoku does not have.
The design and every tuning decision are in `docs/layout.md`; this file is the map.

## Files

| File | Holds |
|------|-------|
| `rules.mk` | Miryoku's options (set here, never on the command line), `CONVERT_TO = liatris`, and the `SRC` list |
| `config.h` | the `LAYOUT_miryoku` wrapper in two variants, one per Miryoku thumb mapping; the custom keycodes; the split transaction id |
| `keymap.c` | the hooks: `housekeeping_task_user` (sync, link health, OLED tasks, sleep), `process_record_user` (SYS, ANIM, PAGE, Mac redo), the encoders. QMK compiles it on its own, so it is never in `SRC` |
| `host_os.c` / `.h` | `host_is_mac()`, the one switch for everything OS-specific |
| `tap_hold.c` | the Chordal Hold and Flow Tap callbacks for the home-row mods |
| `split_state.c` / `.h` | the 8-byte state the master gathers for the other half: modifiers in finger order, layers, locks, Caps Word, host OS, misfires, the chosen animation, the stored page |
| `split_sync.c` / `.h` | the custom RPC that carries it: on change plus a 250 ms heartbeat, double-buffered on the slave |
| `split_health.c` / `.h` | link health: the stats page (hold SYS two seconds), self-reset of a half whose link stays dead, the hardware watchdog |
| `oled.h` / `oled.c` | the OLED module's interface and its dispatcher: boot splash, then master or slave; and the bootloader screen, painted on the way into the bootloader |
| `oled_canvas.c` / `.h` | the frame every renderer draws into, committed to the driver once per frame |
| `oled_master_zones.c` | the master panel: status bar, a 128x40 layer picture, the mod row; its pages, turned with PAGE; and what an encoder just did. |
| `oled_map.c` / `.h` | the live layer map: while a thumb layer is held, the panel over its keys shows what they do, in a big grid or shaped like the Kyria |
| `map_legend.c` / `.h` | what each key's legend is on the map, on Windows and on the Mac, and its one-cell form for the Layer map page |
| `oled_draw.c` / `.h` | drawing at any row: boxes, art, legends and text, for the map and the pages |
| `oled_layer_page.c` / `.h` | the Layer map page: the current layer, both halves, on the master's panel |
| `oled_pages.c` / `.h` | the Status sheet and Typing pages |
| `oled_event_log.c` / `.h` | the Event log page: the last eight link and host events, stamped with the uptime |
| `oled_settings.c` / `.h` | the one EEPROM word the animation and the page share; the other half saves what the master sends |
| `speed_tier.h` | WPM as four speed tiers, for Bjorn and the master's kitty |
| `oled_slave_select.c` | what the half without the cable shows: its map, or the chosen animation, persisted on the master and sent across |
| `oled_slave_bjorn.c` | Bjorn: the idle ladder, blink, poses, props |
| `oled_slave_world.c` / `.h` | the weather behind Bjorn, across the whole panel, season from `__DATE__` |
| `oled_slave_spaceship.c` | the ship over three parallax planes, speed from WPM |
| `oled_slave_logo.c` | the static screens: the Kyria logo and the three Monitor ERP ones |
| `oled_slave_bongo.c` | bongo cat, slapping faster as WPM climbs |
| `oled_slave_mario.c` | Mario, Yoshi and Bullet Bill: they walk with your typing and jump Bill as he comes |
| `art/` | **generated and vendored data only, never edited by hand.** `*_art.c`, `bjorn_poses.c`, `drashna_*.c`, `status_frame.c`, `painter_*.c` and `panel_*.c` come out of `tools/`, whose README says how to regenerate each; the `oled_*.h` are the vendored ship and logos. Where everything came from is in `VENDORED.md`. The art from drashna's userspace is generated ahead of the stages of `docs/oled-v3.md` that draw it, so not all of `art/` is in `SRC` yet |

Naming: `u_` / `U_` is this keymap's prefix, following Miryoku's own `U_` for its keycodes
(`U_BASE`, `U_NAV`, `U_RDO`); it is not QMK's Unicode `U`. Custom keycodes are `QK_USER_n`
aliases under the same prefix, because QMK already uses the short names (`OS_TOGG`).

## Keys outside Miryoku

| Key | Where | Does |
|-----|-------|------|
| SYS (`U_SYS`) | left half, outer column, top row | flips between the Mac and the detected non-Mac host (Windows, or Linux on a Linux machine; an iPad flips iOS and Windows); held two seconds, the stats page; held while plugging in, the bootloader (Bootmagic) |
| ANIM (`U_ANIM`) | right half, outer column, top row | cycles the other half's animation: Bjorn, spaceship, Kyria logo, and three Monitor ERP screens. Saved on both halves, so it follows the cable |
| PAGE (`U_PAGE`) | left half, outer column, middle row, under SYS | cycles the master half's pages: the bands, the bands with the small kitty, with the striped kitty, with the QMK logo, with the Monitor ERP logo, Stats. Dots in the status bar show where you landed. Saved on both halves, so it follows the cable |

Thumb arc, five positions per half from the outside in: position 1 is the encoder (left:
volume, right: screen brightness; its click is unassigned), positions 3 to 5 carry Miryoku's
thumb keys. Position 2, the upper thumb keys and the rest of the outer columns are free.

## Host OS

QMK's OS detection sets `host_is_mac()` on plug-in (`OS_DETECTION_SINGLE_REPORT`, because Apple
Silicon Macs re-trigger detection minutes later); SYS flips it when detection guesses wrong; the
master OLED shows the host's logo (Windows, Apple, or Tux for Linux, which behaves like Windows;
iOS behaves like the Mac). On the Mac, Ctrl and GUI swap, so Cmd shortcuts use the same fingers as
Ctrl shortcuts on Windows; redo is special-cased to Cmd+Shift+Z. Nothing hardcodes Ctrl or Cmd for
one OS: anything OS-specific reads `host_is_mac()`.

## The OLEDs, as the hardware actually works

The design rules are in `docs/layout.md`, OLED section. These are the facts the code is built
on, checked against QMK `7a1bbf37c5`.

- 128x64 per half over I²C at 400 kHz; `OLED_ROTATION_180` keeps it 128 wide. The buffer is
  1024 bytes, 8 pages of 128 columns. **A byte is a vertical strip of 8 pixels, bit 0 at the
  top**, all art is stored in that page format, and a set bit is a lit pixel.
- 16 dirty blocks of 64 bytes, `block = page * 2 + (x >= 64)`, about 1.7 ms of I²C each.
  `oled_render()` runs every main loop and sends one dirty block per loop; only
  `oled_task_user` is gated to 20 Hz by `OLED_UPDATE_INTERVAL`.
- **Any byte that changes marks its block dirty, even if it changes back within the same
  frame**: `oled_write_raw_byte()` returns early only on a byte that already matches. A dirty
  block calls `oled_on()`, and `oled_on()` re-arms the display timeout. So never clear and
  redraw into the driver's buffer: compose the frame in `u_canvas` and commit it once, and an
  unchanged frame must dirty nothing. This is what once kept both panels awake for ever and
  burned the badge in; the unit tests assert a dirty mask of zero for a frame rendered twice.
- Because any drawing re-arms QMK's own timeout, `keymap.c` also reads
  `last_input_activity_elapsed()`, a clock drawing cannot touch, and switches both panels off
  after `OLED_TIMEOUT` (120 s). Brightness is 255: lit-on-black lights about a quarter of the
  pixels, so that is a lighter load than 128 was when the panel ran inverted.
- `oled_task_user` must return `false`, or `kyria.c`'s own `oled_task_kb` paints over ours.
  Rotation is not set here: `kyria.c` already returns 180.
- Keep every buffer index below 1024. For 1024 the driver clamps and then writes one byte past
  the end of its buffer; the test stubs carry a canary byte for exactly that.
- The master is the half with the USB cable, decided once at boot from VBUS on GP19. It draws
  the three bands; the other half draws whichever animation is chosen.
- Bitmaps live in `PROGMEM` and are read with `pgm_read_byte`, both no-ops on ARM, kept so the
  code stays portable.

## Split

- State crosses on the custom RPC `U_SYNC_STATE` (`split_state.c`, `split_sync.c`), not on
  QMK's `SPLIT_LAYER_STATE_ENABLE` / `SPLIT_MODS_ENABLE` / `SPLIT_LED_STATE_ENABLE`: those
  resend every 100 ms whether anything changed or not, and cannot carry Caps Word or the SYS
  override.
- WPM crosses through `SPLIT_WPM_ENABLE`, the activity clock through `SPLIT_ACTIVITY_ENABLE`,
  panel on/off through `SPLIT_OLED_ENABLE` from the board's `keyboard.json`, and the master's
  matrix is mirrored to the slave (`SPLIT_TRANSPORT_MIRROR`).
- The slave's RPC callback runs on a HIGHPRIO thread holding the shared-memory lock: check the
  length, `memcpy`, publish. Nothing else belongs there.
- **Both halves run the same build.** `NUM_TOTAL_TRANSACTIONS` is XORed into every split
  handshake, so a mismatched pair fails every transaction, the matrix included, and the other
  half simply stops typing. A down link looks the same, because the master zeroes the other
  half's matrix when the transport counts as disconnected.

## Tests

`tests/run_unit.sh` compiles the real renderers and the transport against stub headers, with
nothing but `cc`; `--dump` prints the panel's frames as text. After a tap-hold or host-OS
change, `tests/run_scenarios.sh new` runs the GoogleTest scenarios through a real QMK tree.
`tests/README.md` has the details, and `docs/flash-checklist.md` lists what only the board
itself can show.
