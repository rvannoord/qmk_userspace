// The master panel's zones, drawn into a buffer laid out like the driver's. This
// compiles the real oled_master_zones.c, not a copy, so the geometry asserted here is
// the geometry that ships. Run with --dump to print the frames, or --raw for the
// buffers themselves, which is how the zones get looked at without a flash.
#include <stdlib.h>
#include <string.h>
#include "unit.h"
#include "qmk_keyboard.h"
#include "split_state.h"
#include "split_sync.h"
#include "split_health.h"

#include "driver.h"

uint8_t  fake_oled[OLED_MATRIX_SIZE + 1];
uint16_t fake_dirty;
uint32_t fake_now;

void oled_write_raw_byte(const char data, uint16_t index) { u_fake_write(data, index); }

// split_health.c is linked in because the panel draws the stats page from it; these
// are the few things it expects from the world around it.
bool     fake_is_master = true;
bool     fake_connected = true;
bool     is_transport_connected(void) { return fake_connected; }

static u_split_state_t drive;
static bool            drive_link_down;

u_split_state_t u_sync_build(void) { return drive; }
bool            u_sync_link_down(void) { return drive_link_down; }
void            u_sync_note_record(uint16_t keycode, keyrecord_t *record) { (void)keycode; (void)record; }
u_split_state_t u_sync_state(void) { return drive; }
uint32_t        u_sync_age_ms(void) { return 0; }
bool            u_sync_soft_stale(void) { return false; }
bool            u_sync_hard_stale(void) { return false; }
void            u_sync_init(void) {}
void            u_sync_task(void) {}

#ifndef QMK_KEYBOARD_H
#    define QMK_KEYBOARD_H "qmk_keyboard.h"
#endif
#include "oled_master_zones.c"

// --- looking at the buffer --------------------------------------------------

static bool lit(uint8_t x, uint8_t y) {
    return (fake_oled[(y / 8) * OLED_DISPLAY_WIDTH + x] >> (y % 8)) & 1;
}

static bool any_lit(uint8_t y0, uint8_t y1) {
    for (uint8_t y = y0; y <= y1; y++)
        for (uint8_t x = 0; x < OLED_DISPLAY_WIDTH; x++)
            if (lit(x, y)) return true;
    return false;
}

static int count(uint8_t y0, uint8_t y1) {
    int n = 0;
    for (uint8_t y = y0; y <= y1; y++)
        for (uint8_t x = 0; x < OLED_DISPLAY_WIDTH; x++)
            if (lit(x, y)) n++;
    return n;
}

static bool any_lit_in(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1) {
    for (uint8_t y = y0; y <= y1; y++)
        for (uint8_t x = x0; x <= x1; x++)
            if (lit(x, y)) return true;
    return false;
}

static void margins(uint8_t y0, uint8_t y1, int *left, int *right) {
    int lo = -1, hi = -1;
    for (uint8_t x = 0; x < OLED_DISPLAY_WIDTH; x++) {
        for (uint8_t y = y0; y <= y1; y++) {
            if (lit(x, y)) {
                if (lo < 0) lo = x;
                hi = x;
                break;
            }
        }
    }
    *left  = lo;
    *right = lo < 0 ? -1 : OLED_DISPLAY_WIDTH - 1 - hi;
}

// Centred to within `slop` columns. Measuring the ink rather than the text box,
// because the box is what the code positions but the ink is what the eye judges —
// DUNGEON.mode's glyphs do not fill their cells symmetrically, so a perfectly placed
// word still has unequal margins. 4 is one 2x pixel of side bearing each side.
static bool centred(uint8_t y0, uint8_t y1, int slop) {
    int left, right;
    margins(y0, y1, &left, &right);
    if (left < 0) return false;
    const int off = left - right;
    return (off < 0 ? -off : off) <= slop;
}

// The whole 32-column cell solid on its own top row: what full-cell inversion used to
// look like, and what must never come back — two cells of it light exactly as many
// pixels as the alarm row, which is the rung it would be stealing.
static bool cell_inverted(uint8_t cell) {
    for (uint8_t x = cell * 32; x < (cell + 1) * 32; x++)
        if (!lit(x, 8)) return false;
    return true;
}

static bool cell_drawn(uint8_t cell) {
    return any_lit_in((uint8_t)(cell * 32 + 8), (uint8_t)(cell * 32 + 23), 8, 23);
}

