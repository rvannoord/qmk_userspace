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

#ifdef OLED_MASTER_PANEL_TEXT

// Miryoku's own layer list, so there is no second copy of the layer order here.
// Overriding MIRYOKU_LAYER_LIST in custom_config.h changes these names.
static const char *const layer_names[] = {
#define MIRYOKU_X(LAYER, STRING) [U_##LAYER] = STRING,
    MIRYOKU_LAYER_LIST
#undef MIRYOKU_X
};

static void render_mods(void) {
    uint8_t mods = get_mods();

    // A slot is a finger, pinky to index — not a modifier. get_mods() returns post-swap
    // bits, because mod_config() runs when a keycode becomes an action, so on the Mac the
    // pinky key reports Ctrl and the middle finger reports GUI. The slot never moves, only
    // the label. Five-wide fields, blanks the same width, so nothing shifts.
    if (host_is_mac()) {
        oled_write_P(mods & MOD_MASK_CTRL  ? PSTR("CTRL ") : PSTR("     "), false);
        oled_write_P(mods & MOD_MASK_ALT   ? PSTR("OPT  ") : PSTR("     "), false);
        oled_write_P(mods & MOD_MASK_GUI   ? PSTR("CMD  ") : PSTR("     "), false);
        oled_write_P(mods & MOD_MASK_SHIFT ? PSTR("SHIFT") : PSTR("     "), false);
    } else {
        oled_write_P(mods & MOD_MASK_GUI   ? PSTR("GUI  ") : PSTR("     "), false);
        oled_write_P(mods & MOD_MASK_ALT   ? PSTR("ALT  ") : PSTR("     "), false);
        oled_write_P(mods & MOD_MASK_CTRL  ? PSTR("CTRL ") : PSTR("     "), false);
        oled_write_P(mods & MOD_MASK_SHIFT ? PSTR("SHIFT") : PSTR("     "), false);
    }
    oled_write_P(PSTR("\n"), false);        // clears to end of line, so the locks start clean
}

void oled_render_master(void) {
    static const char PROGMEM qmk_logo[] = {
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,
        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0};

    oled_write_P(qmk_logo, false);
    // 20 columns, so the line doesn't wrap before the newline
    oled_write_P(PSTR("Kyria rev3       "), false);
    oled_write_P(host_is_mac() ? PSTR("Mac\n\n") : PSTR("Win\n\n"), false);

    oled_write_P(PSTR("Layer: "), false);
    oled_write(layer_names[get_highest_layer(layer_state | default_layer_state)], false);
    oled_write_P(PSTR("\n"), false);

    render_mods();

    const led_t led_usb_state = host_keyboard_led_state();
    oled_write_P(led_usb_state.num_lock    ? PSTR("NUMLCK ") : PSTR("       "), false);
    oled_write_P(led_usb_state.caps_lock   ? PSTR("CAPLCK ") : PSTR("       "), false);
    oled_write_P(led_usb_state.scroll_lock ? PSTR("SCRLCK ") : PSTR("       "), false);
}

#endif     // OLED_MASTER_PANEL_TEXT
