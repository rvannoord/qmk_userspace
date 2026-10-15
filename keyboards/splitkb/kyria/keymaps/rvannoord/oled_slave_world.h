// The world layer: weather across the whole slave panel, behind everything.
#pragma once

#include <stdbool.h>
#include <stdint.h>

// Which field is on screen. Driven by the season today; by the host when the
// daemon exists, with the season as the fallback that is never wrong.
enum u_world_state {
    U_WORLD_NONE,
    U_WORLD_RAIN,
    U_WORLD_SNOW,
    U_WORLD_LEAVES,
    U_WORLD_STARS,
};

// Per main loop. Advances the scroll and the leaves on their own clocks.
void u_world_task(void);

// Draws the field over the whole panel, cut out around Bjorn. Call AFTER he is
// composed: it reads his pixels out of the canvas to find his outline.
void u_world_draw(void);

enum u_world_state u_world_current(void);
