// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.
// Miryoku's keymap, with this board's hooks. There is no keymaps[] here: the layers
// come from users/manna-harbour_miryoku/ (USER_NAME in rules.mk), through the
// LAYOUT_miryoku wrapper in config.h. This file only adds what Miryoku does not have:
// the host OS switch, the split state sync, the OLED tasks and the encoders.
#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"
#include "host_os.h"
#include "split_state.h"
#include "split_sync.h"
#include "oled.h"
#include "split_health.h"

// Before the split transport starts, so the slave's handler can never be called while
// the callback pointer is still null.
void keyboard_pre_init_user(void) {
    u_sync_init();
}

// Every main loop, on both halves. QMK allows exactly one definition of this, so it
// lives here rather than inside whichever OLED renderer happens to be selected.
void housekeeping_task_user(void) {
    u_health_task();
    u_sync_task();

#ifdef OLED_ENABLE
    if (is_keyboard_master()) {
        oled_master_task();
        // Both panels off two idle minutes after the last key, whatever is being drawn.
        // Here rather than in the 20 Hz render, so a stray dirty block can light the
        // panel for one main loop at most. The other half follows within one forced
        // sync; waking is QMK's own (keyboard.c calls oled_on() on activity).
        if (u_oled_should_sleep(is_oled_on(), last_input_activity_elapsed())) oled_off();
    } else {
        oled_slave_task();
    }
#endif
}

// The herdr key (docs/layout.md, The terminal round, 2a): tapped alone, herdr's prefix;
// held, every key tapped meanwhile is the prefix, then Alt+that key.
static bool herdr_held;
static bool herdr_used;

// The key a press types, or KC_NO for modifiers, layer holds and custom keys.
static uint16_t herdr_typed_key(uint16_t keycode, const keyrecord_t *record) {
    if (IS_QK_MOD_TAP(keycode)) return record->tap.count ? QK_MOD_TAP_GET_TAP_KEYCODE(keycode) : KC_NO;
    if (IS_QK_LAYER_TAP(keycode)) return record->tap.count ? QK_LAYER_TAP_GET_TAP_KEYCODE(keycode) : KC_NO;
    return IS_BASIC_KEYCODE(keycode) ? keycode : KC_NO;
}

// tap_code16 never passes through mod_config(), so this is Ctrl+B whatever the host,
// where a C(KC_B) in the keymap would follow any Ctrl/GUI swap.
static void herdr_prefix(void) {
    tap_code16(C(KC_B));
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    u_sync_note_record(keycode, record);
    if (record->event.pressed && keycode != U_SYS) u_stats_note_key();   // closes the page

    if (herdr_held && record->event.pressed) {
        const uint16_t key = herdr_typed_key(keycode, record);
        if (key != KC_NO) {
            // Two separate taps: Ctrl+Alt is AltGr on EurKEY. Held mods (Shift from a
            // home-row key) stay off the prefix and go with the key. Left Alt, because
            // Ghostty keeps the right Option for letters.
            const uint8_t mods = get_mods();
            herdr_used         = true;
            clear_mods();
            herdr_prefix();
            set_mods(mods);
            tap_code16(A(key));
            return false;
        }
    }

    switch (keycode) {
        case U_SYS:
            // The OS now flips on *release* of a short press, because a long press is
            // how the stats page is asked for. Bootmagic is untouched: that is a
            // boot-time matrix scan, not a keypress.
            u_stats_note_sys(record->event.pressed);
            if (!record->event.pressed && u_stats_sys_was_tap()) {
                host_os_toggle();
            }
            return false;
        case U_ANIM:
            // On release, like SYS: a key that acts on press fires while you are
            // still deciding, and this one is cosmetic enough that a mis-tap
            // should be cheap to undo by pressing it again.
            if (!record->event.pressed) u_anim_next();
            return false;
        case U_PAGE:
            // On release, for the same reasons as ANIM.
            if (!record->event.pressed) u_page_next();
            return false;
#ifdef OLED_ENABLE
        case KC_MUTE:
            // Shown like an encoder turn; the key itself goes on to the host.
            if (record->event.pressed) u_mute_note();
            break;
#endif
        case U_HERDR:
            // The prefix goes out on release, once it is clear no key was tapped.
            if (record->event.pressed) {
                herdr_held = true;
                herdr_used = false;
            } else {
                herdr_held = false;
                if (!herdr_used) herdr_prefix();
            }
            return false;
        case U_UND:
        case U_CUT:
        case U_CPY:
        case U_PST:
        case U_RDO:
            // Miryoku's clipboard keys are Ctrl-based (MIRYOKU_CLIPBOARD = WIN). The mods
            // stay put on the Mac, so these are the keys that turn into Cmd there, redo as
            // Cmd+Shift+Z. Registered while held, so a held Undo repeats.
            if (host_is_mac()) {
                const uint16_t mac = keycode == U_RDO ? LSG(KC_Z) : LGUI(QK_MODS_GET_BASIC_KEYCODE(keycode));
                if (record->event.pressed) {
                    register_code16(mac);
                } else {
                    unregister_code16(mac);
                }
                return false;
            }
            break;
    }
    return true;
}

#ifdef ENCODER_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {

#    ifdef OLED_ENABLE
    u_encoder_note(index, clockwise);
#    endif
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
