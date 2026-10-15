# OLED v2: plan

The next round of work on the OLEDs: the two bugs found while daily-driving (the panels never sleep, and the link between the halves was seen down), the idle director, the goat, weather by season, and a place for every one of Bjorn's seventeen sprite sets. Written 2026-10-14 on the work PC, from a read of `main` at `c4c93b9` and stock QMK 0.34.6 at `7a1bbf37c5`, plus host harnesses run against the real renderer. It is worked through on the Mac in `~/qmk_userspace`, top to bottom, one stage at a time, stopping at every checkpoint. The repo plus this file are enough.

QMK line numbers refer to `7a1bbf37c5`. Repo line numbers refer to `c4c93b9`. If the repo has moved on, read `git log c4c93b9..HEAD` first and adjust.

Keep the Progress table below current, in the same commit that finishes a stage. The design itself stays in `docs/layout.md`; this file is the build order. When every stage is done it goes, the way `MIGRATION.md` will.

## Progress

| Stage | Status |
|-------|--------|
| 0. Docs tell the truth | **done 2026-10-14** (firmware byte-identical, proven) |
| 1. Bug A: the panels sleep, Bjorn keeps blinking, sequence per send | **done 2026-10-14**, flashed `c95356d`, checked on the board |
| 2. Bug B: link health, stats page, self-heal | **done 2026-10-14**, cause still unknown; §2.5 skipped for want of evidence |
| 3. Survey of all 17 sprite sets | fetched and sheets rendered 2026-10-14; tools committed. Mapping with the animator |
| 4. Director, layered compositor, preview harness | preview harness and one idle clock done 2026-10-14; compositor and director open |
| 4a. Master panel reworked into three bands (unplanned) | **done 2026-10-15**, not flashed. Layer icons from splitkb's Halcyon module, QMK's own font replaces DUNGEON.mode. See the note below |
| 5. The goat | open |
| 6. Weather by season | open |
| 7. Doze extras and the heart | open |
| 8. Every sprite set gets its moment | open |
| 9. Remaining spec items, the leaf on the helmet | open |
| 10. Tuning, the inverted look | open |
| 11. Migration leftovers | open |

### Note: stage 4a reworked the master panel, 2026-10-15

Not in this plan — Roel asked for the master half to look like splitkb's Halcyon TFT
panel. It replaced the five zones with three bands, so parts of this document written
against the old map are now history rather than instructions. **Diagnosis and commit
records stay as they are** (they were true when written); what is stale is the
forward-looking text:

- §12 item 4's description of what `test_panel.c` asserts — done, differently: the test
  now asserts the alarm ladder as a ratio rather than "dark at rest".
- The stats page is still 16 characters wide but is drawn in **QMK's own 6x8 font**, not
  DUNGEON.mode, which is no longer used anywhere.
- "the master already draws a rule under the layer name" — under the layer *icon* now,
  and only while a held layer hides a non-base default.
- Anything below that plans new art "in DUNGEON.mode's weight language" should read
  `tools/panel/glyphs.py`'s language instead, which is where the panel's hand-drawn
  glyphs actually live.

The alarm's move is the one with a reason worth carrying forward: a 128x8 inverted banner
had fallen to about two held mod cells, so it could no longer outshine a routine frame.

---

## 1. Working rules (Roel's, binding)

- **Attribution.** The rule in `CLAUDE.md` applies to every commit, tag and pushed file, including the tools and generated files this plan adds.
- **Commit and push only when Roel asks. Ask before every push.** Commit verified states only: built clean with no warnings, tests run. Conventional commits with an emoji, like the history (`✨ feat(oled): ...`, `🐛 fix(oled): ...`, `✅ test: ...`, `📝 docs(...)`, `♻️ refactor(...)`). Identity `rvannoord <rvannoord@icloud.com>`, SSH-signed through 1Password. Check `git config user.email` before every commit. If signing fails, stop and tell Roel. Never bypass signing.
- **Stages with checkpoints.** Each stage ends at a checkpoint where Roel looks before the next stage starts; some stages also have an earlier stop. Honour every "stop here".
- **Never change QMK core** (`quantum/`, `tmk_core/`, `platforms/`, `drivers/` in `~/qmk_firmware`). If something seems to need it, stop and say so. Don't update QMK during this work unless Roel asks.
- **Don't reinstall working tools, and don't install anything without asking.**
- **Verify in the real build instead of assuming**: `cflags.txt`, preprocessing with the build's own flags, reading symbols and data out of the ELF (Appendix A). Every report says plainly what was checked and what was not.
- **Flash both halves together, with the same build.** `NUM_TOTAL_TRANSACTIONS` is XORed into every split handshake (`platforms/chibios/drivers/serial_protocol.c:69`), so two builds with different transaction tables fail every transaction, matrix included. Today this rule is recorded only in the message of commit `df5c23b`; Stage 0 puts it in the docs.
- **An unknown build on the board is read back first**, per half, with `picotool save -a -v` (MIGRATION.md step 8), before anything is flashed.
- **Roel's design rules in `docs/layout.md` (OLED section) are binding** unless he changes them: dark at rest; "an expression is for something wrong, a prop is for something true, idle is for everything else"; nothing loops while typing; a budget of 8 dirty blocks per animation step when idle and 2 while typing, with priorities fault > alarm > one-shot reaction > Bjorn pose > goat locomotion > ambient, and over budget defers by one step; non-harmonic ambient clocks; "a beat on a fixed timer is wallpaper within a week"; the Motion rules (pre-dithered bits at integer offsets, gestures differ in place, cuts for identity and motion for position, overlap only on soft masses, an overlay is a whole 64-byte block); never freeze mid-stride, because frozen with no blink means the link died.

---

## 2. Start here (Mac session)

### 2.1 Pull and look

```sh
cd ~/qmk_userspace
git status --short                      # must be empty
git pull --ff-only                      # expect c4c93b9 or later
git log --oneline c4c93b9..HEAD         # anything newer than this plan
git -C ~/qmk_firmware log -1 --format='%h %s'   # expect 7a1bbf37c5
```

### 2.2 Environment check (record the output, install nothing)

```sh
qmk --version                           # 1.1.8 (Homebrew)
qmk config user.qmk_home user.overlay_dir
cc --version
python3 --version                       # tools in this plan use the standard library only
which picotool arm-none-eabi-gcc arm-none-eabi-size arm-none-eabi-nm arm-none-eabi-objdump gh
picotool version                        # 2.3.1
git -C ~/qmk_userspace config user.email      # rvannoord@icloud.com
git -C ~/qmk_userspace config gpg.format      # ssh
git -C ~/qmk_userspace config commit.gpgsign  # true
ls -l ~/kyria-firmware-backup
```

If `arm-none-eabi-size`/`nm` are missing, ask Roel before installing anything; meanwhile the `.uf2` size divided by 2 is the payload size (each 512-byte UF2 block carries 256 bytes).

### 2.3 Baseline before any change

```sh
qmk compile -kb splitkb/kyria/rev3 -km rvannoord     # must be clean, no warnings
tests/run_unit.sh                                    # split_sync and panel: all good
tests/run_scenarios.sh new                           # all pass
arm-none-eabi-size -A ~/qmk_firmware/.build/splitkb_kyria_rev3_rvannoord.elf
arm-none-eabi-nm -S ~/qmk_firmware/.build/splitkb_kyria_rev3_rvannoord.elf | grep split_transaction_table
ls -l ~/qmk_userspace/splitkb_kyria_rev3_rvannoord.uf2
shasum -a 256 ~/qmk_userspace/splitkb_kyria_rev3_rvannoord.uf2
```

If anything fails or warns at baseline, stop and report before changing anything.

Reference values from the work-PC build of `c4c93b9` (arm-none-eabi-gcc 15.2; the Mac uses gcc 8, so expect different numbers and measure all deltas on the Mac): `.text` 43,384, `.rodata` 12,384, `.data` 3,524, `.bss` 9,508 bytes; `.uf2` 119,808 bytes (about 59.9 KB of payload); `split_transaction_table` 180 bytes, i.e. 15 transactions of 12 bytes. Bjorn's pixel data is 7,232 bytes (a 512-byte base plus 105 patches of 64 bytes).

### 2.4 Read, in this order

1. This file, completely.
2. `CLAUDE.md`, `MIGRATION.md` (Progress table, "Environment (Mac)"), `docs/layout.md` (whole OLED section), `.claude/rules/oled.md` (stale, see 3.5), `tests/README.md`, `VENDORED.md`.
3. Keymap folder `keyboards/splitkb/kyria/keymaps/rvannoord/`: `oled.c`, `oled_panel.h`, `oled_master_zones.c`, `oled_master_text.c`, `oled_slave_bjorn.c`, `bjorn_art.h` (and the frame table at the end of `bjorn_art.c`), `split_state.c/.h`, `split_sync.c/.h`, `keymap.c`, `host_os.c`, `tap_hold.c`, `rules.mk`, `config.h`; then `users/manna-harbour_miryoku/custom_config.h`.
4. Tests: `tests/run_unit.sh`, `tests/unit/test_panel.c`, `tests/unit/test_split_sync.c`, `tests/unit/stub/*`, `tests/scenarios/*`.

### 2.5 Ask Roel first, then wait

**About the link loss (Bug B).** Ask all of these before Stage 1 changes anything, because the answers describe the old build:

