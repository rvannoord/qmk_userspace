// The two stage 6 slave scenes, from oled_slave_bongo.c and oled_slave_mario.c themselves.
// Bongo cat picks his frames by WPM. The Mario scene runs over scripted WPM curves, and on
// every tick Bill's pixels must never meet Mario's or Yoshi's, no sprite may enter the WPM
// corner or go above the top edge, and a scene at rest must cost nothing to redraw.
#include <stdlib.h>
#include <string.h>
#include "unit.h"
#include "qmk_keyboard.h"
#include "driver.h"
#include "oled_canvas.h"

uint8_t  fake_oled[OLED_MATRIX_SIZE + 1];
uint16_t fake_dirty;
uint32_t fake_now;
bool     fake_is_left = true;

void oled_write_raw_byte(const char data, uint16_t index) { u_fake_write(data, index); }

static uint8_t  fake_wpm;
static uint32_t fake_idle_ms;
uint8_t         get_current_wpm(void) { return fake_wpm; }
uint32_t        last_input_activity_elapsed(void) { return fake_idle_ms; }
static bool     fake_stale;
bool            u_sync_hard_stale(void) { return fake_stale; }

#include "oled_slave_bongo.c"
#include "oled_slave_mario.c"
#include "oled_slave_logo.c"

// How many of `n` renders were glitch burst frames, with the link dead.
static int glitches(void (*render)(void), int n) {
    int g = 0;
    fake_stale = true;
    for (int k = 0; k < n; k++) {
        const uint8_t was = u_glitch_clock.left;
        fake_now += 50;
        render();
        if (u_glitch_clock.left != was && u_glitch_clock.armed) g++;
    }
    fake_stale = false;
    render();
    return g;
}

static void mario_both(void) { u_slave_mario_task(); u_slave_mario_render(); }
static void kyria(void) { u_slave_logo_render(U_ANIM_KYRIA); }
static void monitor(void) { u_slave_logo_render(U_ANIM_MONITOR); }
static void stacked(void) { u_slave_logo_render(U_ANIM_MONITOR_STACKED); }
static void future(void) { u_slave_logo_render(U_ANIM_MONITOR_FUTURE); }

// --- looking --------------------------------------------------------------------------

static bool px(const uint8_t *f, int x, int y) { return (f[(y / 8) * OLED_DISPLAY_WIDTH + x] >> (y % 8)) & 1; }

static bool any_in(const uint8_t *f, int x0, int x1, int y0, int y1) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (px(f, x, y)) return true;
    return false;
}

// A sprite's ink: leftmost and rightmost column, top and bottom row.
static void ink(const uint8_t *src, int w, int pages, int *l, int *r, int *t, int *b) {
    *l = w;
    *r = -1;
    *t = pages * 8;
    *b = -1;
    for (int q = 0; q < pages; q++)
        for (int x = 0; x < w; x++)
            for (int k = 0; k < 8; k++)
                if ((src[q * w + x] >> k) & 1) {
                    if (x < *l) *l = x;
                    if (x > *r) *r = x;
                    if (q * 8 + k < *t) *t = q * 8 + k;
                    if (q * 8 + k > *b) *b = q * 8 + k;
                }
}

static uint8_t bill_layer[OLED_MATRIX_SIZE], chars_layer[OLED_MATRIX_SIZE];

static void layers(void) {
    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    draw_bill();
    memcpy(bill_layer, u_canvas, OLED_MATRIX_SIZE);
    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    draw_mario();
    draw_yoshi();
    memcpy(chars_layer, u_canvas, OLED_MATRIX_SIZE);
}

// --- the scripted runs ----------------------------------------------------------------

static int collisions, cornered, above, passes, jumps[2], ticks;

static void one_tick(uint8_t wpm, bool typing) {
    const bool was_on = sc.bill_on, jumped[2] = {sc.jump[0].on, sc.jump[1].on};
    fake_now += U_TICK_MS;
    fake_wpm     = wpm;
    fake_idle_ms = typing ? 0 : fake_idle_ms + U_TICK_MS;
    u_slave_mario_task();
    ticks++;
    if (was_on && !sc.bill_on) passes++;
    for (int c = 0; c < 2; c++)
        if (!jumped[c] && sc.jump[c].on) jumps[c]++;

    layers();
    for (int i = 0; i < OLED_MATRIX_SIZE; i++)
        if (bill_layer[i] & chars_layer[i]) {
            collisions++;
            break;
        }
    if (any_in(bill_layer, 84, 127, 0, 25) || any_in(chars_layer, 84, 127, 0, 25)) cornered++;
    if (mario_y() < 0 || yoshi_y() < 0) above++;
}

