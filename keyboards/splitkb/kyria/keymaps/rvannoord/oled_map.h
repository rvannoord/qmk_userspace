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

// This half's map of `layer`, composed into the canvas; the caller commits. Legends follow
// `mac`; pressed keys come from this half's own matrix.
void u_map_render(uint8_t layer, bool left, bool mac, bool shaped);

// Miryoku's own name for a layer, from its list: "BASE", "NAV" and so on.
const char *u_layer_name(uint8_t layer);

// The Layer map page of `layer`, from the keymap and the whole matrix, which the master
// has: the live map's slots drawn by oled_layer_page.c. `deflt` is the default layer.
void u_map_page_render(uint8_t layer, uint8_t deflt, bool mac);
