// The goat: infinition's, redrawn at panel size, wandering the right half while the
// keyboard is idle. Design in docs/layout.md, "The goat, designed 2026-10-23".

#pragma once

#include <stdbool.h>
#include <stdint.h>

// One render step. `goat_pose` says Bjorn is in one of his drawings with a goat in it, when
// this one must not come in; `dead` is the link gone hard stale.
void u_goat_step(uint32_t idle_ms, bool dead, bool goat_pose);

// Into the canvas, after Bjorn and the floor and before the weather.
void u_goat_draw(void);

// Its filled silhouette, one row a uint64_t with bit 0 at x=64, for the weather to stop at.
void u_goat_silhouette(uint64_t rows[64]);

// On screen, at least in part; Bjorn skips his goat drawings while it is.
bool u_goat_present(void);
