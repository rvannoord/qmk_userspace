// The master-side panel, in zones rather than lines of text. Dark at rest, so any
// light means something happened. Four page-aligned zones; see docs/layout.md,
// "Master half", for what each one is for and why.
//
//   page 0    status bar   WIN/MAC badge, underlined when SYS set it by hand
//   pages 1-2 mod row      four 32 px cells, one per finger, inverted when live
//   pages 3-4 layer band   the layer name at 2x, centred, blank on base
//   page 5    separator    always dark
//   pages 6-7 event strip  only the icons whose condition holds, centred as a group

#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"
#include "oled_panel.h"
#include "host_os.h"
#include "split_state.h"
#include "split_sync.h"
#include "panel_font.h"

#ifdef OLED_MASTER_PANEL_ZONES

// Miryoku's own layer list, so the order lives in one place. Overriding
// MIRYOKU_LAYER_LIST in custom_config.h changes these.
static const char *const layer_names[] = {
#    define MIRYOKU_X(LAYER, STRING) [U_##LAYER] = STRING,
    MIRYOKU_LAYER_LIST
#    undef MIRYOKU_X
};

// Rebuilt every main loop by oled_master_task, so a modifier held and released
// between two 20 Hz frames still reaches the panel.
static u_split_state_t panel;

// --- the buffer -------------------------------------------------------------
// 8 pages of 128 columns, each byte a vertical strip of 8 pixels with bit 0 at the
// top. Raw bytes rather than oled_write, because a zone has to clear exactly its own
// area: text drawing leaves whatever was to the right of it in place. Unchanged bytes
// cost nothing — oled_write_raw_byte returns early and leaves the block clean.

static void fill(uint8_t page, uint8_t x, uint8_t w, uint8_t byte) {
    for (uint8_t i = 0; i < w; i++) {
        oled_write_raw_byte((char)byte, (uint16_t)page * OLED_DISPLAY_WIDTH + x + i);
    }
}

// One column of an 8x8 glyph, as the page byte that holds it.
static uint8_t col_1x(const uint8_t *g, uint8_t sx) {
    uint8_t out = 0;
    for (uint8_t r = 0; r < 8; r++) {
        if (pgm_read_byte(&g[r]) & (0x80u >> sx)) out |= 1u << r;
    }
    return out;
}

// The same column doubled: half q of the 16 rows, so each source row lights two.
static uint8_t col_2x(const uint8_t *g, uint8_t sx, uint8_t q) {
    uint8_t out = 0;
    for (uint8_t k = 0; k < 8; k++) {
        if (pgm_read_byte(&g[q * 4 + (k >> 1)]) & (0x80u >> sx)) out |= 1u << k;
    }
    return out;
}

static const uint8_t *glyph_of(char c) {
    const uint8_t u = (uint8_t)c;
    return u_font8[(u < U_FONT_FIRST || u > U_FONT_LAST) ? 0 : u - U_FONT_FIRST];
}

static void draw_glyph_1x(uint8_t x, uint8_t page, const uint8_t *g, uint8_t ink) {
    for (uint8_t sx = 0; sx < 8; sx++) {
        oled_write_raw_byte((char)(col_1x(g, sx) ^ ink),
                            (uint16_t)page * OLED_DISPLAY_WIDTH + x + sx);
    }
}

static void draw_glyph_2x(uint8_t x, uint8_t page, const uint8_t *g, uint8_t ink) {
    for (uint8_t q = 0; q < 2; q++) {
        for (uint8_t sx = 0; sx < 8; sx++) {
            const uint8_t b = col_2x(g, sx, q) ^ ink;
            const uint16_t at = (uint16_t)(page + q) * OLED_DISPLAY_WIDTH + x + sx * 2;
            oled_write_raw_byte((char)b, at);
            oled_write_raw_byte((char)b, at + 1);
        }
    }
}

static void draw_text_1x(uint8_t x, uint8_t page, const char *s, uint8_t ink) {
    for (; *s; s++, x += 8) {
        draw_glyph_1x(x, page, glyph_of(*s), ink);
    }
}

static void draw_text_2x(uint8_t x, uint8_t page, const char *s, uint8_t ink) {
    for (; *s; s++, x += 16) {
        draw_glyph_2x(x, page, glyph_of(*s), ink);
    }
}

static uint8_t text_len(const char *s) {
    uint8_t n = 0;
    while (*s++) n++;
    return n;
}

// --- status bar -------------------------------------------------------------

