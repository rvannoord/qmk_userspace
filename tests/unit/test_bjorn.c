// Bjorn, on the host: does an unchanged frame cost nothing, and does he keep blinking?
//
// Both are regressions with teeth. The frame cost is Bug A on the slave side: laying
// the base down and patching poses over it rewrote every block an overlay covered,
// every frame, which in the driver meant permanent I2C traffic. The blink is the
// design's fault signal — a figure that moves but never blinks is how this keyboard
// says the link is dead — and it used to stop after every doze for about 55 minutes.
#include <stdlib.h>
#include <string.h>
#include "unit.h"
#include "qmk_keyboard.h"
#include "driver.h"
#include "split_state.h"
#include "split_sync.h"
#include "art/bjorn_art.h"
#include "art/bjorn_poses.h"
#include "oled_canvas.h"

uint8_t  fake_oled[OLED_MATRIX_SIZE + 1];
uint16_t fake_dirty;
uint32_t fake_now;

void oled_write_raw_byte(const char data, uint16_t index) { u_fake_write(data, index); }

static uint8_t         fake_wpm;
static uint32_t        fake_idle_ms;
static u_split_state_t drive;
static bool            stale_soft, stale_hard;

uint8_t         get_current_wpm(void) { return fake_wpm; }
uint32_t        last_input_activity_elapsed(void) { return fake_idle_ms; }
u_split_state_t u_sync_state(void) { return drive; }
bool            u_sync_soft_stale(void) { return stale_soft; }
bool            u_sync_hard_stale(void) { return stale_hard; }

#ifndef QMK_KEYBOARD_H
#    define QMK_KEYBOARD_H "qmk_keyboard.h"
#endif
#include "oled_slave_bjorn.c"

// --- watching him -----------------------------------------------------------

static bool frame_present(uint8_t id) {
    const bjorn_frame_t *f = &bjorn_frames[id];
    for (uint8_t p = 0; p < f->count; p++) {
        const uint16_t base = (uint16_t)(f->patches[p].block & 0x0F) * BJORN_PATCH_BYTES;
        for (uint8_t i = 0; i < BJORN_PATCH_BYTES; i++) {
            if (fake_oled[base + i] != (uint8_t)f->patches[p].data[i]) return false;
        }
    }
    return true;
}

// Any of the eyes-shut frames fully present. Checking the whole frame rather than a
// pixel keeps this honest when a blink lands during a breath, which uses its own art.
static bool eyes_shut(void) {
    static const uint8_t ids[5] = {BJORN_F_BLINK_T0, BJORN_F_BLINK_T1, BJORN_F_BLINK_T2,
                                   BJORN_F_BLINK_T3, BJORN_F_BLINK_LIFT};
    for (uint8_t i = 0; i < 5; i++) {
        if (frame_present(ids[i])) return true;
    }
    return false;
}

// Is one of the authored drawings on screen? Compared loosely on purpose: the breath
// changes a few dozen pixels of it, and a test that re-implemented the breath exactly
// would be checking its own copy of the code rather than the code.
static int pose_showing(void) {
    // Rows 53 and below are untouched by the breath — the lift stops at the seam and
    // the swell at row 52 — so they identify a drawing no matter where in the cycle it
    // is caught. Pages 7 and the top half of 6 are exactly those rows.
    for (uint8_t w = 0; w < U_POSE_COUNT; w++) {
        bool match = true;
        for (uint8_t x = 0; x < 64 && match; x++) {
            if (fake_oled[7 * 128 + x] != u_poses[w][7 * 64 + x]) match = false;
            const uint8_t hi = 0xE0;                 // rows 53-55 of page 6
            if ((fake_oled[6 * 128 + x] & hi) != (u_poses[w][6 * 64 + x] & hi)) match = false;
        }
        if (match) return w;
    }
    return -1;
}

static uint8_t  prev_canvas[OLED_MATRIX_SIZE];
static bool     prev_valid;
static int      free_redraws, paid_redraws;   // identical frame, nothing written / something written
static int      blinks;
static int      two_goats, goat_ticks;   // his goat drawing and the wandering goat at once
static bool     blinking;
static uint32_t last_blink_at;
static uint32_t first_blink_at;   // since the last arm_first(), so "how soon" is measurable

