// The master-side panel: three bands of pictures, no text. Modelled on splitkb's own
// Halcyon TFT panel (hlc_tft_display.c) and adapted to 128x64 monochrome, which is why
// the layer is an icon rather than its name. See docs/layout.md, "Master half".
//
//   page 0    status bar   the latched locks as 8x8 glyphs in the dead right end
//   pages 0-1 OS logo      drashna's 12x16 Windows or Apple logo at (0,0): its top
//                          half is the status bar's left end, its bottom half sits in
//                          the band's top-left corner, which every band keeps dark.
//                          Underlined at y=16 when SYS set the OS by hand
//   pages 1-5 layer band   one 128x40 picture per layer, from panel_layer_icons.c.
//                          A rule under it while a held layer hides a non-base
//                          default. This band is also the alarm
//   pages 6-7 mod row      four 32 px cells, one per finger, each showing its glyph
//                          always. A held finger fills its cell; a finger that just
//                          misfired fills only its glyph box
//
// Two things moved when the bands did, and both had to, because the band that used to
// hold them is now 40 px of picture:
//
// The alarm is no longer a banner. A 128x8 inverted row is about 850 dark pixels and a
// held mod cell is 405, so against the new mod row a banner reads as two modifiers —
// the exact collision docs/layout.md records from when the mod row last inverted. So
// the alarm now takes the whole band, which nothing routine can reach.
//
// The misfire trace is no longer a strip across the bottom. watch_edges() fires on the
// bit that just dropped, so the next 1.5 s is precisely when the shortcut is retried;
// covering the row would hide live modifiers at that moment. It is now a chip inside
// the offending finger's own cell — the same drawing as a hold, at half the area, so
// pressing again visibly grows it to fill the cell.
//
// All of that is the first of the master's pages (oled.h, docs/oled-v3.md D1). The next
// three are the same bands with something else in the base layer's band, one of two
// kitties or the QMK logo, and the Stats page is the SYS page. The U_PAGE key turns them, and
// for a moment after it a row of dots in the status bar says where you landed.

#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"
#include "oled.h"
#include "host_os.h"
#include "split_state.h"
#include "split_sync.h"
#include "art/panel_font.h"
#include "art/panel_glyphs.h"
#include "art/panel_layer_icons.h"
#include "art/drashna_icons.h"
#include "art/kitty_art.h"
#include "oled_canvas.h"
#include "split_health.h"
#include "oled_settings.h"
#include "speed_tier.h"

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

