// The pixel drawing behind oled_draw.h.

#include "quantum.h"
#include <string.h>
#include "oled_draw.h"
#include "oled_canvas.h"
#include "art/panel_font.h"
#include "art/drashna_icons.h"

void u_put(int16_t x, int16_t y, bool on) {
    if (x < 0 || x >= OLED_DISPLAY_WIDTH || y < 0 || y >= OLED_DISPLAY_HEIGHT) return;
    uint8_t *b = &u_canvas[(uint16_t)(y >> 3) * OLED_DISPLAY_WIDTH + (uint16_t)x];
    if (on) {
        *b |= (uint8_t)(1u << (y & 7));
    } else {
        *b &= (uint8_t)~(1u << (y & 7));
    }
}

void u_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool on) {
    for (int16_t j = y; j < y + h; j++)
        for (int16_t i = x; i < x + w; i++) u_put(i, j, on);
}

// The art is page-aligned and its place on the panel is not, so each source byte lands
// across two canvas pages.
void u_blit(const uint8_t *src, uint8_t w, uint8_t pages, uint8_t x0, uint8_t y0) {
    const uint8_t sh = y0 & 7;
    for (uint8_t q = 0; q < pages; q++) {
        const uint16_t at = (uint16_t)((y0 >> 3) + q) * OLED_DISPLAY_WIDTH + x0;
        for (uint8_t i = 0; i < w; i++) {
            const uint16_t b = (uint16_t)(pgm_read_byte(&src[(uint16_t)q * w + i]) << sh);
            u_canvas[at + i] |= (uint8_t)b;
            if (sh) u_canvas[at + OLED_DISPLAY_WIDTH + i] |= (uint8_t)(b >> 8);
        }
    }
}

// --- legends -----------------------------------------------------------------------

static const uint8_t *glyph(uint8_t c) {
    return u_font8[((c < U_FONT_FIRST || c > U_FONT_LAST) ? U_FONT_FIRST : c) - U_FONT_FIRST];
}

// The columns a character inks, for U_LEG_TIGHT.
static void ink_of(uint8_t c, uint8_t *c0, uint8_t *c1) {
    const uint8_t *g = glyph(c);
    uint8_t        m = 0;
    for (uint8_t r = 0; r < 8; r++) m |= pgm_read_byte(&g[r]);
    uint8_t first = 8;
    *c1 = 0;
    for (uint8_t x = 0; x < 8; x++) {
        if (!(m & (0x80u >> x))) continue;
        if (first == 8) first = x;
        *c1 = x;
    }
    *c0 = first == 8 ? 0 : first;
}

uint8_t u_legend_cells(u_legend_t g) {
    uint8_t n = 0;
    while (n < 3 && g.cell[n]) n++;
    return n;
}

// Text cells are 6 apart with the last one's gap dropped; an icon brings its own blank
// column; tight characters are their ink, a pixel apart.
uint8_t u_legend_w(u_legend_t g) {
    if (g.flags & U_LEG_ICON12) return U_ICON12_W;
    const uint8_t n = u_legend_cells(g);
    if (n == 0) return 0;
    if (g.flags & U_LEG_TIGHT) {
        uint8_t w = (uint8_t)(n - 1), c0, c1;
        for (uint8_t i = 0; i < n; i++) {
            ink_of(g.cell[i], &c0, &c1);
            w = (uint8_t)(w + c1 - c0 + 1);
        }
        return w;
    }
    return (uint8_t)(6 * n - (g.cell[n - 1] < 0x80 ? 1 : 0));
}

static bool char_px(uint8_t c, uint8_t x, uint8_t y) {
    return pgm_read_byte(&glyph(c)[y]) & (0x80u >> x);
}

bool u_legend_px(u_legend_t g, uint8_t x, uint8_t y) {
    bool on = false;
    if (g.flags & U_LEG_ICON12) {
        on = (pgm_read_byte(&u_icon12[g.cell[0]][x]) >> y) & 1u;
    } else if (g.flags & U_LEG_TIGHT) {
        for (uint8_t i = 0, c0, c1; i < u_legend_cells(g); i++) {
            ink_of(g.cell[i], &c0, &c1);
            const uint8_t w = (uint8_t)(c1 - c0 + 1);
            if (x < w) {
                on = char_px(g.cell[i], (uint8_t)(c0 + x), y);
                break;
            }
            if (x == w) break;   // the pixel between two characters
            x = (uint8_t)(x - w - 1);
        }
    } else {
        const uint8_t c = g.cell[x / 6], sx = x % 6;
        on = c >= 0x80 ? (pgm_read_byte(&u_icon6[c - 0x80][sx]) >> y) & 1u : char_px(c, sx, y);
    }
    // Dimmed: every other pixel, the only grey a 1-bit panel has.
    if (on && (g.flags & U_LEG_DIM)) on = ((x + y) & 1u) == 0;
    return on;
}

void u_legend_at(u_legend_t g, int16_t x, int16_t y, uint8_t k, bool on) {
    const int16_t w = (int16_t)(u_legend_w(g) * k), h = (int16_t)(8 * k);
    for (int16_t j = 0; j < h; j++)
        for (int16_t i = 0; i < w; i++)
            if (u_legend_px(g, (uint8_t)(i / k), (uint8_t)(j / k))) u_put(x + i, y + j, on);
}

// --- text --------------------------------------------------------------------------

uint8_t u_text_w(const char *s) {
    const uint8_t n = (uint8_t)strlen(s);
    return n ? (uint8_t)(6 * n - 1) : 0;
}

void u_text(int16_t x, int16_t y, const char *s, uint8_t k, bool on) {
    for (uint8_t i = 0; s[i]; i++) {
        u_legend_at((u_legend_t){{(uint8_t)s[i]}, 0}, (int16_t)(x + 6 * k * i), y, k, on);
    }
}

void u_text_bar(int16_t x, int16_t y, const char *s) {
    u_rect((int16_t)(x - 1), (int16_t)(y - 1), (int16_t)(u_text_w(s) + 2), 9, true);
    u_text(x, y, s, 1, false);
}