// The banner is lit with dark text through it, so "inverted" is the background being
// solid on a row the font never reaches, not the row being entirely lit.
static bool row_solid(uint8_t y) {
    for (uint8_t x = 0; x < OLED_DISPLAY_WIDTH; x++)
        if (!lit(x, y)) return false;
    return true;
}

static bool hand_bar(uint8_t cell, bool right) {
    const uint8_t x = (uint8_t)(cell * 32 + (right ? 28 : 1));
    for (uint8_t y = 8; y <= 23; y++)
        if (!lit(x, y)) return false;
    return true;
}

// --- driving ----------------------------------------------------------------

// One main loop, then one render. The buffer is NOT cleared between frames: the
// driver's is not either, and clearing it here is exactly what hid Bug A — every byte
// looked new every frame, so nothing could be seen to be unchanged.
static void step(void) {
    u_health_task();          // drives the stats page's open/close timing
    oled_master_task();
    oled_render_master();
}

// Render a state, let 50 ms pass, render the same state again. The second render must
// write nothing at all. When this fails the panels cannot sleep: a dirty block makes
// the driver call oled_on(), which re-arms OLED_TIMEOUT, for ever.
//
// The regression it guards, measured before the fix: at rest block 0 (the badge), Nav
// blocks 0/6/7/8/9, a held Ctrl 0/3/5, LINK DOWN steady 0/10/11.
static uint16_t redraw_cost(u_split_state_t s, bool link) {
    drive = s; drive_link_down = link;
    step();
    fake_now += 50;
    fake_dirty = 0;
    step();
    return fake_dirty;
}

static void frame(u_split_state_t s, bool link) {
    drive           = s;
    drive_link_down = link;
    step();
}

static void dump(const char *label) {
    printf("--- %s\n", label);
    for (uint8_t y = 0; y < 64; y++) {
        if (y % 8 == 0) printf("    page %u\n", y / 8);
        printf("%2u |", y);
        for (uint8_t x = 0; x < OLED_DISPLAY_WIDTH; x++) putchar(lit(x, y) ? '#' : ' ');
        printf("|\n");
    }
}

// --- the frames -------------------------------------------------------------

static const u_split_state_t at_rest = {.layers = U_BASE, .flags = 0};

static const u_split_state_t on_nav = {
    .layers  = U_NAV,
    .fingers = U_FIN_L_INDEX | U_FIN_R_RING,
    .flags   = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL | U_FLAG_CAPS_WORD,
};

static const u_split_state_t latched = {
    .layers = U_BUTTON,
    .flags  = U_FLAG_CAPS_LOCK | U_FLAG_SCROLL_LOCK,
};

// Hold SYS for two seconds, which is what opens the page.
static void open_stats(void) {
    u_stats_note_sys(true);
    for (int i = 0; i < 45; i++) { fake_now += 50; step(); }
}

static void each_state(void (*then)(const char *)) {
    frame(at_rest, false); then("base, windows, nothing held");
    frame(on_nav, false);  then("nav, mac by hand, right option + left shift, caps word");
    frame(latched, true);  then("button, caps lock + scroll lock, link down");

    // A misfire needs two builds: the count rises on the same loop the finger bit
    // drops, so a single frame can never show one.
    frame((u_split_state_t){.layers = U_BASE, .fingers = U_FIN_L_PINKY, .anomaly = 1}, false);
    frame((u_split_state_t){.layers = U_BASE, .fingers = 0, .anomaly = 2}, false);
    then("base, windows, a left pinky misfire just happened");

    drive = at_rest;
    open_stats();
    then("the stats page, asked for by holding SYS");
    u_stats_note_key();
}

