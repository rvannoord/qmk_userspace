# Scenario tests against current QMK, staged into qmk_firmware/tests/ by tests/run_scenarios.sh.
# rvannoord_keymap.c is the keymap's keymap.c under another name: the test framework has its own keymap.c.

OS_DETECTION_ENABLE = yes
CAPS_WORD_ENABLE = yes      # as the firmware has it, so split_state.c builds the same way

SRC += tap_hold.c host_os.c split_state.c split_sync.c split_health.c rvannoord_keymap.c chordal_hold_layout.c oled_stub.c
