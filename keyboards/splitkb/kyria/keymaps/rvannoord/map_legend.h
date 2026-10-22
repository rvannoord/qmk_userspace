// What a key's legend is on the live layer map: up to three cells, each a character in
// Ein or one of drashna's 6x8 icons, or one of his 12x8 modifier icons. Pure, so the
// scenario tests can run every key of every map layer through it. Design in
// docs/layout.md, "The live layer map"; the Mac legends are docs/oled-v3.md's D15.

#pragma once

#include <stdbool.h>
#include <stdint.h>

// A cell below 0x80 is a character in Ein; from 0x80 it is u_icon6[cell - 0x80]. A zero
// cell ends the legend, so a legend of all zeros draws nothing.
#define U_LEG_ICON6(i) ((uint8_t)(0x80u | (i)))

#define U_LEG_ICON12  0x01    // cell[0] is a u_icon12 index: the legend is that one icon
#define U_LEG_DIM     0x02    // every other pixel: a key that does nothing on this host
#define U_LEG_UNKNOWN 0x04    // not in the table; drawn as a dot, and the tests fail on it

typedef struct {
    uint8_t cell[3];
    uint8_t flags;
} u_legend_t;

u_legend_t u_map_legend(uint16_t keycode, bool mac);
