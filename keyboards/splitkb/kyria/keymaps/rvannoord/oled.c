// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

// Dispatcher only. Which panel and which art are chosen in oled_panel.h; the
// renderers live in oled_master_*.c and oled_slave_*.c. Rotation is not set here
// because kyria.c's oled_init_kb already returns 180. The driver does call
// oled_init_user on top of it (oled_driver.c:295) — this keymap simply defines none.

#include QMK_KEYBOARD_H
#include <string.h>
#include "oled_canvas.h"
#include "panel_font.h"
#include "oled_panel.h"

#ifdef OLED_ENABLE

// --- the splash ------------------------------------------------------------
// "QMK Firmware" for a moment at boot, on both halves. It is not new art: the
// logo is 63 characters of the panel's own font, which is how the stock keymaps
// draw it - see tools/panel/gen_font.py.
//
// It doubles as the cheapest possible confirmation that a flash took, which is
// worth something on a split keyboard where the usual symptom of a half-flashed
// pair is that one side silently stops typing.

#    define U_SPLASH_MS 1200

static bool splash_done;

static bool splash(void) {
    if (splash_done) return false;
    if (timer_read32() >= U_SPLASH_MS) {
        splash_done = true;
        return false;
    }

    // Centred exactly rather than snapped to a page: 126x24 leaves one column
    // each side and 20 rows above, which is not a page boundary. A splash is
    // drawn a handful of times, so per-pixel costs nothing and avoids carrying a
    // shifted copy of the art.
    const uint8_t x0 = (OLED_DISPLAY_WIDTH - U_LOGO_W) / 2;
    const uint8_t y0 = (OLED_DISPLAY_HEIGHT - U_LOGO_PAGES * 8) / 2;

    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    for (uint8_t p = 0; p < U_LOGO_PAGES; p++) {
        for (uint8_t i = 0; i < U_LOGO_W; i++) {
            const uint8_t col = pgm_read_byte(&u_qmk_logo[(uint16_t)p * U_LOGO_W + i]);
            for (uint8_t k = 0; k < 8; k++) {
                if (!((col >> k) & 1u)) continue;
                const uint8_t y = (uint8_t)(y0 + p * 8 + k);
                u_canvas[(uint16_t)(y >> 3) * OLED_DISPLAY_WIDTH + x0 + i] |=
                    (uint8_t)(1u << (y & 7));
            }
        }
    }
    u_canvas_commit();
    return true;
}

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

	if (splash()) return false;

	if (is_keyboard_master()) {
		oled_render_master();
	} else {
		oled_render_slave();
	}
	return false;     // false, or kyria.c's oled_task_kb paints its own text over ours
}

#endif     // OLED_ENABLE
