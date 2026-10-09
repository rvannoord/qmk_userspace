// Firmware builds generate chordal_hold_layout from keyboard.json; the test build has none. QMK's
// default chordal_hold_handedness() reads it, and still has to link even though tap_hold.c replaces it,
// so this placeholder is never read.
#include "quantum.h"

const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS] PROGMEM = {{0}};
