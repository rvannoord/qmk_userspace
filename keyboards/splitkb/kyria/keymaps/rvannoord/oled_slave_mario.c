// Mario, Yoshi and Bullet Bill: drashna's sprites in a small scene driven by WPM. Design in
// docs/layout.md, "Mario, Yoshi and Bullet Bill, driven by WPM"; build order in
// docs/oled-v3.md, stage 6.
//
// The scene runs on its own clock, a tick every 50 ms, from u_slave_mario_task, and only
// while it is shown, so it comes back where it left off rather than having run unseen. The
// render only draws.

#include QMK_KEYBOARD_H
#include <string.h>
#include "oled.h"
#include "oled_canvas.h"
#include "oled_draw.h"
#include "speed_tier.h"
#include "art/mario_art.h"

#ifdef OLED_ENABLE

#    define U_TICK_MS  50
#    define U_FEET_Y0  31     // the sprites' feet are on their row 31, so on y=62
#    define U_MARIO_X  14
#    define U_YOSHI_X  40
#    define U_BILL_Y0  39     // his ink on y 41-61, along the ground
#    define U_LIFT     31     // the arc's height: Yoshi's flutter apex reaches y=0
#    define U_CURL_MS  4000   // D13: Yoshi curls up 4 s after the last key
#    define U_CURL_STEP_MS 150

// Ink, measured from the art; test_scenes.c checks these against it. In the air Mario is his
// jump frame and Yoshi his fly frames, so those are what Bill has to clear.
#    define U_BILL_INK_L 2
#    define U_BILL_INK_R 29
static const struct {
    int16_t l, r;
} air[2] = {{U_MARIO_X + 0, U_MARIO_X + 15}, {U_YOSHI_X + 2, U_YOSHI_X + 26}};

// drashna's frame orders, with his empty slots dropped (art/mario_art.h).
static const uint8_t yoshi_idle[]  = {0, 0, 1, 2, 1, 2, 1, 2, 1, 0};
static const uint8_t yoshi_ready[] = {0, 1, 2, 3, 4, 3, 2, 1};
static const uint8_t yoshi_fly[]   = {0, 1, 2, 1};
static const uint8_t yoshi_run[]   = {0, 1};

enum { U_MARIO, U_YOSHI };

typedef struct {
    bool     on;
    uint16_t at, len;     // ticks
} u_jump_t;

static struct {
    bool           started;
    uint32_t       last;            // when the last tick ran, on the keyboard's clock
    uint32_t       ms;              // the scene's own clock, 50 a tick
    uint16_t       tick;
    u_speed_tier_t speed;
    uint8_t        wpm;
    uint8_t        floor_x;         // how far the floor has scrolled, mod 128
    uint16_t       floor_acc;       // fifteenths of a pixel
    uint8_t        mario_step;      // 0 or 1: frames 1 and 2
    uint32_t       mario_at;
    uint8_t        yoshi_set, yoshi_i;
    uint32_t       yoshi_at;
    uint8_t        curl;            // 0 up, 3 curled: hide frames 1-3
    uint32_t       curl_at;
    bool           resting;
    bool           bill_on, bill_armed;
    int16_t        bill_x;
    uint32_t       bill_due;
    u_jump_t       jump[2];
} sc;

