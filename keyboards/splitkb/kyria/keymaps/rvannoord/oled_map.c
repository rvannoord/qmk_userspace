// The live layer map behind oled_map.h. Geometry is the stage 4 mock's, as rendered for
// D15: the big grid's columns at 13, 38, 64, 90 and 115 with rows of 16 and single glyphs
// at 2x, and the Kyria-shaped map at 19 x 12 px a key unit on keyboard.json's stagger and
// thumb arc.

#include QMK_KEYBOARD_H
#include <string.h>
#include "manna-harbour_miryoku.h"
#include "oled_map.h"
#include "map_legend.h"
#include "oled_canvas.h"
#include "oled_draw.h"
#include "oled_layer_page.h"

#ifdef OLED_ENABLE

// --- where the keys are -----------------------------------------------------------
// Miryoku's 40 slots run three rows of ten, left pinky to left inner then right inner to
// right pinky, and a thumb row whose slots 32-34 are the left thumbs and 35-37 the right.
// LAYOUT_miryoku is what puts them in the matrix for keymaps[]; given slot numbers instead
// of keycodes, it puts each number where its key is, so the map cannot disagree with the
// keymap about either the thumb mapping or the right half's column order. Zero is a hole.

static const uint16_t slot_at[MATRIX_ROWS][MATRIX_COLS] = LAYOUT_miryoku(
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40);

#    define U_SLOTS 40

static keypos_t pos_of[U_SLOTS];
static bool     pos_found;

static void find_slots(void) {
    if (pos_found) return;
    pos_found = true;
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            const uint16_t v = slot_at[r][c];
            if (v >= 1 && v <= U_SLOTS) pos_of[v - 1] = (keypos_t){.col = c, .row = r};
        }
    }
}

// The 18 keys a half's map draws: 0-14 are the three rows, each from the pinky inwards,
// and 15-17 the thumbs from the outside in.
#    define U_MAP_KEYS 18

static uint8_t slot_of(bool left, uint8_t i) {
    if (i < 15) {
        const uint8_t r = i / 5, c = i % 5;
        return left ? (uint8_t)(r * 10 + c) : (uint8_t)(r * 10 + 9 - c);
    }
    return left ? (uint8_t)(32 + i - 15) : (uint8_t)(37 - (i - 15));
}

// --- which half --------------------------------------------------------------------

enum u_map_side u_map_side(uint8_t layer) {
    switch (layer) {
        case U_NAV:
        case U_MOUSE:
        case U_MEDIA:  return U_MAP_RIGHT;   // held with the left thumb
        case U_SYM:
        case U_NUM:
        case U_FUN:    return U_MAP_LEFT;    // held with the right thumb
        case U_BUTTON: return U_MAP_BOTH;    // held on a pinky, keys under both hands
        default:       return U_MAP_NONE;
    }
}

bool u_map_here(uint8_t layer, bool left) {
    const enum u_map_side s = u_map_side(layer);
    return s == U_MAP_BOTH || s == (left ? U_MAP_LEFT : U_MAP_RIGHT);
}

// Miryoku's own list, so a layer added there is named here too.
static const char *const layer_names[] = {
#    define MIRYOKU_X(LAYER, STRING) [U_##LAYER] = #LAYER,
    MIRYOKU_LAYER_LIST
#    undef MIRYOKU_X
};

const char *u_layer_name(uint8_t layer) {
    return layer < sizeof layer_names / sizeof layer_names[0] ? layer_names[layer] : "?";
}

// --- drawing -----------------------------------------------------------------------

// Single glyphs and icons go 2x in the big grid; anything with letters stays 1x. A pair
// of icons counts as one glyph, which is how the Mac's sun and triangle were approved.
static bool scales(u_legend_t g) {
    if (g.flags & U_LEG_ICON12) return true;
    const uint8_t n = u_legend_cells(g);
    if (n == 1) return true;
    for (uint8_t i = 0; i < n; i++) {
        if (g.cell[i] < 0x80) return false;
    }
    return n > 0;
}

