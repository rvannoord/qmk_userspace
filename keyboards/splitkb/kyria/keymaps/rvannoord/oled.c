// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

// Dispatcher only. Which panel and which art are chosen in oled_panel.h; the
// renderers live in oled_master_*.c and oled_slave_*.c. Rotation is not set here
// because kyria.c's oled_init_kb already returns 180. The driver does call
// oled_init_user on top of it (oled_driver.c:295) — this keymap simply defines none.

#include QMK_KEYBOARD_H
#include "oled_panel.h"

#ifdef OLED_ENABLE

bool oled_task_user(void) {
	if (!is_oled_on()) return false;

#ifdef U_OLED_INVERT
	// Black and white swapped, in the panel itself rather than in the buffer: the
	// SSD1306 has an invert-display command and QMK exposes it. Bjorn's source art is
	// dark ink on a white page — it is an e-paper original — so this is the medium he
	// was drawn for, and it is what makes the hooded poses legible: a solid cowl
	// instead of a 33% blob.
	//
	// Called every frame on purpose. oled_invert() returns early when the panel is
	// already in that state, so it costs a comparison, and it re-applies itself if
	// anything ever resets the display.
	oled_invert(true);
#endif

	if (is_keyboard_master()) {
		oled_render_master();
	} else {
		oled_render_slave();
	}
	return false;     // false, or kyria.c's oled_task_kb paints its own text over ours
}

#endif     // OLED_ENABLE
