// A whole-frame canvas, so a frame that did not change writes nothing.
//
// The driver marks a block dirty for any byte that *changes*, even one that changes
// back to its old value inside the same frame (oled_driver.c:667-672), and a dirty
// block calls oled_on(), which re-arms OLED_TIMEOUT (:445, :453, :731). Both renderers
// used to clear each zone and draw it again straight into the driver's buffer, so every
// lit byte changed twice a frame, every block stayed dirty for ever, and neither panel
// could ever sleep.
//
// So: compose here, then commit once. Zones and layers may still clear and redraw as
// much as they like — it now happens in RAM, where it costs nothing.

#pragma once

#include <stdint.h>

// 1 KB against roughly 249 KB of free heap. One canvas serves both halves: a half is
// master or slave for the whole session, so only one renderer ever draws into it.
extern uint8_t u_canvas[OLED_MATRIX_SIZE];

// Copy the canvas into the driver's buffer, writing only the bytes that differ.
// Returns the mask of 64-byte blocks actually written — the driver's own dirty bits
// stay the single source of truth; this is for the tests, and later for the budget
// arbiter that has to know what a step cost.
uint16_t u_canvas_commit(void);
