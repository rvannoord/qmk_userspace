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
#include "bjorn_art.h"
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

static uint8_t  prev_canvas[OLED_MATRIX_SIZE];
static bool     prev_valid;
static int      free_redraws, paid_redraws;   // identical frame, nothing written / something written
static int      blinks;
static bool     blinking;
static uint32_t last_blink_at;
static uint32_t first_blink_at;   // since the last arm_first(), so "how soon" is measurable

// One render tick: 50 ms of panel-on time.
static void tick(void) {
    fake_now += 50;
    fake_dirty = 0;
    oled_slave_task();
    oled_render_slave();

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

    check("an unchanged frame was seen many times", free_redraws > 500);
    check("and never cost a single block", paid_redraws == 0);
    check("the driver's one-past-the-end byte was never touched",
          fake_oled[OLED_MATRIX_SIZE] == 0);

    return unit_done("bjorn");
}
