// Generated from DUNGEON.mode by gen_font.py. Do not edit by hand.
// Source sheet: dungeonmode/bitmap/dungeon-mode.png, a 16x16 grid of 8x8 cells.

#pragma once

#include <stdint.h>
#include "progmem.h"

#define U_FONT_FIRST 0x20
#define U_FONT_LAST  0x7e

// One byte a row, bit 7 leftmost. Characters the sheet does not carry are blank.
extern const uint8_t PROGMEM u_font8[95][8];

// Symbols, same format. Drawn at 2x, so each one covers 16x16.
enum u_sym {
    U_SYM_CAPS_LOCK,
    U_SYM_CAPS_WORD,
    U_SYM_ALTGR,
    U_SYM_LINK,
    U_SYM_COUNT,
};

extern const uint8_t PROGMEM u_sym8[U_SYM_COUNT][8];