// One render tick: 50 ms of panel-on time.
static void tick(void) {
    fake_now += 50;
    fake_dirty = 0;
    u_slave_bjorn_task();
    u_slave_bjorn_render();

    // The invariant: if the composed frame did not change, the commit must have
    // written nothing. This catches Bug A whatever pose he happens to be in, instead
    // of needing the test to guess a moment when he is holding still.
    if (prev_valid && memcmp(prev_canvas, u_canvas, OLED_MATRIX_SIZE) == 0) {
        if (fake_dirty == 0) {
            free_redraws++;
        } else {
            paid_redraws++;
        }
    }
    memcpy(prev_canvas, u_canvas, OLED_MATRIX_SIZE);
    prev_valid = true;

    if (u_goat_present()) goat_ticks++;
    if (u_goat_present() && bjorn_vig != U_POSE_COUNT && u_pose_still_x0[bjorn_vig] != 255) two_goats++;

    const bool shut = eyes_shut();
    if (shut && !blinking) {
        blinks++;
        last_blink_at = fake_now;
        if (first_blink_at == 0) first_blink_at = fake_now;
    }
    blinking = shut;
}

static void arm_first(void) { first_blink_at = 0; }

static void run(uint32_t ms, uint8_t wpm, bool typing) {
    for (uint32_t t = 0; t < ms; t += 50) {
        fake_wpm = wpm;
        if (typing) {
            fake_idle_ms = 0;
        } else {
            fake_idle_ms += 50;
        }
        tick();
    }
}

// Let the panel sleep, type briefly, then idle past 20 s, which starts the next drawing in
// the cursor. The sleep sends the goat away, so his own goat drawings are not passed over.
// Searching by idling alone does not work: at 75 s he dozes and vignettes stop, so a
// long search finds nothing however finely it steps.
static int next_pose(void) {
    run(126000, 0, false);            // to the end of the show, and the panel off a while:
    fake_now += 60000;                // the goat has wandered off
    run(1000, 40, true);
    run(21000, 0, false);
    return pose_showing();
}

static int blinks_in(uint32_t ms, uint8_t wpm, bool typing) {
    const int before = blinks;
    run(ms, wpm, typing);
    return blinks - before;
}