static void render_status(void) {
    const char *badge = (panel.flags & U_FLAG_IS_MAC) ? "MAC" : "WIN";

    fill(0, 0, OLED_DISPLAY_WIDTH, 0x00);
    draw_text_1x(0, 0, badge, 0x00);
    if (panel.flags & U_FLAG_OS_MANUAL) {
        // Row 7 under the badge: detection was overridden by hand, so the badge is a
        // decision rather than a reading. Redrawing the glyph columns with bit 7 set
        // keeps it one pass, and none of the six letters the badge can hold reaches
        // row 7 on its own.
        for (uint8_t x = 0; x < 24; x++) {
            oled_write_raw_byte((char)(0x80u | col_1x(glyph_of(badge[x / 8]), x % 8)), x);
        }
    }
}

// --- mod row ----------------------------------------------------------------
// A cell is a finger, outer to inner: pinky, ring, middle, index. Not a modifier —
// the slot never moves, only the label follows the OS, which is why this used to
// light the wrong cell on the Mac where Ctrl and GUI swap.

static void render_mods(void) {
    // Interim art: one letter at 2x fills the same 16x16 the drawn glyph will.
    static const char win[4] = {'G', 'A', 'C', 'S'};
    static const char mac[4] = {'C', 'O', 'M', 'S'};
    const char   *label = (panel.flags & U_FLAG_IS_MAC) ? mac : win;
    const uint8_t cells = U_FIN_CELLS(panel.fingers);

    fill(1, 0, OLED_DISPLAY_WIDTH, 0x00);
    fill(2, 0, OLED_DISPLAY_WIDTH, 0x00);

    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t x   = i * 32;
        const uint8_t ink = (cells & (1u << i)) ? 0xff : 0x00;
        if (ink) {
            fill(1, x, 32, 0xff);
            fill(2, x, 32, 0xff);
        }
        draw_glyph_2x(x + 8, 1, glyph_of(label[i]), ink);

        // Handedness lives in the padding: three rows at the middle of the cell's own
        // side. Four cells for eight states, because the hold decision compares the
        // hands as a pair and one side of that comparison misleads on its own.
        const uint8_t tick = (uint8_t)(0x07u << 3) ^ ink;      // rows 3-5 of the page
        if (U_FIN_LEFT(panel.fingers) & (1u << i)) fill(2, x + 2, 3, tick);
        if (U_FIN_RIGHT(panel.fingers) & (1u << i)) fill(2, x + 27, 3, tick);
    }
}

// --- layer band -------------------------------------------------------------

static void render_layer(void) {
    const uint8_t active = panel.layers & 0x0F;

    fill(3, 0, OLED_DISPLAY_WIDTH, 0x00);
    fill(4, 0, OLED_DISPLAY_WIDTH, 0x00);
    if (active == U_BASE) return;                 // base is the resting state: dark

    const char   *name = layer_names[active];
    const uint8_t w    = (uint8_t)(text_len(name) * 16);
    draw_text_2x((uint8_t)((OLED_DISPLAY_WIDTH - w) / 2), 3, name, 0x00);
}

// --- event strip ------------------------------------------------------------
// Only what is true right now, centred as a group, so the eye does not have to learn
// fixed positions for things that are usually absent.

#    define U_EVENT_GAP 8

static void render_events(void) {
    uint8_t icons[U_SYM_COUNT];
    uint8_t n = 0;

    if (panel.flags & U_FLAG_CAPS_LOCK) icons[n++] = U_SYM_CAPS_LOCK;
    if (panel.flags & U_FLAG_CAPS_WORD) icons[n++] = U_SYM_CAPS_WORD;
    if (panel.flags & (U_FLAG_ALTGR_L | U_FLAG_ALTGR_R)) icons[n++] = U_SYM_ALTGR;
    if (u_sync_link_down()) icons[n++] = U_SYM_LINK;

    fill(5, 0, OLED_DISPLAY_WIDTH, 0x00);         // separator, always dark
    fill(6, 0, OLED_DISPLAY_WIDTH, 0x00);
    fill(7, 0, OLED_DISPLAY_WIDTH, 0x00);
    if (n == 0) return;

    const uint8_t w = (uint8_t)(n * 16 + (n - 1) * U_EVENT_GAP);
    uint8_t       x = (uint8_t)((OLED_DISPLAY_WIDTH - w) / 2);
    for (uint8_t i = 0; i < n; i++, x += 16 + U_EVENT_GAP) {
        draw_glyph_2x(x, 6, u_sym8[icons[i]], 0x00);
    }
}

// --- entry points -----------------------------------------------------------

void oled_master_task(void) {
    panel = u_sync_build();
}

void oled_render_master(void) {
    render_status();
    render_mods();
    render_layer();
    render_events();
}

#endif     // OLED_MASTER_PANEL_ZONES
