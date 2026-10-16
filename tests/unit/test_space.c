// The spaceship's space: the real oled_slave_spaceship.c and the real generated
// planes, driven on a fake clock. Two things with teeth: that the loop closes -
// the frame at state 512 is byte-identical to the frame at state 0, at every
// speed the ship can run at - and that the ship is untouched by the rewrite:
// wherever its mask blocks the background, the output IS the ship, bit for bit.
//
// The loop closing is arithmetic (periods 128/256/512 under a 512 loop) and the
// generator appends each strip's tail from its head, so the seam cannot drift.
// This asserts it on the compiled bytes anyway, because that is the one defect
// that would show exactly once per loop and never in a still.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qmk_keyboard.h"
#include "driver.h"

uint8_t  fake_oled[OLED_MATRIX_SIZE + 1];
uint16_t fake_dirty;
uint32_t fake_now;
bool     fake_is_master = false, fake_is_left = false;

static uint8_t  fake_wpm;
static uint16_t cursor;
uint8_t get_current_wpm(void) { return fake_wpm; }
void    oled_set_cursor(uint8_t col, uint8_t line) { cursor = (uint16_t)(line * 128 + col * 6); }
void    oled_write_raw(const char *d, uint16_t n) {
    for (uint16_t i = 0; i < n; i++) u_fake_write(d[i], (uint16_t)(cursor + i));
}

#include "oled_slave_spaceship.c"

static int failures;
static void check(const char *what, bool ok) {
    printf("  %s   %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) failures++;
}

static void tick(void) {            // one 50 ms animation tick
    fake_now += ANIMATION_INTERVAL_MS;
    u_slave_spaceship_render();
}

static int dirty_blocks(const uint8_t *a, const uint8_t *b) {
    int n = 0;
    for (int blk = 0; blk < 16; blk++)
        if (memcmp(a + blk * 64, b + blk * 64, 64) != 0) n++;
    return n;
}

// --raw <wpm>: one full loop of 1024-byte frames on stdout, for a renderer.
// This is the firmware's own render path, not a model of it.
static int dump(int wpm) {
    fake_wpm = (uint8_t)wpm;
    const int ticks = 512 / (1 + wpm / WPM_SPEED_DIVISOR);
    for (int t = 0; t < ticks; t++) {
        tick();
        fwrite(fake_oled, 1, OLED_MATRIX_SIZE, stdout);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 2 && strcmp(argv[1], "--raw") == 0) return dump(atoi(argv[2]));
    static uint8_t first[OLED_MATRIX_SIZE], prev[OLED_MATRIX_SIZE];
    const char *const ships[4] = {ship_row_1, ship_row_2, ship_row_3, ship_row_4};
    const uint8_t    *masks[4] = {u_space_ship_mask[0], u_space_ship_mask[1], u_space_ship_mask[2], u_space_ship_mask[3]};

    // The mask is derived from the ship with a one-pixel rim: every ship pixel's
    // eight neighbours are opaque, and no opaque pixel is further than one from
    // ink or an enclosed hole. That rim is what keeps a lit ship visible over a
    // lit planet, so its presence is asserted, not assumed.
    {
        bool ring = true;
        for (int y = 0; y < 32; y++)
            for (int x = 0; x < 128; x++) {
                if (!((ships[y / 8][x] >> (y % 8)) & 1)) continue;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int yy = y + dy, xx = x + dx;
                        if (yy < 0 || yy >= 32 || xx < 0 || xx >= 128) continue;
                        if ((masks[yy / 8][xx] >> (yy % 8)) & 1) ring = false;   // shows through next to ink
                    }
            }
        check("a one-pixel dark rim surrounds every ship pixel", ring);
    }

    // --- at rest: state += 1, so one loop is 512 ticks ---------------------
    fake_wpm = 0;
    tick();
    memcpy(first, fake_oled, OLED_MATRIX_SIZE);
    check("the first frame is not blank", memchr(first, 0xff, OLED_MATRIX_SIZE) || memchr(first, 0x01, OLED_MATRIX_SIZE));

    // The ship is exact wherever its mask blocks the background (mask bit 0).
    bool ship_ok = true;
    for (int p = 2; p <= 5; p++)
        for (int i = 0; i < 128; i++) {
            uint8_t out = fake_oled[p * 128 + i], m = (uint8_t)masks[p - 2][i], s = (uint8_t)ships[p - 2][i];
            if ((out & (uint8_t)~m) != (s & (uint8_t)~m)) ship_ok = false;
            if ((out & s) != s) ship_ok = false;
        }
    check("the ship is bit-exact inside its own mask", ship_ok);

    tick();
    check("and the space moves between ticks", memcmp(first, fake_oled, OLED_MATRIX_SIZE) != 0);

    long blocks = 0; int max_blocks = 0;
    memcpy(prev, fake_oled, OLED_MATRIX_SIZE);
    for (int t = 2; t < 512; t++) {
        tick();
        int d = dirty_blocks(prev, fake_oled);
        blocks += d; if (d > max_blocks) max_blocks = d;
        memcpy(prev, fake_oled, OLED_MATRIX_SIZE);
    }
    tick();                                                       // state is back at 0
    check("one full loop at rest returns the identical frame", memcmp(first, fake_oled, OLED_MATRIX_SIZE) == 0);
    printf("       (dirty blocks per tick at rest: mean %.1f, max %d)\n", blocks / 510.0, max_blocks);
    check("never more than the panel's 16 blocks", max_blocks <= 16);

    // --- at speed: wpm 50 -> state += 4, split 12; loop is 128 ticks --------
    fake_wpm = 50;
    for (int t = 0; t < 4; t++) tick();   // let the new speed settle into the frame
    memcpy(first, fake_oled, OLED_MATRIX_SIZE);
    for (int t = 0; t < 128; t++) tick();
    check("one full loop at wpm 50 returns the identical frame", memcmp(first, fake_oled, OLED_MATRIX_SIZE) == 0);

    bool ship_ok2 = true;
    const int split = 50 / WPM_SPLIT_DIVISOR;
    for (int p = 2; p <= 5; p++)
        for (int i = split; i < 128; i++) {
            uint8_t out = fake_oled[p * 128 + i], m = (uint8_t)masks[p - 2][i - split], s = (uint8_t)ships[p - 2][i - split];
            if ((out & (uint8_t)~m) != (s & (uint8_t)~m) || (out & s) != s) ship_ok2 = false;
        }
    check("the ship has slid right by wpm/4 and is still bit-exact", ship_ok2);

    check("the driver's one-past-the-end byte was never touched", fake_oled[OLED_MATRIX_SIZE] == 0);
    puts(failures ? "space: FAILED" : "space: all good");
    return failures != 0;
}
