// The director behind oled_slave_director.h.

#include QMK_KEYBOARD_H
#include "oled_slave_director.h"
#include "oled.h"

_Static_assert(U_IDLE_FREEZE_MS == U_RAIN_AFTER_MS, "the scene stops where the rain starts");

static bool     claimed;
static uint32_t rng = 0x2545f491u;
static uint32_t idle_was;

void u_director_step(uint32_t idle_ms) {
    claimed = false;
    if (idle_ms < idle_was) rng ^= 0x9e3779b9u * (idle_was | 1u);   // a key: a new period
    idle_was = idle_ms;
}

bool u_director_claim(void) {
    if (claimed) return false;
    claimed = true;
    return true;
}

uint16_t u_director_rand(uint16_t span) {
    rng = rng * 1664525u + 1013904223u;
    return span ? (uint16_t)((rng >> 16) % span) : 0;
}
