// Bongo cat and the Mario scene on a fake clock, every frame written out the way preview.c
// does it for Bjorn, so tools/preview.py can turn them into a strip and a GIF.
//
//   scenes <bongo|mario> <out-dir>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
bool            u_sync_hard_stale(void) { return false; }

#include "oled_slave_bongo.c"
#include "oled_slave_mario.c"

static FILE    *frames, *events;
static uint32_t tick_no;
static bool     mario;

static void run(uint32_t ms, uint8_t wpm, bool typing, const char *what) {
    for (uint32_t t = 0; t < ms; t += 50) {
        fake_now += 50;
        fake_wpm     = wpm;
        fake_idle_ms = typing ? 0 : fake_idle_ms + 50;
        fake_dirty   = 0;
        if (mario) {
            u_slave_mario_task();
            u_slave_mario_render();
        } else {
            u_slave_bongo_render();
        }
        fwrite(fake_oled, 1, OLED_MATRIX_SIZE, frames);
        fprintf(events, "%u,%u,%s,%u\n", tick_no++, fake_now, what, fake_dirty);
    }
}

int main(int argc, char **argv) {
    const char *scenario = argc > 1 ? argv[1] : "mario";
    const char *dir      = argc > 2 ? argv[2] : ".";
    char        path[512];

    snprintf(path, sizeof(path), "%s/frames.bin", dir);
    frames = fopen(path, "wb");
    snprintf(path, sizeof(path), "%s/events.csv", dir);
    events = fopen(path, "w");
    if (!frames || !events) {
        fprintf(stderr, "scenes: cannot write into %s\n", dir);
        return 1;
    }
    fprintf(events, "tick,ms,what,blocks_mask\n");
    fake_now = 100000;
    mario    = !strcmp(scenario, "mario");

    if (!mario && strcmp(scenario, "bongo")) {
        fprintf(stderr, "scenes: unknown scenario '%s'\n", scenario);
        return 2;
    }
    // At rest, a slow start, steady typing in each tier, and a stop long enough to curl up.
    run(3000, 0, false, "rest");
    run(3000, 6, true, "slow");
    run(4000, 15, true, "prep");
    run(12000, 40, true, "typing");
    run(12000, 90, true, "fast");
    run(8000, 0, false, "stopped");

    fclose(frames);
    fclose(events);
    printf("%u ticks (%.1f s) -> %s\n", tick_no, tick_no * 0.05, dir);
    return 0;
}
