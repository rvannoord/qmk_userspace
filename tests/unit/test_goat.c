// The goat, from oled_slave_goat.c itself, on the director's real stream: when it may come
// in, that it never stops mid-stride, never touches x=127 or y=0, keeps to its own three
// pages, curls up at Bjorn's doze, is still from 119.5 s, and starts nothing after 110 s.
// Bjorn's side of the rules (his goat drawings) is checked in test_bjorn.c.
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

#include "oled_slave_goat.c"
#include "oled_slave_world.h"

static uint32_t idle;
static bool     dead, goat_pose;
static uint8_t  before[OLED_MATRIX_SIZE];

// One render step, 50 ms: the director opens it, the goat steps and is drawn on a clear canvas.
static void step(void) {
    fake_now += 50;
    memcpy(before, u_canvas, sizeof before);
    u_director_step(idle);
    u_goat_step(idle, dead, goat_pose);
    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    u_goat_draw();
    idle += 50;
}

static bool any_at(int x0, int x1, int y0, int y1) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if ((u_canvas[(y / 8) * 128 + x] >> (y % 8)) & 1) return true;
    return false;
}

static bool changed_outside_its_pages(void) {
    for (int i = 0; i < OLED_MATRIX_SIZE; i++) {
        const int page = i / 128, x = i % 128;
        if (before[i] != u_canvas[i] && (x < 64 || page < 4)) return true;
    }
    return false;
}

static bool mid_stride(void) { return g.frame == U_GOAT_WALK_A || g.frame == U_GOAT_WALK_B; }

