// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

// Dispatcher only. The entry points are declared in oled.h; the renderers live in
// oled_master_zones.c and oled_slave_*.c. Rotation is not set here
// because kyria.c's oled_init_kb already returns 180. The driver does call
// oled_init_user on top of it (oled_driver.c:295) — this keymap simply defines none.

#include QMK_KEYBOARD_H
#include <string.h>
#include "oled_canvas.h"
#include "art/panel_font.h"
#include "oled.h"

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

static void put_px(uint8_t x, uint8_t y, bool on) {
    uint8_t *b = &u_canvas[(uint16_t)(y >> 3) * OLED_DISPLAY_WIDTH + x];
    if (on) {
        *b |= (uint8_t)(1u << (y & 7));
    } else {
        *b &= (uint8_t)~(1u << (y & 7));
    }
}

// The logo is placed per pixel rather than snapped to a page: 126x24 centred leaves
// 20 rows above, which is not a page boundary. It is drawn a handful of times, so
// per-pixel costs nothing and avoids carrying a shifted copy of the art.
static void draw_logo(uint8_t y0) {
    const uint8_t x0 = (OLED_DISPLAY_WIDTH - U_LOGO_W) / 2;
    for (uint8_t p = 0; p < U_LOGO_PAGES; p++) {
        for (uint8_t i = 0; i < U_LOGO_W; i++) {
            const uint8_t col = pgm_read_byte(&u_qmk_logo[(uint16_t)p * U_LOGO_W + i]);
            for (uint8_t k = 0; k < 8; k++) {
                if ((col >> k) & 1u) put_px((uint8_t)(x0 + i), (uint8_t)(y0 + p * 8 + k), true);
            }
        }
    }
}

static bool splash(void) {
    if (splash_done) return false;
    if (timer_read32() >= U_SPLASH_MS) {
        splash_done = true;
        return false;
    }

    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    draw_logo((OLED_DISPLAY_HEIGHT - U_LOGO_PAGES * 8) / 2);
    u_canvas_commit();
    return true;
}

// --- the bootloader screen -------------------------------------------------
// reset_keyboard() calls shutdown_quantum(true) (quantum.c:232, :208), which calls
// shutdown_kb() and so this (:473), and then jumps. Without it the panel freezes on its
// last frame, which looks the same as a Boot double-tap that did nothing. With it the
// panel shows the logo over an inverted BOOTLOADER, and the SSD1306 keeps showing it,
// because the panel stays powered while the bootloader runs. Only the master gets here:
// keys are processed there, so only the half with the cable shows it.
//
// soft_reset_keyboard() comes through here too, with jump_to_bootloader false: nothing
// to announce then. The link's self-heal does not, it calls mcu_reset() directly.

#    define U_BOOT_LOGO_Y 12
#    define U_BOOT_TEXT_Y 44       // the label's top row; its bar has 2 rows above, 2 below
#    define U_BOOT_PAD_X   3
#    define U_BOOT_PAD_Y   2
#    define U_CELL_INK     5       // the font is 5 columns of ink in an 8 px cell

// Text knocked out of a filled bar, centred on its ink rather than its cells.
static void draw_label(const char *s, uint8_t y) {
    const uint8_t n  = (uint8_t)strlen(s);
    const uint8_t w  = (uint8_t)(n * 8 - (8 - U_CELL_INK));
    const uint8_t x0 = (uint8_t)((OLED_DISPLAY_WIDTH - w) / 2);
    for (uint8_t yy = (uint8_t)(y - U_BOOT_PAD_Y); yy < y + 8 + U_BOOT_PAD_Y - 1; yy++) {
        for (uint8_t xx = (uint8_t)(x0 - U_BOOT_PAD_X); xx < x0 + w + U_BOOT_PAD_X; xx++) put_px(xx, yy, true);
    }
    for (uint8_t c = 0; c < n; c++) {
        const uint8_t *g = u_font8[(uint8_t)s[c] - U_FONT_FIRST];
        for (uint8_t r = 0; r < 8; r++) {
            const uint8_t row = pgm_read_byte(&g[r]);
            for (uint8_t k = 0; k < 8; k++) {
                if (row & (0x80u >> k)) put_px((uint8_t)(x0 + c * 8 + k), (uint8_t)(y + r), false);
            }
        }
    }
}

bool shutdown_user(bool jump_to_bootloader) {
    if (!jump_to_bootloader) return true;

    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    draw_logo(U_BOOT_LOGO_Y);
    draw_label("BOOTLOADER", U_BOOT_TEXT_Y);
    u_canvas_commit();
    // Synchronously: there is no next main loop to send the dirty blocks, and
    // oled_render_dirty(false) would send only one of them.
    oled_render_dirty(true);
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
