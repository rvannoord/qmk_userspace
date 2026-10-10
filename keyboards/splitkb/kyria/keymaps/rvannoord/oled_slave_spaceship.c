// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

// The spaceship: a WPM-driven parallax scroll with the ship masked over it.
// Moved here unchanged from oled.c, tabs and all, so it stays byte-identical
// to the version that has been on the keyboard since April 2026.

#include QMK_KEYBOARD_H
#include "oled_panel.h"

#ifdef OLED_SLAVE_ANIMATION_SPACESHIP

#include "art/oled_spaceship.h"

// OLED dimensions
#define OLED_WIDTH 128
#define OLED_SCROLL_MODULO (OLED_WIDTH * 2)

// Animation interval in milliseconds
#define ANIMATION_INTERVAL_MS 50

// WPM-based rendering thresholds
#define WPM_SPLIT_DIVISOR 4
#define WPM_SPEED_DIVISOR 15

static void render_space(void) {
	static uint16_t state = 0;
	static uint32_t last_anim = 0;
	static const char *const space_rows[8] = {
		space_top_row_1,
		space_top_row_2,
		space_row_1,
		space_row_2,
		space_row_3,
		space_row_4,
		space_bottom_row_1,
		space_bottom_row_2,
	};
	static const char *const ship_rows[4]      = {ship_row_1,  ship_row_2,  ship_row_3,  ship_row_4};
	static const char *const mask_rows[4]       = {mask_row_1,  mask_row_2,  mask_row_3,  mask_row_4};

	uint8_t wpm = get_current_wpm();
	if (timer_elapsed32(last_anim) < ANIMATION_INTERVAL_MS) return;
	last_anim = timer_read32();

	uint8_t split = wpm / WPM_SPLIT_DIVISOR;
	uint8_t render_row[OLED_WIDTH];

	// Top 2 rows: background art completing the planet
	for (uint8_t p = 0; p < 2; p++) {
		for (uint8_t i = 0; i < OLED_WIDTH; i++) {
			render_row[i] = pgm_read_byte(space_rows[p] + i + state);
		}
		oled_set_cursor(0, p);
		oled_write_raw((char *)render_row, OLED_WIDTH);
	}

	// Ship rows (OLED rows 2–5): animation, vertically centred
	for (uint8_t row = 0; row < 4; row++) {
		for (uint8_t i = 0; i < split; i++) {
			render_row[i] = pgm_read_byte(space_rows[row + 2] + i + state);
		}
		for (uint8_t i = split; i < OLED_WIDTH; i++) {
			render_row[i] = (pgm_read_byte(space_rows[row + 2] + i + state) & pgm_read_byte(mask_rows[row] + i - split))
			                  | pgm_read_byte(ship_rows[row] + i - split);
		}
		oled_set_cursor(0, row + 2);
		oled_write_raw((char *)render_row, OLED_WIDTH);
	}

	// Bottom 2 rows: background art
	for (uint8_t p = 0; p < 2; p++) {
		for (uint8_t i = 0; i < OLED_WIDTH; i++) {
			render_row[i] = pgm_read_byte(space_rows[p + 6] + i + state);
		}
		oled_set_cursor(0, p + 6);
		oled_write_raw((char *)render_row, OLED_WIDTH);
	}

	state = (state + 1 + (wpm / WPM_SPEED_DIVISOR)) % OLED_SCROLL_MODULO;
}

void oled_render_slave(void) {
	render_space();
}

#endif     // OLED_SLAVE_ANIMATION_SPACESHIP
