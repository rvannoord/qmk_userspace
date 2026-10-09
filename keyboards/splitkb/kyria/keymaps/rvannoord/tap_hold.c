#include "quantum.h"

char chordal_hold_handedness(keypos_t key) {
    return key.row < MATRIX_ROWS / 2 ? 'L' : 'R';
}

bool get_chordal_hold(uint16_t tap_hold_keycode, keyrecord_t *tap_hold_record, uint16_t other_keycode, keyrecord_t *other_record) {
    if (IS_QK_LAYER_TAP(tap_hold_keycode)) {
        return true;
    }
    uint16_t crossover = (QK_MOD_TAP_GET_MODS(tap_hold_keycode) & MOD_LGUI) ? 120 : 80;
    if (TIMER_DIFF_16(other_record->event.time, tap_hold_record->event.time) <= crossover) {
        return false;
    }
    return get_chordal_hold_default(tap_hold_record, other_record);
}

uint16_t get_flow_tap_term(uint16_t keycode, keyrecord_t *record, uint16_t prev_keycode) {
    if (!IS_QK_MOD_TAP(keycode) || (QK_MOD_TAP_GET_MODS(keycode) & 0x0F) == MOD_LSFT) {
        return 0;
    }
    return is_flow_tap_key(keycode) && is_flow_tap_key(prev_keycode) ? FLOW_TAP_TERM : 0;
}
