// Hand-drawn for this panel, in DUNGEON.mode's weight language. Page format:
// one byte a column, bit 0 at the top, page 0 then page 1. Generated — the grids
// they came from are span lists, not this array, so edit those and regenerate.

#pragma once

#include <stdint.h>
#include "progmem.h"

#define U_GLYPH_BYTES 32        // 16x16, two pages of 16 columns

enum u_glyph {
    U_GLYPH_WINDOW,    // Windows GUI, the four-pane mark
    U_GLYPH_OPTION,    // Alt on Windows, Option on the Mac: one key, two names
    U_GLYPH_CARET,    // Ctrl. Moves cell with the OS swap, middle finger to pinky
    U_GLYPH_CMD,    // Command, Mac only
    U_GLYPH_SHIFT,    // Shift. One drawing, both columns
    U_GLYPH_COUNT,
};

extern const uint8_t PROGMEM u_glyph16[U_GLYPH_COUNT][U_GLYPH_BYTES];

// The status bar's latched flags, 7x7 inside an 8x8 cell: column 7 is the
// gutter that keeps the 1 px channel between neighbours from closing under
// bloom, row 7 belongs to the SYS underline. Row 0 carries ink in all three
// and in none of the six badge letters, so the cluster sits a pixel proud of
// any text beside it. Drawn at 1 px, which the sheet's own symbol cells also
// are — and which means they must never be inverted in place. Nothing does:
// the alarm row is a different page.
enum u_lock {
    U_LOCK_CAPS_LOCK,    // padlock. The only place Caps Lock appears at all
    U_LOCK_CAPS_WORD,    // latched shift, rule stopping short: it ends at the word
    U_LOCK_SCROLL_LOCK,    // struck ring: something latched that you did not mean
    U_LOCK_COUNT,
};

extern const uint8_t PROGMEM u_lock8[U_LOCK_COUNT][8];
