// The master-side panel, in zones rather than lines of text. Dark at rest, so any
// light means something happened — and that is now literally true: at rest this draws
// the host badge and nothing else. See docs/layout.md, "Master half".
//
//   page 0    status bar   WIN/MAC badge, underlined when SYS set it by hand, plus
//                          the latched locks as 8x8 glyphs in the dead right end
//   pages 1-2 mod row      four 32 px cells, one per finger. A cell draws only while
//                          that finger holds something; nothing here ever inverts
//   pages 3-4 layer band   the layer name at 2x, centred, blank on base. A rule under
//                          it when the default layer is not base
//   page 5    alarm row    rung 2 of the ladder: an inverted banner, blinking three
//                          times on entry. Dark otherwise, which is also what keeps
//                          the row below from reading as a fifth line of the band
//   pages 6-7 trace strip  the panel's only past tense: which finger just misfired
//
// Inversion is spent only by the alarm row. That is the ladder's own rule — nothing
// routine may light more of the panel than the panel's own alarm — and it is why the
// mod row went dark at rest rather than inverting a cell per held modifier: two held
// modifiers used to invert exactly as many pixels as rung 2 does.

#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"
#include "oled_panel.h"
#include "host_os.h"
#include "split_state.h"
#include "split_sync.h"
#include "panel_font.h"
#include "panel_glyphs.h"
#include "oled_canvas.h"

#ifdef OLED_MASTER_PANEL_ZONES

// Miryoku's own layer list, so the order lives in one place. Overriding
// MIRYOKU_LAYER_LIST in custom_config.h changes these.
static const char *const layer_names[] = {
#    define MIRYOKU_X(LAYER, STRING) [U_##LAYER] = STRING,
    MIRYOKU_LAYER_LIST
#    undef MIRYOKU_X
};

// Rebuilt every main loop by oled_master_task. That is a resampling, not a capture:
// u_sync_build() reports only what is held at the instant it is called, and each build
// overwrites the last, so a modifier pressed and released between two 20 Hz frames is
// never seen live. The trace below is the exception — it watches for an edge every
// loop and latches it, which is the only reason a misfire can be shown at all.
static u_split_state_t panel;

// --- the canvas -------------------------------------------------------------
// 8 pages of 128 columns, each byte a vertical strip of 8 pixels with bit 0 at the
// top. Everything below draws into u_canvas, never into the driver's buffer, and
// oled_render_master commits once at the end. That is what lets a zone clear itself
// and redraw — which it must, since text drawing leaves whatever was to the right of
// it in place — without the frame counting as changed. Every page is filled by some
// zone each frame, so the canvas is always fully defined.

