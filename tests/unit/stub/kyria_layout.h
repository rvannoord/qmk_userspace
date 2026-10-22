// The Kyria rev3 matrix and LAYOUT, copied from what QMK generates out of
// keyboards/splitkb/kyria/rev3/keyboard.json (.build/.../src/default_keyboard.h). The map
// places keys through the keymap's LAYOUT_miryoku, which expands to this, so the tests
// place them the way the firmware does.
#pragma once

#define MATRIX_ROWS 8
#define MATRIX_COLS 7

#define LAYOUT_split_3x6_5(k0G, k0F, k0E, k0D, k0C, k0B, k4B, k4C, k4D, k4E, k4F, k4G, k1G, k1F, k1E, k1D, k1C, k1B, k5B, k5C, k5D, k5E, k5F, k5G, k2G, k2F, k2E, k2D, k2C, k2B, k3D, k2A, k6A, k7D, k6B, k6C, k6D, k6E, k6F, k6G, k3E, k3C, k3B, k3F, k3A, k7A, k7F, k7B, k7C, k7E) { \
    { KC_NO, k0B, k0C, k0D, k0E, k0F, k0G }, \
    { KC_NO, k1B, k1C, k1D, k1E, k1F, k1G }, \
    { k2A, k2B, k2C, k2D, k2E, k2F, k2G }, \
    { k3A, k3B, k3C, k3D, k3E, k3F, KC_NO }, \
    { KC_NO, k4B, k4C, k4D, k4E, k4F, k4G }, \
    { KC_NO, k5B, k5C, k5D, k5E, k5F, k5G }, \
    { k6A, k6B, k6C, k6D, k6E, k6F, k6G }, \
    { k7A, k7B, k7C, k7D, k7E, k7F, KC_NO } \
}


#define LAYOUT LAYOUT_split_3x6_5
