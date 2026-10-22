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
#include "map_legend.h"

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

// The pages' EEPROM word and the kitty's typing speed, both the test's to set.
static uint32_t fake_ee;
static uint8_t  fake_wpm;
static int      ee_writes;
uint32_t        eeconfig_read_user(void) { return fake_ee; }
void            eeconfig_update_user(uint32_t val) { fake_ee = val; ee_writes++; }
uint8_t         get_current_wpm(void) { return fake_wpm; }
static bool     is_keyboard_master(void) { return fake_is_master; }

// The map's questions to QMK. Every key reads as an F-key here; test_map.c draws the maps.
uint16_t   keymap_key_to_keycode(uint8_t layer, keypos_t key) { (void)layer; (void)key; return 1; }
bool       matrix_is_on(uint8_t row, uint8_t col) { (void)row; (void)col; return false; }
u_legend_t u_map_legend(uint16_t keycode, bool mac) { (void)keycode; (void)mac; return (u_legend_t){{'F', '1'}, 0}; }
// Whether OS detection has reported, which the event log asks host_os.c.
static bool fake_detected;
bool        host_os_detected(void) { return fake_detected; }
u_legend_t u_map_short(uint16_t keycode, bool mac) { (void)keycode; (void)mac; return (u_legend_t){{'F'}, 0}; }

#ifndef QMK_KEYBOARD_H
#    define QMK_KEYBOARD_H "qmk_keyboard.h"
#endif
#include "oled_master_zones.c"
#include "oled_settings.c"

// A replug, as these two modules see one: RAM forgets what it loaded, EEPROM keeps it.
static void replug(void) {
    ee_loaded   = false;
    page_loaded = false;
}

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

// A kitty exactly as drawn at her place in the base band.
static bool kitty_is(const uint8_t (*art)[U_KITTY_BYTES], uint8_t f) {
    for (uint8_t y = 0; y < 32; y++)
        for (uint8_t x = 0; x < U_KITTY_W; x++)
            if (lit((uint8_t)(U_KITTY_X + x), (uint8_t)(U_KITTY_Y + y)) !=
                (bool)((art[f][(y / 8) * U_KITTY_W + x] >> (y % 8)) & 1))
                return false;
    return true;
}

static bool monitor_is(void) {
    for (uint8_t y = 0; y < U_MONITOR_BAND_H; y++)
        for (uint8_t x = 0; x < U_MONITOR_BAND_W; x++)
            if (lit((uint8_t)(U_MONITOR_X + x), (uint8_t)(U_MONITOR_Y + y)) !=
                (bool)((u_monitor_band[(y / 8) * U_MONITOR_BAND_W + x] >> (y % 8)) & 1))
                return false;
    return true;
}

static bool qmk_is(void) {
    for (uint8_t y = 0; y < U_LOGO_PAGES * 8; y++)
        for (uint8_t x = 0; x < U_LOGO_W; x++)
            if (lit((uint8_t)(U_QMK_X + x), (uint8_t)(U_QMK_Y + y)) !=
                (bool)((u_qmk_logo[(y / 8) * U_LOGO_W + x] >> (y % 8)) & 1))
                return false;
    return true;
}

// Lit pixels in a box, outside another one (pass an empty one to count the lot).
static int count_in_except(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1,
                           uint8_t ex0, uint8_t ex1, uint8_t ey0, uint8_t ey1) {
    int n = 0;
    for (uint8_t y = y0; y <= y1; y++)
        for (uint8_t x = x0; x <= x1; x++)
            if (lit(x, y) && !(x >= ex0 && x <= ex1 && y >= ey0 && y <= ey1)) n++;
    return n;
}

