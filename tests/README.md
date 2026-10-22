# Tests

Two mechanisms, because they cover different things:

| Runner | What | Needs |
|--------|------|-------|
| `tests/run_scenarios.sh` | tap-hold timing and host OS, as scenarios through stock QMK's tapping code | a `qmk_firmware` tree |
| `tests/run_unit.sh` | the split transport's master half, the master panel, the spaceship's background, Bjorn, the bootloader screen, and that every art file compiles | just `cc` |
| `tests/preview/run_preview.sh` | Bjorn's idle show, bongo cat and the Mario scene, rendered on the host into a GIF to watch before flashing | `cc` and Python 3 |

## Scenario tests

QMK unit tests (GoogleTest) for the tap-hold timing and the host OS handling. They run the real `tap_hold.c`, `host_os.c` and `keymap.c` against stock QMK's tapping code.

```bash
tests/run_scenarios.sh new   # current QMK at ~/qmk_firmware with this userspace's keymap code
tests/run_scenarios.sh old   # the old fork at ~/miryoku_qmk_old (tag archive/miryoku-with-latest-qmk)
```

A second argument picks another QMK tree. QMK only finds tests inside its own `tests/` folder, so the script copies the files to `<tree>/tests/rvannoord_hrm/` and deletes them afterwards. Run it from WSL (or anywhere QMK builds).

| Folder | What |
|--------|------|
| `scenarios/` | `scenario.hpp` (keys, clock, report recorder), `test_home_row_mods.cpp` (scenarios 1–9 and the crossover boundaries, both trees), `test_host_os.cpp` (scenario 10, new only), `test_split_state.cpp`, and `test_map_legend.cpp`: every key a live layer map draws has a legend on both hosts, from Miryoku's layer macros expanded with `custom_config.h` and RGB Matrix on, since the scenario build's `keymaps[]` is the framework's own; and D15's Mac legends and the mods following the host. It also draws the Layer map page from every layer through the real `oled_layer_page.c`: every key has a one-cell legend, the held layer key shows its base legend, no page outshines the alarm, and `U_LAYER_PAGES=<file>` writes the stage 5 mock's twelve Layer map frames for a byte compare |
| `new/` | `config.h` and `test.mk` matching the firmware, a placeholder `chordal_hold_layout` the test build needs to link, and `oled_stub.c`, which answers the keymap's two calls into the animation selector without the panel |
| `old/` | `config.h` with the old `BILATERAL_COMBINATIONS*` settings, and `test.mk` |

The scenarios, as the tests encode them. A scenario that passes in one tree and fails in the other is a behaviour difference.

Colemak-DH home row: A=GUI, R=Alt, S=Ctrl, T=Shift | N=Shift, E=Ctrl, I=Alt, O=GUI.

