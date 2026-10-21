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
bool     fake_is_left   = true;
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

// The OS logo's corner: x 0-15, y 8-17, which every band keeps dark. 15 because iOS
// puts an "i" and a 2 px gap in front of the logo and moves the logo 4 px right.
#define CORNER_X1 15
#define CORNER_Y1 17
#define LOGO_LINE 16     // the SYS underline, a row clear of the logo

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
//
// Over the band only, and blind to the OS logo's corner (x 0-15, y 8-17), which holds
// the logo's lower half rather than any of the layer's icon.
static bool centred(uint8_t y0, uint8_t y1, int slop) {
    int left = -1, right = -1;
    for (uint8_t x = 0; x < OLED_DISPLAY_WIDTH; x++) {
        for (uint8_t y = y0; y <= y1; y++) {
            if (x <= CORNER_X1 && y <= CORNER_Y1) continue;
            if (lit(x, y)) {
                if (left < 0) left = x;
                right = OLED_DISPLAY_WIDTH - 1 - x;
                break;
            }
        }
    }
    if (left < 0) return false;
    const int off = left - right;
    return (off < 0 ? -off : off) <= slop;
}

// The three bands. Everything below addresses rows, so the map lives here once.
#define BAND_Y0 8
#define BAND_Y1 47
#define MOD_Y0  48
#define MOD_Y1  63
#define RULE_Y0 45     // the default-layer rule, the band's last three rows

// Which of the three cell states is on screen, read off the cell's top row — padding
// that no glyph reaches, so it says how far the fill got and nothing else.
//
// Columns 5 and 26 are the probes rather than the whole row, because a held cell is
// not solid: the hand bars at +1..+3 and +28..+30 are cut out of it, so "every column
// lit" is false for a legitimate hold. 5 and 26 are clear of both bars and of the
// glyph's own box at +8..+23.
#define CELL_PAD_L 5
#define CELL_PAD_R 26
#define CELL_GLYPH 12

static bool cell_filled(uint8_t cell) {
    return lit((uint8_t)(cell * 32 + CELL_PAD_L), MOD_Y0)
        && lit((uint8_t)(cell * 32 + CELL_PAD_R), MOD_Y0);
}

// A misfire chip fills only the glyph's own 16 px box, so the padding stays clear.
static bool cell_chipped(uint8_t cell) {
    return lit((uint8_t)(cell * 32 + CELL_GLYPH), MOD_Y0)
        && !lit((uint8_t)(cell * 32 + CELL_PAD_L), MOD_Y0);
}

static bool cell_drawn(uint8_t cell) {
    return any_lit_in((uint8_t)(cell * 32 + 8), (uint8_t)(cell * 32 + 23), MOD_Y0, MOD_Y1);
}

// The default-layer rule is the one thing on the band that is a solid block: three
// whole rows spanning the icon's ink box. No reduced icon is solid across its own full
// width for three rows, so this identifies the rule and nothing else.
static bool solid_block(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1) {
    for (uint8_t y = y0; y <= y1; y++)
        for (uint8_t x = x0; x <= x1; x++)
            if (!lit(x, y)) return false;
    return true;
}

static bool any_dark_in(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1) {
    for (uint8_t y = y0; y <= y1; y++)
        for (uint8_t x = x0; x <= x1; x++)
            if (!lit(x, y)) return true;
    return false;
}

// From x0 to the right edge. The band's rows are probed from just past the OS logo's
// corner, because under the alarm the logo is cut out of the lit band there.
static bool row_solid_from(uint8_t x0, uint8_t y) {
    for (uint8_t x = x0; x < OLED_DISPLAY_WIDTH; x++)
        if (!lit(x, y)) return false;
    return true;
}

// The logo exactly as drawn: its top half on the status bar, its bottom half lit on the
// dark corner, or cut out of the lit band under the alarm. Exact bytes, so this also
// proves the corner around the logo is uniformly dark, or uniformly lit.
static bool logo_at(uint8_t which, uint8_t x0, bool cut) {
    for (uint8_t i = 0; i < U_OS_LOGO_W; i++) {
        const uint8_t top = u_os_logo[which][i], bot = u_os_logo[which][U_OS_LOGO_W + i];
        if (fake_oled[x0 + i] != top) return false;
        if (fake_oled[OLED_DISPLAY_WIDTH + x0 + i] != (uint8_t)(cut ? ~bot : bot)) return false;
    }
    return true;
}

