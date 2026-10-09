// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

#include QMK_KEYBOARD_H
#include "oled_logos.h"

#ifdef OLED_ENABLE

// Layer indices — must stay in sync with keymap.c
enum layers {
	_MIRYOKU = 0,
	_EXTRA,
	_TAP,
	_BUTTON,
	_NAV,
	_MOUSE,
	_MEDIA,
	_NUM,
	_SYM,
	_FUN,
};

// Animation interval in milliseconds
#define ANIMATION_INTERVAL_MS 50

// WPM-based rendering thresholds
#define WPM_SPLIT_DIVISOR 4
#define WPM_SPEED_DIVISOR 15

// OLED dimensions
#define OLED_WIDTH 128
#define OLED_HEIGHT 8
#define OLED_SCROLL_MODULO (OLED_WIDTH * 2)

oled_rotation_t oled_init_user(oled_rotation_t rotation) { return OLED_ROTATION_180; }

#ifdef OLED_SLAVE_ANIMATION_SPACESHIP
#include "oled_frames.h"

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
#endif     // OLED_SLAVE_ANIMATION_SPACESHIP

static void render_mods(void) {
	uint8_t mods = get_mods();

	oled_write_P(mods & MOD_MASK_GUI ? PSTR("GUI") : PSTR("   "), false);
	oled_write_P(mods & MOD_MASK_ALT ? PSTR("ALT") : PSTR("   "), false);
	oled_write_P(mods & MOD_MASK_CTRL ? PSTR("CTRL") : PSTR("   "), false);
	oled_write_P(mods & MOD_MASK_SHIFT ? PSTR("SHIFT") : PSTR("   "), false);
}

bool oled_task_user(void) {
	if (!is_oled_on()) return false;

	if (is_keyboard_master()) {
		static const char PROGMEM qmk_logo[] = {
					0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,
					0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,
					0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0};

		oled_write_P(qmk_logo, false);
		oled_write_P(PSTR("Kyria rev3\n\n"), false);

		oled_write_P(PSTR("Layer: "), false);
		switch (get_highest_layer(layer_state | default_layer_state)) {
			case _MIRYOKU: oled_write_P(PSTR("Miryoku\n"),    false); break;
			case _EXTRA:   oled_write_P(PSTR("Extra\n"),      false); break;
			case _TAP:     oled_write_P(PSTR("Tap\n"),        false); break;
			case _NAV:     oled_write_P(PSTR("Navigation\n"), false); break;
			case _MOUSE:   oled_write_P(PSTR("Mouse\n"),      false); break;
			case _BUTTON:  oled_write_P(PSTR("Button\n"),     false); break;
			case _MEDIA:   oled_write_P(PSTR("Media\n"),      false); break;
			case _NUM:     oled_write_P(PSTR("Num\n"),        false); break;
			case _SYM:     oled_write_P(PSTR("Symbols\n"),    false); break;
			case _FUN:     oled_write_P(PSTR("Function\n"),   false); break;
			default:       oled_write_P(PSTR("Undefined\n"),  false); break;
		}

		render_mods();
		led_t led_usb_state = host_keyboard_led_state();
		oled_write_P(led_usb_state.num_lock        ? PSTR("NUMLCK ") : PSTR("         "), false);
		oled_write_P(led_usb_state.caps_lock       ? PSTR("CAPLCK ") : PSTR("         "), false);
		oled_write_P(led_usb_state.scroll_lock ? PSTR("SCRLCK ") : PSTR("         "), false);
	} else {
#if defined(OLED_SLAVE_ANIMATION_SPACESHIP)
		render_space();
#elif defined(OLED_SLAVE_ANIMATION_MONITOR1)
		oled_write_raw_P(monitor_logo1, sizeof(monitor_logo1));
#elif defined(OLED_SLAVE_ANIMATION_MONITOR2)
		oled_write_raw_P(monitor_logo2, sizeof(monitor_logo2));
#elif defined(OLED_SLAVE_ANIMATION_KYRIA)
		oled_write_raw_P(kyria_logo, sizeof(kyria_logo));
#endif
	}
	return false;
}

#endif     // OLED_ENABLE