// From one WPM to another over `ms`, typing all the while or not at all.
static void ramp(uint32_t ms, int from, int to, bool typing) {
    const int n = (int)(ms / U_TICK_MS);
    for (int i = 0; i < n; i++) one_tick((uint8_t)(from + (to - from) * i / (n > 1 ? n - 1 : 1)), typing);
}

int main(void) {
    fake_now = 100000;

    // --- the art's measurements, which the scene's constants are --------------------
    {
        int l, r, t, b;
        ink(u_bill, U_BILL_W, U_BILL_PAGES, &l, &r, &t, &b);
        check("Bill's ink runs from his nose at x 2 to his tail at x 29",
              l == U_BILL_INK_L && r == U_BILL_INK_R);
        check("and from y 41 to y 61 where he flies", U_BILL_Y0 + t == 41 && U_BILL_Y0 + b == 61);
        ink(u_mario[U_MARIO_JUMP], U_MARIO_W, U_MARIO_PAGES, &l, &r, &t, &b);
        check("Mario in the air is inside the extent Bill clears", U_MARIO_X + l >= air[U_MARIO].l && U_MARIO_X + r <= air[U_MARIO].r);
        bool inside = true;
        for (int f = 0; f < U_YOSHI_FLY_N; f++) {
            ink(u_yoshi[U_YOSHI_FLY + f], U_YOSHI_W, U_YOSHI_PAGES, &l, &r, &t, &b);
            inside = inside && U_YOSHI_X + l >= air[U_YOSHI].l && U_YOSHI_X + r <= air[U_YOSHI].r;
        }
        check("and so is Yoshi on every fly frame", inside);
        ink(u_mario[U_MARIO_STAND], U_MARIO_W, U_MARIO_PAGES, &l, &r, &t, &b);
        const bool mario_feet = U_FEET_Y0 + b == 62;
        ink(u_yoshi[U_YOSHI_IDLE], U_YOSHI_W, U_YOSHI_PAGES, &l, &r, &t, &b);
        check("both stand with their feet on y=62", mario_feet && U_FEET_Y0 + b == 62);
    }

    // --- Bill never touches them ------------------------------------------------------
    static const uint8_t steady[] = {10, 12, 16, 20, 30, 45, 60, 80, 110, 140, 200};
    for (unsigned i = 0; i < sizeof steady; i++) ramp(30000, steady[i], steady[i], true);
    ramp(20000, 0, 120, true);
    ramp(20000, 120, 10, true);
    ramp(10000, 10, 160, true);
    ramp(10000, 160, 30, true);
    // Typing stops with Bill on his way: everyone stays up until he has gone.
    for (int k = 0; k < 6; k++) {
        ramp(7000, 60, 60, true);
        ramp(9000, 60, 0, false);
    }
    printf("  (%d ticks, %d of Bill's passes, %d and %d jumps)\n", ticks, passes, jumps[0], jumps[1]);
    check("Bill came by often enough to mean something", passes >= 80 && jumps[0] >= 80 && jumps[1] >= 80);
    check("and his pixels never met Mario's or Yoshi's", collisions == 0);
    check("nobody ever entered the WPM corner", cornered == 0);
    check("and nothing went above the top edge", above == 0);

    // --- Yoshi curls up 4 s after the last key (D13) ----------------------------------
    {
        ramp(4000, 40, 40, true);
        while (sc.bill_on || sc.jump[0].on || sc.jump[1].on) one_tick(0, false);
        fake_idle_ms = 0;
        ramp(3900, 0, 0, false);
        check("not yet at 3.9 s", !sc.curl && yoshi_frame() < U_YOSHI_HIDE);
        // The frames he shows from here, each once, from the first of hide on.
        uint8_t seen[8], n = 0, last = yoshi_frame();
        for (int k = 0; k < 40; k++) {
            one_tick(0, false);
            const uint8_t f = yoshi_frame();
            if (f != last && f >= U_YOSHI_HIDE && n < sizeof seen) seen[n++] = f;
            last = f;
        }
        check("from 4 s he curls up through hide, and stays curled",
              n == 3 && seen[0] == U_YOSHI_HIDE && seen[1] == U_YOSHI_HIDE + 1 && seen[2] == U_YOSHI_HIDE + 2 &&
                  yoshi_frame() == U_YOSHI_HIDE + 2);
        check("while Mario stands and the floor is still", mario_frame() == U_MARIO_STAND);
        const uint8_t floor_was = sc.floor_x;
        ramp(1000, 3, 3, false);
        check("even with a little WPM left to decay", sc.floor_x == floor_was);

        // Curled, nothing moves, so a frame costs nothing.
        one_tick(0, false);
        u_slave_mario_render();
        fake_dirty = 0;
        one_tick(0, false);
        u_slave_mario_render();
        check("at rest, a redraw writes nothing", fake_dirty == 0);

        n    = 0;
        last = yoshi_frame();
        for (int k = 0; k < 20; k++) {
            one_tick(2, true);
            if (yoshi_frame() != last && n < sizeof seen) seen[n++] = last = yoshi_frame();
        }
        check("the next key uncurls him the same way back, then he is ready",
              n >= 3 && seen[0] == U_YOSHI_HIDE + 1 && seen[1] == U_YOSHI_HIDE && seen[2] >= U_YOSHI_READY &&
                  seen[2] < U_YOSHI_READY + U_YOSHI_READY_N);
    }

    // --- the tiers ----------------------------------------------------------------------
    {
        ramp(3000, 0, 0, true);
        check("at 0 WPM Mario stands and Yoshi idles", mario_frame() == U_MARIO_STAND && yoshi_frame() < U_YOSHI_READY);
        ramp(3000, 5, 5, true);
        bool stepped = false;
        for (int k = 0; k < 20; k++) {
            one_tick(5, true);
            stepped = stepped || mario_frame() == U_MARIO_STEP;
        }
        check("typing below 10 he steps and Yoshi is ready",
              stepped && yoshi_frame() >= U_YOSHI_READY && yoshi_frame() < U_YOSHI_READY + U_YOSHI_READY_N);
        ramp(3000, 70, 70, true);
        while (sc.jump[1].on) one_tick(70, true);
        check("from 10 Yoshi runs", yoshi_frame() >= U_YOSHI_RUN);
        while (sc.bill_on) one_tick(0, false);
    }

    // --- bongo cat ----------------------------------------------------------------------
    {
        fake_wpm = 0;
        fake_now += 300 - fake_now % 300;
        u_slave_bongo_render();
        const int k = (int)((fake_now / 300) % U_BONGO_IDLE_FRAMES);
        check("below 10 WPM he idles, a frame every 300 ms",
              !memcmp(&fake_oled[4 * OLED_DISPLAY_WIDTH], u_bongo_idle[k], U_BONGO_BYTES));
        fake_dirty = 0;
        u_slave_bongo_render();
        check("and a redraw in the same frame writes nothing", fake_dirty == 0);
        fake_wpm = 15;
        u_slave_bongo_render();
        check("from 10 to 20 he holds his paws up", !memcmp(&fake_oled[4 * OLED_DISPLAY_WIDTH], u_bongo_prep[0], U_BONGO_BYTES));
        fake_wpm = 60;
        fake_now += 1000;
        u_slave_bongo_render();
        const uint8_t first = slap;
        fake_now += 299;
        u_slave_bongo_render();
        const bool held = slap == first;
        fake_now += 1;
        u_slave_bongo_render();
        check("at 60 WPM he slaps every 300 ms", held && slap != first &&
              !memcmp(&fake_oled[4 * OLED_DISPLAY_WIDTH], u_bongo_tap[slap], U_BONGO_BYTES));
        fake_wpm = 200;
        fake_now += 100;
        u_slave_bongo_render();
        const uint8_t fast = slap;
        fake_now += 100;
        u_slave_bongo_render();
        check("and never faster than every 100 ms", slap != fast);
        check("the WPM corner shows 200", any_in(fake_oled, 90, 124, 1, 15) && any_in(fake_oled, 108, 124, 18, 25));
    }

    // --- a dead link breaks every animation on this half up -----------------------------
    {
        static const struct { void (*render)(void); const char *name; } anims[] = {
            {u_slave_bongo_render, "bongo cat"}, {mario_both, "the Mario scene"}, {kyria, "the Kyria logo"},
            {monitor, "the Monitor logo"}, {stacked, "the stacked Monitor logo"}, {future, "Monitor your future"},
        };
        bool all = true;
        for (unsigned i = 0; i < sizeof anims / sizeof anims[0]; i++) {
            const int g = glitches(anims[i].render, 60);
            if (g < 6) printf("    %s: %d burst frames in 3 s\n", anims[i].name, g);
            all = all && g >= 6;
        }
        check("with the link dead, bongo cat, Mario and the logo screens break up", all);
        bool quiet = true;
        for (int k = 0; k < 60; k++) {
            fake_now += 50;
            future();
            quiet = quiet && !u_glitch_clock.armed;
        }
        check("and with it alive they never do", quiet);
    }

    check("the driver's one-past-the-end byte was never touched", fake_oled[OLED_MATRIX_SIZE] == 0);
    return unit_done("scenes");
}
