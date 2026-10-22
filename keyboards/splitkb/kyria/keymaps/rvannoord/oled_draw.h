// Pixel drawing into u_canvas, for what sits at any row rather than on a page: the live
// layer map and stage 5's pages. Text is Ein (art/panel_font.c), a character every 6 px;
// legends are map_legend.h's. Nothing here touches the driver: the caller commits.

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "map_legend.h"

// Off the panel is ignored, so a box or a word may run over an edge.
void u_put(int16_t x, int16_t y, bool on);
void u_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool on);

// A page-format image (one byte a column, bit 0 the top row) ORed in at any row. It must
// fit on the panel: this is for art placed by hand, and copies bytes, not pixels.
void u_blit(const uint8_t *src, uint8_t w, uint8_t pages, uint8_t x0, uint8_t y0);
// The same, pixel by pixel and clipped, for a sprite that walks off an edge.
void u_sprite(const uint8_t *src, uint8_t w, uint8_t pages, int16_t x0, int16_t y0);

// A legend's cells, its width at 1x, one of its pixels, and the whole of it with its top
// left at (x, y), k times size, lit or cut out of what is there.
uint8_t u_legend_cells(u_legend_t g);
uint8_t u_legend_w(u_legend_t g);
bool    u_legend_px(u_legend_t g, uint8_t x, uint8_t y);
void    u_legend_at(u_legend_t g, int16_t x, int16_t y, uint8_t k, bool on);

// A word is 6n - 1 wide: the last character's gap is dropped.
uint8_t u_text_w(const char *s);
void    u_text(int16_t x, int16_t y, const char *s, uint8_t k, bool on);
// Lit on dark: a bar one pixel round the ink, the word cut out of it.
void u_text_bar(int16_t x, int16_t y, const char *s);
// `v` right-aligned in three places, the way WPM is printed: four bytes with the end.
void u_num3(char *b, uint8_t v);
