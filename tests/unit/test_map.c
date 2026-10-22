// The live layer map, compiled from the real oled_map.c against the keymap's own
// LAYOUT_miryoku. What it checks: every key's matrix position against the plan's table,
// which half each layer draws on, the right half's column order, that a pressed key's box
// stays inside its cell, that a redraw is free, and that no map outshines the alarm.
// --raw emits the Fun and Nav maps exactly as D15 was decided from, for a byte compare.
#include <string.h>
#include "unit.h"
#include "qmk_keyboard.h"
#include "driver.h"
#include "map_legend.h"
#include "art/drashna_icons.h"

uint8_t  fake_oled[OLED_MATRIX_SIZE + 1];
uint16_t fake_dirty;
uint32_t fake_now;
bool     fake_is_left = true;

void oled_write_raw_byte(const char data, uint16_t index) { u_fake_write(data, index); }

// The keymap: a key's code says where it is, so a legend can be chosen per position.
uint16_t keymap_key_to_keycode(uint8_t layer, keypos_t key) {
    return (uint16_t)((layer << 8) | (key.row << 4) | key.col) + 1;
}

static bool pressed[MATRIX_ROWS][MATRIX_COLS];
bool        matrix_is_on(uint8_t row, uint8_t col) { return pressed[row][col]; }

// Each test picks its legends with this.
static u_legend_t (*legend_at)(uint8_t layer, keypos_t pos, bool mac);
u_legend_t u_map_legend(uint16_t keycode, bool mac) {
    const uint16_t k = (uint16_t)(keycode - 1);
    return legend_at((uint8_t)(k >> 8), (keypos_t){.col = (uint8_t)(k & 0xF), .row = (uint8_t)((k >> 4) & 0xF)}, mac);
}
// The Layer map page is test_panel.c's and the scenario tests'; the map only links it.
u_legend_t u_map_short(uint16_t keycode, bool mac) { return u_map_legend(keycode, mac); }

#include "oled_map.c"

// --- the plan's position table (docs/oled-v3.md, stage 4), written out independently -----

// Alpha key i (0-14: rows of five from the pinky inwards) or thumb 15-17 (outside in).
static keypos_t plan_pos(bool left, uint8_t i) {
    if (i < 15) return (keypos_t){.col = (uint8_t)(5 - i % 5), .row = (uint8_t)((left ? 0 : 4) + i / 5)};
    static const uint8_t thumb_col[3] = {1, 5, 0};   // positions 3, 4, 5
    return (keypos_t){.col = thumb_col[i - 15], .row = left ? 3 : 7};
}

static int8_t plan_index(bool left, keypos_t p) {
    for (uint8_t i = 0; i < U_MAP_KEYS; i++) {
        const keypos_t q = plan_pos(left, i);
        if (q.row == p.row && q.col == p.col) return (int8_t)i;
    }
    return -1;
}

// --- legends ----------------------------------------------------------------

#define TXT(a, b, c) ((u_legend_t){{a, b, c}, 0})
#define ICO(i)       U_LEG_ICON6(U_ICON6_##i)

// The Fun and Nav maps as D15 was decided from (d15_maps.py), keyed by the plan's index.
static u_legend_t d15(uint8_t layer, keypos_t pos, bool mac) {
    static const char *fun[18] = {"F12", "F7", "F8", "F9", "PSc", "F11", "F4", "F5", "F6", "ScL",
                                  "F10", "F1", "F2", "F3", "Pau", "App", "Spc", "Tab"};
    static const char *nav[18] = {"Und", "Cut", "Cpy", "Pst", "Rdo", ">", "^", "v", "<", "CW",
                                  "End", "PgU", "PgD", "Hom", "Ins", "Del", "Bsp", "Ent"};
    const bool   left = layer == U_FUN;
    const int8_t i    = plan_index(left, pos);
    if (i < 0) return TXT(0, 0, 0);
    const char *s = (left ? fun : nav)[i];
    if (mac && left && i == 4) return TXT('F', '1', '3');
    if (mac && left && i == 9) return TXT(ICO(SUN), ICO(TRI_DOWN), 0);
    if (mac && left && i == 14) return TXT(ICO(SUN), ICO(TRI_UP), 0);
    if (mac && left && i == 15) return (u_legend_t){{'A', 'p', 'p'}, U_LEG_DIM};
    if (mac && !left && i == 14) return TXT('H', 'l', 'p');
    if (!s[1]) {
        switch (s[0]) {
            case '<': return TXT(ICO(ARROW_LEFT), 0, 0);
            case '>': return TXT(ICO(ARROW_RIGHT), 0, 0);
            case '^': return TXT(ICO(ARROW_UP), 0, 0);
            case 'v': return TXT(ICO(ARROW_DOWN), 0, 0);
        }
    }
    return TXT((uint8_t)s[0], (uint8_t)s[1], (uint8_t)(s[1] ? s[2] : 0));
}