- Q1. What exactly did you see: (a) the inverted `LINK DOWN` banner on the master panel (page 5), (b) Bjorn not blinking (he still breathes when settled; no blink is the design's "link dead" signal), (c) both, or (d) something else (blank or garbled slave panel)?
- Q2. Which machine (Windows work PC or Mac), which half had the USB cable, and was the computer asleep, locked with the screen off, or awake? Roughly how long were you away?
- Q3. Did keys on the other half still type? Did the master still show held modifiers?
- Q4. Did it clear by itself, after typing (how soon), or only after a replug? Which half was replugged?
- Q5. Was Bjorn asleep (eyes shut) or awake when you came back? Did he blink at all afterwards?
- Q6. Once or several times? Which build is on the board (date, commit, which machine built and flashed it)?
- Q7. Anything else odd at the time (RGB, stuck or repeating keys, panel artefacts)?
- Q8. If it is happening right now: don't replug; look at both panels first.

Tell Roel what reading the code found (section 3.4): Bjorn stops blinking after every doze and after any stale period, for up to about 55 minutes, which shows the "link dead" signal with nothing wrong on the link. If his answer is (b) only, that is the likely explanation. If it is (a), the master's sends really failed for at least 1.25 s while it was awake, and Stage 2's diagnostics are needed.

**Decisions:**

- D1. Decided 2026-10-14: this plan lives in the repo as `docs/oled-v2.md`.
- D2. Generators and tools go under `tools/` in the repo, Python 3 standard library only, nothing installed? (Default yes.)
- D3. Download permission for Stage 3: the files under `resources/images/status/` (106 BMPs) and `resources/images/static/` (24 files) of `github.com/infinition/Bjorn` at commit `61a4758e5ee3d37d43c10eb690340626139912c0`, about 1.7 MB in all, into `~/bjorn-src/` outside the repo. Separately, and only if the goat can't be cut cleanly from the frames: `resources/images/Bjorn.psd` (15.8 MB) and `Alva.psd` (5.4 MB).
- D4. Stats page trigger for Stage 2. Default: hold SYS for 2 s; the Win/Mac toggle then happens on release of a shorter press instead of on press.
- D5. Keep the last link outage across a replug (one EEPROM write per outage)? Default no: RAM only, so look before replugging.
- D6. Slave self-reset (Stage 2): enabled by default, with backoff. Optional hardware watchdog on the slave: decide after the diagnostics.
- D7. May the idle show run past 120 s? Default no.
- D8. For the docs: which builds were flashed since step 8 (`2985c76`, 2026-10-10), the verdict of the stage 1 legibility test, and anything for the Tuning log since 2026-10-10.
- D9. Where on the Mac were `bjorn_art.c`, `panel_font.c` (`gen_font.py`) and `panel_glyphs.c` (span lists) generated? If those generators still exist, they get committed under `tools/` in Stage 3.

---

## 3. What was found on 2026-10-14

### 3.1 QMK facts this plan relies on

| Fact | Where (QMK `7a1bbf37c5`) |
|------|------|
| `oled_render_dirty()` calls `oled_on()` whenever any block is dirty | `drivers/oled/oled_driver.c:445`, `:453` |
| `oled_on()` re-arms `oled_timeout = timer_read32() + OLED_TIMEOUT` | `oled_driver.c:731`, `:737` |
| `oled_task()` calls `oled_task_kb()` every `OLED_UPDATE_INTERVAL` (50 ms), then `oled_render()` (one block per call), then switches off when the timeout expires. That check is not gated on master | `oled_driver.c:923-962` (`:932`, `:947`, `:951`) |
| `oled_write_raw_byte()` returns on an equal byte, otherwise stores it and marks its 64-byte block dirty. It clamps `index > 1024` to 1024 and then writes one byte past the buffer | `oled_driver.c:667-672` |
| `oled_read_raw()` exists, so the buffer can be read back | `oled_driver.c:659`, `oled_driver.h:384` |
| `oled_init()` calls `oled_init_user(oled_init_kb(rotation))`: a keymap `oled_init_user` would run (the docs say it would not) | `oled_driver.c:295` |
| 128x64: 16 blocks of 64 bytes; block = page x 2 + (x >= 64) | `oled_driver.h:29-48` |
| Defaults: `OLED_I2C_TIMEOUT` 100 ms per transfer, `OLED_UPDATE_INTERVAL` 50 (split), `OLED_UPDATE_PROCESS_LIMIT` 1 | `oled_driver.h:300-310` |
| I2C runs at 400 kHz on RP2040: a 64-byte block costs roughly 1.6-1.8 ms of blocking time in the main loop (computed, not measured) | `platforms/chibios/chibios_config.h:60-62` |
| Main loop order: `protocol_pre_task`, `keyboard_task`, `protocol_post_task`, ..., `housekeeping_task` last | `quantum/main.c` |
| `keyboard_task`: `matrix_task` ... `oled_task()` at 755, then `if (activity_has_occurred) oled_on()` at 758 | `quantum/keyboard.c:710-758` |
| `last_input_activity_elapsed()` uses the split sync timer; on the master it covers both halves' keys and both encoders (slave encoder events are processed by the master's `encoder_task`) | `keyboard.c:161`, `quantum/encoder.c:59-82` |
| On the slave the master's matrix is mirrored (`SPLIT_TRANSPORT_MIRROR`), so the slave's own `last_input_activity_elapsed()` also sees the master half's keys | `quantum/matrix_common.c:91-115`, `transactions.c` master matrix handlers |
| When the transport counts as disconnected, the master zeroes the other half's matrix | `matrix_common.c:101` |
| OLED on/off is sent on change or every `FORCED_SYNC_THROTTLE_MS` (100 ms); the slave calls `oled_on()` or `oled_off()` every loop from the shared-memory value | `quantum/split_common/transactions.c:71`, `:635-651` |
| Standard handlers retry up to 10 times per loop when connected, once when not | `transactions.c:97-98` |
| `transaction_rpc_send()` is `transaction_rpc_exec(id, n, buf, 0, NULL)`. It returns false at once when `!is_transport_connected()`, then runs four transactions (`PUT_RPC_INFO`, `PUT_RPC_REQ_DATA`, `EXECUTE_RPC`, `GET_RPC_RESP_DATA`, the last even with zero bytes), one attempt each, no retry. RPC failures never count towards `connection_errors` | `transactions.h:52`, `transactions.c:1011-1048` |
| `slave_rpc_info_callback()` takes the lengths from the wire before any CRC check (the CRC is checked at `EXECUTE_RPC`), so a corrupted info packet leaves a wrong expected length until the next good one; every send starts with a fresh info packet, so it self-corrects | `transactions.c:1050-1072` |
| Connected means `connection_errors < 10`; while disconnected, one attempt per 500 ms | `quantum/split_common/split_util.c:47`, `:54`, `:230-266` |
| Master is decided once at boot by `usb_vbus_state()` (VBUS on GP19). `SPLIT_USB_DETECT` is not defined | `split_util.c:78`, `:181`; `platforms/chibios/converters/elite_c_to_rp2040_ce/converter.mk`; the build's `src/info_config.h` |
| `SPLIT_WATCHDOG_ENABLE` (not used here) only resets a slave that never heard from the master after boot | `split_util.c:113-119` |
| Slave transport: a HIGHPRIO thread; callbacks run with the shared-memory lock held; handshake = id XOR `NUM_TOTAL_TRANSACTIONS` | `platforms/chibios/drivers/serial_protocol.c:38`, `:63`, `:69` |
| Serial: 230,400 baud (about 43 µs a byte), 20 ms timeout per transfer | `platforms/chibios/drivers/serial_usart.h:30`, `:44` |
| USB suspend: a loop of `suspend_power_down()` (which calls `oled_off()`, then waits 17 ms) and `suspend_wakeup_condition()` (`matrix_scan()`, so the standard split transactions keep flowing). `housekeeping_task` does not run, so our RPC stops | `tmk_core/protocol/chibios/chibios.c:180-198`, `platforms/chibios/suspend.c`, `quantum/quantum.c:480`, `:521`, `platforms/suspend.c` |
| Double-tap reset is on for this board: every boot busy-waits 500 ms in `__late_init()`, and a second reset inside that window jumps to the USB bootloader. `magic_location` lives in `.ram0.bootloader_magic`, which the map places after `__ram0_noinit__` (not cleared at boot) | `platforms/chibios/boards/QMK_PM2040/configs/config.h` (`RP2040_BOOTLOADER_DOUBLE_TAP_RESET`, 500U); `platforms/chibios/bootloaders/rp2040.c:32`, `:48`, `:54` |
| `mcu_reset()` is `NVIC_SystemReset()` | `rp2040.c:16-18` |
| Bootmagic clears EEPROM before jumping to the bootloader | `quantum/bootmagic/bootmagic.c:63-75` |
| pico-sdk `hardware_watchdog` is compiled into RP2040 builds | `platforms/chibios/vendors/RP/RP2040.mk:32` |
| WPM is a moving window of 5 s (25 periods of 200 ms) | `quantum/wpm.h`, `quantum/wpm.c` |
| The linker's flash region is 2 MB | build map: `flash1 0x10000000 0x00200000` |
| No version string is in today's firmware: `QMK_BUILDDATE` and `QMK_USERSPACE_VERSION` exist in the generated `version.h` but nothing references them | grep of the work-PC ELF and binary |

Confirmed earlier today by preprocessing with the build's `cflags.txt`: `OLED_TIMEOUT 120000`, `OLED_BRIGHTNESS 128`, `OLED_UPDATE_INTERVAL 50`, `OLED_UPDATE_PROCESS_LIMIT 1`, `SPLIT_OLED_ENABLE`, `SPLIT_TRANSPORT_MIRROR`, `SPLIT_WPM_ENABLE`, `USB_VBUS_PIN 19U`, `SPLIT_HAND_PIN B5`, `SERIAL_DRIVER_VENDOR`, and `NO_DEBUG`/`NO_PRINT` (so there is no console). Not defined: `SPLIT_WATCHDOG_ENABLE`, `SPLIT_ACTIVITY_ENABLE`, `SPLIT_USB_DETECT`.

### 3.2 Bug A: the panels never turn off (confirmed)

Mechanism, in stock QMK: any dirty block makes `oled_render_dirty()` call `oled_on()`, which re-arms the 120 s timeout; `oled_task()` only switches off when it expires (3.1).

Root cause, master: `oled_master_zones.c` renders every frame by blanking each zone and then drawing its content: `render_status()` line 190, `render_mods()` 227-228, `render_layer()` 256-257, `render_alarm()` 283, `render_trace()` 303-304 all start with `fill(page, 0, 128, 0x00)`. The final buffer is identical, but every lit byte changes twice, so its block is marked dirty every 50 ms, `oled_on()` re-arms the timeout, and the master never sleeps; the slave mirrors it through the OLED transaction. A host test earlier today (driver-accurate dirty tracking, the real `oled_master_zones.c`, the same state rendered twice 50 ms apart) found these blocks dirty on an unchanged frame:

| State | Dirty blocks on an unchanged frame |
|-------|-----|
| at rest, Win or Mac | 0 (page 0 left, the badge) |
| Nav | 0, 6, 7, 8, 9 (badge, pages 3-4 both halves) |
| Ctrl finger held (Windows: middle finger) | 0, 3, 5 (badge, pages 1-2 right) |
| LINK DOWN, steady | 0, 10, 11 (badge, page 5 both halves) |

Consequences: the panels never sleep (the badge burns in), and one block goes over I2C every loop, for ever.

The existing test hid it: `tests/unit/test_panel.c` `step()` memsets the buffer before every render, and its `oled_write_raw_byte()` stub does no dirty tracking.

Checked by reading today (all halves, all renderers):

