// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

// Slave-side OLED art. Pick ONE (defaults to SPACESHIP if none set).
//   OLED_SLAVE_ANIMATION_SPACESHIP   the WPM-driven parallax scroll
//   OLED_SLAVE_ANIMATION_MONITOR1
//   OLED_SLAVE_ANIMATION_MONITOR2
//   OLED_SLAVE_ANIMATION_KYRIA
#define OLED_SLAVE_ANIMATION_SPACESHIP

// Master panel. Pick ONE (defaults to TEXT if none set).
//   OLED_MASTER_PANEL_TEXT
#define OLED_MASTER_PANEL_TEXT

/* OLED panel. Both halves sleep together (keyboard.json syncs it). See docs/layout.md.
   120 s so the idle states are actually seen; 128 to pay for it. OLED wear rises
   superlinearly with drive current, so dropping 255 -> 128 buys back more life than
   doubling the on-time costs, and it matches the board's RGB cap. */
#define OLED_TIMEOUT 120000
#define OLED_BRIGHTNESS 128

#ifdef RGB_MATRIX_ENABLE
#    define RGB_MATRIX_KEYPRESSES
#    define ENABLE_RGB_MATRIX_TYPING_HEATMAP
#    define RGB_MATRIX_TYPING_HEATMAP_DECREASE_DELAY_MS 50
#endif

/* QMK */
#define TAPPING_TERM 200
#define SPLIT_WPM_ENABLE   // Enable WPM across split keyboards (+268).

/* Host OS: Apple Silicon Macs re-trigger detection minutes after plug-in, so report once */
#define OS_DETECTION_SINGLE_REPORT

/* Home-row mods: stock Chordal Hold + Flow Tap, tuned in the keymap's tap_hold.c. See docs/layout.md */
#define CHORDAL_HOLD
#define FLOW_TAP_TERM 160

/* Keycode shims: QMK renamed these after Miryoku's layers were generated. See VENDORED.md */
#define KC_MS_U MS_UP
#define KC_MS_D MS_DOWN
#define KC_MS_L MS_LEFT
#define KC_MS_R MS_RGHT
#define KC_WH_U MS_WHLU
#define KC_WH_D MS_WHLD
#define KC_WH_L MS_WHLL
#define KC_WH_R MS_WHLR
#define KC_BTN1 MS_BTN1
#define KC_BTN2 MS_BTN2
#define KC_BTN3 MS_BTN3

#if defined(RGB_MATRIX_ENABLE)
#    define RGB_TOG RM_TOGG
#    define RGB_MOD RM_NEXT
#    define RGB_HUI RM_HUEU
#    define RGB_SAI RM_SATU
#    define RGB_VAI RM_VALU
#elif defined(RGBLIGHT_ENABLE)
#    define RGB_TOG UG_TOGG
#    define RGB_MOD UG_NEXT
#    define RGB_HUI UG_HUEU
#    define RGB_SAI UG_SATU
#    define RGB_VAI UG_VALU
#else
#    define RGB_TOG KC_NO
#    define RGB_MOD KC_NO
#    define RGB_HUI KC_NO
#    define RGB_SAI KC_NO
#    define RGB_VAI KC_NO
#endif
