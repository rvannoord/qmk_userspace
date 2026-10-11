// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

// The master-side text panel: logo, board, host OS, layer, held mods, lock state.
// 21 columns x 8 lines in the 6x8 font. Lines 0-2 logo, 3 board + OS, 4 blank,
// 5 layer, 6 mods, 7 locks — exactly full, nothing wraps.

#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"
#include "oled_panel.h"
#include "host_os.h"
#include "split_state.h"

#ifdef OLED_MASTER_PANEL_TEXT

// Miryoku's own layer list, so there is no second copy of the layer order here.
// Overriding MIRYOKU_LAYER_LIST in custom_config.h changes these names.
static const char *const layer_names[] = {
#define MIRYOKU_X(LAYER, STRING) [U_##LAYER] = STRING,
    MIRYOKU_LAYER_LIST
#undef MIRYOKU_X
};

// The state the panel draws from, rebuilt every main loop by oled_master_task so a
// modifier cannot be missed between two 20 Hz frames.
static u_split_state_t panel;

static void render_mods(void) {
    // A cell is a finger, pinky to index — not a modifier. The slot comes from the
    // key's declared mod, so it never moves; only the label follows the OS. That is
    // why this used to light the wrong cell on the Mac, where Ctrl and GUI swap.
    static const char *const win[4] = {"GUI  ", "ALT  ", "CTRL ", "SHIFT"};
    static const char *const mac[4] = {"CTRL ", "OPT  ", "CMD  ", "SHIFT"};
    const char *const *label = (panel.flags & U_FLAG_IS_MAC) ? mac : win;
    const uint8_t      cells = U_FIN_CELLS(panel.fingers);

    for (uint8_t i = 0; i < 4; i++) {
        oled_write((cells & (1u << i)) ? label[i] : "     ", false);
    }
    oled_write_P(PSTR("\n"), false);        // clears to end of line
}

void oled_master_task(void) {
    panel = u_sync_build();
}

void oled_render_master(void) {
    static const char PROGMEM qmk_logo[] = {
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,
        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0};

    oled_write_P(qmk_logo, false);
    // 20 columns, so the line doesn't wrap before the newline
    oled_write_P(PSTR("Kyria rev3       "), false);
    oled_write_P(panel.flags & U_FLAG_IS_MAC ? PSTR("Mac\n\n") : PSTR("Win\n\n"), false);

    oled_write_P(PSTR("Layer: "), false);
    oled_write(layer_names[panel.layers & 0x0F], false);
    oled_write_P(PSTR("\n"), false);

    render_mods();

    oled_write_P(panel.flags & U_FLAG_NUM_LOCK    ? PSTR("NUMLCK ") : PSTR("       "), false);
    oled_write_P(panel.flags & U_FLAG_CAPS_LOCK   ? PSTR("CAPLCK ") : PSTR("       "), false);
    oled_write_P(panel.flags & U_FLAG_SCROLL_LOCK ? PSTR("SCRLCK ") : PSTR("       "), false);
}

#endif     // OLED_MASTER_PANEL_TEXT