static void emit(const char *label) {
    (void)label;
    fwrite(fake_oled, 1, sizeof(fake_oled), stdout);
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--raw") == 0) {
        each_state(emit);
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--dump") == 0) {
        each_state(dump);
        return 0;
    }

    // Dark at rest, and this time literally: the badge and nothing else. It is the
    // premise the layer band, the alarm row and the trace all rest on.
    frame(at_rest, false);
    check("at rest, everything below the status bar is dark", !any_lit(8, 63));
    {
        int left, right;
        margins(0, 7, &left, &right);
        check("the badge starts at the left edge", left == 0);
        check("and takes only its own three characters", right == OLED_DISPLAY_WIDTH - 24);
    }
    check("no underline while detection is trusted", !lit(0, 7) && !lit(23, 7));

    // Set by hand: the badge becomes a decision rather than a reading.
    frame(on_nav, false);
    {
        bool solid = true;
        for (uint8_t x = 0; x < 24; x++) solid = solid && lit(x, 7);
        check("set by hand underlines the badge", solid);
        check("and the underline stops with it", !lit(24, 7));
    }

    // Cells are fingers, outer to inner, and they do not move with the OS: this frame
    // is on the Mac, where Ctrl and GUI swap, holding left index and right ring.
    check("a held finger draws its cell", cell_drawn(1) && cell_drawn(3));
    check("an idle finger draws nothing", !cell_drawn(0) && !cell_drawn(2));
    check("nothing on the mod row inverts",
          !cell_inverted(0) && !cell_inverted(1) && !cell_inverted(2) && !cell_inverted(3));
    check("the right hand bars on the right", hand_bar(1, true) && !hand_bar(1, false));
    check("the left hand bars on the left", hand_bar(3, false) && !hand_bar(3, true));

    check("the layer name is centred", centred(24, 39, 4));
    check("the alarm row is dark with no alarm", !any_lit(40, 47));
    check("caps word shows in the status bar's right end", any_lit_in(100, 127, 0, 7));
    check("and nothing is left in the old event strip", !any_lit(48, 63));

    // The mod row must stay under the alarm row it is not allowed to outshine. All
    // four fingers of both hands at once is the worst case; the alarm row is 128x8.
    {
        u_split_state_t all = on_nav;
        all.fingers = 0xff;
        frame(all, false);
        const int mods = count(8, 23);
        check("four mods on both hands stay under the alarm row's 1024 px", mods < 1024);
    }

    // The alarm. Three blinks over 600 ms on entry, then steady — and it inverts,
    // which nothing else on this panel is allowed to do.
    fake_now = 10000;
    frame(latched, true);
    check("the alarm row inverts on entry", row_solid(40) && count(40, 47) > 700);
    fake_now += 100;
    step();
    check("and is dark a blink later", !any_lit(40, 47));
    fake_now += 100;
    step();
    check("and lit again", any_lit(40, 47));
    fake_now += 1000;
    step();
    check("steady once the blinking is over", row_solid(40) && count(40, 47) > 700);
    check("the layer name survives under the alarm", any_lit(24, 39));

    // Locks: two of three on, and Num Lock can never be, so its slot is never drawn.
    check("both latched locks show", any_lit_in(100, 108, 0, 7) && any_lit_in(118, 126, 0, 7));
    check("the unused lock slot stays dark", !any_lit_in(109, 117, 0, 7));

    // The trace. A misfire is a count that changed and a finger bit that dropped on
    // the same loop; it is latched because the render would otherwise miss it.
    {
        u_split_state_t held = {.layers = U_BASE, .fingers = U_FIN_L_PINKY, .anomaly = 3};
        frame(held, false);
        check("no trace while the finger is still down", !any_lit(48, 63));

        u_split_state_t gone = {.layers = U_BASE, .fingers = 0, .anomaly = 4};
        frame(gone, false);
        check("the misfire shows once the finger lifts", any_lit(48, 63));
        check("and it is centred", centred(48, 63, 4));

        fake_now += U_TRACE_MS + 1;
        step();
        check("and clears itself", !any_lit(48, 63));
    }

    // Shift is excluded the way the mascot's axe excludes it: held constantly as part
    // of typing, so flagging it would say nothing.
    {
        u_split_state_t held = {.layers = U_BASE, .fingers = U_FIN_R_INDEX, .anomaly = 7};
        frame(held, false);
        u_split_state_t gone = {.layers = U_BASE, .fingers = 0, .anomaly = 8};
        frame(gone, false);
        check("a shift misfire is not traced", !any_lit(48, 63));
    }

    // A default layer that is not base has to survive a thumb hold — it used to
    // vanish the moment the band switched to the held layer.
    {
        u_split_state_t quirk = {.layers = (uint8_t)(U_NAV | (U_EXTRA << 4))};
        frame(quirk, false);
        check("a non-base default rules the layer band", any_lit(38, 39));
        check("while the held layer still names itself", any_lit(24, 37));

        u_split_state_t plain = {.layers = U_NAV};
        frame(plain, false);
        check("and a base default does not", !any_lit(38, 39));
    }

    frame(at_rest, false);
    check("returning to rest clears everything again", !any_lit(8, 63));

    // --- the stats page -----------------------------------------------------
    // Dark at rest is kept by only ever showing it on demand.
    {
        check("the page is shut until it is asked for", !u_stats_page_open());

        u_stats_note_sys(true);
        fake_now += 1000;
        step();
        check("a second of holding SYS is not enough", !u_stats_page_open());

        fake_now += 1200;
        step();
        check("two seconds opens it", u_stats_page_open());
        check("and it fills the panel", any_lit(0, 7) && any_lit(24, 31) && any_lit(56, 63));

        u_stats_note_sys(false);
        fake_now += 500;
        step();
        check("it stays after SYS is let go", u_stats_page_open());

        u_stats_note_key();
        step();
        check("any other key closes it", !u_stats_page_open());
        check("and the zones come back", !any_lit(24, 63));

        // A short press is still the OS toggle, which is the behaviour being traded.
        u_stats_note_sys(true);
        fake_now += 300;
        check("a short press still reads as a tap", u_stats_sys_was_tap());
        u_stats_note_sys(false);

        u_stats_note_sys(true);
        fake_now += 2500;
        check("a long one does not", !u_stats_sys_was_tap());
        u_stats_note_sys(false);
        u_stats_note_key();
        step();

        // And it gives up on its own, so it cannot be left burning into the panel.
        u_stats_note_sys(true);
        fake_now += 2100;
        step();
        u_stats_note_sys(false);
        check("it is open", u_stats_page_open());
        fake_now += U_STATS_LINGER_MS + 100;
        step();
        check("and closes itself after fifteen seconds", !u_stats_page_open());
    }

    // --- an unchanged frame must write nothing -----------------------------
    {
        const u_split_state_t caps = {.layers = U_BASE,
                                      .flags  = U_FLAG_CAPS_LOCK | U_FLAG_SCROLL_LOCK};
        const u_split_state_t word = {.layers = U_BASE, .flags = U_FLAG_CAPS_WORD};
        const u_split_state_t dflt = {.layers = (uint8_t)(U_NAV | (U_EXTRA << 4))};

        check("at rest on Windows, a redraw is free", redraw_cost(at_rest, false) == 0);
        check("at rest on the Mac, a redraw is free",
              redraw_cost((u_split_state_t){.layers = U_BASE, .flags = U_FLAG_IS_MAC}, false) == 0);
        check("on Nav, a redraw is free",
              redraw_cost((u_split_state_t){.layers = U_NAV}, false) == 0);
        check("with the locks lit, a redraw is free", redraw_cost(caps, false) == 0);
        check("with Caps Word on, a redraw is free", redraw_cost(word, false) == 0);
        check("with a non-base default, a redraw is free", redraw_cost(dflt, false) == 0);

        bool every_finger_free = true;
        for (uint8_t i = 0; i < 8; i++) {
            const u_split_state_t held = {.layers = U_BASE, .fingers = (uint8_t)(1u << i)};
            if (redraw_cost(held, false) != 0) every_finger_free = false;
        }
        check("every finger of both hands, a redraw is free", every_finger_free);
        check("all eight at once, a redraw is free",
              redraw_cost((u_split_state_t){.layers = U_BASE, .fingers = 0xff}, false) == 0);

        // Past the 600 ms entry blink, where the alarm row is steady.
        drive = at_rest; drive_link_down = true;
        step();
        fake_now += 1000;
        check("LINK DOWN steady, a redraw is free", redraw_cost(at_rest, true) == 0);

        // Inside the 1.5 s the trace is shown.
        frame((u_split_state_t){.layers = U_BASE, .fingers = U_FIN_L_PINKY, .anomaly = 1}, false);
        frame((u_split_state_t){.layers = U_BASE, .fingers = 0, .anomaly = 2}, false);
        fake_dirty = 0;
        fake_now += 50;
        step();
        check("a trace being shown costs nothing to redraw", fake_dirty == 0);
    }

    // --- the sleep predicate ------------------------------------------------
    check("awake at 119,999 ms idle", !u_oled_should_sleep(true, 119999));
    check("asleep at 120,000 ms idle", u_oled_should_sleep(true, 120000));
    check("and nothing to do when already off", !u_oled_should_sleep(false, 500000));

    check("the driver's one-past-the-end byte was never touched",
          fake_oled[OLED_MATRIX_SIZE] == 0);

    return unit_done("panel");
}