int main(void) {
    fake_now = 100000;

    // --- he blinks, and keeps blinking -------------------------------------
    check("he blinks while awake", blinks_in(30000, 40, true) >= 3);

    // Idle long enough to settle and then doze. The blink schedule falls due while his
    // eyes are shut, which is where the old code lost it.
    run(170000, 0, false);
    const int asleep = blinks_in(20000, 0, false);
    check("he does not blink while asleep", asleep == 0);

    // A key wakes him. This is the case Roel saw after every walk away.
    const uint32_t woke = fake_now;
    arm_first();
    const int after_doze = blinks_in(120000, 40, true);
    check("he blinks again after a doze", after_doze >= 10);
    check("and the first one comes within about ten seconds",
          first_blink_at != 0 && first_blink_at - woke <= 10500);

    // Same thing after the link has been dead: no blink while stale, by design, and
    // blinking again once it returns.
    run(30000, 40, true);
    stale_soft = stale_hard = true;
    check("no blink while the link is dead", blinks_in(75000, 40, true) == 0);
    stale_soft = stale_hard = false;
    const uint32_t back = fake_now;
    arm_first();
    check("he blinks again once the link returns", blinks_in(120000, 40, true) >= 10);
    check("and that first one is within about ten seconds too",
          first_blink_at != 0 && first_blink_at - back <= 10500);

    // --- the authored poses -------------------------------------------------
    // Infinition's own drawings, shown only in deep idle. The rule that matters is the
    // boundary: a key event must end a pose at once, because that is what keeps an
    // authored prop from being read as a signal — a drawn sword and the axe are twenty
    // seconds apart by construction, and this closes the one instant between them.
    {
        drive.flags = 0; drive.fingers = 0;
        run(10000, 40, true);
        check("no authored pose while typing", pose_showing() < 0);

        run(15000, 0, false);                 // past settle, not yet deep idle
        check("none before deep idle", pose_showing() < 0);

        run(10000, 0, false);                 // past 20 s
        const int first = pose_showing();
        check("a pose appears in deep idle", first >= 0);

        run(50, 40, true);                    // one key
        check("and a keypress ends it at once", pose_showing() < 0);

        // The cursor advances rather than re-rolling, so the whole cast is seen.
        run(25000, 0, false);
        const int second = pose_showing();
        check("the next one is a different drawing", second >= 0 && second != first);
    }

    // --- the breath moves him, not the picture ------------------------------
    // Watched through the real renderer rather than by re-deriving the transform. Over
    // a full breath cycle on a pose that has a goat in it: his head and beard move, the
    // body and the floor do not, and the goat never does — he is a separate animal on
    // the same ground, not part of Bjorn's chest.
    {
        uint8_t seen[8][128];
        bool    have = false;
        int     which = -1;

        drive.flags = 0; drive.fingers = 0;

        // Find a goat pose by walking the cursor; each hold is 18 s.
        for (int tries = 0; tries < U_POSE_COUNT * 2 && which < 0; tries++) {
            const int w = next_pose();
            if (w >= 0 && u_pose_still_x0[w] != 255) which = w;
        }
        check("a pose with a goat in it comes round", which >= 0);

        if (which >= 0) {
            const uint8_t gx0 = u_pose_still_x0[which], gx1 = u_pose_still_x1[which];
            const uint8_t gy0 = u_pose_still_y0[which];
            bool beard_moved = false, eyes_moved = false, goat_moved = false;

            // Long enough to contain a blink, which is six to ten seconds apart.
            for (int i = 0; i < 260; i++) {
                run(50, 0, false);
                if (pose_showing() != which) break;
                if (have) {
                    for (uint8_t page = 0; page < 8; page++)
                        for (uint8_t x = 0; x < 64; x++) {
                            const uint8_t diff = seen[page][x] ^ fake_oled[page * 128 + x];
                            if (!diff) continue;
                            for (uint8_t k = 0; k < 8; k++) {
                                if (!((diff >> k) & 1u)) continue;
                                const uint8_t y = page * 8 + k;
                                if (y >= 31 && y <= 35) eyes_moved = true;
                                else beard_moved = true;
                                if (x >= gx0 && x <= gx1 && y >= gy0) goat_moved = true;
                            }
                        }
                }
                for (uint8_t page = 0; page < 8; page++)
                    for (uint8_t x = 0; x < 128; x++) seen[page][x] = fake_oled[page * 128 + x];
                have = true;
            }
            check("a drawing with a goat does not breathe", !beard_moved);
            check("but it does blink, so it is not frozen", eyes_moved);
            check("and the goat never moves", !goat_moved);
        }
    }

    // The spectacled pose must not blink: the lids are one pair of patches at fixed
    // coordinates, so on that drawing they close over the lenses rather than under them.
    //
    // This is asserted on the data rather than on pixels, and the reason is worth
    // keeping. The goat test above isolates a blink by watching rows 31-35, which works
    // *because* those poses do not breathe. A pose that breathes lifts everything above
    // its seam - row 40 here - so the eye rows move every breath cycle and the two
    // causes cannot be told apart from outside. Watching pixels there would assert
    // "something changed near his eyes", which is true either way and would pass if the
    // blink came back.
    {
        bool found = false, flagged_breathes = true;
        for (uint8_t i = 0; i < U_POSE_COUNT; i++) {
            if (u_pose_blinks[i]) continue;
            found = true;
            if (u_pose_still_x0[i] != 255) flagged_breathes = false;   // it has a goat
        }
        check("a pose is marked as not blinking", found);
        check("and it is one that breathes instead", flagged_breathes);
    }

    // The invariant behind both of the above: every pose moves somehow. A pose that
    // neither breathes nor blinks is drawn but still, which is this panel's signal that
    // the link is dead - so it would report a fault that is not there.
    {
        bool all_move = true;
        for (uint8_t i = 0; i < U_POSE_COUNT; i++) {
            const bool breathes = u_pose_still_x0[i] == 255;
            if (!breathes && !u_pose_blinks[i]) all_move = false;
        }
        check("every pose either breathes or blinks", all_move);
    }

    // A drawing without a goat still breathes, and must: these poses do not blink, so
    // a figure where nothing moves is this panel's signal that the link has died.
    {
        uint8_t prev[8][64];
        bool    have = false, moved = false;
        int     which = -1;
        for (int tries = 0; tries < U_POSE_COUNT * 2 && which < 0; tries++) {
            const int w = next_pose();
            if (w >= 0 && u_pose_still_x0[w] == 255) which = w;
        }
        check("a drawing without a goat comes round", which >= 0);
        for (int i = 0; i < 80 && which >= 0; i++) {
            run(50, 0, false);
            if (pose_showing() != which) break;
            if (have) {
                for (uint8_t page = 0; page < 8; page++)
                    for (uint8_t x = 0; x < 64; x++)
                        if (prev[page][x] != fake_oled[page * 128 + x]) moved = true;
            }
            for (uint8_t page = 0; page < 8; page++)
                for (uint8_t x = 0; x < 64; x++) prev[page][x] = fake_oled[page * 128 + x];
            have = true;
        }
        check("and it breathes", moved);
    }

    // --- nothing stale replays ---------------------------------------------
    // A misfire during an outage is over by the time the link returns. Raising the axe
    // then would report a second event that never happened.
    {
        run(10000, 40, true);
        const uint8_t before = drive.anomaly;
        stale_soft = stale_hard = true;
        run(30000, 40, true);
        drive.anomaly = (uint8_t)(before + 3);     // three misfires we never saw
        stale_soft = stale_hard = false;

        // The first fresh frames must not start the raise. The misfire animation
        // droops him to tier 0 and swaps in the raise frames, so the axe-at-rest frame
        // appearing is the tell.
        bool raised = false;
        for (int i = 0; i < 40; i++) {           // two seconds, longer than the 1.75 s raise
            run(50, 40, true);
            if (frame_present(BJORN_F_MISFIRE_R2) || frame_present(BJORN_F_MISFIRE_R3)) raised = true;
        }
        check("a misfire the link missed does not replay", !raised);

        // But a real one, after the link is back, still does.
        drive.anomaly++;
        bool raised_now = false;
        for (int i = 0; i < 40; i++) {
            run(50, 40, true);
            if (frame_present(BJORN_F_MISFIRE_R2) || frame_present(BJORN_F_MISFIRE_R3)) raised_now = true;
        }
        check("a misfire after the link returns still shows", raised_now);
    }

    // --- an unchanged frame costs nothing ----------------------------------
    // Cover the tiers and the quiet states, so the run above and this one between them
    // have been through typing at every tier, settling, breathing, dozing and waking.
    run(5000, 0, true);                       // tier 0
    run(5000, 35, true);                      // tier 2
    run(5000, 60, true);                      // tier 3
    run(20000, 0, false);                     // settle, breathe
    drive.flags = U_FLAG_CAPS_WORD;           // crown and inverted tile
    run(10000, 60, true);
    drive.fingers = U_FIN_L_PINKY;            // axe held
    run(10000, 60, true);
    drive.flags = 0;
    drive.fingers = 0;
    run(90000, 0, false);                     // doze again

    // One goat on screen: his two drawings with a goat are passed over while it is out,
    // and it does not come in while one of them shows.
    for (int k = 0; k < 12; k++) {
        run(2000, 40, true);
        run(70000, 0, false);
    }
    check("the goat came out in the long idles", goat_ticks > 2000);
    check("and never with Bjorn's own goat on screen", two_goats == 0);

    check("an unchanged frame was seen many times", free_redraws > 500);
    check("and never cost a single block", paid_redraws == 0);
    check("the driver's one-past-the-end byte was never touched",
          fake_oled[OLED_MATRIX_SIZE] == 0);

    return unit_done("bjorn");
}