1. Same-hand fast roll: S, T 30 ms apart → `st`
2. Same-hand slow: S held 250 ms, then T → Ctrl+T new, `st` old (known difference)
3. Opposite-hand roll: T, then U 40 ms later, T released first → `tu`
4. Opposite-hand early press: T, then U at 40 ms, T held past 200 ms → `t` then `u`. T settles as a tap but stays down until released, so expect T held in the reports until T goes up.
5. Opposite-hand shortcut: T, then U at 120 ms, T held past 200 ms → `U`
6. Typing streak: `a`, then S within 100 ms and held 300 ms → `s`, no Ctrl
7. Shift mid-word: `e`, then T within 100 ms and held past 200 ms, then U → `U`
8. GUI head start: O, then W at 100 ms → `ow`; O held past 200 ms with W at 150 ms → GUI+W
9. Thumb layer: Space (Nav) held past 200 ms, a right-hand Nav key pressed within 50 ms of Space → that Nav key, Space not typed
10. Host OS: call `host_os_toggle()` directly (OS detection runs on USB setup packets and can't be exercised here); then `U_CPY` → Cmd+C, `U_RDO` → Cmd+Shift+Z

Expectations are the new setup's intended behaviour, written as what the host would show: `st`, `U` (Shift+u), `<Ctrl+t>`, `<LEFT>`. They encode the current values (`FLOW_TAP_TERM 160`, crossover 80 ms, 120 ms for GUI), so a deliberate tuning change updates the matching test in the same commit.

Results on 2026-10-09, when the migration from the old fork finished: new 13/13. Old 10/12; the two failures are the known differences, S2 (same-hand chord after 200 ms: old `st`, new Ctrl+T) and S4 (a mod-tap settled as a tap stays down while held; the old code sent a single tap).

The old tree needs `appdirs`, which the installed QMK CLI lacks, so `old` runs `make` inside a throwaway `uv` environment with that tree's `requirements.txt`.

# Unit tests

```bash
tests/run_unit.sh          # build and run all of them
tests/run_unit.sh --dump   # print the panel's frames as text instead of checking them
tests/run_unit.sh --raw    # the raw 1024-byte buffers, one per frame, for a renderer
tests/run_unit.sh --raw-map           # the live maps as D15 was decided from
tests/run_unit.sh --raw-pages 30,42   # stage 5's Status sheets, Typing fed that WPM series, the Event log
```

Plain C against stub headers in `unit/stub/`, with `unit.h` as the entire framework. They
exist for what the scenario tests cannot reach:

| File | What |
|------|------|
| `unit/test_split_sync.c` | the master send path. The scenario tests build `split_sync.c`'s non-`SPLIT_KEYBOARD` branch, where every function is a stub, so nothing there covers it — and a regression is silent and total: no sends at all, which on the keyboard looks like the other half's mascot going inert for no reason. Boot, the heartbeat, the hard window and recovery, on a clock that is a variable |
| `unit/test_scenes.c` | bongo cat and the Mario scene, from their own files. Bongo's frames by WPM and his slap rate; then the scene over scripted WPM curves, steady, ramping and stopping, about a hundred of Bill's passes: on every tick his pixels never meet Mario's or Yoshi's, no sprite enters the WPM corner (x 84-127, y 0-25) or goes above the top edge, and the scene's ink constants match the art. Also Yoshi curling up at 4 s and back, and a scene at rest costing nothing to redraw. A jump 1.5 ticks early or late fails it |
| `unit/test_bjorn.c` | the mascot. Compiles the real `oled_slave_bjorn.c` and `bjorn_art.c` on a fake clock. Two things with teeth: that he keeps blinking after a doze and after a dead link — a figure that moves but never blinks is this design's way of saying the link died, so losing the blink reports a fault that is not there — and that whenever two consecutive composed frames are identical, the commit writes no block at all |
| `unit/test_panel.c` | the master panel's three bands. Compiles the real `oled_master_zones.c` against a buffer laid out like the driver's, then measures what was drawn: that the band carries the layer's picture and the mod row every finger's glyph, that a held finger fills its cell while a misfire chips only the glyph box, which side the handedness bar takes and whether it is drawn or cut out, that the alarm inverts the whole band and the chain is knocked out of it, that the default-layer rule appears in exactly the two cases it should, and the pages: the cycle and its replug through a fake EEPROM word, the dots, the other half saving what the master sends, the master's map, the style toggle and the alarm rule, both kitties', the QMK logo's and the Monitor logo's base bands, the Stats stop, and on every page a free redraw and staying under the alarm. For stage 5's pages also the link-down rule, the Status sheet's striped kitty, Typing sampling on every page, and the event ring's wrap, order, stamp and cut, with every event the watcher logs. `--dump` prints the frames and `--raw` the buffers, which is how the bands get looked at without a flash |
| `unit/test_sync_slave.c` | the other half's side of the transport, from boot: that nothing counts as fresh before a state arrives, not even inside the first 600 ms, when the clock alone would say so; that the first real state does, with its `view` byte; that a wrong-sized state is dropped; and that silence makes it stale again. The other half saves the master's page and animation only from a fresh state, so this is what keeps zeros from overwriting a real choice |
| `unit/test_map.c` | the live layer map. Compiles the real `oled_map.c` against the keymap's `LAYOUT_miryoku` and the rev3 matrix (`stub/kyria_layout.h`, copied from what QMK generates). It checks every key's matrix position against the plan's table, which halves each layer draws on, that the right half runs inner to pinky, that a pressed key's box stays inside its cell in both styles, that a redraw is free, and that no map outshines the alarm. It also checks that the `@` is drawn whole and centred, and that the `|` is one bar on its key's centre. `--raw` emits the Fun and Nav maps as D15 was decided from (on 2026-10-22 all eight matched that render byte for byte), then Sym and Num |
| `unit/test_boot.c` | the bootloader screen. Compiles the real `oled.c` and records every flush. It checks that the jump to the bootloader draws the QMK logo whole and BOOTLOADER cut out of its bar, and sends it at once, one flush of every dirty block after the commit. That matters because nothing runs after `shutdown_user()` to send it later. It also checks that a soft reset, which comes through the same hook, leaves the panel alone |
| `unit/test_space.c` | the spaceship's background. Drives the real render on a fake clock and asserts, on the compiled bytes, that one loop at rest and at 50 WPM returns the identical frame, that the ship is bit-exact inside its mask, and that a one-pixel dark rim surrounds every ship pixel. `--raw <wpm>` emits the frames for a renderer |

`unit/stub/manna-harbour_miryoku.h` copies Miryoku's layer list rather than including the
real header, which would drag in all of QMK. The firmware build still uses the real one.

`unit/stub/driver.h` mirrors the driver's `oled_write_raw_byte` exactly: it returns early on a
byte that already matches, and otherwise marks the 64-byte block dirty. That is what makes "an
unchanged frame writes nothing" checkable on the host, and it is the check that would have caught
the panels never sleeping. The fake buffer is one byte longer than the panel, as a canary for the
driver's one-past-the-end write.

The panel test asserts the **ladder**, not the layout: whatever the alarm is, nothing
routine may light more of the panel than it does. That phrasing is deliberate, and it has
already earned itself. The mod row was once dark at rest with nothing on it inverting,
because the alarm was a 128x8 row and two held modifiers matched it exactly. The alarm is
now the whole 128x40 band, so a filled cell no longer comes near it and the row went back
to showing every glyph. The rule did not change; the measurement did. A test written
against "dark at rest" would have had to be deleted, and would have taken the reasoning
with it — one asserting the ratio survives the move and still fails if the rung is spent.

# Preview

```bash
tests/preview/run_preview.sh [idle|wake|tiers|props|bongo|mario] [--invert]
```

Compiles the real `oled_slave_bjorn.c` on a fake clock, like the unit test, but records every
frame of a scenario into `tests/preview/out/<scenario>/` (git-ignored) and hands them to
`tools/preview.py`, which turns them into `strip.png` and `anim.gif`. It is how the idle show is
watched before it is flashed; what it cannot show is in `docs/flash-checklist.md`.
