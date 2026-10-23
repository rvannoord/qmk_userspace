// The idle show's director: one clock for the half without the cable, the times things
// may happen on it, and the rule that only one thing starts in a step. Design in
// docs/layout.md, "The goat, designed 2026-10-23"; the older plan's director is
// docs/oled-v2.md, stage 4.

#pragma once

#include <stdbool.h>
#include <stdint.h>

// On last_input_activity_elapsed(), which SPLIT_ACTIVITY_ENABLE makes the master's clock too.
#define U_IDLE_SETTLE_MS      8000     // Bjorn settles; the goat's tail and ear may move
#define U_IDLE_ACTORS_MS     20000     // Bjorn's drawings and the goat may begin
#define U_IDLE_DOZE_MS       75000     // Bjorn dozes and the goat curls up
#define U_IDLE_LAST_START_MS 90000     // nothing new starts
#define U_IDLE_FREEZE_MS     100000    // nothing moves: the rain covers the scene (U_RAIN_AFTER_MS)

// Once a render step, before any actor: opens the step. A key starts a new idle period,
// which stirs the stream with how long the last one lasted, so no two periods are alike
// and a preview is still repeatable.
void u_director_step(uint32_t idle_ms);

// The one actor that may start something this step gets true; any later asker waits a step.
// Bjorn asks first, so his cut to a new drawing wins over the goat.
bool u_director_claim(void);

// The director's own stream, apart from Bjorn's blink: a goat's chance never moves a blink.
uint16_t u_director_rand(uint16_t span);