// A frame to compare against, and whether a box of the panel matches it.
static uint8_t ref[OLED_MATRIX_SIZE];
static void    keep(void) { memcpy(ref, fake_oled, OLED_MATRIX_SIZE); }
static bool    same_in(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1) {
    for (uint8_t y = y0; y <= y1; y++)
        for (uint8_t x = x0; x <= x1; x++)
            if (lit(x, y) != (bool)((ref[(y / 8) * OLED_DISPLAY_WIDTH + x] >> (y % 8)) & 1)) return false;
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

// Move the clock forward to the next multiple of `ms`, which pins which frame a kitty is on.
static void at_multiple_of(uint32_t ms) { fake_now += ms - fake_now % ms; }

// Turn to a page with the key, then let the dots go, so what is checked is the page.
static void show_page(enum u_master_page p) {
    // From the base layer: with a map's layer held, PAGE turns the map's style instead.
    drive.layers = U_BASE;
    step();
    for (int k = 0; u_page_get() != p && k < 2 * U_PAGE_COUNT; k++) u_page_next();
    fake_now += U_DOTS_MS;
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
    u_stats_note_sys(false);
    u_stats_note_key();

    u_page_next();
    frame(at_rest, false);    then("PAGE pressed: the small kitty's bands, the dots showing");
    show_page(U_PAGE_BANDS_KITTY);
    frame(at_rest, false);    then("the small kitty's bands, base, asleep");
    for (uint8_t wpm = 35; wpm <= 60; wpm += 25) {
        fake_wpm = wpm;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
        then(wpm < 50 ? "the small kitty's bands, 35 WPM, watching" : "the small kitty's bands, 60 WPM, bouncing");
    }
    fake_wpm = 0;
    for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
    show_page(U_PAGE_BANDS_STRIPED);
    frame(at_rest, false);    then("the striped kitty's bands, base, asleep");
    fake_wpm = 20;
    for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
    then("the striped kitty's bands, 20 WPM, pawing");
    fake_wpm = 0;
    for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
    show_page(U_PAGE_BANDS_QMK);
    frame(at_rest, false);    then("the QMK logo's bands, base");
    show_page(U_PAGE_BANDS_MONITOR);
    frame(at_rest, false);    then("the Monitor logo's bands, base");
    show_page(U_PAGE_STATS);  then("the Stats page");
    show_page(U_PAGE_BANDS);
}

static void emit(const char *label) {
    (void)label;
    fwrite(fake_oled, 1, sizeof(fake_oled), stdout);
}

// --raw-pages <series>: the stage 5 mock's Status sheets, its Typing page fed `series` (a
// comma-separated WPM a sample), and its Event log, in the mock's order, for a byte compare.
static void canvas_out(void) { fwrite(u_canvas, 1, OLED_MATRIX_SIZE, stdout); }

static void raw_pages(const char *series) {
    const struct { u_split_state_t s; uint8_t wpm, kitty; } sheets[] = {
        {{.layers = U_BASE, .host = U_HOST_WINDOWS}, 0, 0},
        {{.layers = U_NAV, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL | U_FLAG_CAPS_WORD, .host = U_HOST_MAC,
          .fingers = U_FIN_L_PINKY | U_FIN_R_INDEX}, 64, 8},
        {{.layers = U_EXTRA | (U_EXTRA << 4), .flags = U_FLAG_IS_MAC | U_FLAG_CAPS_LOCK, .host = U_HOST_IOS}, 35, 5},
        {{.layers = U_SYM, .flags = U_FLAG_SCROLL_LOCK, .host = U_HOST_LINUX, .fingers = U_FIN_L_INDEX}, 12, 4},
    };
    for (uint8_t i = 0; i < 4; i++) {
        u_status_draw(&sheets[i].s, sheets[i].wpm, sheets[i].kitty);
        canvas_out();
    }
    uint8_t last = 0;
    for (const char *c = series; *c;) {
        last = (uint8_t)strtoul(c, (char **)&c, 10);
        u_typing_task(last);
        fake_now += U_TYPING_MS;
        if (*c == ',') c++;
    }
    u_typing_draw(last);
    canvas_out();
    static const struct { uint32_t min; const char *what; } events[] = {
        {0, "boot e839e2"}, {0, "link up"}, {0, "os win auto"}, {41, "SYS win>mac"},
        {70, "link down"}, {70, "heal reset"}, {70, "link up 1.1s"}, {82, "misfire L pnk"},
    };
    for (uint8_t i = 0; i < 8; i++) {
        fake_now = events[i].min * 60000u;
        u_log_add(events[i].what);
    }
    u_log_draw();
    canvas_out();
}

static bool newest_is(const char *what) {
    const char  *l = u_log_line((uint8_t)(u_log_count() - 1));
    const size_t n = strlen(l), w = strlen(what);
    return n >= w && strcmp(l + n - w, what) == 0;
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
    if (argc > 2 && strcmp(argv[1], "--raw-pages") == 0) {
        raw_pages(argv[2]);
        return 0;
    }

    // At rest the band is not dark any more: it carries the Kyria logo, and the mod
    // row shows all four glyphs whether or not anything is held. That is the Halcyon
    // panel's grammar — the row is a legend you read, and a hold fills a cell in it.
    frame(at_rest, false);
    check("the event log's first line is the boot, with the build's first six characters",
          u_log_count() == 1 && strcmp(u_log_line(0), "+0:00 boot testin") == 0);
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
    // base band's other two versions are checked on their pages, below.
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

    // --- the pages ------------------------------------------------------------
    {
        frame(at_rest, false);
        check("no dots before PAGE is ever pressed", !any_lit_in(21, 105, 0, 7));

        fake_ee = 0;
        replug();
        check("a fresh EEPROM shows the bands", u_page_get() == U_PAGE_BANDS);
        u_page_next();
        check("PAGE turns to the kitty's bands", u_page_get() == U_PAGE_BANDS_KITTY);
        check("and stores it under the tag, with the animation's default beside it",
              fake_ee == (U_SETTINGS_TAG | ((uint32_t)U_PAGE_BANDS_KITTY << 8) | U_ANIM_DEFAULT));
        u_page_next();
        check("then the striped kitty's", u_page_get() == U_PAGE_BANDS_STRIPED);
        u_page_next();
        check("then the QMK logo's", u_page_get() == U_PAGE_BANDS_QMK);
        u_page_next();
        check("then the Monitor logo's", u_page_get() == U_PAGE_BANDS_MONITOR);
        u_page_next();
        check("then the Status sheet", u_page_get() == U_PAGE_STATUS);
        u_page_next();
        check("then the Layer map", u_page_get() == U_PAGE_LAYER_MAP);
        u_page_next();
        check("then Typing", u_page_get() == U_PAGE_TYPING);
        u_page_next();
        check("then Stats", u_page_get() == U_PAGE_STATS);
        u_page_next();
        check("then the Event log", u_page_get() == U_PAGE_EVENT_LOG);
        u_page_next();
        check("and the cycle wraps to the bands", u_page_get() == U_PAGE_BANDS);

        u_page_next();
        u_page_next();
        replug();
        check("a replug keeps the page", u_page_get() == U_PAGE_BANDS_STRIPED);
        u_setting_set(U_SETTING_ANIM, U_ANIM_MONITOR);     // what an ANIM press does
        replug();
        check("an ANIM press keeps the page", u_page_get() == U_PAGE_BANDS_STRIPED);
        u_page_next();
        replug();
        check("and a PAGE press keeps the animation", u_setting_get(U_SETTING_ANIM) == U_ANIM_MONITOR);

        // The word as every build before this one wrote it: the animation, zero above.
        fake_ee = U_SETTINGS_TAG | U_ANIM_KYRIA;
        replug();
        check("a word from before the pages reads as the bands", u_page_get() == U_PAGE_BANDS);
        check("and keeps its animation", u_setting_get(U_SETTING_ANIM) == U_ANIM_KYRIA);

        // The map style's bit (stage 4) beside the page's four, which must not be read as
        // part of it; then a page a later build might have, which the fourth bit allows.
        fake_ee = U_SETTINGS_TAG | ((uint32_t)(0x10 | U_PAGE_STATS) << 8);
        replug();
        check("the map style's bit is not read as part of the page", u_page_get() == U_PAGE_STATS);
        fake_ee = U_SETTINGS_TAG | ((uint32_t)(0x10 | 0x0F) << 8);
        replug();
        check("a page this build has not got reads as the bands", u_page_get() == U_PAGE_BANDS);
        u_page_next();
        check("and PAGE leaves the map style's bit alone",
              ((fake_ee >> 8) & 0xff) == (0x10 | U_PAGE_BANDS_KITTY));

        fake_is_master = false;
        u_page_next();
        fake_is_master = true;
        check("the slave never turns the page", u_page_get() == U_PAGE_BANDS_KITTY);

        // The other half keeps what the master sends, so the next half to get the cable
        // shows the same. One write when something differs, none when nothing does.
        {
            const uint32_t keep_ee = fake_ee;
            fake_ee = U_SETTINGS_TAG | ((uint32_t)U_PAGE_BANDS_QMK << 8) | U_ANIM_SPACESHIP;
            replug();
            ee_writes = 0;
            u_settings_follow(U_ANIM_KYRIA, U_PAGE_STATS);
            check("the other half saves what the master sent, in one write",
                  ee_writes == 1 && fake_ee == (U_SETTINGS_TAG | ((uint32_t)U_PAGE_STATS << 8) | U_ANIM_KYRIA));
            u_settings_follow(U_ANIM_KYRIA, U_PAGE_STATS);
            u_settings_follow(U_ANIM_KYRIA, U_PAGE_STATS);
            check("and writes nothing while it already holds it", ee_writes == 1);
            u_settings_follow(U_ANIM_KYRIA, 0x10 | U_PAGE_STATS);
            check("the map style's bit is kept with the page", ee_writes == 2 && ((fake_ee >> 8) & 0xff) == (0x10 | U_PAGE_STATS));
            replug();
            check("so that half, given the cable, shows the master's page",
                  u_page_get() == U_PAGE_STATS && u_setting_get(U_SETTING_ANIM) == U_ANIM_KYRIA);

            // A fresh EEPROM, joined to a master that sends the defaults: nothing to save.
            fake_ee = 0;
            replug();
            ee_writes = 0;
            u_settings_follow(U_ANIM_DEFAULT, 0);
            check("defaults onto a fresh EEPROM write nothing", ee_writes == 0);
            u_settings_follow(U_ANIM_DEFAULT, U_PAGE_BANDS_KITTY);
            check("and a real choice is written with the tag", ee_writes == 1
                  && fake_ee == (U_SETTINGS_TAG | ((uint32_t)U_PAGE_BANDS_KITTY << 8) | U_ANIM_DEFAULT));

            fake_ee = keep_ee;
            replug();
        }

        // The dots: all ten pages, so x 21-105, the small kitty's the second and larger.
        frame(at_rest, false);
        bool small = true;
        for (uint8_t i = 0; i < 10; i++)
            if (i != 1) small = small && solid_block((uint8_t)(25 + i * 8), (uint8_t)(27 + i * 8), 3, 4);
        check("after a press the dots show, the current one larger",
              solid_block(32, 36, 2, 5) && small && count_in_except(21, 105, 0, 7, 1, 0, 1, 0) == 20 + 9 * 6);
        check("between the OS logo and the locks", !any_lit_in(U_OS_LOGO_W, 23, 0, 7) && !any_lit_in(106, 108, 0, 7));
        fake_dirty = 0;
        fake_now += 50;
        step();
        check("the dots cost nothing to redraw", fake_dirty == 0);
        fake_now += U_DOTS_MS;
        step();
        check("and are gone after 700 ms", !any_lit_in(21, 105, 0, 7));

        // --- the bands, four ways
        fake_wpm = 0;
        show_page(U_PAGE_BANDS);
        frame(at_rest, false);
        keep();
        show_page(U_PAGE_BANDS_KITTY);
        at_multiple_of(1000);
        frame(at_rest, false);
        check("the kitty sits in the emblem's place, asleep at rest", kitty_is(u_kitty_small, 0));
        check("the wordmark is kept", same_in(U_KITTY_CLEAR_W, 127, BAND_Y0, BAND_Y1));
        check("and the emblem is gone, with nothing else in its half",
              count_in_except(0, U_KITTY_CLEAR_W - 1, CORNER_Y1 + 1, BAND_Y1, U_KITTY_X, U_KITTY_X + 31, U_KITTY_Y, U_KITTY_Y + 31) == 0
              && count_in_except(CORNER_X1 + 1, U_KITTY_CLEAR_W - 1, BAND_Y0, CORNER_Y1, U_KITTY_X, U_KITTY_X + 31, U_KITTY_Y, U_KITTY_Y + 31) == 0);
        check("the status bar and the mod row are the bands page's", same_in(0, 127, 0, 7) && same_in(0, 127, MOD_Y0, MOD_Y1));
        fake_now += 500;
        step();
        check("she breathes: the second sleep frame half a second later", kitty_is(u_kitty_small, 1));

        fake_wpm = 35;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
        at_multiple_of(1000);
        step();
        check("at 35 WPM she watches", kitty_is(u_kitty_small, 2));
        fake_wpm = 60;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
        at_multiple_of(600);     // an even number of 300 ms frames, so the first bounce frame
        step();
        // Four samples 300 ms apart alternate only if the frames are 300 ms: 500 ms ones
        // cannot change three times in 900 ms.
        bool bounce = true;
        for (uint8_t k = 0; k < 4; k++) {
            bounce = bounce && kitty_is(u_kitty_small, (uint8_t)(4 + (k & 1)));
            fake_now += 300;
            step();
        }
        check("at 60 WPM she bounces, a frame every 300 ms", bounce);
        fake_wpm = 20;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
        at_multiple_of(1000);
        step();
        check("at 20 WPM, the paw tier, she looks asleep again", kitty_is(u_kitty_small, 0));
        fake_wpm = 45;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
        at_multiple_of(1000);
        step();
        check("Bjorn's hysteresis: rising to 45 from below 50 is still watching", kitty_is(u_kitty_small, 2));
        fake_wpm = 0;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }

        // The striped kitty: the same place, tiers and clock, and paw frames of her own.
        show_page(U_PAGE_BANDS_STRIPED);
        at_multiple_of(1000);
        frame(at_rest, false);
        check("the striped kitty sits in the same place, asleep at rest", kitty_is(u_kitty_striped, 0));
        check("with the wordmark kept and nothing else in the emblem's half",
              same_in(U_KITTY_CLEAR_W, 127, BAND_Y0, BAND_Y1)
              && count_in_except(0, U_KITTY_CLEAR_W - 1, CORNER_Y1 + 1, BAND_Y1, U_KITTY_X, U_KITTY_X + 31, U_KITTY_Y, U_KITTY_Y + 31) == 0
              && count_in_except(CORNER_X1 + 1, U_KITTY_CLEAR_W - 1, BAND_Y0, CORNER_Y1, U_KITTY_X, U_KITTY_X + 31, U_KITTY_Y, U_KITTY_Y + 31) == 0);
        fake_wpm = 20;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
        at_multiple_of(1500);     // three paw frames of 500 ms, so the first
        step();
        const bool paw0 = kitty_is(u_kitty_striped, 2);
        fake_now += 500;
        step();
        const bool paw1 = kitty_is(u_kitty_striped, 3);
        fake_now += 500;
        step();
        check("at 20 WPM she paws, three frames of her own", paw0 && paw1 && kitty_is(u_kitty_striped, 4));
        fake_wpm = 60;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }
        at_multiple_of(600);
        step();
        check("and bounces at 60 WPM", kitty_is(u_kitty_striped, 7));
        fake_wpm = 0;
        for (int i = 0; i < 20; i++) { fake_now += 50; step(); }

        show_page(U_PAGE_BANDS_QMK);
        frame(at_rest, false);
        check("the QMK logo takes the base band, centred below the corner", qmk_is());
        check("and nothing else is in the band",
              count_in_except(0, 127, CORNER_Y1 + 1, BAND_Y1, U_QMK_X, U_QMK_X + U_LOGO_W - 1, U_QMK_Y, U_QMK_Y + 23) == 0
              && !any_lit_in(CORNER_X1 + 1, 127, BAND_Y0, CORNER_Y1));
        check("the status bar and the mod row are the bands page's", same_in(0, 127, 0, 7) && same_in(0, 127, MOD_Y0, MOD_Y1));

        show_page(U_PAGE_BANDS_MONITOR);
        frame(at_rest, false);
        check("the Monitor logo takes the base band, centred below the corner", monitor_is());
        check("with nothing else in the band",
              count_in_except(0, 127, CORNER_Y1 + 1, BAND_Y1, U_MONITOR_X, U_MONITOR_X + U_MONITOR_BAND_W - 1, U_MONITOR_Y,
                              U_MONITOR_Y + U_MONITOR_BAND_H - 1) == 0
              && !any_lit_in(CORNER_X1 + 1, 127, BAND_Y0, CORNER_Y1));

        // The corner, on all four versions, with the busiest thing it holds: iOS set by hand.
        const u_split_state_t ios_hand = {.layers = U_BASE, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL, .host = U_HOST_IOS};
        show_page(U_PAGE_BANDS);
        frame(ios_hand, false);
        keep();
        show_page(U_PAGE_BANDS_KITTY);
        frame(ios_hand, false);
        bool kitty_corner = same_in(0, CORNER_X1, 0, CORNER_Y1);
        show_page(U_PAGE_BANDS_STRIPED);
        frame(ios_hand, false);
        kitty_corner = kitty_corner && same_in(0, CORNER_X1, 0, CORNER_Y1);
        show_page(U_PAGE_BANDS_QMK);
        frame(ios_hand, false);
        kitty_corner = kitty_corner && same_in(0, CORNER_X1, 0, CORNER_Y1);
        show_page(U_PAGE_BANDS_MONITOR);
        frame(ios_hand, false);
        check("all four leave the OS logo's corner as the bands page has it",
              kitty_corner && same_in(0, CORNER_X1, 0, CORNER_Y1));

        // Every other layer, and the alarm, look the same on all three.
        const u_split_state_t others[] = {on_nav, latched, {.layers = (uint8_t)(U_NAV | (U_EXTRA << 4))}};
        bool alike = true;
        for (uint8_t i = 0; i < 3; i++) {
            show_page(U_PAGE_BANDS);
            frame(others[i], false);
            keep();
            show_page(U_PAGE_BANDS_KITTY);
            frame(others[i], false);
            alike = alike && same_in(0, 127, 0, 63);
            show_page(U_PAGE_BANDS_STRIPED);
            frame(others[i], false);
            alike = alike && same_in(0, 127, 0, 63);
            show_page(U_PAGE_BANDS_QMK);
            frame(others[i], false);
            alike = alike && same_in(0, 127, 0, 63);
            show_page(U_PAGE_BANDS_MONITOR);
            frame(others[i], false);
            alike = alike && same_in(0, 127, 0, 63);
        }
        check("every other layer looks the same on all five", alike);

        bool alarm_alike = true;
        show_page(U_PAGE_BANDS);
        frame(at_rest, true);
        fake_now += 1000;     // past the entry blink
        step();
        keep();
        show_page(U_PAGE_BANDS_KITTY);
        frame(at_rest, true);
        alarm_alike = alarm_alike && same_in(0, 127, 0, 63);
        show_page(U_PAGE_BANDS_STRIPED);
        frame(at_rest, true);
        alarm_alike = alarm_alike && same_in(0, 127, 0, 63);
        show_page(U_PAGE_BANDS_QMK);
        frame(at_rest, true);
        alarm_alike = alarm_alike && same_in(0, 127, 0, 63);
        show_page(U_PAGE_BANDS_MONITOR);
        frame(at_rest, true);
        alarm_alike = alarm_alike && same_in(0, 127, 0, 63);
        check("the alarm still fills the base band, whichever version", alarm_alike);
        frame(at_rest, false);

        // --- Stats as a page
        show_page(U_PAGE_STATS);
        frame(at_rest, false);
        memset(u_canvas, 0, OLED_MATRIX_SIZE);
        render_stats();
        check("the Stats page is the SYS page", memcmp(u_canvas, fake_oled, OLED_MATRIX_SIZE) == 0);
        check("without the SYS hold's overlay", !u_stats_page_open());
        u_stats_note_key();
        fake_now += U_STATS_LINGER_MS + 100;
        frame(at_rest, false);
        memset(u_canvas, 0, OLED_MATRIX_SIZE);
        render_stats();
        check("and it stays: no key and no timer closes it", memcmp(u_canvas, fake_oled, OLED_MATRIX_SIZE) == 0);

        show_page(U_PAGE_BANDS_KITTY);
        drive = at_rest;
        open_stats();
        memset(u_canvas, 0, OLED_MATRIX_SIZE);
        render_stats();
        check("holding SYS still opens it from another page", memcmp(u_canvas, fake_oled, OLED_MATRIX_SIZE) == 0);
        u_stats_note_sys(false);
        u_stats_note_key();
        at_multiple_of(1000);
        frame(at_rest, false);
        check("and a key goes back to that page", kitty_is(u_kitty_small, 0));

        // --- every page: a redraw is free, and nothing outshines the alarm
        const u_split_state_t busy[] = {
            at_rest, on_nav, latched,
            {.layers = U_BASE, .fingers = 0xff, .flags = U_FLAG_CAPS_LOCK | U_FLAG_SCROLL_LOCK | U_FLAG_OS_MANUAL},
            {.layers = U_NAV, .fingers = 0xff, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL | U_FLAG_CAPS_WORD, .host = U_HOST_IOS},
        };
        show_page(U_PAGE_BANDS);
        frame(latched, true);
        fake_now += 1000;
        step();
        const int alarm = count(0, 63);
        frame(at_rest, false);

        static const enum u_master_page built[] = {U_PAGE_BANDS, U_PAGE_BANDS_KITTY, U_PAGE_BANDS_STRIPED,
                                                   U_PAGE_BANDS_QMK, U_PAGE_BANDS_MONITOR, U_PAGE_STATS};
        bool free = true, under = true;
        for (uint8_t p = 0; p < 6; p++) {
            show_page(built[p]);
            int worst = 0;
            for (uint8_t i = 0; i < 5; i++) {
                at_multiple_of(1500);     // 300 and 500 ms frames and the stats second all start here,
                fake_now += 100;          // so the redraw's 50 ms crosses none of their edges
                if (redraw_cost(busy[i], false) != 0) free = false;
                for (uint8_t wpm = 0; wpm <= 60; wpm += 30) {
                    fake_wpm = wpm;
                    for (int k = 0; k < 20; k++) { fake_now += 50; step(); }
                    const int n = count(0, 63);
                    if (n > worst) worst = n;
                }
                fake_wpm = 0;
            }
            printf("        page %u lights %d px at most\n", (unsigned)built[p], worst);
            if (worst >= alarm) under = false;
        }
        printf("        the alarm frame lights %d px\n", alarm);
        check("on every page, a redraw is free", free);
        check("and every page stays under the alarm", under);

        for (int k = 0; k < 20; k++) { fake_now += 50; step(); }
        show_page(U_PAGE_BANDS);
    }

    // --- the live layer map ---------------------------------------------------
    // The map itself is test_map.c's; here, when the master shows one, and what PAGE does.
    {
        show_page(U_PAGE_BANDS);
        const u_split_state_t fun = {.layers = U_FUN};
        const bool shaped = u_setting_get(U_SETTING_VIEW) & U_VIEW_MAP_SHAPED;
        frame(fun, false);
        memset(u_canvas, 0, OLED_MATRIX_SIZE);
        u_map_render(U_FUN, true, false, shaped);
        check("holding Fun on a left master shows the Fun map", memcmp(u_canvas, fake_oled, OLED_MATRIX_SIZE) == 0);

        frame((u_split_state_t){.layers = U_FUN, .flags = U_FLAG_IS_MAC}, false);
        memset(u_canvas, 0, OLED_MATRIX_SIZE);
        u_map_render(U_FUN, true, true, shaped);
        check("with the Mac's legends on the Mac", memcmp(u_canvas, fake_oled, OLED_MATRIX_SIZE) == 0);

        frame(on_nav, false);
        check("holding Nav leaves it on the bands: that map is the other half's",
              any_lit(BAND_Y0, BAND_Y1) && cell_drawn(0) && logo_is(U_OS_LOGO_APPLE, false));

        // PAGE turns the map's style while a map is up, on either half, and not the page.
        const uint8_t view = u_setting_get(U_SETTING_VIEW);
        frame(fun, false);
        u_page_next();
        check("PAGE with a map up turns its style",
              u_setting_get(U_SETTING_VIEW) == (view ^ U_VIEW_MAP_SHAPED) && u_page_get() == U_PAGE_BANDS);
        frame(fun, false);
        memset(u_canvas, 0, OLED_MATRIX_SIZE);
        u_map_render(U_FUN, true, false, !shaped);
        check("and the map is drawn in it", memcmp(u_canvas, fake_oled, OLED_MATRIX_SIZE) == 0);
        frame(on_nav, false);
        u_page_next();
        check("the same while the map is on the other half", u_setting_get(U_SETTING_VIEW) == view && u_page_get() == U_PAGE_BANDS);
        replug();
        check("the style is kept across a replug", u_setting_get(U_SETTING_VIEW) == view);

        // Never over the alarm: with the link down there is no map, so PAGE pages again.
        frame((u_split_state_t){.layers = U_BUTTON}, true);
        fake_now += 1000;
        step();
        check("with the link down, holding Button shows the alarm, not the map", row_solid_from(CORNER_X1 + 1, BAND_Y0));
        u_page_next();
        check("and PAGE turns the page", u_page_get() == U_PAGE_BANDS_KITTY && u_setting_get(U_SETTING_VIEW) == (uint8_t)((view & ~U_VIEW_PAGE_MASK) | U_PAGE_BANDS_KITTY));
        frame(at_rest, false);
        show_page(U_PAGE_BANDS);

        // A map redraws for free, like every page.
        check("holding Fun, a redraw is free", redraw_cost(fun, false) == 0);
        frame(at_rest, false);
    }

    // --- stage 5's pages ----------------------------------------------------
    {
        // Everything a page can show at once: every lock, every finger, the Mac by hand.
        const u_split_state_t busy = {.layers = U_BASE, .fingers = 0xff, .host = U_HOST_MAC,
                                      .flags = U_FLAG_CAPS_LOCK | U_FLAG_SCROLL_LOCK | U_FLAG_CAPS_WORD |
                                               U_FLAG_IS_MAC | U_FLAG_OS_MANUAL};
        static const struct { enum u_master_page p; const char *name; bool alarm; } pages[] = {
            {U_PAGE_STATUS, "the Status sheet", false}, {U_PAGE_LAYER_MAP, "the Layer map", false},
            {U_PAGE_TYPING, "Typing", false},           {U_PAGE_EVENT_LOG, "the Event log", true},
        };
        char name[96];
        fake_wpm = 0;
        for (uint8_t i = 0; i < 4; i++) {
            show_page(pages[i].p);
            at_multiple_of(1500);      // the kitty's frame and the typing sample both stand still
            snprintf(name, sizeof name, "%s costs nothing to redraw", pages[i].name);
            check(name, redraw_cost(busy, false) == 0);
            snprintf(name, sizeof name, "%s lights less than the alarm frame", pages[i].name);
            check(name, count(0, 63) < 4584);
            frame(busy, false);
            keep();
            fake_now += 1000;          // past the alarm's three blinks
            frame(busy, true);
            const bool shown = same_in(0, 127, 0, 63);
            show_page(U_PAGE_BANDS);
            fake_now += 1000;
            frame(busy, true);
            const bool bands = same_in(0, 127, 0, 63);
            show_page(pages[i].p);
            fake_now += 1000;
            frame(busy, true);
            if (pages[i].alarm) {
                snprintf(name, sizeof name, "with the link down, %s keeps showing", pages[i].name);
                check(name, shown && !bands);
            } else {
                snprintf(name, sizeof name, "with the link down, %s gives way to the bands", pages[i].name);
                keep();
                show_page(U_PAGE_BANDS);
                fake_now += 1000;
                frame(busy, true);
                check(name, !shown && same_in(0, 127, 0, 63));
                show_page(pages[i].p);
            }
            frame(at_rest, false);
        }
        show_page(U_PAGE_BANDS);

        // The Status sheet's kitty is the striped one, on the bands' clock.
        show_page(U_PAGE_STATUS);
        at_multiple_of(1000);
        frame(at_rest, false);
        {
            bool same = true;
            for (uint8_t y = 0; y < 32; y++)
                for (uint8_t x = 0; x < U_KITTY_W; x++)
                    same = same && lit((uint8_t)(4 + x), (uint8_t)(9 + y)) ==
                                       (bool)((u_kitty_striped[0][(y / 8) * U_KITTY_W + x] >> (y % 8)) & 1);
            check("the Status sheet's kitty is the striped one asleep, at (4,9)", same);
        }
        show_page(U_PAGE_BANDS);

        // Typing keeps sampling on every page, and the peak is the session's.
        fake_wpm = 90;
        for (int k = 0; k < 10; k++) { fake_now += 50; step(); }
        fake_wpm = 20;
        for (int k = 0; k < 10; k++) { fake_now += 50; step(); }
        show_page(U_PAGE_TYPING);
        frame(at_rest, false);
        check("Typing's graph has a column at the top for the 90 WPM burst", any_lit_in(66, 127, 1, 6));
        show_page(U_PAGE_BANDS);

        // The event ring: eight lines, the ninth pushes out the first, oldest first.
        char want[32];
        for (uint8_t i = 0; i < 10; i++) {
            snprintf(want, sizeof want, "e%u", i);
            u_log_add(want);
        }
        check("the ring holds eight", u_log_count() == 8);
        check("oldest first, so the first two are gone", newest_is("e9") && strcmp(u_log_line(0) + strlen(u_log_line(0)) - 2, "e2") == 0);
        fake_now += 70u * 60000u - fake_now % 60000u;
        const uint32_t m = fake_now / 60000u;
        u_log_add("a line too long for the panel to hold");
        snprintf(want, sizeof want, "+%u:%02u a line too long for the panel to hold", (unsigned)(m / 60), (unsigned)(m % 60));
        want[U_LOG_CHARS] = 0;
        check("stamped in hours and minutes since boot, and cut to a line", strcmp(u_log_line(7), want) == 0);

        // The events, as the watcher finds them on the master.
        char before[U_LOG_CHARS + 1];
        strcpy(before, u_log_line(7));
        drive_link_down = true;
        step();
        drive_link_down = false;
        step();
        check("before the first reply the link has no edges to log", strcmp(u_log_line(7), before) == 0);
        const u_health_slave_t reply = {.resets = 0};
        u_health_note_reply(&reply, sizeof reply);
        step();
        check("the first reply logs the link up", newest_is("link up"));
        drive_link_down = true;
        step();
        check("then a lost link logs down", newest_is("link down"));
        fake_now += 1100;
        drive_link_down = false;
        step();
        check("and back up, with how long it was gone", newest_is("link up 1.1s"));
        const u_health_slave_t healed = {.resets = 1};
        u_health_note_reply(&healed, sizeof healed);
        step();
        check("a self-heal reset of the other half is logged", newest_is("heal reset"));
        drive_link_down = true;
        step();
        fake_now += 2500;
        const u_health_slave_t again = {.resets = 2};
        u_health_note_reply(&again, sizeof again);
        drive_link_down = false;
        step();
        check("and when it ends an outage, the log reads down, reset, up",
              newest_is("link up 2.5s") && strstr(u_log_line(6), "heal reset") && strstr(u_log_line(5), "link down"));
        fake_detected = true;
        frame(at_rest, false);
        check("detection saying Windows is logged, though nothing changed", newest_is("os win auto"));
        frame((u_split_state_t){.layers = U_BASE, .host = U_HOST_MAC, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL}, false);
        check("SYS is logged as from and to", newest_is("SYS win>mac"));
        frame((u_split_state_t){.layers = U_BASE, .host = U_HOST_MAC, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL,
                                .fingers = U_FIN_L_PINKY, .anomaly = 100}, false);
        frame((u_split_state_t){.layers = U_BASE, .host = U_HOST_MAC, .flags = U_FLAG_IS_MAC | U_FLAG_OS_MANUAL,
                                .anomaly = 101}, false);
        check("a misfire is logged with its hand and finger", newest_is("misfire L pnk"));
        frame(at_rest, false);
        strcpy(before, u_log_line(7));
        const char *slot = u_log_line(0);
        frame(at_rest, false);
        frame(at_rest, false);
        check("and nothing repeats while nothing happens", strcmp(u_log_line(7), before) == 0 && u_log_line(0) == slot);

        show_page(U_PAGE_EVENT_LOG);
        frame(at_rest, false);
        check("the Event log draws its eight lines", any_lit_in(0, 30, 0, 6) && any_lit_in(0, 30, 56, 62));
        show_page(U_PAGE_BANDS);
        frame(at_rest, false);
    }

    // --- the sleep predicate ------------------------------------------------
    check("awake at 119,999 ms idle", !u_oled_should_sleep(true, 119999));
    check("asleep at 120,000 ms idle", u_oled_should_sleep(true, 120000));
    check("and nothing to do when already off", !u_oled_should_sleep(false, 500000));

    check("the driver's one-past-the-end byte was never touched",
          fake_oled[OLED_MATRIX_SIZE] == 0);

    return unit_done("panel");
}