static u_legend_t only;            // the one legend `single` draws
static int8_t     only_at = -1;    // at this plan index, on the half being drawn
static bool       only_left;
static u_legend_t single(uint8_t layer, keypos_t pos, bool mac) {
    (void)layer; (void)mac;
    return plan_index(only_left, pos) == only_at ? only : TXT(0, 0, 0);
}

// Sym and Num as Miryoku has them, keyed by the plan's index (left half, pinky first).
static u_legend_t sym_num(uint8_t layer, keypos_t pos, bool mac) {
    (void)mac;
    static const char sym[] = "{&*(}:$%^+~!@#|()_";
    static const char num[] = "[789];456=`123\\.0-";
    const int8_t i = plan_index(true, pos);
    if (i < 0) return TXT(0, 0, 0);
    return TXT((uint8_t)(layer == U_SYM ? sym : num)[i], 0, 0);
}

static u_legend_t every(uint8_t layer, keypos_t pos, bool mac) {
    (void)layer; (void)pos; (void)mac;
    return TXT('M', 'M', 'B');      // as wide as text gets
}

// --- looking at the canvas --------------------------------------------------

static bool lit(const uint8_t *f, int x, int y) {
    return (f[(y / 8) * OLED_DISPLAY_WIDTH + x] >> (y % 8)) & 1;
}

static int count(const uint8_t *f) {
    int n = 0;
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 128; x++) n += lit(f, x, y);
    return n;
}

typedef struct { int x0, y0, x1, y1; } box_t;

// Where two frames differ.
static box_t diff(const uint8_t *a, const uint8_t *b) {
    box_t r = {128, 64, -1, -1};
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 128; x++)
            if (lit(a, x, y) != lit(b, x, y)) {
                if (x < r.x0) r.x0 = x;
                if (y < r.y0) r.y0 = y;
                if (x > r.x1) r.x1 = x;
                if (y > r.y1) r.y1 = y;
            }
    return r;
}

// Where a key's legend is centred, from the renderer's own geometry.
static void centre(bool left, bool shaped, uint8_t i, int *cx, int *cy) {
    const keypos_t p = plan_pos(left, i);
    if (!shaped) {
        const uint8_t col = i < 15 ? i % 5 : (uint8_t)(2 + i - 15);
        *cx = grid_x[left ? col : 4 - col];
        *cy = i < 15 ? (i / 5) * 16 + 8 : 56;
    } else {
        *cx = i < 15 ? shape_x[i % 5] : arc_x[p.col];
        *cy = i < 15 ? (i / 5) * 12 + shape_drop[i % 5] + 7 : arc_y[p.col];
        if (!left) *cx = 127 - *cx;
    }
}

static uint8_t frame[OLED_MATRIX_SIZE];

static void draw(uint8_t layer, bool left, bool mac, bool shaped) {
    u_map_render(layer, left, mac, shaped);
    memcpy(frame, u_canvas, OLED_MATRIX_SIZE);
}