// A legend centred on (cx, cy). Pressed, it is cut out of a box no wider or taller than
// its cell, so a press never spills onto the next key.
static void draw_legend(u_legend_t g, int16_t cx, int16_t cy, uint8_t k, bool pressed,
                        uint8_t cell_w, uint8_t cell_h) {
    const int16_t w = (int16_t)(u_legend_w(g) * k), h = (int16_t)(8 * k);
    if (pressed) {
        int16_t bw = w + 6 > 14 ? w + 6 : 14, bh = h + 4;
        if (bw > cell_w) bw = cell_w;
        if (bh > cell_h) bh = cell_h;
        u_rect((int16_t)(cx - bw / 2), (int16_t)(cy - bh / 2), bw, bh, true);
    }
    u_legend_at(g, (int16_t)(cx - w / 2), (int16_t)(cy - h / 2), k, !pressed);
}

// The layer's name, cut out of a filled tag.
static void draw_tag(const char *name, int16_t x0, int16_t y0) {
    u_rect(x0, y0, (int16_t)(6 * strlen(name) + 5), 10, true);
    u_text((int16_t)(x0 + 3), (int16_t)(y0 + 1), name, 1, false);
}

// --- the two styles ----------------------------------------------------------------

static const uint8_t grid_x[5] = {13, 38, 64, 90, 115};   // left half's columns, pinky first

// The Kyria-shaped map, left half: columns 19 px apart, the stagger in rows of 12 (0.75,
// 0.25, 0, 0.25 and 0.5 units), and the thumb arc keyed by matrix column, because that is
// what says which physical thumb key a slot is on: the five arc keys sit in columns 4, 2,
// 1, 5 and 0 of rows 3 and 7, from the outside in (keyboard.json). The right half mirrors.
static const uint8_t shape_x[5]    = {11, 30, 49, 68, 87};
static const uint8_t shape_drop[5] = {9, 3, 0, 3, 6};
static const uint8_t arc_x[6]      = {116, 78, 59, 0, 40, 97};
static const uint8_t arc_y[6]      = {56, 47, 44, 0, 44, 53};

void u_map_render(uint8_t layer, bool left, bool mac, bool shaped) {
    find_slots();
    memset(u_canvas, 0, OLED_MATRIX_SIZE);

    for (uint8_t i = 0; i < U_MAP_KEYS; i++) {
        const keypos_t   pos = pos_of[slot_of(left, i)];
        const u_legend_t g   = u_map_legend(keymap_key_to_keycode(layer, pos), mac);
        if (!u_legend_w(g)) continue;
        const bool pressed = matrix_is_on(pos.row, pos.col);

        int16_t cx, cy;
        if (!shaped) {
            // Thumbs under the three inner columns, the outermost thumb furthest out.
            const uint8_t col = i < 15 ? (uint8_t)(i % 5) : (uint8_t)(2 + (i - 15));
            cx = grid_x[left ? col : 4 - col];
            cy = i < 15 ? (int16_t)((i / 5) * 16 + 8) : 56;
            draw_legend(g, cx, cy, scales(g) ? 2 : 1, pressed, 25, 16);
        } else {
            if (i < 15) {
                cx = shape_x[i % 5];
                cy = (int16_t)((i / 5) * 12 + shape_drop[i % 5] + 7);
            } else {
                cx = arc_x[pos.col];
                cy = arc_y[pos.col];
            }
            if (!left) cx = (int16_t)(OLED_DISPLAY_WIDTH - 1 - cx);
            draw_legend(g, cx, cy, 1, pressed, 19, 12);
        }
    }

    // The name in the free thumb slots: outside the thumbs, on the half's outer edge.
    const char   *name = u_layer_name(layer);
    const int16_t w    = (int16_t)(6 * strlen(name) + 6);
    if (!shaped) {
        draw_tag(name, left ? 6 : (int16_t)(OLED_DISPLAY_WIDTH - 6 - w), 51);
    } else {
        draw_tag(name, left ? 4 : (int16_t)(OLED_DISPLAY_WIDTH - 4 - w), 50);
    }
}

// --- the Layer map page --------------------------------------------------------------

void u_map_page_render(uint8_t layer, uint8_t deflt, bool mac) {
    find_slots();
    uint16_t keys[U_PAGE_SLOTS], base[U_PAGE_SLOTS];
    uint64_t down = 0;
    for (uint8_t s = 0; s < U_PAGE_SLOTS; s++) {
        const keypos_t pos = pos_of[s];
        keys[s]            = keymap_key_to_keycode(layer, pos);
        base[s]            = keymap_key_to_keycode(deflt, pos);
        if (matrix_is_on(pos.row, pos.col)) down |= 1ull << s;
    }
    u_layer_page_draw(layer, u_layer_name(layer), keys, base, down, mac);
}

#endif     // OLED_ENABLE
