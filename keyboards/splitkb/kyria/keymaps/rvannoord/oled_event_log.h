// The Event log page's record: the last eight things that happened to the link and the host,
// each stamped with the uptime, since there is no clock. RAM only, like the rest of the link
// evidence (split_health.h). Design in docs/layout.md, the Event log row of the pages table.

#pragma once

#include <stdint.h>
#include "split_state.h"

#define U_LOG_LINES 8
#define U_LOG_CHARS 21      // what a line of Ein holds across the panel

// Stamped "+1:10 " (hours and minutes since boot) and cut to a line. The ninth pushes out
// the first.
void u_log_add(const char *what);

// What there is, oldest first.
uint8_t     u_log_count(void);
const char *u_log_line(uint8_t i);

// The events, from oled_master_task every main loop on the master: boot, the link up and
// down, a self-heal reset of the other half, and the OS set by detection or by SYS. A
// misfire is reported by the panel, which already finds the finger.
void u_log_watch(const u_split_state_t *now);
void u_log_misfire(uint8_t cells, uint8_t left);

// The page.
void u_log_draw(void);
