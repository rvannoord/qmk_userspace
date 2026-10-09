// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

// False home row mods adjustments https://docs.qmk.fm/tap_hold
#define BILATERAL_COMBINATIONS

// Slave-side OLED animation. Pick ONE (defaults to SPACESHIP if none set).
//   OLED_SLAVE_ANIMATION_SPACESHIP
//   OLED_SLAVE_ANIMATION_MONITOR1
//   OLED_SLAVE_ANIMATION_MONITOR2
//   OLED_SLAVE_ANIMATION_KYRIA
#define OLED_SLAVE_ANIMATION_SPACESHIP

#ifdef RGB_MATRIX_ENABLE
#    define RGB_MATRIX_KEYPRESSES
#    define ENABLE_RGB_MATRIX_TYPING_HEATMAP
#    define RGB_MATRIX_TYPING_HEATMAP_DECREASE_DELAY_MS 50
#endif

/* QMK */
#define TAPPING_TERM 200
#define SPLIT_WPM_ENABLE   // Enable WPM across split keyboards (+268).

/* Miryoku */
#define BILATERAL_COMBINATIONS_LIMIT_CHORD_TO_N_KEYS 4 /* GUI, Alt, Ctrl, Shift */
#define BILATERAL_COMBINATIONS_DELAY_MODS_THAT_MATCH MOD_MASK_GUI
#define BILATERAL_COMBINATIONS_DELAY_MATCHED_MODS_BY 120  /* ms */
#define BILATERAL_COMBINATIONS_ALLOW_CROSSOVER_AFTER 80   /* ms */
#define BILATERAL_COMBINATIONS_ALLOW_SAMESIDED_AFTER 3000 /* ms */
#define BILATERAL_COMBINATIONS_TYPING_STREAK_TIMEOUT 160  /* ms */
#define BILATERAL_COMBINATIONS_TYPING_STREAK_MODMASK (~MOD_MASK_SHIFT)
