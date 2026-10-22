// Two of the master's stage 5 pages: the Status sheet and Typing. The Layer map page is
// oled_layer_page.h and the Event log oled_event_log.h. Each draws a whole frame into the
// canvas; the caller commits. Design in docs/layout.md, the pages table.

#pragma once

#include <stdint.h>
#include "split_state.h"

// drashna's left screen with this keyboard's state. `kitty` is the striped kitty's frame.
void u_status_draw(const u_split_state_t *s, uint8_t wpm, uint8_t kitty);

// Typing: a sample every 300 ms into a ring of 62, one graph column each (18.6 s), and the
// session's peak. The task runs every main loop whatever the page, so the graph has a past
// when you turn to it.
#define U_TYPING_SAMPLES 62
#define U_TYPING_MS      300
void u_typing_task(uint8_t wpm);
void u_typing_draw(uint8_t wpm);