// Floor division, for times that may already be past.
static int32_t div_floor(int32_t a, int32_t b) {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static uint16_t lift_of(const u_jump_t *j) {
    if (!j->on) return 0;
    const uint32_t k = (uint16_t)(sc.tick - j->at);
    return (uint16_t)(4u * U_LIFT * k * (j->len - k) / ((uint32_t)j->len * j->len));
}

// Each character still on the ground jumps when Bill's overlap with it, at his current speed,
// would sit in the middle half of the arc, where the lift clears his top.
static void plan_jumps(void) {
    for (uint8_t c = 0; c < 2; c++) {
        u_jump_t *j = &sc.jump[c];
        if (j->on && (uint16_t)(sc.tick - j->at) >= j->len) j->on = false;
        if (j->on || !sc.bill_on) continue;
        const int32_t nose = sc.bill_x + U_BILL_INK_L, tail = sc.bill_x + U_BILL_INK_R;
        if (tail < air[c].l) continue;
        // In quarter ticks, at 4 px a tick plus the floor's wpm/15.
        const int32_t v  = 60 + sc.wpm;
        const int32_t ts = div_floor((nose - air[c].r) * 60, v);
        const int32_t te = div_floor((tail - air[c].l + 1) * 60 + v - 1, v);
        int32_t       t4 = 2 * (te - ts) + 8;
        if (t4 < 64) t4 = 64;
        if (ts + te <= t4) {
            j->on  = true;
            j->at  = sc.tick;
            j->len = (uint16_t)((t4 + 3) / 4);
        }
    }
}

static void yoshi_set(uint8_t set) {
    if (sc.yoshi_set == set) return;
    sc.yoshi_set = set;
    sc.yoshi_i   = 0;
    sc.yoshi_at  = sc.ms;
}

static void tick(uint8_t wpm, uint32_t idle_ms) {
    sc.ms += U_TICK_MS;
    sc.tick++;
    sc.wpm = wpm;
    u_speed_tier_update(&sc.speed, wpm);
    const uint8_t tier = sc.speed.tier;

    // Resting: a while since the last key and nobody in the air or on the way.
    sc.resting = idle_ms >= U_CURL_MS && !sc.bill_on && !sc.jump[U_MARIO].on && !sc.jump[U_YOSHI].on;

    uint8_t moved = 0;
    if (!sc.resting) {
        sc.floor_acc = (uint16_t)(sc.floor_acc + wpm);
        moved        = (uint8_t)(sc.floor_acc / 15);
        sc.floor_acc %= 15;
        sc.floor_x = (uint8_t)((sc.floor_x + moved) & 127u);
    }

    // Bill: only while typing at 10 WPM or more, and after each pass a wait, shorter from 30.
    const uint32_t wait = tier >= 2 ? 2800 : 5000;
    if (sc.bill_on) {
        sc.bill_x = (int16_t)(sc.bill_x - 4 - moved);
        if (sc.bill_x + U_BILL_INK_R < 0) {
            sc.bill_on    = false;
            sc.bill_armed = true;
            sc.bill_due   = sc.ms + wait;
        }
    } else if (tier >= 1 && idle_ms < U_CURL_MS) {
        if (!sc.bill_armed) {
            sc.bill_armed = true;
            sc.bill_due   = sc.ms + wait;
        } else if ((int32_t)(sc.ms - sc.bill_due) >= 0) {
            sc.bill_on    = true;
            sc.bill_armed = false;
            sc.bill_x     = OLED_DISPLAY_WIDTH;
        }
    } else {
        sc.bill_armed = false;
    }
    plan_jumps();

    // Mario steps whenever there is typing, at bongo cat's rate.
    if (wpm && !sc.resting) {
        const int16_t step = (int16_t)(450 - (wpm * 5) / 2);
        if (sc.ms - sc.mario_at >= (uint32_t)(step > 100 ? step : 100)) {
            sc.mario_at   = sc.ms;
            sc.mario_step = (uint8_t)(sc.mario_step ^ 1u);
        }
    }

    // Yoshi curls up a frame at a time, and uncurls the same way back.
    const uint8_t want = sc.resting ? 3 : 0;
    if (sc.curl != want && sc.ms - sc.curl_at >= U_CURL_STEP_MS) {
        sc.curl    = (uint8_t)(sc.curl + (want > sc.curl ? 1 : -1));
        sc.curl_at = sc.ms;
    }
    const uint8_t  set = !wpm ? 0 : tier == 0 ? 1 : 2;
    const int16_t  run = (int16_t)(300 - 3 * wpm);
    const uint32_t per = set == 2 ? (uint32_t)(run > 80 ? run : 80) : 300;
    yoshi_set(set);
    if (sc.ms - sc.yoshi_at >= per) {
        sc.yoshi_at = sc.ms;
        sc.yoshi_i++;
    }
}

// --- what is where -------------------------------------------------------------------

static uint8_t mario_frame(void) {
    if (sc.jump[U_MARIO].on) return U_MARIO_JUMP;
    if (!sc.wpm || sc.resting) return U_MARIO_STAND;
    return sc.mario_step ? U_MARIO_STEP : U_MARIO_STAND_ALT;
}

static uint8_t yoshi_frame(void) {
    const u_jump_t *j = &sc.jump[U_YOSHI];
    if (j->on) return (uint8_t)(U_YOSHI_FLY + yoshi_fly[((uint16_t)(sc.tick - j->at) / 2) % sizeof yoshi_fly]);
    if (sc.curl) return (uint8_t)(U_YOSHI_HIDE + sc.curl - 1);
    switch (sc.yoshi_set) {
        case 0:  return (uint8_t)(U_YOSHI_IDLE + yoshi_idle[sc.yoshi_i % sizeof yoshi_idle]);
        case 1:  return (uint8_t)(U_YOSHI_READY + yoshi_ready[sc.yoshi_i % sizeof yoshi_ready]);
        default: return (uint8_t)(U_YOSHI_RUN + yoshi_run[sc.yoshi_i % sizeof yoshi_run]);
    }
}

static int16_t mario_y(void) { return (int16_t)(U_FEET_Y0 - lift_of(&sc.jump[U_MARIO])); }
static int16_t yoshi_y(void) { return (int16_t)(U_FEET_Y0 - lift_of(&sc.jump[U_YOSHI])); }

// --- drawing -------------------------------------------------------------------------

// A dotted rule like Bjorn's, and a few tufts of grass, scrolling left together.
static void draw_floor(void) {
    static const uint8_t tufts[3] = {9, 51, 94};
    for (int16_t x = 0; x < OLED_DISPLAY_WIDTH; x++)
        if (((x + sc.floor_x) & 1) == 0) u_put(x, 63, true);
    for (uint8_t i = 0; i < sizeof tufts; i++) {
        const int16_t x = (int16_t)((tufts[i] - sc.floor_x) & 127);
        u_put(x, 62, true);
        u_put((int16_t)(x + 1), 61, true);
        u_put((int16_t)(x + 1), 62, true);
        u_put((int16_t)(x + 2), 62, true);
    }
}

static void draw_mario(void) {
    u_sprite(u_mario[mario_frame()], U_MARIO_W, U_MARIO_PAGES, U_MARIO_X, mario_y());
}

static void draw_yoshi(void) {
    u_sprite(u_yoshi[yoshi_frame()], U_YOSHI_W, U_YOSHI_PAGES, U_YOSHI_X, yoshi_y());
}

static void draw_bill(void) {
    if (sc.bill_on) u_sprite(u_bill, U_BILL_W, U_BILL_PAGES, sc.bill_x, U_BILL_Y0);
}

// --- entry points --------------------------------------------------------------------

void u_slave_mario_task(void) {
    if (!sc.started) {
        sc.started = true;
        sc.last    = timer_read32();
    }
    // At most a second of catching up, so a return after a long while starts from now.
    for (uint8_t n = 0; timer_elapsed32(sc.last) >= U_TICK_MS; n++) {
        if (n == 20) {
            sc.last = timer_read32();
            break;
        }
        sc.last += U_TICK_MS;
        tick(get_current_wpm(), last_input_activity_elapsed());
    }
}

void u_slave_mario_render(void) {
    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    draw_floor();
    draw_mario();
    draw_yoshi();
    draw_bill();
    u_slave_wpm_corner(get_current_wpm());
    u_canvas_commit();
}

#endif     // OLED_ENABLE
