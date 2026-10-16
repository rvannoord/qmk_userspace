// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

// Mine, not Miryoku's. Miryoku's rules.mk includes it, which is how a Miryoku layout
// gets a keyboard's own features and tuning without touching the vendored files around
// it. Key positions are in the keymap folder's config.h; everything else is here.

// The other half's animation at boot, on a fresh EEPROM. All five are compiled in and
// the U_ANIM key cycles through them (see the keymap's oled.h); this names the default.
//   OLED_SLAVE_ANIMATION_BJORN       the Viking, reacting to typing speed
//   OLED_SLAVE_ANIMATION_SPACESHIP   the WPM-driven parallax scroll
//   OLED_SLAVE_ANIMATION_KYRIA / _MONITOR1 / _MONITOR2   the static logos
#define OLED_SLAVE_ANIMATION_BJORN

/* OLED panel. Both halves sleep together (keyboard.json syncs it). See docs/layout.md.
   120 s so the idle states are actually seen. Full brightness, because lit-on-black
   lights about a quarter of the pixels: 255 here is a lighter load than 128 was while
   the panel ran inverted (2026-10-14 to -15), and OLED wear follows drive current. */
#define OLED_TIMEOUT 120000
#define OLED_BRIGHTNESS 255

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

// The other half restarts itself if the link stays dead, and a hardware watchdog
// catches the case a software reset cannot: a half whose code has stopped running.
// See docs/oled-v2.md and the comments in split_health.h for every guard on these.
#define U_SELF_HEAL
#define U_WATCHDOG_MS 2000

// Swap black and white on both panels, in hardware. Tried in a6bd2f9 for Bjorn's sake
// (his source is dark ink on a white page) and reverted on 2026-10-15: the lit field was
// not wanted. The art is identical either way; docs/layout.md has what flips with it.
// #define U_OLED_INVERT

// The world layer: weather across the whole slave panel, behind Bjorn. Season
// comes from __DATE__ until the host daemon exists, so the panel is never dead
// and never wrong by more than a rebuild. See docs/layout.md.
#define OLED_SLAVE_WORLD

// Give the other half the master's activity timestamps, so last_input_activity_elapsed()
// is the same clock on both. Without it the mascot derives idle from how long the WPM
// reading has been still, and WPM keeps changing while it decays over its five-second
// window — so idle started up to five seconds late on that half while the master was
// already counting down to switching the panels off.
#define SPLIT_ACTIVITY_ENABLE
