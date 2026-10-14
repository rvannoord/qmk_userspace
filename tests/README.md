# Tests

Two mechanisms, because they cover different things:

| Runner | What | Needs |
|--------|------|-------|
| `tests/run_scenarios.sh` | tap-hold timing and host OS, as scenarios through stock QMK's tapping code | a `qmk_firmware` tree |
| `tests/run_unit.sh` | the split transport's master half and the panel's drawing | just `cc` |

## Scenario tests

QMK unit tests (GoogleTest) for the tap-hold timing and the host OS handling. They run the real `tap_hold.c`, `host_os.c` and `keymap.c` against stock QMK's tapping code.

```bash
tests/run_scenarios.sh new   # current QMK at ~/qmk_firmware with this userspace's keymap code
tests/run_scenarios.sh old   # the old fork at ~/miryoku_qmk_old (tag archive/miryoku-with-latest-qmk)
```

A second argument picks another QMK tree. QMK only finds tests inside its own `tests/` folder, so the script copies the files to `<tree>/tests/rvannoord_hrm/` and deletes them afterwards. Run it from WSL (or anywhere QMK builds).

| Folder | What |
|--------|------|
| `scenarios/` | `scenario.hpp` (keys, clock, report recorder), `test_home_row_mods.cpp` (scenarios 1–9 and the crossover boundaries, both trees), `test_host_os.cpp` (scenario 10, new only) |
| `new/` | `config.h` and `test.mk` matching the firmware, and a placeholder `chordal_hold_layout` the test build needs to link |
| `old/` | `config.h` with the old `BILATERAL_COMBINATIONS*` settings, and `test.mk` |

Expectations are the new setup's intended behaviour, written as what the host would show: `st`, `U` (Shift+u), `<Ctrl+t>`, `<LEFT>`. They encode the current values (`FLOW_TAP_TERM 160`, crossover 80 ms, 120 ms for GUI), so a deliberate tuning change updates the matching test in the same commit.

Results on 2026-10-09 (MIGRATION.md step 7): new 13/13. Old 10/12; the two failures are the known differences, S2 (same-hand chord after 200 ms: old `st`, new Ctrl+T) and S4 (a mod-tap settled as a tap stays down while held; the old code sent a single tap).

The old tree needs `appdirs`, which the installed QMK CLI lacks, so `old` runs `make` inside a throwaway `uv` environment with that tree's `requirements.txt`.

# Unit tests

```bash
tests/run_unit.sh          # build and run both
tests/run_unit.sh --dump   # print the panel's frames as text instead of checking them
tests/run_unit.sh --raw    # the raw 1024-byte buffers, one per frame, for a renderer
```

Plain C against stub headers in `unit/stub/`, with `unit.h` as the entire framework. They
exist for the two things the scenario tests cannot reach:

| File | What |
|------|------|
| `unit/test_split_sync.c` | the master send path. The scenario tests build `split_sync.c`'s non-`SPLIT_KEYBOARD` branch, where every function is a stub, so nothing there covers it — and a regression is silent and total: no sends at all, which on the keyboard looks like the other half's mascot going inert for no reason. Boot, the heartbeat, the hard window and recovery, on a clock that is a variable |
| `unit/test_panel.c` | the master panel's zones. Compiles the real `oled_master_zones.c` against a buffer laid out like the driver's, then measures what was drawn: that the panel is dark at rest below the badge, that a held finger draws its cell while an idle one draws nothing, which side the handedness bar takes, that each zone's content is centred, and that the alarm row is the only thing that inverts. `--dump` prints the frames and `--raw` the buffers, which is how the zones get looked at without a flash |

`unit/stub/manna-harbour_miryoku.h` copies Miryoku's layer list rather than including the
real header, which would drag in all of QMK. The firmware build still uses the real one.

The panel test asserts the mod row as built: **dark at rest**, a cell drawing its glyph
only while that finger holds something, and nothing on that row inverting. Inversion is
reserved for the alarm row, which is what keeps a routine held modifier from spending the
alarm's own register. If that changes, the test and `docs/layout.md` change with it.
