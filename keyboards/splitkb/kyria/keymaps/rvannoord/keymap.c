// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.
#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"
#include "host_os.h"
#ifdef OLED_ENABLE
#    include "oled_panel.h"   // only needed by housekeeping_task_user below
#endif

// Every main loop, on both halves. QMK allows exactly one definition of this, so it
// lives here rather than inside whichever OLED renderer happens to be selected.
void housekeeping_task_user(void) {
#ifdef OLED_ENABLE
    if (is_keyboard_master()) {
        oled_master_task();
    } else {
        oled_slave_task();
    }
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case U_SYS:
            if (record->event.pressed) {
                host_os_toggle();
            }
            return false;
        case U_RDO:
            if (host_is_mac()) {
                if (record->event.pressed) {
                    tap_code16(LSG(KC_Z));
                }
                return false;
            }
            break;
    }
    return true;
}

#ifdef ENCODER_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {

    if (index == 0) {
        // Volume control
        if (clockwise) {
            tap_code(KC_VOLU);
        } else {
            tap_code(KC_VOLD);
        }
    } else if (index == 1) {
        // Brightness up/Brightness down
        if (clockwise) {
            tap_code(KC_BRIU);
        } else {
            tap_code(KC_BRID);
        }
    }
    return false;
}
#endif