int main(void) {
    // --- one idle period, start to switch-off, many times over -------------------------
    int  entered_early = 0, edges = 0, outside = 0, came = 0, curled = 0, after_last = 0, moved_frozen = 0;
    bool was_present = false;
    for (int period = 0; period < 60; period++) {
        idle = 0;
        while (idle < 125000) {
            const bool     present_before = u_goat_present();
            const uint32_t at             = idle;
            step();
            if (u_goat_present() && !present_before && at < U_IDLE_ACTORS_MS) entered_early++;
            if (u_goat_present() && !present_before && at >= U_IDLE_LAST_START_MS) after_last++;
            if (u_goat_present() && !present_before) came++;
            if (any_at(127, 127, 0, 63) || any_at(0, 127, 0, 0)) edges++;
            if (changed_outside_its_pages()) outside++;
            if (at >= U_IDLE_DOZE_MS + 2000 && at < U_IDLE_FREEZE_MS && u_goat_present() && g.mode != G_CURL &&
                !(g.mode == G_WALK && g.target == U_GOAT_X_OUT))
                curled++;
            if (at > U_IDLE_FREEZE_MS && memcmp(before, u_canvas, sizeof before)) moved_frozen++;
        }
        was_present = was_present || u_goat_present();
    }
    printf("  (60 idle periods: it came in %d times)\n", came);
    check("it comes in, and not every time", came >= 20 && came <= 120);
    check("never before 20 s of idle", entered_early == 0);
    check("and never after 110 s", after_last == 0);
    check("it never lights x=127 or y=0", edges == 0);
    check("and changes nothing outside the right column's lower four pages", outside == 0);
    check("from Bjorn's doze it is curled up (or on its way out)", curled == 0);
    check("from 119.5 s nothing of it changes", moved_frozen == 0);

    // --- a key mid-walk: it finishes the frame and stands -------------------------------
    {
        int strides = 0, stopped = 0, moved_after = 0;
        for (int k = 0; k < 200; k++) {
            idle = 20000;
            while (!(u_goat_present() && g.mode == G_WALK && g.x < U_GOAT_X_MAX)) {
                step();
                if (idle > 110000) idle = 20000;
            }
            for (int n = (int)(u_director_rand(8)); n > 0; n--) step();   // somewhere in the stride
            if (g.mode != G_WALK) continue;
            strides++;
            idle = 0;                                                       // a key
            const uint16_t end = g.frame_end;
            while ((int16_t)(g.t - end) < 0) step();
            step();
            if (!mid_stride() && g.mode == G_STAND) stopped++;
            const int16_t x = g.x;
            const uint8_t f = g.frame;
            for (int n = 0; n < 300; n++) {                                  // 15 s of typing on
                idle = (uint32_t)n * 50 % 7000;
                step();
                if (g.x != x || g.frame != f) moved_after++;
            }
        }
        printf("  (%d walks interrupted)\n", strides);
        check("a key mid-walk lets the frame play out, then it stands", strides > 20 && stopped == strides);
        check("and it does not move again while typing goes on", moved_after == 0);
    }

    // --- curled, a key gets it up -------------------------------------------------------
    {
        idle = 20000;
        while (!u_goat_present()) step();
        idle = U_IDLE_DOZE_MS;
        for (int n = 0; n < 200 && g.mode != G_CURL; n++) step();
        const bool was_curled = g.mode == G_CURL;
        idle = 0;
        step();
        const bool kneels = g.frame == U_GOAT_HOP_CROUCH;
        for (int n = 0; n < 8; n++) step();
        check("curled at the doze, the next key gets it up through a kneel", was_curled && kneels && g.mode == G_STAND);

        // A map over this half pauses it, and it is still there after.
        while (!u_goat_present()) {
            idle = 30000;
            step();
        }
        fake_now += 3000;
        idle = 0;
        step();
        const bool kept = u_goat_present();
        // The show run to its end and the panel off for a while: it comes back without it.
        idle = U_IDLE_FREEZE_MS + 200;
        step();
        fake_now += 60000;
        idle = 0;
        step();
        check("a pause in the middle keeps the goat; after the panel has been off, it is gone", kept && !u_goat_present());
    }

    // --- not while Bjorn shows his own goat -------------------------------------------
    {
        while (u_goat_present()) {
            idle = 30000;
            step();
        }
        goat_pose = true;
        int came_in = 0;
        idle = 20000;
        for (int n = 0; n < 2000; n++) {
            step();
            if (u_goat_present()) came_in++;
            if (idle > 100000) idle = 20000;
        }
        goat_pose = false;
        check("it never comes in while Bjorn is in a drawing with a goat", came_in == 0);
    }

    // --- a dead link: the frame plays out, then nothing moves -------------------------
    {
        idle = 20000;
        while (!(u_goat_present() && g.mode == G_WALK && g.x < U_GOAT_X_MAX)) step();
        dead = true;
        for (int n = 0; n < 10; n++) step();
        uint8_t still[OLED_MATRIX_SIZE];
        memcpy(still, u_canvas, sizeof still);
        int moved = 0;
        for (int n = 0; n < 400; n++) {
            step();
            if (memcmp(still, u_canvas, sizeof still)) moved++;
        }
        dead = false;
        check("with the link dead it stands and nothing of it moves", moved == 0 && !mid_stride());
    }

    // --- the weather stops at its outline ----------------------------------------------
    // oled_slave_world.c is linked as it is built, with OLED_SLAVE_WORLD, so whatever the
    // build month makes it (leaves, rain or snow) falls past the goat and not through it.
    {
        int inside = 0, weather = 0;
        idle       = 20000;
        while (!u_goat_present()) step();
        for (int n = 0; n < 6000; n++) {
            idle = 30000 + (uint32_t)(n % 1000) * 50;
            fake_now += 50;
            u_director_step(idle);
            u_goat_step(idle, false, false);
            u_world_task();
            memset(u_canvas, 0, OLED_MATRIX_SIZE);
            u_goat_draw();
            uint8_t ink[OLED_MATRIX_SIZE];
            memcpy(ink, u_canvas, sizeof ink);
            uint64_t hull[64];
            u_goat_silhouette(hull);
            u_world_draw();
            for (int y = 0; y < 64; y++)
                for (int x = 64; x < 127; x++) {
                    const int  i  = (y / 8) * 128 + x;
                    const bool on = (u_canvas[i] >> (y % 8)) & 1, goat = (ink[i] >> (y % 8)) & 1;
                    if (!on || goat) continue;
                    if ((hull[y] >> (x - 64)) & 1) inside++;
                    else weather++;
                }
        }
        check("the weather falls in the goat's half", weather > 1000);
        check("and never inside its outline", inside == 0);
    }

    check("the driver's one-past-the-end byte was never touched", fake_oled[OLED_MATRIX_SIZE] == 0);
    (void)was_present;
    return unit_done("goat");
}
