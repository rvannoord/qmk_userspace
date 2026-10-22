# Scenario tests against current QMK, staged into qmk_firmware/tests/ by tests/run_scenarios.sh.
# rvannoord_keymap.c is the keymap's keymap.c under another name: the test framework has its own keymap.c.

OS_DETECTION_ENABLE = yes
CAPS_WORD_ENABLE = yes      # as the firmware has it, so split_state.c builds the same way

SRC += tap_hold.c host_os.c split_state.c split_sync.c split_health.c rvannoord_keymap.c chordal_hold_layout.c oled_stub.c map_legend.c
# The Layer map page, drawn from every Miryoku layer by test_map_legend.cpp, at the panel's size.
SRC += oled_layer_page.c oled_draw.c art/panel_font.c art/drashna_icons.c
OPT_DEFS += -DOLED_DISPLAY_WIDTH=128 -DOLED_DISPLAY_HEIGHT=64 -DOLED_MATRIX_SIZE=1024
