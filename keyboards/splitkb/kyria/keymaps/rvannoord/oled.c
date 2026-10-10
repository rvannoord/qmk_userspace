// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

// Dispatcher only. Which panel and which art are chosen in oled_panel.h; the
// renderers live in oled_master_*.c and oled_slave_*.c. Rotation is not set here:
// kyria.c's oled_init_kb returns 180 itself and never calls oled_init_user.

#include QMK_KEYBOARD_H
#include "oled_panel.h"

#ifdef OLED_ENABLE

bool oled_task_user(void) {
	if (!is_oled_on()) return false;

	if (is_keyboard_master()) {
		oled_render_master();
	} else {
		oled_render_slave();
	}
	return false;     // false, or kyria.c's oled_task_kb paints its own text over ours
}

#endif     // OLED_ENABLE