int main(int argc, char **argv) {
    // --raw: Fun and Nav, grid then shaped, Windows then Mac, as d15_maps.py lays them out.
    if (argc > 1 && strcmp(argv[1], "--raw") == 0) {
        legend_at = d15;
        for (uint8_t L = 0; L < 2; L++)
            for (uint8_t shaped = 0; shaped < 2; shaped++)
                for (uint8_t mac = 0; mac < 2; mac++) {
                    draw(L ? U_NAV : U_FUN, L == 0, mac, shaped);
                    fwrite(frame, 1, OLED_MATRIX_SIZE, stdout);
                }
        // Then Sym and Num, grid then shaped.
        legend_at = sym_num;
        for (uint8_t L = 0; L < 2; L++)
            for (uint8_t shaped = 0; shaped < 2; shaped++) {
                draw(L ? U_NUM : U_SYM, true, false, shaped);
                fwrite(frame, 1, OLED_MATRIX_SIZE, stdout);
            }
        return 0;
    }

    // Every key where the plan's table says it is, found through LAYOUT_miryoku.
    find_slots();
    bool where = true;
    for (uint8_t half = 0; half < 2; half++)
        for (uint8_t i = 0; i < U_MAP_KEYS; i++) {
            const keypos_t a = pos_of[slot_of(half == 0, i)], b = plan_pos(half == 0, i);
            if (a.row != b.row || a.col != b.col) {
                printf("        %s key %u at [%u,%u], the plan says [%u,%u]\n", half ? "right" : "left", i,
                       a.row, a.col, b.row, b.col);
                where = false;
            }
        }
    check("every key the map draws is where the plan's table puts it", where);

    // Which half draws which layer.
    check("Nav, Mouse and Media draw on the right only",
          u_map_here(U_NAV, false) && !u_map_here(U_NAV, true) && u_map_here(U_MOUSE, false) &&
          !u_map_here(U_MOUSE, true) && u_map_here(U_MEDIA, false) && !u_map_here(U_MEDIA, true));
    check("Sym, Num and Fun on the left only",
          u_map_here(U_SYM, true) && !u_map_here(U_SYM, false) && u_map_here(U_NUM, true) &&
          !u_map_here(U_NUM, false) && u_map_here(U_FUN, true) && !u_map_here(U_FUN, false));
    check("Button on both", u_map_here(U_BUTTON, true) && u_map_here(U_BUTTON, false));
    check("and base, Extra and Tap on neither",
          u_map_side(U_BASE) == U_MAP_NONE && u_map_side(U_EXTRA) == U_MAP_NONE && u_map_side(U_TAP) == U_MAP_NONE);

    // The right half runs inner to pinky, left to right: its inner column is drawn leftmost.
    legend_at = single;
    only      = TXT('X', 0, 0);
    only_left = false;
    {
        // Against the same map with no legends, so the name tag drops out of the diff.
        uint8_t bare[OLED_MATRIX_SIZE];
        only_at = -1;
        draw(U_NAV, false, false, false);
        memcpy(bare, frame, sizeof bare);
        only_at = 4;      // top row, inner
        draw(U_NAV, false, false, false);
        const box_t inner = diff(frame, bare);
        only_at = 0;      // top row, pinky
        draw(U_NAV, false, false, false);
        const box_t pinky = diff(frame, bare);
        check("on the right, the inner column is drawn on the left and the pinky on the right",
              inner.x1 < 26 && pinky.x0 > 102);
        only_at = -1;
        draw(U_NAV, false, false, true);
        memcpy(bare, frame, sizeof bare);
        only_at = 17;     // the innermost thumb, Enter on Nav
        draw(U_NAV, false, false, true);
        const box_t ent = diff(frame, bare);
        check("and its innermost thumb sits at the inner end of the arc", ent.x1 < 20 && ent.y0 > 48);
    }

    // QMK's @ is drawn whole, its left column included; ZXPIX's | sits on the key's centre.
    {
        uint8_t bare[OLED_MATRIX_SIZE];
        only_left = true;
        only_at   = -1;
        draw(U_SYM, true, false, false);
        memcpy(bare, frame, sizeof bare);
        only_at = 12;                    // the @ key: bottom row, middle finger
        only    = TXT('@', 0, 0);
        draw(U_SYM, true, false, false);
        const box_t at = diff(frame, bare);
        int cx, cy;
        centre(true, false, 12, &cx, &cy);
        check("the @ is drawn whole, five columns at 2x", at.x1 - at.x0 + 1 == 10);
        check("and centred on its key", at.x0 == cx - 5);
        only_at = 14;                    // the | key: bottom row, inner
        only    = TXT('|', 0, 0);
        draw(U_SYM, true, false, false);
        const box_t bar = diff(frame, bare);
        centre(true, false, 14, &cx, &cy);
        check("the | is one unbroken bar on its key's centre",
              bar.x1 - bar.x0 + 1 == 2 && bar.x0 >= cx - 2 && bar.x1 <= cx + 1 && bar.y1 - bar.y0 + 1 == 12);
    }

    // A press lights a box no bigger than its cell, with the widest legends there are: the
    // Mac's sun and triangle at 2x in the grid, three letters at 1x in the shaped map.
    {
        const u_legend_t wide[2] = {TXT(ICO(SUN), ICO(TRI_DOWN), 0), TXT('M', 'M', 'B')};
        bool inside = true;
        for (uint8_t shaped = 0; shaped < 2; shaped++)
            for (uint8_t half = 0; half < 2; half++)
                for (uint8_t i = 0; i < U_MAP_KEYS; i++) {
                    only      = wide[shaped];
                    only_at   = (int8_t)i;
                    only_left = half == 0;
                    const uint8_t layer = half == 0 ? U_FUN : U_NAV;
                    draw(layer, half == 0, false, shaped);
                    uint8_t idle[OLED_MATRIX_SIZE];
                    memcpy(idle, frame, sizeof idle);
                    const keypos_t p = plan_pos(half == 0, i);
                    pressed[p.row][p.col] = true;
                    draw(layer, half == 0, false, shaped);
                    pressed[p.row][p.col] = false;
                    const box_t b = diff(frame, idle);
                    int cx, cy;
                    centre(half == 0, shaped, i, &cx, &cy);
                    const int hw = shaped ? 19 / 2 : 25 / 2, hh = shaped ? 12 / 2 : 16 / 2;
                    if (b.x1 < 0 || b.x0 < cx - hw || b.x1 > cx + hw || b.y0 < cy - hh || b.y1 > cy + hh - 1) {
                        printf("        %s %s key %u: box x %d-%d y %d-%d, cell centred on (%d,%d)\n",
                               shaped ? "shaped" : "grid", half ? "right" : "left", i, b.x0, b.x1, b.y0, b.y1, cx, cy);
                        inside = false;
                    }
                }
        check("a pressed key's box stays inside its own cell, every key, both styles", inside);
    }

    // Dimmed is every other pixel; unknown is a dot; a blank key draws nothing at all.
    {
        only_left = true;
        only_at   = -1;
        draw(U_FUN, true, false, false);
        const int tag = count(frame);       // the name tag, which every map has
        only_at   = 15;
        only      = TXT('A', 'p', 'p');
        draw(U_FUN, true, false, false);
        const int full = count(frame) - tag;
        only = (u_legend_t){{'A', 'p', 'p'}, U_LEG_DIM};
        draw(U_FUN, true, false, false);
        const int dimmed = count(frame) - tag;
        printf("        App lights %d px, dimmed %d\n", full, dimmed);
        // A checkerboard over strokes one pixel wide lands near half, not on it.
        check("a dimmed legend lights clearly fewer pixels, and still some", dimmed * 4 > full && dimmed * 4 < full * 3);
        only = (u_legend_t){{U_LEG_ICON6(U_ICON6_BULLET)}, U_LEG_UNKNOWN};
        draw(U_FUN, true, false, false);
        check("an unknown key still draws something", count(frame) > tag);
    }

    // A redraw is free, and nothing a map draws outshines the alarm (4,584 px in
    // test_panel.c), with every key wide and four of them held.
    {
        legend_at = every;
        bool free = true, under = true;
        int  worst = 0;
        static const uint8_t layers[] = {U_NAV, U_MOUSE, U_MEDIA, U_SYM, U_NUM, U_FUN, U_BUTTON};
        for (uint8_t L = 0; L < sizeof layers; L++)
            for (uint8_t half = 0; half < 2; half++) {
                if (!u_map_here(layers[L], half == 0)) continue;
                for (uint8_t shaped = 0; shaped < 2; shaped++) {
                    for (uint8_t k = 0; k < 4; k++) {
                        const keypos_t p = plan_pos(half == 0, (uint8_t)(k * 4));
                        pressed[p.row][p.col] = true;
                    }
                    u_map_render(layers[L], half == 0, false, shaped);
                    u_canvas_commit();
                    fake_dirty = 0;
                    u_map_render(layers[L], half == 0, false, shaped);
                    if (u_canvas_commit() != 0 || fake_dirty != 0) free = false;
                    const int n = count(u_canvas);
                    if (n > worst) worst = n;
                    if (n >= 4584) under = false;
                    memset(pressed, 0, sizeof pressed);
                }
            }
        printf("        the brightest map lights %d px\n", worst);
        check("every map redraws for free", free);
        check("and none outshines the alarm", under);
    }

    check("the driver's one-past-the-end byte was never touched", fake_oled[OLED_MATRIX_SIZE] == 0);
    return unit_done("map");
}
