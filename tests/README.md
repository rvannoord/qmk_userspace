# Scenario tests

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
