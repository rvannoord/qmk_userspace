// The live layer map: while a thumb layer is held, the panel over the keys that layer is
// for shows what each of them does. Design in docs/layout.md, "The live layer map"; build
// order in docs/oled-v3.md, stage 4.

#pragma once

#include <stdbool.h>
#include <stdint.h>

enum u_map_side { U_MAP_NONE, U_MAP_LEFT, U_MAP_RIGHT, U_MAP_BOTH };

// Bit 4 of the stored view byte (oled_settings.h), which also crosses the link in `view`.
#define U_VIEW_MAP_SHAPED 0x10u

// Which panels a layer's map fills. The one answer both the drawing and the PAGE key ask.
enum u_map_side u_map_side(uint8_t layer);
bool            u_map_here(uint8_t layer, bool left);

// On the half without the cable: the master's map covers its own panel and not this one,
// so the master's screen moves over here (docs/layout.md, "The master's screen moves over").
bool u_map_moves_master(uint8_t layer, bool this_left);

// This half's map of `layer`, composed into the canvas; the caller commits. Legends follow
// `mac`; pressed keys come from this half's own matrix.
void u_map_render(uint8_t layer, bool left, bool mac, bool shaped);