- `oled_master_text.c` does **not** have the problem: it draws through `oled_write`, and the driver's `oled_write_char()` compares the finished glyph with what was there before marking anything dirty; each cell is written once a frame.
- `oled_slave_bjorn.c` **does**: `bjorn_blit_base()` writes the whole left column, then `bjorn_blit()` writes pose and overlay patches over it, so every block an overlay covers changes twice each frame. Measured with a host harness (the real `oled_slave_bjorn.c` and `bjorn_art.c`, a driver-accurate stub): steady typing at tier 3, 6.0 blocks per frame; settled, averaged over one breath cycle, 2.7; dozing, 8.0 (the whole left column, every frame). At rest on tier 1 with the breath at rest it is 0, because tier 1 is the base itself. This does not cause Bug A (the slave follows the master's on/off), but it keeps the slave's I2C busy for roughly 13 ms of every 50 ms while dozing (computed from 8 blocks at about 1.7 ms; with Bug A, for hours), and it means the documented per-step budget is spent on every frame rather than on change.
- `oled_slave_spaceship.c` composes each row and writes it once, but scrolls by at least one column every frame by design; `oled_slave_logo.c` is static. Neither is selected.
- Nothing writes the OLED buffer outside `oled_task_user`: `housekeeping_task_user` (`keymap.c:22-32`) only runs `u_sync_task()`, `oled_master_task()` and `oled_slave_task()`, which build state. Every `oled_write*` call is in a render path.

Fix direction (Stage 1): compose each frame in a 1 KB RAM canvas and commit it once, so an unchanged frame writes no byte; plus a safety net on the master that switches the panels off when `last_input_activity_elapsed()` reaches `OLED_TIMEOUT`, whatever is drawn.

### 3.3 Bug B: "link lost" after being away (not pinned down)

Roel: "when I came back after a while I saw the link was lost between the two halves, that should never happen, or it should wake up again when typing."

Ruled out, with evidence:

- **Role swap after host sleep.** Master is decided once at boot by VBUS on GP19 (`split_util.c:78`, `:181`). A sleeping host cannot make both halves slaves.
- **The host-sleep path itself.** During USB suspend the loop repeatedly calls `suspend_power_down()`, which runs `oled_off()` (`quantum.c:521`), and `suspend_wakeup_condition()`, which runs `matrix_scan()` and so the standard split transactions (`matrix_common.c:96`), including the OLED-off message. The slave goes dark. `housekeeping_task_user` does not run during suspend, so our RPC stops and the slave goes hard stale while dark; on wake the first loop sends again (the heartbeat age is long past 250 ms), so it recovers within one loop if the transport is connected.
- So a **persistent** LINK DOWN on the master (no successful send for 1250 ms, `split_sync.c:67-70`) means sends kept failing while the keyboard was awake. Because of Bug A the panels were lit the whole time, so any hiccup was visible.

Precise RPC facts (3.1): `transaction_rpc_exec()` does not go through `transport_master_if_connected()` or its 500 ms throttle; it checks the connected flag and refuses at once while the standard transport counts as disconnected, so it recovers in the same loop as the first standard round that succeeds. Otherwise it runs four single-shot transactions, where standard handlers get up to ten tries per loop. A degraded line therefore hurts the RPC much more than matrix sync, but `u_sync_master_task()` retries every main loop, so the banner needs about 1.25 s of continuous failure. The reverse (matrix sync failing while the RPC works) cannot last: ten failed standard rounds mark the transport disconnected, and from then on the RPC is refused too. While the standard transport is disconnected, the slave half's keys don't work either (its matrix is zeroed). So the answer to Q3 splits the cases: slave half still typing means an RPC-specific failure; slave half dead means the whole link, a hung slave, or power.

A web search earlier today found no matching QMK bug. Suspend-related knobs exist (`USB_SUSPEND_WAKEUP_DELAY`, `NO_SUSPEND_POWER_DOWN`, `NO_USB_STARTUP_CHECK`; see `qmk/qmk_firmware` PR 20205), but nothing points at them.

Code-level causes found today (details in 3.4): the blink stall (D-1) produces the "link dead" signal on the slave with nothing wrong; the per-build sequence counter (D-3) could make a healthy link look stale on the slave; a dead link leaves the slave panel lit indefinitely (D-6). None of them produces the master's banner.

### 3.4 Other defects found today

- **D-1. Bjorn stops blinking after every doze and after any stale period (reproduced).** `oled_render_slave()` reschedules a blink only when `(uint16_t)(bjorn_ticks - bjorn_blink_at) == BJORN_BLINK_OPEN` (`oled_slave_bjorn.c:356-376`), and the doze, falling-asleep, waking and hard-stale paths return before that code (`:270-274`, `:340-354`). If the scheduled tick passes while he is asleep or stale, the difference only comes round again when the 16-bit tick counter wraps: 65,536 render ticks, about 54.6 minutes of panel-on time. Host harness against the real renderer: awake 0-30 s, 6 blinks; idle to 200 s (settle, doze), then a key and 120 s awake, **0 blinks**; separately, 150 s awake 24 blinks, 75 s hard stale, then 120 s fresh, **0 blinks**. A blink was counted when every patch of one of the eyes-shut frames was in the buffer. With Bug A the doze could last hours; when Roel came back, Bjorn woke, followed the typing and breathed when settled, but did not blink: exactly the design's "link died" signal.
- **D-2. The slave writes overlay blocks twice every frame** (measured; 3.2).
- **D-3. The sequence counter advances per build, not per send (by reading, not reproduced).** `u_sync_build()` does `++seq` (`split_state.c:161`) and the master builds twice per main loop: `u_sync_master_task()` (`split_sync.c:75`) and `oled_master_task()` (`oled_master_zones.c:328`), both from `housekeeping_task_user`. Between two heartbeats `L` loops apart, the sent `seq` changes by `2L mod 256`, so when `L` is a multiple of 128 the slave sees the same `seq` and its staleness clock (`split_sync.c:94-100`) keeps ageing. Five such heartbeats in a row make it hard stale while every send succeeds (no banner on the master; no blink on the slave, props dropped). How likely that is depends on main-loop timing; not measured. `tests/scenarios/test_split_state.cpp` `SeqAdvances` currently asserts the per-build behaviour.
- **D-4. A misfire that happened during an outage replays on recovery (by reading).** Soft staleness blanks fingers and flags but not `anomaly` (`oled_slave_bjorn.c:145-148`); when the link returns, the first fresh state has a different count and `oled_render_slave()` starts the misfire animation (`:278-283`) for an old event. The axe code's own rule says a stale indicator must not replay.
- **D-5. The slave's idle clock lags by up to about 5 s.** `bjorn_gather()` uses `min(time since WPM changed, last_input_activity_elapsed())` (`oled_slave_bjorn.c:129-131`). WPM keeps changing while it decays over its 5 s window, so idle starts late, while the master's timeout counts from the last key. `last_input_activity_elapsed()` on the slave already includes the master's keys through the mirrored matrix.
- **D-6. A dead link leaves the slave panel lit (by reading).** The slave applies the last received OLED state every loop (`transactions.c:641-651`). If the link dies while it says "on", the slave stays on with a frozen frame until the link returns or power is removed. A slave whose code has stopped (a crash) also keeps its last frame lit, since the SSD1306 holds its picture on its own.
- **D-7. The driver writes one byte past its buffer for `index >= 1024`** (`oled_driver.c:668`). Keep every index below 1024; host stubs should mirror it with a canary byte.
- **D-8. `bjorn_art.c` uses `NULL` without including `<stddef.h>`.** The firmware gets it indirectly; a host build of it fails without it (add the include to the test stub `progmem.h`, not to the generated file).

### 3.5 Doc staleness (fix in Stage 0, first of all the rules file)

1. `.claude/rules/oled.md` describes the old design: master shows logo, "Kyria rev3" and text; slave shows the spaceship or logos; the slave learns state through `SPLIT_LAYER_STATE_ENABLE`/`SPLIT_MODS_ENABLE`/`SPLIT_LED_STATE_ENABLE`; and line 14 says a keymap `oled_init_user` does nothing (false, `oled_driver.c:295`). This rules file is loaded automatically whenever an OLED file is edited, so it has to be right before anything else changes.
2. `docs/layout.md`: line 73 says "Specified 2026-10-10, not yet in the firmware"; the Open experiments row (line 348) says "specified, not yet built"; line 105 gives the breath as 1.2 s (built: 3.6 s); the Stage 1 table gives the doze breath as 5.4 s, 1.8 in and 3.6 out (code: `BJORN_DOZE_PERIOD` 120 ticks = 6 s, `BJORN_DOZE_INHALE` 30 = 1.5 s in, 4.5 s out); the misfire row says 1200 ms in all with 750 ms at the top (code: 1750 ms, 100/150/200 ms rising, 1000 ms held, 3 x 100 ms down); "Art is 4672 bytes" (now 7,232 bytes of pixel data); the Files table lacks `split_state.c/.h` and `split_sync.c/.h`; the Tuning log is empty although the daily-driver week started 2026-10-10; there is no built / still-to-do list.
3. `oled_slave_bjorn.c` comments: lines 3-6 ("the axe, Caps Word, the alarms and the goat ... arrive in stage 2"), 55-56 ("Nothing draws from these yet"), 118 ("Stage 2 adds layer, mods, locks and flags here").
4. `oled.c` lines 6-8 say `kyria.c`'s `oled_init_kb` "never calls oled_init_user": true of `kyria.c`, but the driver calls it.
5. `tests/README.md` lines 51-54 say the panel test asserts glyphs always drawn and inverted for held mods; `test_panel.c` now asserts the opposite (dark at rest, nothing on the mod row inverts). `--raw` is not documented.
6. `MIGRATION.md`: "Fix while in there" has 3 of 4 done (`oled_init_user` gone, layer names from `MIRYOKU_LAYER_LIST`, blank widths fixed in the text panel), and its `oled_init_user` explanation is wrong; only `RGBLIGHT_LIMIT_VAL` remains. The Progress table has no row for step 9. Nothing records which builds were flashed since step 8 or the stage 1 legibility result. The flash-together rule is only in commit `df5c23b`. The fallback `.uf2` on a GitHub release is still open (the per-half read-backs exist only on the Mac). On the work PC, `C:\src\personal\kyria-handoff` can be deleted (Roel does that).
7. `VENDORED.md` has no section for Bjorn's sprites, although `layout.md` (Attribution) says it records where they came from.
8. `oled_master_zones.c` comments at lines 47-51 ("Unchanged bytes cost nothing") and 124-125 ("The driver has no way to read a byte back") are wrong; Stage 1 rewrites that code anyway.
9. `CLAUDE.md` "Build and flash" doesn't state the flash-together rule; flashing on the Mac is only described in `MIGRATION.md`.
10. Not in the repo: the generator of `bjorn_art.c`, `gen_font.py` (named in `panel_font.h`), the span lists behind `panel_glyphs.c`, and the "state-machine simulator" and "blitter harness" that commit messages and `layout.md` mention.
11. `panel_font.c` still carries `u_sym8`, the symbols of the event strip that `c4c93b9` cut. Unreferenced, dropped by the linker; leave it until the font generator is back.
12. Unbuilt spec items, verified in code: the goat (wander from 20 s, pet at 45 s); Caps Lock on the mascot (crown plus edge frame; Bjorn reacts only to Caps Word today); the default-layer-not-Base defeated pose with the edge frame (the master already draws a rule under the layer name); the "Invert the whole OLED" look (after the goat); raising the WPM tiers once speed settles.

---

## 4. Stages

Every stage follows the same pattern: do the work, run `tests/run_unit.sh` and `tests/run_scenarios.sh new`, build clean, verify in the build (Appendix A), **stop** and report (what changed, test output, size delta, what was and was not checked), commit only when Roel says so, flash both halves when the stage changes firmware, **stop** for Roel's check on the board, update the docs listed, commit the docs. New source files go into `SRC` in the keymap's `rules.mk` (never `keymap.c`); slave-only modules compile only under `OLED_SLAVE_ANIMATION_BJORN`, the way `oled_panel.h` already gates the art. Every stage that adds or changes behaviour also updates `docs/layout.md` (OLED section, "Built and still to do") and, for each flash, the Tuning log.

Flashing procedure, used by every stage that flashes:

1. Build from a clean, committed tree, so the flash log can name the commit.
2. Per half: USB into that half; bootloader by double-tapping the half's reset button, or Miryoku's Boot key on the half that has USB. Not Bootmagic: holding SYS while plugging in clears EEPROM (`bootmagic.c:63-75`).
3. If the build on the board is unknown: `picotool save -a -v ~/kyria-firmware-backup/readback-<left|right>-<date>.uf2` first.
4. `qmk flash -kb splitkb/kyria/rev3 -km rvannoord` (macOS mounts `RPI-RP2`), or `picotool load -v -x <file>.uf2`.
5. Repeat for the other half; the halves can't talk until both run the same build. Then USB back into the usual master half.
6. Keep the flashed file as `~/kyria-firmware-backup/<date>-<shortsha>.uf2` and add a Tuning-log row (date, commit, what changed, why).

### Stage 0: docs tell the truth (no firmware change)

Goal: every document that is loaded or read matches `c4c93b9`, before any OLED file is edited.

1. **Rewrite `.claude/rules/oled.md` first.** Keep the frontmatter; widen `paths` to `keyboards/**/oled*`, `keyboards/**/*logo*`, `keyboards/**/bjorn_*`, `keyboards/**/panel_*`, `keyboards/**/split_*`, `tests/unit/**`, `tools/**`. Content, all verified:
   - The panel: SSD1306 128x64, page format (a byte is a vertical strip of 8 px, bit 0 on top), 1024-byte buffer, 16 dirty blocks of 64 bytes (block = page x 2 + right column), I2C at 400 kHz, roughly 1.7 ms per block, one block sent per main loop.
   - The driver rule that matters most: any byte that changes, even back to its old value within the same frame, marks its block dirty, and a dirty block calls `oled_on()`, which restarts `OLED_TIMEOUT`. Never clear-then-redraw into the driver buffer. (After Stage 1: compose in `u_canvas`, commit once; an unchanged frame must dirty nothing, and the tests check it.)
   - Keep every buffer index below 1024 (the driver writes one byte past the end otherwise).
   - `oled_task_user` runs every 50 ms (20 Hz), must return false (`kyria.c`'s `oled_task_kb` paints its own screen otherwise), and returns early while the panel is off. Rotation: the driver calls `oled_init_user(oled_init_kb())`; `kyria.c` returns 180; this keymap defines no `oled_init_user`.
   - Which half shows what: master (the half with USB, VBUS on GP19) is the zones panel, `oled_master_zones.c` (alternative `oled_master_text.c`); the other half is Bjorn, `oled_slave_bjorn.c` (alternatives spaceship and logos). Picked in `custom_config.h`, enforced by `oled_panel.h`.
   - Split: the custom 5-byte RPC `U_SYNC_STATE` (`split_state.c`, `split_sync.c`), not `SPLIT_LAYER_STATE_ENABLE`/`MODS`/`LED_STATE`; WPM through `SPLIT_WPM_ENABLE`; panel on/off through `SPLIT_OLED_ENABLE` (from `keyboard.json`), applied by the slave every loop; the master's matrix mirrored to the slave. The slave's RPC callback runs on a HIGHPRIO thread holding the shared-memory lock: memcpy only.
   - Both halves are flashed with the same build (the handshake rule above).
   - Edges are latched in the per-loop tasks (`oled_master_task`, `oled_slave_task`, from `housekeeping_task_user`); `oled_render_*` only draws; transient flags must stay set for at least 100 ms.
   - The design rules are in `docs/layout.md`, OLED section; read it before changing behaviour.
   - Run `tests/run_unit.sh` after touching any OLED or split file; `--dump` and `--raw` show frames.
   - Generated files (`bjorn_art.*`, `panel_font.*`, `panel_glyphs.*`, and later the new art files) are never edited by hand.
   - Burn-in: `OLED_TIMEOUT 120000`, `OLED_BRIGHTNESS 128`; the master switches both panels off.
   - `PROGMEM`/`pgm_read_byte` are no-ops on ARM, kept for portability.
2. **`docs/layout.md`**: replace line 73's "not yet in the firmware" with a pointer to a new subsection **"Built and still to do"** in the OLED section:
   - Built: Bjorn's postures, blink, breath, doze and waking (`4465d74`, `cefa650`, `4cae216`, `85ac2bc`, `f4dad75`, `9d066bb`); per-half tasks (`591a32f`); the split sync (`d195f6a`, `df5c23b`); Bjorn reads the state (`162fb40`); the axe (`ef99af4`, `fe95b61`, `c158c8c`); the misfire raise (`7b78012`, `eafad3b`); Caps Word crown and inversion (`43ef4ae`); master zones and the DUNGEON.mode font (`df9231f`); dark at rest, alarm row, trace strip, latched locks (`c4c93b9`); the old art and the text panel still selectable.
   - Still to do: the items in 3.5 #12, and the new work in this plan.
   - Known defects (2026-10-14), each with the stage that fixes it: Bug A, D-1 to D-6. Delete each line in the commit that fixes it.

   Also: the Open experiments row ("partly built, see OLED"); line 105 breath 3.6 s; the doze-breath and misfire timings to match the code (or ask Roel whether the code or the doc is what he wants); "Art is 4672 bytes" to 7,232; Files table rows for `split_state.c/.h` and `split_sync.c/.h`; the stage 1 legibility verdict and Tuning-log rows from Roel's answers (D8).
3. **Comments only**: `oled_slave_bjorn.c` lines 3-6, 55-56, 118; `oled.c` lines 6-8. Rebuild and check the `.uf2` is byte-identical to the baseline (`shasum -a 256`): no version strings are referenced, so comment-only changes should produce the same image. If two builds of the unchanged baseline already differ on the Mac, say so and compare section sizes plus an `arm-none-eabi-objdump -d` diff instead.
4. **`tests/README.md`**: replace lines 51-54 with what `test_panel.c` asserts today (dark at rest; a held finger draws its cell and nothing on the mod row inverts; the alarm row is the only inversion, blinking three times over 600 ms); document `--raw`.
5. **`MIGRATION.md`**: mark the three done "Fix while in there" items with their commits and correct the `oled_init_user` explanation; add a Progress row "9. Archive: open; conditions: a week or two as the daily driver on both machines without the step 0 file, archive tag on GitHub, Roel archives"; record the flash-together rule under step 8; note which builds were flashed (D8).
6. **`CLAUDE.md`** "Build and flash": add "Always flash both halves with the same build; a mismatched pair fails every split transaction", and a line for the Mac (`qmk flash` works there because macOS mounts `RPI-RP2`).
7. **`VENDORED.md`**: add a Bjorn section with what is known now: `github.com/infinition/Bjorn`, MIT, "Copyright (c) 2024 infinition"; frames `IDLE1`, `IDLE3`, `IDLE8`, `IDLE9` from `resources/images/status/IDLE/`, 78x78, box-filtered to 64x64, Floyd-Steinberg dithered per 64-column pass with region locking; the blink, breath, doze, axe, crown and misfire frames are edits of those. Source commit not recorded; GitHub shows `main` unchanged since `61a4758e5e` (2026-05-04), so the art generated on 2026-10-10 almost certainly came from it (inference, confirm in Stage 3).
8. **The last "Fix while in there" item, moved here from the leftovers on purpose**: delete the `#ifdef RGBLIGHT_ENABLE` / `RGBLIGHT_LIMIT_VAL 128` block from the keymap's `config.h` (`RGBLIGHT_ENABLE` is off, so nothing reads it; the board caps RGB matrix brightness at 128 in `keyboard.json`). Verify the `.uf2` is byte-identical to the baseline. This proof only works now: from Stage 2 on the build date is compiled in, and two builds of one commit differ. Then mark the item done in `MIGRATION.md`.

Checkpoint: Roel reads the diffs. Suggested commits: `📝 docs(rules): describe the OLED as it is built`, `📝 docs(layout): say what is built, what is still to do, and what is broken`, `📝 docs(oled): bring the comments up to date`, `📝 docs(tests): match the panel test as it now asserts`, `📝 docs(migration): close the fixes that are done, and keep both halves on one build`, `📝 docs(vendored): record where Bjorn's sprites came from`, `♻️ refactor(kyria): drop RGBLIGHT_LIMIT_VAL, which nothing reads`. No flash: the firmware is byte-identical.

### Stage 1: Bug A, and a fault signal that tells the truth

Goal: an unchanged frame writes no byte on either half; the panels go dark 120 s after the last input whatever is drawn; Bjorn keeps blinking after a doze or a stale link; the slave's staleness clock moves once per delivered state. D-1 and D-3 are in this stage rather than in Stage 2 because once the panels sleep, every idle period ends in a doze, so without the D-1 fix Roel would see the "link dead" signal every time he came back, which would poison the Bug B investigation.

**1.1 Canvas.** New `oled_canvas.h`/`oled_canvas.c` in the keymap folder, added to `SRC` in `rules.mk`:

- `extern uint8_t u_canvas[OLED_MATRIX_SIZE];` with `_Static_assert`s that `OLED_MATRIX_SIZE == 1024` and `OLED_BLOCK_SIZE == 64`.
- `uint16_t u_canvas_commit(void);`: for every byte where `u_canvas[i]` differs from `oled_read_raw(0).current_element[i]`, call `oled_write_raw_byte((char)u_canvas[i], i)`; return the mask of 64-byte blocks written. The driver's dirty bits stay the single source of truth; the mask is for tests now and the budget arbiter later.
- Why a whole-frame canvas rather than per-zone scratch rows: both halves have the same flaw (the slave's base-then-patch blit), so one mechanism fixes both; zones and layers may keep "clear, then draw" because it now happens in RAM; one commit point is where the budget can be measured; 1 KB of RAM against roughly 249 KB of free heap.

**1.2 Master.** In `oled_master_zones.c`, `fill()`, `draw_glyph_1x/2x/16()`, `draw_text_1x_inverse()` and the badge underline write into `u_canvas` instead of calling `oled_write_raw_byte()`; `oled_render_master()` ends with `u_canvas_commit()`. Every page is filled by some zone each frame (status 0, mods 1-2, layer 3-4, alarm 5, trace 6-7), so the canvas is fully defined. Rewrite the comments at lines 47-51 and 124-125.

**1.3 Slave.** In `oled_slave_bjorn.c`, `bjorn_blit()` and `bjorn_blit_base()` write into `u_canvas`; each compose starts by clearing the canvas (the right column is otherwise never written); the body of `oled_render_slave()` becomes `static void bjorn_compose(void)` and `oled_render_slave()` calls it and then `u_canvas_commit()`, so every early return still commits. Leave `oled_master_text.c`, `oled_slave_spaceship.c` and `oled_slave_logo.c` as they are (3.2).

**1.4 Safety net, master only.** A pure predicate in `oled_panel.h`, so it can be host-tested, and a call from `keymap.c` `housekeeping_task_user()` inside the `OLED_ENABLE` block:

```c
// oled_panel.h
static inline bool u_oled_should_sleep(bool on, uint32_t idle_ms) {
    return OLED_TIMEOUT > 0 && on && idle_ms >= OLED_TIMEOUT;
}

// keymap.c, housekeeping_task_user(), master branch
if (u_oled_should_sleep(is_oled_on(), last_input_activity_elapsed())) oled_off();
```

QMK's timeout follows `oled_on()` calls, which any dirty block makes; the input clock does not depend on rendering. It runs every loop rather than in the 20 Hz render, so a stray dirty block can light the panel for one loop at most. Waking still works: `keyboard.c:758` calls `oled_on()` on matrix or encoder activity, and `OLED_TIMEOUT` stays above zero. The slave follows within one forced sync (100 ms). On the master nothing is pending at switch-off, because the panel is static at rest.

**1.5 Blink stall (D-1).** Compute the blink offset as a signed difference (`int16_t`), and when the code finds the schedule more than one blink in the past, reschedule from now (6-10 s, the pair logic unchanged). Also reschedule explicitly when leaving the doze, at the end of the waking lids, and when hard staleness clears, so the length of the gap never matters (a signed 16-bit difference alone breaks after about 27 minutes).

**1.6 Sequence per send (D-3).** `u_sync_build()` stops touching `seq`. `u_sync_master_task()` sets `next.seq = (uint8_t)(last_sent.seq + 1)` just before `transaction_rpc_send()`, and `last_sent` (with its `seq`) only advances on success. The slave is unchanged: it tests for inequality.

**1.7 Tests.**

- `tests/unit/stub/qmk_keyboard.h`: `OLED_MATRIX_SIZE` 1024, `OLED_BLOCK_SIZE` 64, `OLED_TIMEOUT` 120000, `oled_buffer_reader_t` and `oled_read_raw()`, a `fake_dirty` mask, and the fake buffer one byte longer as a canary that must never change. `tests/unit/stub/progmem.h` gains `#include <stddef.h>` (D-8). The write stub mirrors the driver (this is the scratch test that found Bug A, with the canary added):

  ```c
  uint8_t  fake_oled[OLED_MATRIX_SIZE + 1];   // the last byte is a canary for the driver's one-past-the-end write
  uint16_t fake_dirty;

  void oled_write_raw_byte(const char data, uint16_t index) {
      if (index > OLED_MATRIX_SIZE) index = OLED_MATRIX_SIZE;   // the driver's clamp, oled_driver.c:668
      if (fake_oled[index] == (uint8_t)data) return;           // compare as uint8_t: char is signed on the host
      fake_oled[index] = (uint8_t)data;
      fake_dirty |= (uint16_t)(1u << (index / OLED_BLOCK_SIZE));
  }
  // render a state once, advance 50 ms, clear fake_dirty, render the same state again: fake_dirty must be 0
  ```
- `test_panel.c`: `step()` stops memsetting the buffer (it persists, like the driver's); a new check renders a state, advances 50 ms, clears `fake_dirty`, renders again and expects 0, for: at rest Win, at rest Mac, Nav, each finger of each hand, all fingers, Caps Lock with Scroll Lock, Caps Word, a non-base default layer, LINK DOWN after its 600 ms entry, and the trace inside its 1.5 s. Name the blocks from 3.2 in a comment as the regression this guards. Add a canary check.
- New `tests/unit/test_bjorn.c`, compiling `oled_slave_bjorn.c`, `bjorn_art.c` and `oled_canvas.c` with `-DOLED_SLAVE_ANIMATION_BJORN` against stubs for `get_current_wpm()`, `last_input_activity_elapsed()`, the clock and `u_sync_state()`/`u_sync_soft_stale()`/`u_sync_hard_stale()`:
  - Unchanged frame dirties nothing: steady typing at tiers 0, 2, 3 (WPM held past the 800 ms dwell, between blinks), settled with the breath at rest, the breath held at full, dozing between breath edges, Caps Word inverted, axe held.
  - Blinks, detected by all patches of an eyes-shut frame being in the buffer: awake 30 s gives at least 3; idle 170 s (doze), a key, then 120 s awake gives at least 10, the first within 10.5 s of waking; 75 s hard stale, then 120 s fresh gives the same; no blink while hard stale or asleep. Before the fix the second and third give 0 (reproduced on the work PC).
- `test_split_sync.c`: the `transaction_rpc_send()` stub captures the payload; consecutive successful sends carry different `seq` whether 0, 128, 256 or 384 builds happen between them; a failed send does not advance it.
- `tests/scenarios/test_split_state.cpp`: replace `SeqAdvances` with "a build is pure" (two builds with nothing in between are equal), and say why in the commit.
- `test_panel.c`: `u_oled_should_sleep()` is false at 119,999 ms idle, true at 120,000, false when already off.
- `tests/run_unit.sh`: build and run `test_bjorn` too.

**1.8 Build and verify.** Clean build; preprocess `drivers/oled/oled_driver.c` with the build's flags and confirm `OLED_TIMEOUT 120000`, `OLED_BRIGHTNESS 128`, `OLED_UPDATE_INTERVAL 50`, `OLED_UPDATE_PROCESS_LIMIT 1` unchanged; `nm` shows `u_canvas` (0x400, `.bss`) and `oled_read_raw` linked; `split_transaction_table` still 15 entries; size delta recorded; all tests pass.

**Stop:** report; Roel says whether to commit and flash.

**1.9 Flash both halves.** Nothing records which build is on the board now, so read both halves back first (that also preserves the build that showed Bug B).

**Checkpoint (Roel, on the board):** leave it idle for 2.5 minutes: both panels go dark at about 120 s; a key wakes both; Bjorn opens his eyes and blinks within about 10 s; after a doze (75 s or more of idle) and a key he keeps blinking; the master looks the same as before (badge at rest, cells, band, alarm); typing feels the same on both halves. Not checkable by eye: the dirty-block counts (covered by the host tests).

**Docs:** `layout.md` (delete the fixed Known-defect lines; Drawing: compose then commit; Split sync: `seq` advances per delivered state; Stage 1 table: blink rescheduling), `.claude/rules/oled.md` (the compose rule is now true), `tests/README.md` (dirty checks, `test_bjorn`), `CLAUDE.md` (run `tests/run_unit.sh` also after changing `oled_canvas.c`, `oled_slave_bjorn.c`, `bjorn_art.c`), Tuning log row for the flash.

**Commits**, each with its tests: `🐛 fix(oled): compose each frame once, so an unchanged panel can sleep`; `🐛 fix(oled): put both panels to sleep after two idle minutes, whatever they draw`; `🐛 fix(oled): let Bjorn blink again after a doze or a dead link`; `🐛 fix(oled): advance the link's sequence per send, not per build`.

### Stage 2: Bug B, evidence first, then self-healing

Goal: if the link drops again, the keyboard records when and how; the slave recovers by itself where it can; recovery is immediate when the link returns; nothing stale replays.

**2.0** Write Roel's answers to Q1-Q8 into the stage report and, briefly, under Known defects in `layout.md`.

**2.1 Recovery without latched state.**

- Slave: on the hard-stale to fresh transition, set `bjorn_last_anomaly = in.anomaly` without animating (D-4); the blink is rescheduled (1.5).
- Master: already immediate (`u_sync_link_down()` reads the last successful send each call; `render_alarm()` reads it each frame). Keep it that way.
- Tests: `test_split_sync.c`, down then up: `u_sync_link_down()` is false on the first send that lands (tighten the existing check); `test_panel.c`: the alarm row is dark on the first frame after the link returns; `test_bjorn.c`: a 30 s outage during which `anomaly` changed, then fresh: no misfire frame is drawn, a blink comes within 10.5 s, and the crown and axe follow the first fresh state.

**2.2 Diagnostics.** New `split_health.c/.h`, built on both halves.

- Master counters, RAM only: failed sends (total and longest streak); outages (rising edges of `u_sync_link_down()`), longest outage, start and end of the last one as uptime; transport drops (`is_transport_connected()` going false, sampled every loop); suspends and wakes (`suspend_power_down_user()` runs every ~17 ms while suspended, so count the first call and re-arm in `suspend_wakeup_init_user()`); uptime.
- Slave counters, returned in the RPC reply: switch the master to `transaction_rpc_exec(U_SYNC_STATE, sizeof(next), &next, sizeof(reply), &reply)` with a reply of 8 to 10 bytes (limit `RPC_S2M_BUFFER_SIZE` 32). The slave callback checks `s2m_size == sizeof(reply)` and copies a struct the slave's main loop publishes with the same two-slot pattern as `slots`. Contents: slave uptime in seconds, hard-stale episodes and the longest, self-resets since power-on and the last cause (2.3), and a 16-bit build id (a hash of `QMK_BUILDDATE` and `QMK_USERSPACE_VERSION`) so the master can show "build mismatch" when the halves differ but still share a transaction count. Cost about 0.35 ms per send (8 bytes at 43 µs); `GET_RPC_RESP_DATA` already runs on every send with zero bytes, so there is no new failure point.
- Why a panel page and not the console: the build has `NO_DEBUG` and `NO_PRINT`, so the console would need `CONSOLE_ENABLE` (an extra USB interface and print code) and a tool on whichever computer is attached; the page costs a few hundred bytes and works anywhere.
- Stats page on the master (dark at rest is kept: it appears only on demand): hold SYS for 2 s (D4); it stays after SYS is released, until the next key press or for 15 s. In `process_record_user()`, `U_SYS` records the press time and toggles the OS on release only if held for less than 2 s; `oled_master_task()` opens the page once SYS has been down for 2 s. Up to 8 lines of 16 characters in DUNGEON.mode: build id and date, master and slave uptime, failed sends, outages and the longest, last outage (how long ago, how long), transport drops, suspends and wakes, slave stale episodes, resets, build match. No `printf` (`NO_PRINT` is set): use `get_numeric_str()` from `quantum.c` or a small helper. This also embeds the userspace hash and build date in the binary, so a read-back can be identified.
- Optional (Roel): a 7x7 "the link dropped since you last looked" glyph in the status bar's dark right end, cleared when the page opens. It is a prop for something true.
- Optional (D5): keep the last outage across a replug with `eeconfig_update_user_datablock()` and `EECONFIG_USER_DATA_SIZE` in `config.h` (`quantum/eeconfig.h:32-36`, `:154-158`), one write per outage end.
- Tests: counters on the fake clock (a 2 s outage counts once and sets the longest to 2 s; failed sends count; recovery stamps the end); scenario tests: tapping SYS still toggles the OS (S10 unchanged), holding it 2.5 s does not and opens the page.

**2.3 Self-heal on the slave.** A pure decision function in `split_health.c`, called from `housekeeping_task_user()` on the slave, resets the slave with `mcu_reset()` only when all of these hold:

- no new state for at least 5000 ms (`u_sync_age_ms()`);
- the panel has been on continuously for at least 5000 ms. During host sleep the master sends "off" every 100 ms (`quantum.c:521`, `transactions.c:635-651`), so the guard is disarmed and cannot fire;
- at least 10 s since boot, far outside the 500 ms double-tap window (a reset inside it enters the bootloader);
- no key on either half for at least 2 s, because a reset drops the slave half's keys for about a second (0.5 s boot wait, boot, reconnect);
- the backoff allows it: the first reset after 5 s of silence, the second only after 30 s, the third after 300 s (silence counted from the previous boot), and at most 3 per power-on. After that the slave stays as it is: Bjorn frozen without a blink and the master's banner, which is the honest state.

The count and the last cause live in a variable in a `.ram0.*` section guarded by a magic word, the mechanism QMK uses for `magic_location`; check in the map that it lands after `__ram0_noinit__`, and on the bench that it survives `mcu_reset()`. Show it on the stats page through the reply.

Side effect worth having: it ends D-6. After the reset the shared memory reads 0, so the slave switches its panel off and stays dark until the master says "on" again. It does not help with master-side failures, nor with a slave whose code has stopped running.

Host tests: never fires with the panel off, within 10 s of boot, while typing, or while the state is fresh; fires once the conditions hold; the backoff schedule; the cap of 3; the counter survives a simulated reset. Optional on-device test (Roel decides): a local, uncommitted build in which holding a key combination makes the master skip RPC sends; check one restart after 5 s without typing, that typing defers it, and the backoff; then flash the committed build again.

**2.4 Optional: hardware watchdog for a hung slave (D6).** pico-sdk `watchdog_enable(ms, true)` on the slave after boot, `watchdog_update()` from housekeeping; timeout at least 2 s, well above the 100 ms OLED I2C timeout and flash-write stalls. Verify with a deliberate hang in a test build. A watchdog reset inside the 500 ms window after a previous boot would enter the bootloader, so the timeout must exceed that window comfortably.

**2.5 Only with evidence: QMK suspend knobs.** `USB_SUSPEND_WAKEUP_DELAY` only affects the remote-wakeup path (`chibios.c:184-196`); `NO_SUSPEND_POWER_DOWN` would leave panels and RGB on during host sleep; `NO_USB_STARTUP_CHECK` skips the suspend loop entirely, so housekeeping and the RPC keep running during sleep but QMK no longer switches anything off on suspend. None is recommended without a matching symptom.

**2.6 Build and verify.** `split_transaction_table` still 180 bytes (15 entries) unless a transaction was added; the reply fits; `strings` on a `.bin` read-back or the ELF shows the userspace hash; the no-init variable sits after `__ram0_noinit__` in the map; size delta; all tests. From this stage on the build date is compiled in, so two builds of one commit are no longer byte-identical; a "nothing changed" check now compares `arm-none-eabi-objdump -d` output and section sizes instead.

**Stop**, then flash both. **Checkpoint:** Roel opens the stats page daily for a few days, always before replugging anything; then decides D5, D6 and whether the self-heal stays on.

**Docs:** `layout.md` (Split sync: reply, stats page, self-heal; Extra keys: SYS hold; Known defects), `.claude/rules/oled.md`, `tests/README.md`, `CLAUDE.md` (the SYS hold), Tuning log.

**Commits:** `🐛 fix(oled): do not replay a misfire the link missed`; `✨ feat(oled): count link outages, and show them while SYS is held`; `✨ feat(oled): let the other half restart itself when the link stays dead`.

### Stage 3: survey all 17 of Bjorn's sprite sets

Goal: know what each set shows at 64x64, and agree with Roel where each one goes. No firmware change in this stage.

**3.1 Generator.** Look for the old one first (D9): `~/kyria-firmware-backup`, the home folder, `grep -iE 'bjorn|dither|gen_font' ~/.zsh_history`. If found: review it against the attribution rule in `CLAUDE.md`, commit it under `tools/`, and check that it regenerates `bjorn_art.c` byte for byte. If not found: leave `bjorn_art.c` frozen as it is (its blink, breath, doze, axe, crown and misfire frames are hand edits that probably can't be reproduced exactly) and generate all new art into new files. Write `tools/bjorn_sprites.py`, Python 3 standard library only:

- BMP reader: the 78x78 frames are uncompressed 24-bit (18,462 bytes = 54-byte header + 78 rows of 236 bytes, bottom-up); each set's base image is a 28x28 status icon (2,406 bytes, 24-bit; `IDLE.bmp` is 174 bytes, 1-bit).
- Greyscale, area-weighted box filter 78 to 64, Floyd-Steinberg in separate 64-column passes, region-locked across the frames of a set (dither frame 0, keep its bits wherever the source grey did not change), output in page format as a base plus block-aligned 64-byte patches, in the `bjorn_patch_t` layout.
- PNG writer (`zlib` + `struct`) for contact sheets: one overview of all 106 images at the 64x64 dithered target, and one sheet per set showing each frame as the 78x78 source grey, the 64x64 grey and the 64x64 dithered result, plus the icon, labelled with a tiny built-in digit font. Render lit pixels light on black at 3x or 4x, so the sheet looks like the panel.
- Downloaded files are data: keep them in `~/bjorn-src/`, keep scripts outside it, run Python with `-I`.

**3.2 Fetch (needs D3).** For example `git clone --filter=blob:none --no-checkout https://github.com/infinition/Bjorn ~/bjorn-src`, then check out only `resources/images/status` and `resources/images/static` at `61a4758e5e`. Tell Roel the size first. The PSDs only if he agrees separately.

**3.3 Look and classify.** Render the sheets, look at every one, and fill a table per set: what it shows, which frames are usable, motion or pose, mood, legibility at 64x64 (good, doubtful, illegible), proposed slot, rarity, blocks per step, bytes. Also look for: frames showing the goat (layout.md says it is already in the idle animation) and whether it can be cut out cleanly; candidate poses for the unbuilt items (glance up, petting, yawn, the defeated helmet-off pose); `static/bjorn1.bmp` (18,462 bytes, so probably another 78x78 Bjorn) and `static/frise.bmp`.

Tentative mapping, to confirm or replace after looking (all idle content unless a frame fits an event; see the rule below):

| Set | 78x78 frames | Tentative slot | Kind |
|-----|------|------|------|
| IDLE | 11 (1, 3, 8, 9 already used for the tiers) | micro-idles in the 8-20 s window: glances toward his world, weight shifts; maybe the goat's source | pose cuts |
| NetworkScanner | 8 | lookout: scanning the horizon | short motion |
| NmapVulnScanner | 8 | inspecting something close up | short motion |
| SSHBruteforce | 13 | chore: chopping wood | motion |
| SMBBruteforce | 13 | chore: sharpening the axe | motion |
| FTPBruteforce, RDPBruteforce, TelnetBruteforce | 1 each | single chore poses, cut into the chores above | pose |
| SQLBruteforce | 0 (icon only) | its 28x28 icon as a prop or thought bubble | icon |
| StealDataSQL | 13 (no frame 4) | counting treasure | motion, rare |
| StealFilesSSH | 13 | hauling a sack of loot | motion, rare |
| StealFilesFTP, StealFilesSMB, StealFilesTelnet | 1 each | "found something" poses | pose |
| StealFilesRDP | 2 | a two-pose loot gesture | pose pair |
| LogStandalone, LogStandalone2 | 1 each | reading a scroll, writing in it (upstream, `LogStandalone` is a template action that only writes log lines) | pose |

The 17 icons can serve as thought bubbles over his head during the matching vignette, which also gives `SQLBruteforce` (icon only) its place. Rule check: these sets are idle (nothing wrong, nothing true). An exception needs a justification against the rule: for example a bowed, helmet-off frame could become the defeated pose for "default layer is not Base", which is an expression for something wrong, as the spec already says.

Budget and size: a full 64x64 cut is 8 blocks, the whole idle budget of a step, so cuts run only when nothing else initiates. Flash is not a constraint: 89 frames x 512 bytes is 45.6 KB at most against a 2 MB region with about 60 KB used, and patches are smaller. Measure the firmware size before and after Stage 8.

If a set is illegible at 64x64: crop to the upper body or the prop, exaggerate the silhouette by hand, keep only the distinctive frames, or use only its icon. "All 17 somehow" still holds.

**Checkpoint:** Roel looks at the sheets and approves or edits the mapping. **Docs:** `VENDORED.md` (pinned commit, files taken, processing, where the tools are), `layout.md` (a "Sprite sets" subsection with the approved mapping). **Commits:** `✨ feat(tools): generate Bjorn's art from his own sprites`; `📝 docs(layout): give each of Bjorn's seventeen sprite sets a place`.

### Stage 4: director, layered compositor, preview harness (no new art)

Goal: the machinery every later stage uses, proved on the host, with the board looking as it does today.

**4.1 One idle clock for both halves.** Define `SPLIT_ACTIVITY_ENABLE` in `custom_config.h` next to `SPLIT_WPM_ENABLE`. The slave then gets the master's activity timestamps (`transactions.c:857-884`, `keyboard.c:198-203`), so `last_input_activity_elapsed()` is the same clock on both halves: the one the master's safety net uses. The slave knows when the panels will go off, and the WPM lag (D-5) goes: idle becomes `last_input_activity_elapsed()`, WPM is used only for the tier. Cost: one more transaction type, sent on change or every 100 ms. `split_transaction_table` grows from 180 to 192 bytes (16 entries); flash both halves.

**4.2 Compositor.** Bjorn's column keeps its patches, composed into `u_canvas` as in Stage 1. New `oled_slave_world.c/.h` composes the right column (x 64-127) from layers in order: floor rule, pile, goat, particles, emotes. Recompute the whole column in RAM every tick (512 bytes at 20 Hz is negligible) and let the commit decide what reaches the panel. Sprites are pre-dithered 1-bit with an optional mask, blitted at integer positions with clipping. Reserved pixels: x = 0, x = 127 and y = 0 stay dark in every layer, because the Caps Lock edge frame needs them (Stage 9); y = 63 is the floor rule. The one exception is Caps Word's inversion of his tile, which lights x = 0 and y = 0 on the left; Stage 9 settles how the frame looks there.

**4.3 Director.** New `oled_slave_director.c/.h`:

- A repertoire table: id, actor (Bjorn, goat, world), rarity (common, uncommon, rare), idle window, weight, cooldown, eligibility (for example "goat present"), start function.
- Weighted pick from its own LCG stream (separate from the blink's), seeded at the start of each idle period from the timer; never the same entry twice in a row; per-entry cooldowns; a rare entry at most once per idle period; at most one actor initiates per step; nothing starts while typing (idle under 8 s).
- Arbiter: each actor reports the blocks it would change this tick and its priority (fault > alarm > one-shot reaction > Bjorn pose > goat locomotion > ambient). Grant in priority order while the total stays within 8 blocks; while typing, world actors together get at most 2 blocks (Bjorn's own reactions are not ambient and are not held to that 2). A denied actor keeps its frame and its timers this tick (deferred, not dropped).
- Timeline (proposal; every number goes to the Tuning log):

| Idle | What may happen |
|------|-----|
| 0-8 s | typing postures only; nothing starts; airborne particles land |
| 8 s | settle to rest (as built) |
| 8-20 s | Bjorn micro-idles, one every 5-9 s |
| 12 s | weather may begin (first spawn 12-18 s) |
| 20 s | the goat may come in (a chance every few seconds, not guaranteed) |
| 20-75 s | vignettes, goat grazing and wandering, weather |
| 45 s on | pet, if the goat is within reach |
| 68-72 s | a yawn, once |
| 75 s | doze (as built), z's, goat curls up |
| 110 s | last start: nothing new, no new spawns, in-flight things finish |
| 119.5 s | freeze: nothing changes, so no block is pending at switch-off |
| 120 s | panels off (the master's timeout and safety net) |

- Animation principles for every later stage: anticipation, action, hold, at 1-3 px; secondary motion only on soft masses (beard, goat tail); easing in hold durations; silhouettes first, judged at 60 cm, tilted, at brightness 128; randomness with cooldowns; never freeze mid-stride; cuts for identity, motion for position.

**4.4 Preview harness.** `tests/preview/preview.c` and `tests/preview/run_preview.sh` compile the real slave renderer, world, director and art against the unit stubs and run scripted scenarios at 20 Hz with a fixed seed (for example: type 5 s at 60 WPM, then idle 125 s), writing `frames.bin` (1024 bytes per tick) and `events.csv` (tick, actor, event, committed block mask) to a git-ignored `tests/preview/out/`. `tools/preview.py` (standard library) turns them into `strip.png` (every frame that changed, with its time) and an animated GIF at real timing (merge identical frames into longer delays; GIF needs a small LZW encoder, about 60 lines). The same tool can render `test_panel --raw` output.

**4.5 Tests** (`tests/unit/test_director.c`, fake clock, injected test actors): no back-to-back repeat; cooldowns; a rare entry once per idle period; nothing starts while typing; one initiator per step; the committed mask never exceeds 8 blocks, and world blocks never exceed 2 while typing; nothing starts after the last-start time; zero committed blocks in the freeze window; same seed, same frames. The preview harness switches its panel off at the moment `u_oled_should_sleep()` says so (the same predicate the master uses) and asserts that nothing was mid-motion and no block was pending at that moment, and that rendering stops while off.

**Stop:** show the 125 s preview GIF, which should look like today's board (settle, breathing, blinks, doze, stillness before switch-off). Then flash both. **Checkpoint:** Roel compares the GIF with the board. **Docs:** `layout.md` (Director: timeline, rarity, budget enforcement, one clock; Files), `tests/README.md`, `.claude/rules/oled.md` (layers and reserved pixels), `.gitignore` (preview output). **Commits:** `♻️ refactor(oled): compose the other half in layers`; `✨ feat(oled): give idle a director, and one clock for both halves`; `✅ test: preview the idle show on the host`.

### Stage 5: the goat

Files: new `oled_slave_goat.c/.h`, generated `goat_art.c/.h`, `tools/sprites/goat_*.txt`, `tools/sprite_art.py`; hooks in `oled_slave_world.c` (the goat layer) and `oled_slave_director.c` (repertoire entries); `rules.mk`.

- Art: cut from the frames found in Stage 3 (or from `Alva.psd` if D3 allows), reduced like Bjorn, and hand-made variants kept as text grids in `tools/sprites/`, generated by `tools/sprite_art.py` into `goat_art.c/.h` (with infinition's notice, since it is derived art). Poses: stand, walk A and B, graze (head down) with jaw up and down, look up, tail flick (two states), ear twitch (two states), hop (crouch, air, landing squash), curled (for Stage 7).
- Behaviour in `oled_slave_goat.c`: absent, enter, graze, look up at Bjorn, wander, rarely hop, leave or stay. Walk: a two-frame walk cut with a 1 px bob per step. Grazing: jaw toggles at irregular holds around 400 ms. Tail on a 1.9 s clock, ear on 4.3 s (from `layout.md`). Hop: crouch 1 px and hold, up 3 px over two ticks, hang, land with a 1 px squash. It enters clipped so column 127 stays dark. On a key: finish the current step and stand, never mid-stride; when the link goes hard stale, the same, then nothing moves.
- Pet at 45 s, only if the goat's head is within reach (about x 64-70); Bjorn's arm stays at x 63 or less; the pose comes from Stage 3, or the pet moves to Stage 7 if there is no art yet.
- Tests: enters only after 20 s idle; never moves while typing once its step is done; never ends a step mid-stride; never draws x = 127 or y = 0; budget; pet needs reach.
- Verify: size delta (`size -A`), the goat's art symbols in the ELF (`nm -S`), all tests.
- **Stop** with the preview GIF; flash; **checkpoint** on the board, at 60 cm, tilted. Docs: `layout.md`, `VENDORED.md` (the goat's source). Commit: `✨ feat(oled): bring the goat`.

### Stage 6: weather, by season

Files: new `oled_slave_weather.c/.h`, generated `world_art.c/.h`, `tools/sprites/*.txt`; the particle and pile layers in `oled_slave_world.c`; director entries; `rules.mk`.

- World column only: at most 3 airborne, a spawn every 4-9 s (uniform random), from 12 s idle; particles appear at row 1 or lower, never on row 0; fall diagonally with a ±1 px sway on a slow clock; a three-frame tumble cut every few pixels; land on the floor (resting on row 62 above the rule) and stamp into the pile, capped at 3 px; the pile clears when the panel wakes from sleep (it was dark, so nobody sees it go). On a key: no new spawns, and airborne ones fall straight and fast until they land, never freezing mid-air.
- Season from the build date: parse the month from `__DATE__` once ("Oct 14 2026"), with a test override. September to November leaves; December to February snow (slower, vertical with sway, white pile); March to May petals; June to August fireflies, at most 2, drifting slowly, each lit for 1-3 s and dark for 2-6 s at random, never in step, so they can't read as a signal. A rebuild every few weeks keeps the season right; from now on builds are date-dependent, so byte-identical comparisons only hold within one day.
- Rain proper is skipped: it needs too many particles for the budget.
- Small icons (leaf tumble frames, snowflake, petal, firefly, z, Z, heart, sparkle, puff) are hand-drawn in DUNGEON.mode's weight language, as text grids in `tools/sprites/`, generated into `world_art.c/.h`. They are new drawings, not taken from the pack (which is not in the repo; re-fetching it needs Roel's permission and a `VENDORED.md` line).
- Tests: at most 3 airborne; spawn intervals; landing joins the pile; no particle still airborne 2 s after typing starts; season by month; reserved pixels never drawn; budget.
- Verify: size delta; `strings` on the ELF shows the expected `__DATE__`; all tests.
- **Stop** with GIFs for each season (override the month in the harness); flash; **checkpoint**. Docs: `layout.md` (weather, seasons). Commit: `✨ feat(oled): let the season fall in his world`.

### Stage 7: doze extras and the heart

Files: `oled_slave_bjorn.c` (the yawn as a pose sequence), `oled_slave_world.c` (z's and heart as emotes), `oled_slave_goat.c` (curling up), `oled_slave_director.c` (yawn and pet entries), art via `tools/sprites/`.

- Yawn once at 68-72 s, before the doze at 75 s; art from Stage 3 or a hand edit. If it doesn't read as a yawn at 60 cm, Roel decides whether to drop it.
- Z's from doze start: one about every 3 s, at most 2 on screen, sizes z, z, Z, each living about 2.4 s, drifting diagonally up from his head (they may cross into the world column; that is just compositing now). Ambient priority. They stop at the freeze time.
- If the goat is present at doze time it walks to his side and curls up (a cut to the curled sprite).
- A small heart above the pair for about 1.2 s when the pet completes, once.
- Tests: z count and lifetime, nothing after the freeze time, the heart once per pet, the yawn once per idle period and never after 75 s. Verify: size delta, all tests. **Stop** with the GIF; flash; **checkpoint**. Docs: `layout.md` (the idle table and the doze), Tuning log. Commits: `✨ feat(oled): yawn before the doze, and let the z's drift`; `✨ feat(oled): a heart when he pets the goat`.

### Stage 8: every remaining sprite set gets its moment

Files: generated `bjorn_vignettes.c/.h` (from `tools/bjorn_sprites.py`), playback in `oled_slave_bjorn.c`, entries in `oled_slave_director.c`, `rules.mk`, `VENDORED.md` (which frames are now used).

- Generate the approved mapping into `bjorn_vignettes.c/.h`, add repertoire entries (weights, rarity, cooldowns, duration, hold times of at least 300 ms per cut, thought-bubble icons).
- A test enumerates the repertoire's source tags against the 17 set names, so "all 17" is checked rather than remembered.
- Measure flash before and after (`size -A`, map file); record it.
- **Stop** with one GIF per vignette; flash; **checkpoint**: Roel watches each one on the board and can veto any. Commit: `✨ feat(oled): give each of Bjorn's sprite sets a moment`.

### Stage 9: the remaining spec items, and the leaf on the helmet

Files: `oled_slave_bjorn.c` (Caps Lock crown and frame, the defeated pose, the glance up), `oled_slave_weather.c` and `oled_slave_world.c` (the gag's particle crossing into his column), art via `tools/`.

- **Caps Lock**: crown plus a 1 px edge frame (y = 0, y = 63, x = 0, x = 127) on Bjorn's panel; `U_FLAG_CAPS_LOCK` is already on the wire. Decide with Roel: y = 63 is the floor rule, so the frame replaces the dotted floor while it is shown; and with Caps Word also on, the left tile is inverted, so the frame needs a defined look there (drawn after the inversion, or XORed).
- **Default layer is not Base**: helmet off, head bowed, axe at his feet, plus the edge frame (an expression for something wrong); from `in.base_layer != U_BASE`; art from Stage 3 or a hand edit; it overrides the tier postures.
- **Leaf on the helmet**: rare, at most once per idle period, about 20%, only in the rest pose: a particle drifts across x = 63 into his column, lands on the helmet, he looks up (glance-up pose), puffs (a 2-3 px puff), and it flutters off into the world column. It touches his blocks, so it runs only when nothing else initiates. With Caps Word inverting his tile, compose the particle before the tile mask so it inverts with him. Season variants (snowflake, petal) follow the weather.
- Tests: the frame lights exactly its pixels and nothing else ever does; the defeated pose follows the default layer; the gag at most once per idle period. Verify: size delta, all tests. **Stop** with GIFs; flash; **checkpoint**. Docs: `layout.md` (move the Caps Lock and default-layer rows from "still to do" to "built"; record the floor-row and inversion decisions), `.claude/rules/oled.md` (reserved pixels). Commits: `✨ feat(oled): frame his panel for Caps Lock`; `✨ feat(oled): unhelm him when the base layer is not Base`; `✨ feat(oled): a leaf on his helmet, once in a while`.

### Stage 10: tuning, and the inverted look

- Tuning from Roel's notes: raise the WPM tiers once his speed has settled (ask for it), adjust the timeline numbers; one value per flash, each logged in the Tuning log with a verdict after a week.
- "Invert the whole OLED" experiment: a compile-time option in `custom_config.h`, default off, calling `oled_invert(true)` on both halves (hardware inversion). Before Roel decides, write down what it breaks per `layout.md` row 349: dark at rest, the master's ladder (full-panel inversion is its top rung), the Caps Word tile inversion turning around, and panel wear (77% lit against 24%). Build it, let him look, and only keep it if he says so.
- Checkpoint: Roel's verdicts go into the Tuning log and the Open experiments row. Commits per change, for example `✨ feat(oled): raise the WPM tiers now that speed has settled`; the inversion only as `✨ feat(oled): invert both panels` if he keeps it.

### Stage 11: leftovers from the migration

- `RGBLIGHT_LIMIT_VAL` was removed in Stage 0 (item 8), where a byte-identical build could still prove it changed nothing.
- Step 9 (archive `rvannoord/miryoku_qmk` and `rvannoord/qmk_firmware`): remind Roel of the conditions; he does it. Check the archive tag exists on GitHub first.
- Fallback `.uf2` on a GitHub release on the archive tag: publishing is public, so only with Roel's go-ahead, through `gh` if it is installed and he agrees, otherwise he does it in the browser. Candidates: the read-backs in `~/kyria-firmware-backup`.
- On the work PC Roel deletes `C:\src\personal\kyria-handoff`.
- When everything in it is done, `MIGRATION.md` goes (Roel's call). Commit for the progress updates: `📝 docs(migration): record the archive and the fallback release`.

---

## 5. Risks and open questions for Roel

1. **Where this plan lives** (D1): decided, `docs/oled-v2.md`.
2. **Old generators missing.** If the Bjorn generator is gone, `bjorn_art.c` can't be regenerated byte for byte; it stays frozen and new art goes into new files. The same applies to `panel_font.c` and `panel_glyphs.c`.
3. **Legibility at 64x64.** Some sets may not survive the reduction (Stage 3 lists the fallbacks). The goat may only exist in a few source poses, so most goat motion will be hand-made, at roughly 20 px wide.
4. **Budget.** Flash is not a constraint (2 MB region, about 60 KB used, art at most about 50 KB more). RAM isn't either. The real budget is I2C time (about 1.7 ms a block, one block per loop on each half) and attention, which the block budget encodes.
5. **Idle past 120 s** (D7). Default no; the show is built to end at 119.5 s.
6. **Self-reset costs** about a second of slave keys (hence "not while typing"), and a reset within 500 ms of a boot enters the bootloader (hence 10 s minimum uptime and the backoff). It can hide a real fault, which is why the counters are shown.
7. **Hardware watchdog** (optional): wrong timeouts cause resets during long blocking calls.
8. **`SPLIT_ACTIVITY_ENABLE`** adds a transaction type and a little traffic; both halves must be flashed together anyway.
9. **SYS semantics change** in Stage 2 (toggle on release of a short press). Bootmagic is unaffected (a boot-time scan).
10. **Seasons by build date** go wrong without a rebuild for months, and make builds date-dependent.
11. **The Caps Lock frame** conflicts with the floor rule on y = 63 and with the Caps Word tile inversion; every layer must keep x = 0, x = 127 and y = 0 dark.
12. **Fireflies** must not read as a signal.
13. **Toolchains differ**: the Mac's gcc 8 against the work PC's 15.2, so sizes and warnings differ; build, measure and flash on the Mac.
14. **What host tests can't see**: I2C timing, the real transport, how the panel looks. On-device checks are by eye.
15. **Not verified**: which build is on the board now; the real likelihood of D-3; whether a `.ram0.*` variable survives `NVIC_SystemReset()` on hardware (QMK relies on it for the reset button); the Mac's Python version and whether `arm-none-eabi-size`/`nm`/`gh` are installed; that `last_input_activity_elapsed()` on the slave equals the master's after 4.1 (by reading only); the I2C time per block (computed, not measured).

---

## Appendix A: verification recipes (Mac)

Paths: build products in `~/qmk_firmware/.build/`, objects and flags in `~/qmk_firmware/.build/obj_splitkb_kyria_rev3_rvannoord/`; `qmk compile` also copies the `.uf2` into `~/qmk_userspace/` (ignored by git).

```sh
# Defines as the build sees them (cflags.txt carries the -D, -I and -include flags)
cd ~/qmk_firmware
arm-none-eabi-gcc $(cat .build/obj_splitkb_kyria_rev3_rvannoord/cflags.txt) -E -dM drivers/oled/oled_driver.c \
  | grep -E ' (OLED_TIMEOUT|OLED_BRIGHTNESS|OLED_UPDATE_INTERVAL|OLED_UPDATE_PROCESS_LIMIT|OLED_BLOCK_SIZE|OLED_MATRIX_SIZE) '
tr ' ' '\n' < .build/obj_splitkb_kyria_rev3_rvannoord/cflags.txt | grep -E '^-D' | sort    # the -D list
grep -n 'SPLIT_' .build/obj_splitkb_kyria_rev3_rvannoord/src/info_config.h

# Sizes and symbols
arm-none-eabi-size -A .build/splitkb_kyria_rev3_rvannoord.elf
arm-none-eabi-nm -S --size-sort .build/splitkb_kyria_rev3_rvannoord.elf | grep -E 'u_canvas|oled_buffer|bjorn_|split_transaction_table'
# NUM_TOTAL_TRANSACTIONS = size of split_transaction_table / 12 (180 bytes = 15 today)
grep -n -A6 '^\.ram0 ' .build/splitkb_kyria_rev3_rvannoord.map                              # no-init placement

# Same firmware?
shasum -a 256 ~/qmk_userspace/splitkb_kyria_rev3_rvannoord.uf2

# Read back a half (bootloader mode), as .uf2 for restoring or .bin for strings
picotool save -a -v ~/kyria-firmware-backup/readback-left-$(date +%F).uf2
picotool save -a -v /tmp/readback-left.bin
# from Stage 2 on the build date and userspace hash are in the image:
strings -a /tmp/readback-left.bin | grep -E '20[0-9]{2}-[0-9]{2}-[0-9]{2}'
```

If the preprocessing command complains about a missing include, compare with `compiler.txt` and `obj.txt` in the same folder; the work-PC session preprocessed the same way.

## Appendix B: Bjorn source inventory

`github.com/infinition/Bjorn`, commit `61a4758e5ee3d37d43c10eb690340626139912c0` (2026-05-04, the head of `main` on 2026-10-14), MIT, "Copyright (c) 2024 infinition". Listed through the GitHub API; nothing downloaded.

`resources/images/status/`: 17 folders, 106 BMPs. Each folder has a 28x28 status icon named after the set, and most have numbered 78x78 frames:

| Set | Numbered 78x78 frames |
|-----|------|
| FTPBruteforce | 1 |
| IDLE | 11 (`IDLE1`-`IDLE11`; the icon `IDLE.bmp` is 1-bit) |
| LogStandalone | 1 |
| LogStandalone2 | 1 (`LogStandalone22.bmp`) |
| NetworkScanner | 8 |
| NmapVulnScanner | 8 |
| RDPBruteforce | 1 |
| SMBBruteforce | 13 |
| SQLBruteforce | 0, icon only |
| SSHBruteforce | 13 |
| StealDataSQL | 13 (1-3, 5-14; no 4) |
| StealFilesFTP | 1 |
| StealFilesRDP | 2 |
| StealFilesSMB | 1 |
| StealFilesSSH | 13 |
| StealFilesTelnet | 1 |
| TelnetBruteforce | 1 |

89 frames plus 17 icons. `resources/images/static/`: 23 BMPs and one PNG (icons such as `attack`, `zombie`, `wifi`, `usb`, `gold`, `money`; `frise.bmp`; `bjorn1.bmp` at 18,462 bytes, probably another 78x78 frame). `resources/images/Bjorn.psd` (15.8 MB) and `Alva.psd` (5.4 MB). The upstream README describes the scanners and brute-force, file-stealing and SQL-dumping actions in network terms; `actions/log_standalone.py` is a template action that only writes log lines.

## Appendix C: numbers

- Render tick 50 ms (20 Hz); one dirty block sent per main loop; about 1.7 ms of I2C per block (computed at 400 kHz).
- Split RPC: payload 5 bytes, about 43 µs per byte on the wire at 230,400 baud; heartbeat 250 ms, minimum interval 20 ms; soft stale 600 ms, hard stale 1250 ms (`split_sync.h:22-23`); failed serial transfer up to 20 ms.
- Bjorn's clocks. Counted in render ticks (they pause while the panel sleeps): blink 6-10 s with about 1 in 5 doubled 400 ms apart; breath 3.6 s; doze breath 6 s (1.5 s in); axe dwell 800 ms; misfire 1.75 s. Wall clock: settle 8 s; doze 75 s; tier dwell 800 ms.
- Firmware (work-PC build of `c4c93b9`): about 59.7 KB of flash, 9.5 KB `.bss`, 249 KB heap free; Bjorn's pixel data 7,232 bytes; `u_font8` 760 bytes; `keymaps` 1,120 bytes.