static bool logo_is(uint8_t which, bool cut) { return logo_at(which, 0, cut); }

// iOS: the "i" at x 0-1, a dark gap, then the Apple logo.
static bool ios_is(bool cut) {
    for (uint8_t i = 0; i < U_IOS_I_W; i++) {
        if (fake_oled[i] != u_ios_i[i]) return false;
        if (fake_oled[OLED_DISPLAY_WIDTH + i] != (uint8_t)(cut ? ~u_ios_i[U_IOS_I_W + i] : u_ios_i[U_IOS_I_W + i])) return false;
    }
    for (uint8_t gap = U_IOS_I_W; gap < U_IOS_I_W + U_IOS_I_GAP; gap++) {
        if (fake_oled[gap] != 0 || fake_oled[OLED_DISPLAY_WIDTH + gap] != (uint8_t)(cut ? 0xff : 0x00)) return false;
    }
    return logo_at(U_OS_LOGO_APPLE, U_IOS_I_W + U_IOS_I_GAP, cut);
}

// The chamfer itself: the outermost pixel of each corner of a filled cell is off.
static bool cell_chamfered(uint8_t cell) {
    const uint8_t x0 = (uint8_t)(cell * 32), x1 = (uint8_t)(cell * 32 + 31);
    return !lit(x0, MOD_Y0) && !lit(x1, MOD_Y0)
        && !lit(x0, MOD_Y1) && !lit(x1, MOD_Y1)
        && lit((uint8_t)(x0 + CELL_PAD_L), MOD_Y0);   // but not a whole bite out of it
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
    .host    = U_HOST_MAC,
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

    // At rest the band is not dark any more: it carries the Kyria logo, and the mod
    // row shows all four glyphs whether or not anything is held. That is the Halcyon
    // panel's grammar — the row is a legend you read, and a hold fills a cell in it.
    frame(at_rest, false);
    check("at rest the band shows the base layer's logo", any_lit(BAND_Y0, BAND_Y1));
    check("and the mod row shows every finger's glyph",
          cell_drawn(0) && cell_drawn(1) && cell_drawn(2) && cell_drawn(3));
    check("with nothing held, no cell is filled",
          !cell_filled(0) && !cell_filled(1) && !cell_filled(2) && !cell_filled(3));
    {
        int left, right;
        margins(0, 7, &left, &right);
        // x=0 specifically: docs/layout.md rests the "no 1 px edge frame" argument on
        // the OS logo lighting the first column, so this is the assertion that keeps it
        // from going quietly false.
        check("the OS logo starts at the left edge", left == 0);
        check("and stays inside its own twelve columns", right >= OLED_DISPLAY_WIDTH - U_OS_LOGO_W);
    }
    check("on Windows it is the Windows logo, its lower half in the band's corner",
          logo_is(U_OS_LOGO_WINDOWS, false) && any_lit_in(0, U_OS_LOGO_W - 1, 8, 15));
    check("no underline while detection is trusted", !any_lit_in(0, CORNER_X1, LOGO_LINE, CORNER_Y1));

    // Set by hand: the logo becomes a decision rather than a reading.
    frame(on_nav, false);
    {
        bool solid = true;
        for (uint8_t x = 0; x < U_OS_LOGO_W; x++) solid = solid && lit(x, LOGO_LINE);
        check("set by hand underlines the logo", solid);
        check("a row clear of it", !any_lit_in(0, CORNER_X1, LOGO_LINE - 1, LOGO_LINE - 1));
        check("and the underline stops with it", !lit(U_OS_LOGO_W, LOGO_LINE));
    }
    check("on the Mac it is the Apple logo", logo_is(U_OS_LOGO_APPLE, false));

    // The logo follows the host byte, not the Mac flag: Linux behaves like Windows and
    // shows Tux, and iOS is a Mac for the mod row and still shows the Apple logo until
    // its own mark is picked.
    frame((u_split_state_t){.layers = U_BASE, .host = U_HOST_LINUX}, false);
    check("on Linux it is Tux", logo_is(U_OS_LOGO_LINUX, false));
    frame((u_split_state_t){.layers = U_BASE, .flags = U_FLAG_IS_MAC, .host = U_HOST_IOS}, false);
    check("on iOS an \"i\" stands in front of the Apple logo", ios_is(false));
    check("and the \"i\" keeps column 0 lit", lit(0, 13));
    frame((u_split_state_t){.layers = U_BASE, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL, .host = U_HOST_IOS}, false);
    {
        bool solid = true;
        for (uint8_t x = 0; x < U_IOS_I_W + U_IOS_I_GAP + U_OS_LOGO_W; x++) solid = solid && lit(x, LOGO_LINE);
        check("set by hand on iOS, the line runs under the \"i\" too",
              solid && !lit(U_IOS_I_W + U_IOS_I_GAP + U_OS_LOGO_W, LOGO_LINE));
    }
    frame(on_nav, false);

    // Cells are fingers, outer to inner, and they do not move with the OS: this frame
    // is on the Mac, where Ctrl and GUI swap, holding left index and right ring.
    check("a held finger fills its cell", cell_filled(1) && cell_filled(3));
    check("and the fill has rounded corners", cell_chamfered(1) && cell_chamfered(3));
    check("an idle finger is drawn but not filled", !cell_filled(0) && !cell_filled(2));
    check("an idle finger still shows its glyph", cell_drawn(0) && cell_drawn(2));
    // Handedness is no longer drawn: a held cell is solid padding to its chamfered
    // corners, with nothing cut out of it but the glyph.
    check("a held cell has no hand bar cut out of it",
          lit((uint8_t)(1 * 32 + 1), MOD_Y0 + 4) && lit((uint8_t)(1 * 32 + 30), MOD_Y0 + 4));

    check("the layer icon is centred", centred(BAND_Y0, BAND_Y1, 4));
    check("caps word shows in the status bar's right end", any_lit_in(100, 127, 0, 7));
    {
        // Caps Word lights the Caps Lock glyph itself, the way drashna's screen does.
        uint8_t word[8], caps[8];
        frame((u_split_state_t){.layers = U_BASE, .flags = U_FLAG_CAPS_WORD}, false);
        memcpy(word, &fake_oled[109], 8);
        frame((u_split_state_t){.layers = U_BASE, .flags = U_FLAG_CAPS_LOCK}, false);
        memcpy(caps, &fake_oled[109], 8);
        check("Caps Word lights the Caps Lock glyph", memcmp(word, caps, 8) == 0 && any_lit_in(109, 117, 0, 7));
        check("and nothing is drawn in the slot Caps Word used to have", !any_lit_in(100, 108, 0, 7));
        frame(on_nav, false);
    }

    // The ladder: nothing routine may light more of the panel than the alarm. The
    // worst routine frame is four fingers of both hands held at once; the alarm is the
    // whole 128x40 band. This is the measurement that retired the old banner, which at
    // 128x8 had fallen to roughly two held cells.
    {
        u_split_state_t all = on_nav;
        all.fingers = 0xff;
        frame(all, false);
        const int mods = count(MOD_Y0, MOD_Y1);
        frame(latched, true);
        const int alarm = count(BAND_Y0, BAND_Y1);
        check("the alarm outshines the worst routine frame", alarm > mods * 2);
        // Put the link back up before leaving: the entry blink is measured from the
        // edge, so an alarm left latched here would make the next block's first frame
        // land in the steady state instead.
        frame(at_rest, false);
    }

    // The alarm. Three blinks over 600 ms on entry, then steady — and it inverts,
    // which nothing else on this panel is allowed to do.
    fake_now = 10000;
    frame(latched, true);
    check("the whole band inverts on entry",
          row_solid_from(CORNER_X1 + 1, BAND_Y0) && count(BAND_Y0, BAND_Y1) > 3500);
    fake_now += 100;
    step();
    check("and returns to normal a blink later", !row_solid_from(CORNER_X1 + 1, BAND_Y0));
    check("with the OS logo lit on the dark corner again", logo_is(U_OS_LOGO_WINDOWS, false));
    fake_now += 100;
    step();
    check("and inverts again", row_solid_from(CORNER_X1 + 1, BAND_Y0));
    fake_now += 1000;
    step();
    check("steady once the blinking is over",
          row_solid_from(CORNER_X1 + 1, BAND_Y0) && count(BAND_Y0, BAND_Y1) > 3500);
    check("the OS logo is cut out of the lit band", logo_is(U_OS_LOGO_WINDOWS, true));
    // Escalation by scope, not replacement. The chain is cut out of the inverted band
    // rather than drawn on it, so its own 40x24 box holds dark pixels while a patch of
    // band well clear of it stays solid — which is also what proves the band really is
    // inverted underneath and not merely blanked.
    check("the chain is knocked out of the inverted band", any_dark_in(2, 41, 16, 39));
    check("and the band around it is still solid", !any_dark_in(90, 125, 16, 39));
    check("the layer icon survives under the alarm", any_dark_in(48, 79, 16, 39));

    // Locks: two of three on, and Num Lock can never be, so its slot is never drawn.
    check("both latched locks show", any_lit_in(109, 117, 0, 7) && any_lit_in(118, 126, 0, 7));
    check("left of them the status bar stays dark", !any_lit_in(U_OS_LOGO_W, 108, 0, 7));

    // Set by hand under the alarm: the underline is cut out of the lit band too, and the
    // corner beside it stays lit. On base the band is filled rather than inverted, and
    // the logo is cut out of that the same way.
    {
        u_split_state_t by_hand = latched;
        by_hand.flags |= U_FLAG_IS_MAC | U_FLAG_OS_MANUAL;
        by_hand.host = U_HOST_MAC;
        frame(by_hand, true);
        check("under the alarm the underline is cut out as well",
              !any_lit_in(0, U_OS_LOGO_W - 1, LOGO_LINE, LOGO_LINE) && lit(U_OS_LOGO_W, LOGO_LINE)
              && logo_is(U_OS_LOGO_APPLE, true));
        frame(at_rest, true);
        check("and on base, the logo is cut out of the filled band", logo_is(U_OS_LOGO_WINDOWS, true));
        frame((u_split_state_t){.layers = U_BASE, .flags = U_FLAG_IS_MAC, .host = U_HOST_IOS}, true);
        check("on iOS, the \"i\" is cut out with it", ios_is(true));
        frame(latched, true);
    }

    // The trace. A misfire is a count that changed and a finger bit that dropped on
    // the same loop; it is latched because the render would otherwise miss it.
    {
        u_split_state_t held = {.layers = U_BASE, .fingers = U_FIN_L_PINKY, .anomaly = 3};
        frame(held, false);
        check("a finger still down is a hold, not a chip", cell_filled(0) && !cell_chipped(0));

        u_split_state_t gone = {.layers = U_BASE, .fingers = 0, .anomaly = 4};
        frame(gone, false);
        check("the misfire chips that finger's own cell", cell_chipped(0));
        check("and leaves the other three alone",
              !cell_chipped(1) && !cell_chipped(2) && !cell_chipped(3));

        // Pressing again while the chip is up grows it to a full hold rather than
        // changing vocabulary — the whole reason the two share one drawing.
        u_split_state_t again = {.layers = U_BASE, .fingers = U_FIN_L_PINKY, .anomaly = 4};
        frame(again, false);
        check("pressing again grows the chip to a hold", cell_filled(0));

        frame(gone, false);
        fake_now += U_TRACE_MS + 1;
        step();
        check("and it clears itself", !cell_chipped(0) && !cell_filled(0));
    }

    // Shift is excluded the way the mascot's axe excludes it: held constantly as part
    // of typing, so flagging it would say nothing.
    {
        u_split_state_t held = {.layers = U_BASE, .fingers = U_FIN_R_INDEX, .anomaly = 7};
        frame(held, false);
        u_split_state_t gone = {.layers = U_BASE, .fingers = 0, .anomaly = 8};
        frame(gone, false);
        check("a shift misfire is not traced", !cell_chipped(3));
    }

    // A default layer that is not base has to survive a thumb hold — it used to
    // vanish the moment the band switched to the held layer.
    {
        u_split_state_t quirk = {.layers = (uint8_t)(U_NAV | (U_EXTRA << 4))};
        frame(quirk, false);
        check("a non-base default rules the layer band", solid_block(48, 78, RULE_Y0, BAND_Y1));
        check("while the held layer still shows its own icon", any_lit(BAND_Y0, 44));

        // Resting on that default: the band shows Extra itself, so a rule under it
        // would only repeat what the icon already says. Probed inside Extra's own ink
        // box — its disc spans x45..83, so columns outside that say nothing either way
        // and a check there would pass whether or not the rule was drawn.
        u_split_state_t resting = {.layers = (uint8_t)(U_EXTRA | (U_EXTRA << 4))};
        frame(resting, false);
        check("resting on it needs no rule", !solid_block(45, 83, RULE_Y0, BAND_Y1));

        u_split_state_t plain = {.layers = U_NAV};
        frame(plain, false);
        check("and a base default never draws one", !any_lit(RULE_Y0, BAND_Y1));
    }

    // The mod row follows the half the panel is on. Outer-to-inner runs left to right
    // on the left half and the other way on the right, so drawing pinky-first on a
    // right-hand master puts every glyph above the wrong finger.
    {
        u_split_state_t pinky = {.layers = U_BASE, .fingers = U_FIN_L_PINKY};
        fake_is_left = true;
        frame(pinky, false);
        check("on a left master the pinky is the outermost cell on the left",
              cell_filled(0) && !cell_filled(3));

        fake_is_left = false;
        frame(pinky, false);
        check("on a right master it is the outermost cell on the right",
              cell_filled(3) && !cell_filled(0));
        fake_is_left = true;
    }

    frame(at_rest, false);
    check("returning to rest shows the base logo again", any_lit(BAND_Y0, BAND_Y1));

    // The OS logo's bottom half and its underline live in the band's top-left corner, so
    // every band has to leave it dark. Asserted on the art itself, all ten bands; the
    // base band's other versions (docs/oled-v3.md stage 3) join this when they exist.
    {
        bool dark = true;
        for (uint8_t b = U_BASE; b <= U_FUN; b++)
            for (uint8_t x = 0; x <= CORNER_X1; x++)
                for (uint8_t y = 0; y <= CORNER_Y1 - BAND_Y0; y++)
                    if ((u_layer_band[b][(y / 8) * U_BAND_W + x] >> (y % 8)) & 1) dark = false;
        check("every band leaves the OS logo's corner dark", dark);
    }

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
        check("and the bands come back", any_lit(BAND_Y0, BAND_Y1) && cell_drawn(0));

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
        // Every lock flag at once: Caps Lock and Caps Word share a glyph.
        check("every lock flag at once, a redraw is free",
              redraw_cost((u_split_state_t){.layers = U_BASE,
                                            .flags = U_FLAG_CAPS_LOCK | U_FLAG_CAPS_WORD | U_FLAG_SCROLL_LOCK},
                          false) == 0);
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
        check("with the OS set by hand, a redraw is free",
              redraw_cost((u_split_state_t){.layers = U_BASE, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL,
                                            .host = U_HOST_MAC}, false) == 0);
        check("on Linux, a redraw is free",
              redraw_cost((u_split_state_t){.layers = U_BASE, .host = U_HOST_LINUX}, false) == 0);
        check("on iOS set by hand, a redraw is free",
              redraw_cost((u_split_state_t){.layers = U_BASE, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL,
                                            .host = U_HOST_IOS}, false) == 0);

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
        check("and on a held layer with the OS set by hand, still free",
              redraw_cost((u_split_state_t){.layers = U_NAV, .flags = U_FLAG_OS_MANUAL}, true) == 0);

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
