// WPM read as four speed tiers, shared by Bjorn on the slave and the kitty on the master
// (docs/oled-v3.md, D9), so both characters read speed the same way. Each caller keeps
// its own state; this only moves it.

#pragma once

#include <stdint.h>

// Deliberately low while the Colemak-DH migration is still settling; with higher
// boundaries the upper tiers would simply never be seen.
#define U_SPEED_TIER_DWELL_MS 800     // a tier hop repaints the character

typedef struct {
    uint8_t  tier;        // 0-3
    uint32_t changed;     // when it last moved
} u_speed_tier_t;

static inline void u_speed_tier_update(u_speed_tier_t *t, uint8_t wpm) {
    static const uint8_t up[3]   = {10, 30, 50};
    static const uint8_t down[3] = { 6, 24, 42};   // each strictly below its up

    if (timer_elapsed32(t->changed) < U_SPEED_TIER_DWELL_MS) return;

    const uint8_t was = t->tier;
    // while, not if: a jump from nothing to 80 WPM lands on the top tier in one tick.
    while (t->tier < 3 && wpm >= up[t->tier]) t->tier++;
    while (t->tier > 0 && wpm < down[t->tier - 1]) t->tier--;
    if (t->tier != was) t->changed = timer_read32();
}