static void fill_or(uint8_t page, uint8_t x, uint8_t w, uint8_t byte) {
    for (uint8_t i = 0; i < w; i++) {
        u_canvas[(uint16_t)page * OLED_DISPLAY_WIDTH + x + i] |= byte;
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

static const uint8_t *glyph_of(char c) {
    const uint8_t u = (uint8_t)c;
    return u_font8[(u < U_FONT_FIRST || u > U_FONT_LAST) ? 0 : u - U_FONT_FIRST];
}

static void draw_glyph_1x(uint8_t x, uint8_t page, const uint8_t *g) {
    for (uint8_t sx = 0; sx < 8; sx++) {
        u_canvas[(uint16_t)page * OLED_DISPLAY_WIDTH + x + sx] = col_1x(g, sx);
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

// Chamfer the four corners of a filled 2-page block, so a lit cell reads as a keycap
// rather than as a black rectangle. Three pixels off each corner, which is the same
// 2 px chamfer splitkb's own Halcyon panel uses (`filled_rounded_rect` in
// hlc_tft_display.c) — ported rather than invented so the two panels agree.
//
// A real rounded corner needs more than 16 rows to read as a curve; at this size the
// chamfer is the whole effect, and taking a fourth pixel starts to look bitten rather
// than rounded.
static void chamfer(uint8_t page, uint8_t x, uint8_t w) {
    const uint16_t top = (uint16_t)page * OLED_DISPLAY_WIDTH + x;
    const uint16_t bot = (uint16_t)(page + 1) * OLED_DISPLAY_WIDTH + x;
    const uint8_t  r   = (uint8_t)(w - 1);
    u_canvas[top]         &= (uint8_t)~0x03u;     // rows 0-1 of the block
    u_canvas[top + 1]     &= (uint8_t)~0x01u;
    u_canvas[top + r]     &= (uint8_t)~0x03u;
    u_canvas[top + r - 1] &= (uint8_t)~0x01u;
    u_canvas[bot]         &= (uint8_t)~0xc0u;     // rows 14-15
    u_canvas[bot + 1]     &= (uint8_t)~0x80u;
    u_canvas[bot + r]     &= (uint8_t)~0xc0u;
    u_canvas[bot + r - 1] &= (uint8_t)~0x80u;
}

// A 16x16 glyph knocked out of a filled cell: the cell is already solid, so writing
// the glyph's complement cuts it out rather than drawing it.
static void draw_glyph_16_knockout(uint8_t x, uint8_t page, uint8_t which) {
    const uint8_t *src = u_glyph16[which];
    for (uint8_t q = 0; q < 2; q++) {
        for (uint8_t i = 0; i < 16; i++) {
            u_canvas[(uint16_t)(page + q) * OLED_DISPLAY_WIDTH + x + i] &=
                (uint8_t)~pgm_read_byte(&src[q * 16 + i]);
        }
    }
}

// A page-format image ORed in at any row. The art is page-aligned and its place on the
// panel is not, so each source byte lands across two canvas pages.
static void blit_at(const uint8_t *src, uint8_t w, uint8_t pages, uint8_t x0, uint8_t y0) {
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

// --- the page ---------------------------------------------------------------
// The stored value is the page's number in D1's order, not its place in the cycle, so
// building a page later does not move the others.

#    define U_DOTS_MS 700

static uint8_t  page_now;
static bool     page_loaded;
static bool     dots_live;      // a flag, not just the time: the clock starts at 0 too
static uint32_t dots_at;

static bool page_built(uint8_t p) {
    return p < U_PAGE_COUNT && ((U_PAGES_BUILT >> p) & 1u);
}

static void page_load(void) {
    if (page_loaded) return;
    page_loaded = true;
    const uint8_t p = u_setting_get(U_SETTING_VIEW) & U_VIEW_PAGE_MASK;
    // A page this build does not have, stored by a later one, reads as the first.
    page_now = page_built(p) ? p : U_PAGE_BANDS;
}

enum u_master_page u_page_get(void) {
    page_load();
    return (enum u_master_page)page_now;
}

void u_page_next(void) {
    if (!is_keyboard_master()) return;
    page_load();
    do {
        page_now = (uint8_t)((page_now + 1) % U_PAGE_COUNT);
    } while (!page_built(page_now));
    const uint8_t view = u_setting_get(U_SETTING_VIEW);
    u_setting_set(U_SETTING_VIEW, (uint8_t)((view & ~U_VIEW_PAGE_MASK) | page_now));
    dots_live = true;
    dots_at   = timer_read32();
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

// Run whatever the page, so the kitty shows the speed you are typing at the moment you
// turn to her, rather than catching up from wherever she was last seen.
static u_speed_tier_t kitty_speed;

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
// The OS logo takes the left end (drawn after the band, below); the right end is
// permanently dark, which is where the latched locks go. One row, so they cannot drift
// off a shared baseline, right-aligned so the last one always sits at the same place.

#    define U_LOCK_XN 118     // the rightmost slot
#    define U_LOCK_DX   9

static void render_status(void) {
    fill(0, 0, OLED_DISPLAY_WIDTH, 0x00);

    // Num Lock is deliberately absent: no layer here carries KC_NUM and the Num layer
    // sends digits rather than keypad codes, so its state can neither be changed from
    // this keyboard nor change anything it types. U_FLAG_NUM_LOCK stays on the wire as
    // a reserved bit. Scroll Lock is the opposite case — it sits on the Fun layer's
    // home row and gets hit by accident — so it is shown.
    //
    // Caps Word lights the Caps Lock glyph, the way drashna's screen does: there is no
    // Caps Word glyph in his set. Named, so the art's lock set can change around it.
    const uint8_t on[U_LOCK_COUNT] = {
        [U_LOCK_CAPS_LOCK]   = (uint8_t)(panel.flags & (U_FLAG_CAPS_LOCK | U_FLAG_CAPS_WORD)),
        [U_LOCK_SCROLL_LOCK] = (uint8_t)(panel.flags & U_FLAG_SCROLL_LOCK),
    };
    for (uint8_t i = 0; i < U_LOCK_COUNT; i++) {
        const uint8_t x = (uint8_t)(U_LOCK_XN - (U_LOCK_COUNT - 1 - i) * U_LOCK_DX);
        if (on[i]) draw_glyph_1x(x, 0, u_lock8[i]);
    }
}

// --- layer band -------------------------------------------------------------
// Pages 1-5, and the only zone that ever inverts. One picture per layer; the picture
// is the whole band, so nothing here decides a position — tools/panel/gen_layer_icons.py
// does, and it is also where the art comes from.

#    define U_BAND_PAGE 1     // canvas page the band starts on

static void blit_band(uint8_t which) {
    const uint8_t *src = u_layer_band[which];
    for (uint8_t q = 0; q < U_BAND_PAGES; q++) {
        for (uint8_t i = 0; i < U_BAND_W; i++) {
            u_canvas[(uint16_t)(U_BAND_PAGE + q) * OLED_DISPLAY_WIDTH + i] =
                pgm_read_byte(&src[(uint16_t)q * U_BAND_W + i]);
        }
    }
}

static void invert_band(void) {
    for (uint8_t q = 0; q < U_BAND_PAGES; q++) {
        for (uint8_t i = 0; i < U_BAND_W; i++) {
            u_canvas[(uint16_t)(U_BAND_PAGE + q) * OLED_DISPLAY_WIDTH + i] ^= 0xffu;
        }
    }
}

// The chain is cut out of the inverted band rather than drawn on it, so it reads lit
// against the dark — the same knockout the mod row uses for a held glyph.
static void knock_chain(uint8_t x) {
    for (uint8_t q = 0; q < U_CHAIN_PAGES; q++) {
        for (uint8_t i = 0; i < U_CHAIN_W; i++) {
            u_canvas[(uint16_t)(U_BAND_PAGE + 1 + q) * OLED_DISPLAY_WIDTH + x + i] &=
                (uint8_t)~pgm_read_byte(&u_chain[(uint16_t)q * U_CHAIN_W + i]);
        }
    }
}

// The base band's other three versions. All start right of x=15 or below y=17, or under
// the OS logo and its underline in the corner. A kitty takes only the emblem's half, so
// the Kyria wordmark stays; the QMK logo is as wide as the panel and takes the whole band.

#    define U_KITTY_X       18
#    define U_KITTY_Y       12
#    define U_KITTY_CLEAR_W 58      // the emblem's columns; the wordmark starts after them
#    define U_KITTY_MS     500      // a frame
#    define U_KITTY_FAST_MS 300     // a frame when she bounces
#    define U_QMK_X          1      // centred: 126 of 128
#    define U_QMK_Y         21      // centred in y 18-47

// Either kitty, on Bjorn's four tiers in drashna's order. The small one's paw frames are
// her sleep frames, so in the first tier she looks asleep (D9); the striped one has her own.
static void base_kitty(const uint8_t (*art)[U_KITTY_BYTES], const u_kitty_run_t *moods) {
    static const uint8_t mood_of_tier[4] = {U_KITTY_SLEEP, U_KITTY_PAW, U_KITTY_WATCH, U_KITTY_BOUNCE};

    for (uint8_t q = 0; q < U_BAND_PAGES; q++) fill((uint8_t)(U_BAND_PAGE + q), 0, U_KITTY_CLEAR_W, 0x00);
    const u_kitty_run_t run = moods[mood_of_tier[kitty_speed.tier]];
    const uint32_t      ms  = kitty_speed.tier == 3 ? U_KITTY_FAST_MS : U_KITTY_MS;
    const uint8_t       f   = (uint8_t)(run.first + (timer_read32() / ms) % run.count);
    blit_at(art[f], U_KITTY_W, U_KITTY_BYTES / U_KITTY_W, U_KITTY_X, U_KITTY_Y);
}

static void base_qmk(void) {
    for (uint8_t q = 0; q < U_BAND_PAGES; q++) fill((uint8_t)(U_BAND_PAGE + q), 0, OLED_DISPLAY_WIDTH, 0x00);
    blit_at(u_qmk_logo, U_LOGO_W, U_LOGO_PAGES, U_QMK_X, U_QMK_Y);
}

// Returns whether the alarm lit the band this frame, which the OS logo needs to know.
static bool render_band(void) {
    const uint8_t active = panel.layers & 0x0F;
    const uint8_t deflt  = panel.layers >> 4;

    // Holding a thumb shows the held layer; at rest the band shows whatever the default
    // is, which on a stock setup is base and its Kyria logo.
    const uint8_t shown = (active == U_BASE && deflt != U_BASE) ? deflt : active;
    blit_band(shown);
    // Before the alarm, which still fills the base band whichever version it is.
    if (shown == U_BASE) {
        switch (u_page_get()) {
            case U_PAGE_BANDS_KITTY:   base_kitty(u_kitty_small, u_kitty_small_moods); break;
            case U_PAGE_BANDS_STRIPED: base_kitty(u_kitty_striped, u_kitty_striped_moods); break;
            case U_PAGE_BANDS_QMK:     base_qmk(); break;
            default:                   break;
        }
    }

    // A default that is not base is a standing fact, and a held layer used to hide it.
    // Both halves of the condition earn their place: a base default has nothing to
    // report, and when the active layer *is* the default the icon on screen already is
    // the statement, so a rule under it would only repeat itself.
    //
    // The second half is also what makes the rule safe at the band's last three rows.
    // It can only draw while a layer is held, every held layer is momentary, and the
    // one icon that reaches row 39 — Extra, splitkb's disc, which fills the cell edge
    // to edge — is a default-layer alternative that can never be held. So the collision
    // cannot arise rather than merely being avoided.
    // Phrased against `shown` rather than `active` on purpose. Today they differ only
    // in a state the encoding cannot produce — `layers` takes its low nibble from
    // get_highest_layer(layer_state | default_layer_state), so resting on an Extra
    // default reads as Extra, not as base — but this way the rule stays correct if that
    // ever changes, and the two guarantees below are then true by construction rather
    // than by argument.
    if (deflt != U_BASE && shown != deflt) {
        const uint8_t x0 = pgm_read_byte(&u_band_x0[shown]);
        const uint8_t x1 = pgm_read_byte(&u_band_x1[shown]);
        fill_or(5, x0, (uint8_t)(x1 - x0 + 1), 0xe0);     // rows 45-47
    }

    if (!u_sync_link_down()) return false;

    // Three blinks over 600 ms on entry, which no routine change ever does. After that
    // it is steady: a fault that stayed is not news any more, but it is still true.
    const uint32_t since = timer_elapsed32(alarm_at);
    if (since < U_BLINK_MS && ((since / U_BLINK_HALF) & 1u) != 0) return false;

    if (shown == U_BASE) {
        // The resting band is the one place the icon is not kept. It carries no
        // information to lose — base is where you already are — and the Kyria wordmark
        // is 81 px wide, so there is no gutter to put the chain in beside it.
        fill(1, 0, OLED_DISPLAY_WIDTH, 0xff);
        fill(2, 0, OLED_DISPLAY_WIDTH, 0xff);
        fill(3, 0, OLED_DISPLAY_WIDTH, 0xff);
        fill(4, 0, OLED_DISPLAY_WIDTH, 0xff);
        fill(5, 0, OLED_DISPLAY_WIDTH, 0xff);
        knock_chain(44);
    } else {
        // Escalation by scope, not replacement: the layer icon survives, lit on dark,
        // so nothing needed while typing is hidden by the alarm.
        invert_band();
        knock_chain(2);
    }
    return true;
}

// --- OS logo ----------------------------------------------------------------
// drashna's 12x16 host logo in place of the old Win/Mac text: Windows, Apple, or Tux for
// Linux. iOS is the Apple logo with a small lowercase "i" in front of it, drawn here
// (docs/oled-v3.md, D14), and the logo moves right to make room. It is 16 rows tall and
// the status bar is 8, so its bottom half borrows the band's top-left corner: x 0-15,
// y 8-17 is dark on all ten bands, which test_panel.c asserts on the art itself. The band
// is redrawn whole every frame, so this has to come after it.
//
// Under the alarm that corner is lit along with the rest of the band, so the logo, the
// "i" and the underline are cut out of it, the way the chain is, instead of being ORed
// onto a field that is already lit. The three logos drawn here and the "i" all light
// column 0, which is what the "no 1 px edge frame" argument in docs/layout.md rests on
// now that the badge is gone.

#    define U_OS_UNDERLINE_PAGE 2       // y=16, the row under the logo
#    define U_OS_UNDERLINE_BIT  0x01u
#    define U_IOS_I_GAP         2       // between the "i" and the logo

// One column of something two pages tall at the top left: the top byte onto the dark
// status bar, the bottom byte onto the band, cut out of it when the alarm has lit it.
static void put_corner(uint8_t x, uint8_t page, uint8_t byte, bool band_lit) {
    uint8_t *at = &u_canvas[(uint16_t)page * OLED_DISPLAY_WIDTH + x];
    if (page > 0 && band_lit) {
        *at &= (uint8_t)~byte;
    } else {
        *at |= byte;
    }
}

static void render_os_logo(bool band_lit) {
    static const uint8_t logo_of[] = {[U_HOST_WINDOWS] = U_OS_LOGO_WINDOWS, [U_HOST_MAC] = U_OS_LOGO_APPLE,
                                      [U_HOST_IOS] = U_OS_LOGO_APPLE, [U_HOST_LINUX] = U_OS_LOGO_LINUX};
    const uint8_t *src = u_os_logo[panel.host < sizeof(logo_of) ? logo_of[panel.host] : U_OS_LOGO_WINDOWS];
    const bool     ios = panel.host == U_HOST_IOS;
    const uint8_t  x0  = ios ? U_IOS_I_W + U_IOS_I_GAP : 0;

    if (ios) {
        for (uint8_t i = 0; i < U_IOS_I_W; i++) {
            put_corner(i, 0, pgm_read_byte(&u_ios_i[i]), band_lit);
            put_corner(i, 1, pgm_read_byte(&u_ios_i[U_IOS_I_W + i]), band_lit);
        }
    }
    for (uint8_t i = 0; i < U_OS_LOGO_W; i++) {
        put_corner((uint8_t)(x0 + i), 0, pgm_read_byte(&src[i]), band_lit);
        put_corner((uint8_t)(x0 + i), 1, pgm_read_byte(&src[U_OS_LOGO_W + i]), band_lit);
    }
    // Set by hand: detection was overridden, so the logo is a decision rather than a
    // reading. The logos and the "i" end at y=14, so the line stands a row clear, and on
    // iOS it runs under the "i" too.
    if (panel.flags & U_FLAG_OS_MANUAL) {
        for (uint8_t x = 0; x < x0 + U_OS_LOGO_W; x++) {
            put_corner(x, U_OS_UNDERLINE_PAGE, U_OS_UNDERLINE_BIT, band_lit);
        }
    }
}

// --- mod row ----------------------------------------------------------------
// A cell is a finger. Not a modifier — the slot never moves, only the glyph follows the
// OS, which is why this used to light the wrong cell on the Mac where Ctrl and GUI swap.
// Ctrl's caret is the same drawing in both sets and simply changes cell, so the swap is
// visible as a hop.
//
// **The order follows the half the panel is on.** Outer to inner is pinky, ring, middle,
// index, and on the left half that runs left to right across the keys — so cell 0 is at
// x=0. On the right half the same fingers run the other way: the index finger is the
// innermost key, which is the *leftmost* one on that half. Drawing pinky-first there put
// every glyph above the wrong finger, mirrored. `is_keyboard_left()` is the master's own
// handedness, so the row simply reverses with it and each glyph sits over its key.
//
// Three states per cell, and they are one drawing at three sizes rather than three
// drawings: idle is the glyph alone, a misfire fills the glyph's own 16 px box, a hold
// fills the whole 32 px cell. So a finger that misfires and is pressed again does not
// change vocabulary, it grows.
//
// Both filled states are chamfered, so a lit cell reads as a key being pressed rather
// than as a black rectangle dropped on the row.
//
// **Handedness is no longer drawn.** A 3x16 bar used to fill the padding on the side of
// the hand that held the key. It was removed because at four cells plus two bars plus a
// chamfer the row had more marks than meaning, and the bars were the least of them: the
// cell already says which finger, and the hand is the one thing you can feel without
// looking. What is given up is honest to record — Chordal Hold resolves on the *pair* of
// hands, so a same-hand hold and a cross-hand hold are now drawn identically. The misfire
// chip is what actually reports a bad hold, and it still names the finger.
// `U_FIN_LEFT`/`U_FIN_RIGHT` stay on the wire; nothing on screen reads them.

static void render_mods(void) {
    static const uint8_t win[4] = {U_GLYPH_WINDOW, U_GLYPH_OPTION, U_GLYPH_CARET, U_GLYPH_SHIFT};
    static const uint8_t mac[4] = {U_GLYPH_CARET,  U_GLYPH_OPTION, U_GLYPH_CMD,    U_GLYPH_SHIFT};
    const uint8_t *set   = (panel.flags & U_FLAG_IS_MAC) ? mac : win;
    const uint8_t  cells = U_FIN_CELLS(panel.fingers);

    fill(6, 0, OLED_DISPLAY_WIDTH, 0x00);
    fill(7, 0, OLED_DISPLAY_WIDTH, 0x00);

    const bool left_half = is_keyboard_left();

    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t slot = left_half ? i : (uint8_t)(3 - i);
        const uint8_t x    = (uint8_t)(slot * 32);
        const bool    held = cells & (1u << i);
        const bool    miss = trace_live && (trace_cells & (1u << i));

        if (held) {
            fill(6, x, 32, 0xff);
            fill(7, x, 32, 0xff);
            chamfer(6, x, 32);
            draw_glyph_16_knockout((uint8_t)(x + 8), 6, set[i]);
        } else if (miss) {
            fill(6, (uint8_t)(x + 8), 16, 0xff);
            fill(7, (uint8_t)(x + 8), 16, 0xff);
            chamfer(6, (uint8_t)(x + 8), 16);
            draw_glyph_16_knockout((uint8_t)(x + 8), 6, set[i]);
        } else {
            draw_glyph_16((uint8_t)(x + 8), 6, set[i]);
        }
    }
}

// --- the page dots ----------------------------------------------------------
// One per page that PAGE can reach, the current one larger, in the status bar's dark
// middle (x 41-85 with five pages), the mock's geometry. Drawn last, over any page, the
// Stats page's first line included: they are only there for 700 ms.

static void render_dots(void) {
    if (!dots_live) return;
    uint8_t n = 0, at = 0;
    for (uint8_t p = 0; p < U_PAGE_COUNT; p++) {
        if (!page_built(p)) continue;
        if (p == page_now) at = n;
        n++;
    }
    const uint8_t x0 = (uint8_t)(OLED_DISPLAY_WIDTH / 2 - n * 4);
    fill(0, (uint8_t)(x0 - 3), (uint8_t)(n * 8 + 5), 0x00);
    for (uint8_t i = 0; i < n; i++) {
        if (i == at) {
            fill_or(0, (uint8_t)(x0 + i * 8), 5, 0x3c);         // 5x4, rows 2-5
        } else {
            fill_or(0, (uint8_t)(x0 + i * 8 + 1), 3, 0x18);     // 3x2, rows 3-4
        }
    }
}

// --- the stats page ---------------------------------------------------------
// Sixteen characters by eight lines at 1x fills the panel exactly. The build has
// NO_PRINT, so there is no printf here and none is wanted: a handful of numbers do not
// justify dragging formatting code into the firmware.

// Appends an unsigned decimal. Returns where the next character goes.
static uint8_t num(char *dst, uint8_t at, uint32_t v) {
    char tmp[10];
    uint8_t n = 0;
    do {
        tmp[n++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v && n < sizeof(tmp));
    while (n && at < 16) dst[at++] = tmp[--n];
    return at;
}

static uint8_t text(char *dst, uint8_t at, const char *s) {
    while (*s && at < 16) dst[at++] = *s++;
    return at;
}

static uint8_t hex4(char *dst, uint8_t at, uint16_t v) {
    static const char digits[] = "0123456789ABCDEF";
    for (int8_t sh = 12; sh >= 0 && at < 16; sh -= 4) {
        dst[at++] = digits[(v >> sh) & 0xF];
    }
    return at;
}

static void line(uint8_t row, const char *buf, uint8_t len) {
    for (uint8_t i = 0; i < 16; i++) {
        draw_glyph_1x((uint8_t)(i * 8), row, glyph_of(i < len ? buf[i] : ' '));
    }
}

static void render_stats(void) {
    const u_health_master_t m = u_health_master();
    const u_health_slave_t  s = u_health_slave_seen();
    char                    b[16];
    uint8_t                 n;

    // Which firmware, and whether the two halves agree on it. Halves running different
    // builds can still share a transaction count and then fail every transaction, which
    // looks exactly like a broken cable.
    n = text(b, 0, "BLD ");
    n = hex4(b, n, u_build_id());
    n = text(b, n, u_health_have_reply() ? (s.build_id == u_build_id() ? " OK" : " !=") : " --");
    line(0, b, n);

    // Uptime, both halves. The other half's being far lower is the tell that it
    // restarted while this one did not.
    n = text(b, 0, "UP ");
    n = num(b, n, m.uptime_s);
    n = text(b, n, "/");
    n = num(b, n, u_health_have_reply() ? s.uptime_s : 0);
    line(1, b, n);

    // Main loop rate: last full second, and the worst second since boot. This is the
    // number that says whether any of the drawing is costing typing.
    n = text(b, 0, "LOOP ");
    n = num(b, n, m.loops_per_s);
    n = text(b, n, "/");
    n = num(b, n, m.worst_loops_per_s);
    line(2, b, n);

    n = text(b, 0, "FAIL ");
    n = num(b, n, m.failed_sends);
    n = text(b, n, "/");
    n = num(b, n, m.worst_fail_streak);
    line(3, b, n);

    n = text(b, 0, "OUT ");
    n = num(b, n, m.outages);
    n = text(b, n, " MX");
    n = num(b, n, m.longest_outage_ds / 10);
    n = text(b, n, "S");
    line(4, b, n);

    n = text(b, 0, "LAST ");
    if (m.outages) {
        n = num(b, n, m.uptime_s - m.last_outage_start_s);
        n = text(b, n, "S AGO");
    } else {
        n = text(b, n, "NONE");
    }
    line(5, b, n);

    n = text(b, 0, "DROP ");
    n = num(b, n, m.transport_drops);
    n = text(b, n, " SLP");
    n = num(b, n, m.suspends);
    line(6, b, n);

    n = text(b, 0, "STALE ");
    n = num(b, n, u_health_have_reply() ? s.stale_episodes : 0);
    n = text(b, n, " RST");
    n = num(b, n, u_health_have_reply() ? s.resets : 0);
    line(7, b, n);
}

// --- entry points -----------------------------------------------------------

void oled_master_task(void) {
    panel = u_sync_build();
    watch_edges(&panel);
    u_speed_tier_update(&kitty_speed, get_current_wpm());
    if (dots_live && timer_elapsed32(dots_at) >= U_DOTS_MS) dots_live = false;
}

void oled_render_master(void) {
    // Holding SYS still opens the stats page from any page, and it still closes itself;
    // as a page it stays. Either way it carries no alarm, as the SYS page never has: its
    // FAIL and OUT lines are how it reports the link.
    if (u_stats_page_open() || u_page_get() == U_PAGE_STATS) {
        render_stats();
    } else {
        render_status();
        render_os_logo(render_band());
        render_mods();
    }
    render_dots();
    u_canvas_commit();     // the only point that touches the driver's buffer
}
