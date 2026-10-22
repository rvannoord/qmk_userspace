// The Layer map page behind oled_layer_page.h, at the geometry Roel approved from the
// stage 5 mock: alpha keys on a 12 px pitch in rows 11 apart, a dotted rule at y=47, and
// the thumbs at 20 and 21 px under it.

#include "quantum.h"
#include <string.h>
#include "oled_layer_page.h"
#include "map_legend.h"
#include "oled_canvas.h"
#include "oled_draw.h"

static bool held_key(uint8_t layer, uint16_t base, bool down) {
    return down && IS_QK_LAYER_TAP(base) && QK_LAYER_TAP_GET_LAYER(base) == layer;
}

void u_layer_page_draw(uint8_t layer, const char *name, const uint16_t keys[U_PAGE_SLOTS],
                       const uint16_t base[U_PAGE_SLOTS], uint64_t down, bool mac) {
    memset(u_canvas, 0, OLED_MATRIX_SIZE);

    u_rect(0, 0, OLED_DISPLAY_WIDTH, 9, true);
    u_text((int16_t)(OLED_DISPLAY_WIDTH / 2 - u_text_w(name) / 2), 1, name, 1, false);

    // One cell a key, centred on the cell a character would take.
    for (uint8_t s = 0; s < 30; s++) {
        const uint8_t    c    = s % 10;
        const bool       on   = (down >> s) & 1u;
        const bool       held = held_key(layer, base[s], on);
        const u_legend_t g    = u_map_short(held ? base[s] : keys[s], mac);
        const uint8_t    w    = u_legend_w(g);
        if (!w) continue;
        const int16_t x  = c < 5 ? (int16_t)(4 + c * 12) : (int16_t)(70 + (c - 5) * 12);
        const int16_t y  = (int16_t)(13 + (s / 10) * 11);
        const int16_t gx = (int16_t)(x + 2 - (w - 5) / 2);
        if (on) u_rect((int16_t)(gx - 2), (int16_t)(y - 2), (int16_t)(w + 4), 11, true);
        u_legend_at(g, gx, y, 1, !on);
    }

    for (int16_t x = 2; x < OLED_DISPLAY_WIDTH - 2; x += 2) u_put(x, 47, true);

    // The thumbs keep their full legends, set from the left of each key.
    for (uint8_t i = 0; i < 6; i++) {
        const uint8_t    s    = (uint8_t)(32 + i);
        const bool       on   = (down >> s) & 1u;
        const bool       held = held_key(layer, base[s], on);
        const u_legend_t g    = u_map_legend(held ? base[s] : keys[s], mac);
        if (!u_legend_w(g)) continue;
        const int16_t x = i < 3 ? (int16_t)(4 + i * 20) : (int16_t)(66 + (i - 3) * 21);
        if (on) u_rect((int16_t)(x - 1), 51, 20, 11, true);
        u_legend_at(g, x, 53, 1, !on);
    }
}