static void fill(uint8_t page, uint8_t x, uint8_t w, uint8_t byte) {
    for (uint8_t i = 0; i < w; i++) {
        u_canvas[(uint16_t)page * OLED_DISPLAY_WIDTH + x + i] = byte;
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

static void draw_glyph_1x(uint8_t x, uint8_t page, const uint8_t *g) {
    for (uint8_t sx = 0; sx < 8; sx++) {
        u_canvas[(uint16_t)page * OLED_DISPLAY_WIDTH + x + sx] = col_1x(g, sx);
    }
}

static void draw_glyph_2x(uint8_t x, uint8_t page, const uint8_t *g) {
    for (uint8_t q = 0; q < 2; q++) {
        for (uint8_t sx = 0; sx < 8; sx++) {
            const uint8_t  b  = col_2x(g, sx, q);
            const uint16_t at = (uint16_t)(page + q) * OLED_DISPLAY_WIDTH + x + sx * 2;
            u_canvas[at]     = b;
            u_canvas[at + 1] = b;
        }
    }
}

// A 16x16 drawn glyph: already in page format, so it is a straight copy.
static void draw_glyph_16(uint8_t x, uint8_t page, uint8_t which) {
    const uint8_t *src = u_glyph16[which];
    for (uint8_t q = 0; q < 2; q++) {
        for (uint8_t i = 0; i < 16; i++) {
            u_canvas[(uint16_t)(page + q) * OLED_DISPLAY_WIDTH + x + i] =
                pgm_read_byte(&src[q * 16 + i]);
        }
    }
}

static void draw_text_1x(uint8_t x, uint8_t page, const char *s) {
    for (; *s; s++, x += 8) draw_glyph_1x(x, page, glyph_of(*s));
}

static void draw_text_2x(uint8_t x, uint8_t page, const char *s) {
    for (; *s; s++, x += 16) draw_glyph_2x(x, page, glyph_of(*s));
}

static uint8_t text_len(const char *s) {
    uint8_t n = 0;
    while (*s++) n++;
    return n;
}

// Dark on lit, composed rather than drawn and then flipped: the row is filled solid
// first and the glyphs are written through it inverted.
static void draw_text_1x_inverse(uint8_t x, uint8_t page, const char *s) {
    for (; *s; s++, x += 8) {
        const uint8_t *g = glyph_of(*s);
        for (uint8_t sx = 0; sx < 8; sx++) {
            u_canvas[(uint16_t)page * OLED_DISPLAY_WIDTH + x + sx] = (uint8_t)~col_1x(g, sx);
        }
    }
}

// --- edges, watched every main loop -----------------------------------------
// The render runs at 20 Hz and only when the screen is awake. Anything that must not
// be missed is latched here instead.

#    define U_TRACE_MS  1500    // how long a misfire stays on screen
#    define U_BLINK_MS   600    // alarm entry: three blinks over this
#    define U_BLINK_HALF 100    // so 200 ms a cycle, three cycles

static uint8_t  trace_cells;        // finger bits that dropped on the misfire
static uint8_t  trace_left;         // which of them were the left hand
static uint32_t trace_at;
static bool     trace_live;

static uint32_t alarm_at;
static bool     alarm_was;

static void watch_edges(const u_split_state_t *now) {
    static u_split_state_t prev;
    static bool            primed;

    if (primed && now->anomaly != prev.anomaly) {
        // The release that counts a misfire is the same event that clears the finger
        // bit, and this runs every loop, so the bits that just dropped are the
        // offenders. Shift is excluded the way the mascot's axe excludes it: it is
        // held constantly as part of typing, so flagging it says nothing.
        const uint8_t gone = (uint8_t)(U_FIN_CELLS(prev.fingers) & ~U_FIN_CELLS(now->fingers));
        const uint8_t keep = (uint8_t)(gone & ~(1u << 3));
        if (keep) {
            trace_cells = keep;
            trace_left  = (uint8_t)(U_FIN_LEFT(prev.fingers) & keep);
            trace_at    = timer_read32();
            trace_live  = true;
        }
    }
    if (trace_live && timer_elapsed32(trace_at) >= U_TRACE_MS) trace_live = false;

    const bool alarm = u_sync_link_down();
    if (alarm && !alarm_was) alarm_at = timer_read32();
    alarm_was = alarm;

    prev   = *now;
    primed = true;
}

// --- status bar -------------------------------------------------------------
// The badge lives at the left; the right end is permanently dark, which is where the
// latched locks go. One row, so they cannot drift off a shared baseline.

#    define U_LOCK_X0 100
#    define U_LOCK_DX   9

static void render_status(void) {
    const char *badge = (panel.flags & U_FLAG_IS_MAC) ? "MAC" : "WIN";

    fill(0, 0, OLED_DISPLAY_WIDTH, 0x00);
    draw_text_1x(0, 0, badge);
    if (panel.flags & U_FLAG_OS_MANUAL) {
        // Row 7 under the badge: detection was overridden by hand, so the badge is a
        // decision rather than a reading. Redrawing the glyph columns with bit 7 set
        // keeps it one pass, and none of the six letters the badge can hold reaches
        // row 7 on its own.
        for (uint8_t x = 0; x < 24; x++) {
            u_canvas[x] = (uint8_t)(0x80u | col_1x(glyph_of(badge[x / 8]), x % 8));
        }
    }

    // Num Lock is deliberately absent: no layer here carries KC_NUM and the Num layer
    // sends digits rather than keypad codes, so its state can neither be changed from
    // this keyboard nor change anything it types. U_FLAG_NUM_LOCK stays on the wire as
    // a reserved bit. Scroll Lock is the opposite case — it sits on the Fun layer's
    // home row and gets hit by accident — so it is shown.
    const uint8_t on[U_LOCK_COUNT] = {(uint8_t)(panel.flags & U_FLAG_CAPS_LOCK),
                                      (uint8_t)(panel.flags & U_FLAG_CAPS_WORD),
                                      (uint8_t)(panel.flags & U_FLAG_SCROLL_LOCK)};
    for (uint8_t i = 0; i < U_LOCK_COUNT; i++) {
        if (on[i]) draw_glyph_1x((uint8_t)(U_LOCK_X0 + i * U_LOCK_DX), 0, u_lock8[i]);
    }
}

// --- mod row ----------------------------------------------------------------
// A cell is a finger, outer to inner: pinky, ring, middle, index. Not a modifier —
// the slot never moves, only the glyph follows the OS, which is why this used to light
// the wrong cell on the Mac where Ctrl and GUI swap. Ctrl's caret is the same drawing
// in both sets and simply changes cell, so the swap is visible as a hop.

static void render_mods(void) {
    static const uint8_t win[4] = {U_GLYPH_WINDOW, U_GLYPH_OPTION, U_GLYPH_CARET, U_GLYPH_SHIFT};
    static const uint8_t mac[4] = {U_GLYPH_CARET,  U_GLYPH_OPTION, U_GLYPH_CMD,    U_GLYPH_SHIFT};
    const uint8_t *set   = (panel.flags & U_FLAG_IS_MAC) ? mac : win;
    const uint8_t  cells = U_FIN_CELLS(panel.fingers);

    fill(1, 0, OLED_DISPLAY_WIDTH, 0x00);
    fill(2, 0, OLED_DISPLAY_WIDTH, 0x00);

    for (uint8_t i = 0; i < 4; i++) {
        if (!(cells & (1u << i))) continue;          // dark unless this finger holds
        const uint8_t x = (uint8_t)(i * 32);
        draw_glyph_16(x + 8, 1, set[i]);

        // Handedness is a bar filling the padding on that hand's side, not a dot: a
        // long coherent edge survives at this distance where texture does not. Both
        // bars for both hands, neither when the tracker missed the press — so a bare
        // glyph honestly means "held, hand unknown".
        if (U_FIN_LEFT(panel.fingers) & (1u << i)) {
            fill(1, x + 1, 3, 0xff);
            fill(2, x + 1, 3, 0xff);
        }
        if (U_FIN_RIGHT(panel.fingers) & (1u << i)) {
            fill(1, x + 28, 3, 0xff);
            fill(2, x + 28, 3, 0xff);
        }
    }
}

// --- layer band -------------------------------------------------------------

static void render_layer(void) {
    const uint8_t active  = panel.layers & 0x0F;
    const uint8_t deflt   = panel.layers >> 4;

    fill(3, 0, OLED_DISPLAY_WIDTH, 0x00);
    fill(4, 0, OLED_DISPLAY_WIDTH, 0x00);
    if (active == U_BASE && deflt == U_BASE) return;     // the resting state: dark

    const char   *name = layer_names[active];
    const uint8_t w    = (uint8_t)(text_len(name) * 16);
    const uint8_t x    = (uint8_t)((OLED_DISPLAY_WIDTH - w) / 2);
    if (active != U_BASE) draw_text_2x(x, 3, name);

    // A default that is not base is a standing fact, and holding a thumb used to hide
    // it: the band showed the held layer and the base underneath vanished. Two rows
    // under the name, drawn whichever layer is active.
    if (deflt != U_BASE) {
        const char   *dn = layer_names[deflt];
        const uint8_t dw = (uint8_t)(text_len(dn) * 16);
        const uint8_t dx = (uint8_t)((OLED_DISPLAY_WIDTH - (active == U_BASE ? dw : w)) / 2);
        if (active == U_BASE) draw_text_2x(dx, 3, dn);
        fill(4, dx, (uint8_t)(active == U_BASE ? dw : w), 0xc0);     // rows 38-39
    }
}

// --- alarm row --------------------------------------------------------------
// Rung 2, and the only thing on this panel that inverts. It is here rather than in the
// status bar because an alarm in page 0 erases the badge, and hiding the host OS at
// the one moment something is wrong is replacement, not escalation by scope.

static void render_alarm(void) {
    fill(5, 0, OLED_DISPLAY_WIDTH, 0x00);
    if (!u_sync_link_down()) return;

    const uint32_t since = timer_elapsed32(alarm_at);
    // Three blinks over 600 ms on entry, which no routine change ever does. After that
    // it is steady: a fault that stayed is not news any more, but it is still true.
    const bool lit = since >= U_BLINK_MS || ((since / U_BLINK_HALF) & 1u) == 0;
    if (!lit) return;

    fill(5, 0, OLED_DISPLAY_WIDTH, 0xff);
    draw_text_1x_inverse(28, 5, "LINK DOWN");
}

// --- trace strip ------------------------------------------------------------
// The panel's only past tense. A home-row mod that resolved as a hold, was held past
// U_ANOMALY_MS and had nothing typed during the hold: the shortcut you meant did not
// happen, and by the time you look down it is over. Which finger is the actionable
// part, because the tap-hold thresholds are per modifier.

static void render_trace(void) {
    fill(6, 0, OLED_DISPLAY_WIDTH, 0x00);
    fill(7, 0, OLED_DISPLAY_WIDTH, 0x00);
    if (!trace_live) return;

    static const char win[4] = {'G', 'A', 'C', 'S'};
    static const char mac[4] = {'C', 'O', 'M', 'S'};
    const char *label = (panel.flags & U_FLAG_IS_MAC) ? mac : win;

    // Two releases in one loop give two dropped bits and one count; show both.
    uint8_t n = 0;
    for (uint8_t i = 0; i < 4; i++) if (trace_cells & (1u << i)) n++;

    const uint8_t w = (uint8_t)(n * 32 + (n - 1) * 8);
    uint8_t       x = (uint8_t)((OLED_DISPLAY_WIDTH - w) / 2);
    for (uint8_t i = 0; i < 4; i++) {
        if (!(trace_cells & (1u << i))) continue;
        const char pair[3] = {label[i], (trace_left & (1u << i)) ? 'L' : 'R', '\0'};
        draw_text_2x(x, 6, pair);
        x = (uint8_t)(x + 40);
    }
}

// --- entry points -----------------------------------------------------------

void oled_master_task(void) {
    panel = u_sync_build();
    watch_edges(&panel);
}

void oled_render_master(void) {
    render_status();
    render_mods();
    render_layer();
    render_alarm();
    render_trace();
    u_canvas_commit();     // the only point that touches the driver's buffer
}

#endif     // OLED_MASTER_PANEL_ZONES
