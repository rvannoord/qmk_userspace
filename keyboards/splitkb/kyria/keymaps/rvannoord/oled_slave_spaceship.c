// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

// The spaceship: a WPM-driven scroll with the ship masked over it. The ship,
// its mask and the mechanism - the wpm/4 slide and the wpm/15 speed - are
// JBaguley's, unchanged since April 2026. The space behind it is new: three
// parallax planes read off one counter at 1/4, 1/2 and full speed, with
// occlusion masks so far stars do not shine through a planet's dark side.
// Periods 128/256/512 under a 512 loop, so the wrap is arithmetic. See
// tools/space/gen_space.py and docs/layout.md.

#include QMK_KEYBOARD_H
#include "oled.h"


#include "art/oled_spaceship.h"
#include "art/space_art.h"

// OLED dimensions
#define OLED_WIDTH 128
#define OLED_SCROLL_MODULO U_SPACE_LOOP

// Animation interval in milliseconds
#define ANIMATION_INTERVAL_MS 50

// WPM-based rendering thresholds
#define WPM_SPLIT_DIVISOR 4
#define WPM_SPEED_DIVISOR 15

static void render_space(void) {
	static uint16_t state = 0;
	static uint32_t last_anim = 0;
	static const char *const ship_rows[4] = {ship_row_1, ship_row_2, ship_row_3, ship_row_4};

	uint8_t wpm = get_current_wpm();
	if (timer_elapsed32(last_anim) < ANIMATION_INTERVAL_MS) return;
	last_anim = timer_read32();

	uint8_t split = wpm / WPM_SPLIT_DIVISOR;
	uint8_t render_row[OLED_WIDTH];

	// One counter, three offsets. The planes' periods divide the loop, so at the
	// wrap all three are back at zero together and the seam cannot show.
	const uint16_t of = state >> 2;
	const uint16_t om = state >> 1;
	const uint16_t on = state;

	for (uint8_t p = 0; p < 8; p++) {
		for (uint8_t i = 0; i < OLED_WIDTH; i++) {
			// Far behind mid behind near, each nearer plane punching its silhouette
			// out of what is behind it before adding its own light.
			uint8_t b = pgm_read_byte(&u_space_far[p][of + i]);
			b = (b & pgm_read_byte(&u_space_mid_mask[p][om + i])) | pgm_read_byte(&u_space_mid[p][om + i]);
			b = (b & pgm_read_byte(&u_space_near_mask[p][on + i])) | pgm_read_byte(&u_space_near[p][on + i]);
			// The ship, exactly as before: pages 2-5, slid right by wpm/4. Its mask is
			// now derived from its own silhouette with a 1 px rim - the rim is what keeps
			// a lit ship visible over a lit planet, so it cannot be zero.
			if (p >= 2 && p <= 5 && i >= split) {
				b = (b & pgm_read_byte(&u_space_ship_mask[p - 2][i - split])) | pgm_read_byte(ship_rows[p - 2] + i - split);
			}
			render_row[i] = b;
		}
		oled_set_cursor(0, p);
		oled_write_raw((char *)render_row, OLED_WIDTH);
	}

	state = (state + 1 + (wpm / WPM_SPEED_DIVISOR)) % OLED_SCROLL_MODULO;
}

void u_slave_spaceship_render(void) {
	render_space();
}

