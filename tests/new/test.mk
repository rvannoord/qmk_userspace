# Scenario tests against current QMK, staged into qmk_firmware/tests/ by tests/run_scenarios.sh.
# rvannoord_keymap.c is the keymap's keymap.c under another name: the test framework has its own keymap.c.

OS_DETECTION_ENABLE = yes

SRC += tap_hold.c host_os.c rvannoord_keymap.c chordal_hold_layout.c
