// Runs the real mascot renderer on a fake clock and writes every frame out, so the idle
// show can be watched on a screen before it is ever flashed.
//
// This exists because looking has beaten assuming every time on this project: the
// breath took four attempts to stop growing out of his helmet, and a day was lost to
// art that was reasoned about rather than rendered. A frame dump is cheap and it is
// the only thing that settles how something looks.
//
//   preview <scenario> <out-dir>
//
// Writes frames.bin — 1024 bytes a tick, page format, exactly the panel buffer — and
// events.csv with the tick, the clock, and what the scenario was doing. tools/preview.py
// turns them into a contact strip and an animated GIF at real timing.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qmk_keyboard.h"
#include "driver.h"
#include "split_state.h"
#include "split_sync.h"
#include "art/bjorn_art.h"
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

static FILE *frames, *events;
static uint32_t tick_no;

// One render tick: 50 ms, the panel's own rate. Bjorn's own entry points, not the
// oled_slave_* dispatch in oled_slave_select.c: the preview is of him, not of whichever
// animation the ANIM key last chose.
static void tick(const char *what) {
    fake_now += 50;
    fake_dirty = 0;
    u_slave_bjorn_task();
    u_slave_bjorn_render();
    fwrite(fake_oled, 1, OLED_MATRIX_SIZE, frames);
    fprintf(events, "%u,%u,%s,%u\n", tick_no++, fake_now, what, fake_dirty);
}

static void run(uint32_t ms, uint8_t wpm, bool typing, const char *what) {
    for (uint32_t t = 0; t < ms; t += 50) {
        fake_wpm = wpm;
        if (typing) {
            fake_idle_ms = 0;
        } else {
            fake_idle_ms += 50;
        }
        tick(what);
    }
}

int main(int argc, char **argv) {
    const char *scenario = argc > 1 ? argv[1] : "idle";
    const char *dir      = argc > 2 ? argv[2] : ".";
    char path[512];

    snprintf(path, sizeof(path), "%s/frames.bin", dir);
    frames = fopen(path, "wb");
    snprintf(path, sizeof(path), "%s/events.csv", dir);
    events = fopen(path, "w");
    if (!frames || !events) {
        fprintf(stderr, "preview: cannot write into %s\n", dir);
        return 1;
    }
    fprintf(events, "tick,ms,what,blocks_mask\n");   // a 16-bit mask, not a count
    fake_now = 100000;

    if (!strcmp(scenario, "idle")) {
        // The whole show: type, settle, doze, and on to the panel switching off.
        run(5000,  60, true,  "typing");
        run(125000, 0, false, "idle");
    } else if (!strcmp(scenario, "wake")) {
        run(90000,  0, false, "idle");        // settle and doze
        run(20000, 40, true,  "typing");      // a key: he should wake and blink
    } else if (!strcmp(scenario, "tiers")) {
        run(6000,   5, true, "tier0");
        run(6000,  20, true, "tier1");
        run(6000,  40, true, "tier2");
        run(6000,  70, true, "tier3");
    } else if (!strcmp(scenario, "dead")) {
        // The link dies for ten seconds: he stops blinking and his frame breaks up.
        run(3000, 40, true, "typing");
        stale_soft = stale_hard = true;
        run(10000, 0, false, "dead");
        stale_soft = stale_hard = false;
        run(3000, 0, false, "back");
    } else if (!strcmp(scenario, "props")) {
        drive.flags = U_FLAG_CAPS_WORD;
        run(8000, 50, true, "capsword");
        drive.flags = 0;
        drive.fingers = U_FIN_L_PINKY;
        run(8000, 50, true, "axe");
        drive.fingers = 0;
        drive.anomaly = 1;
        run(4000, 50, true, "misfire");
    } else {
        fprintf(stderr, "preview: unknown scenario '%s'\n", scenario);
        return 2;
    }

    fclose(frames);
    fclose(events);
    printf("%u ticks (%.1f s) -> %s\n", tick_no, tick_no * 0.05, dir);
    return 0;
}
