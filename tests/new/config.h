// Scenario tests against current QMK: the firmware's tap-hold settings (Miryoku's config.h plus
// custom_config.h) and what the keymap's C files need outside the firmware build.
#pragma once

#include "test_common.h"

#define TAPPING_TERM 200
#define QUICK_TAP_TERM 0
#define CHORDAL_HOLD
#define FLOW_TAP_TERM 160

#define QMK_KEYBOARD_H "quantum.h"
#define MIRYOKU_CLIPBOARD_WIN
#include "rvannoord_keymap_config.h" // the keymap's config.h, for U_SYS
