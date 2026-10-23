// Bongo cat: HorrorTroll's art from drashna's bongocats module, along the bottom, and the
// WPM at 2x in the top right corner. He idles below 10 WPM, holds his paws up to 20, then
// slaps, faster as WPM climbs: max(100, 450 - 2.5 x WPM) ms a slap. docs/layout.md, "Two
// more slave animations".

#include QMK_KEYBOARD_H
#include <string.h>
#include "oled.h"
#include "oled_canvas.h"
#include "oled_draw.h"
#include "split_sync.h"
#include "art/bongo_art.h"

#ifdef OLED_ENABLE

#    define U_BONGO_IDLE_MS 300
#    define U_BONGO_Y       32

// The corner the Mario scene uses too: three digits at 2x from x=90, "wpm" under them.
void u_slave_wpm_corner(uint8_t wpm) {
    char b[4];
    u_num3(b, wpm);
    u_text(90, 1, b, 2, true);
    u_text(108, 18, "wpm", 1, true);
}

static uint8_t  slap;     // which tap frame
static uint32_t slap_at;

void u_slave_bongo_render(void) {
    const uint8_t   wpm = get_current_wpm();
    const uint8_t  *art;

    if (wpm < 10) {
        art = u_bongo_idle[(timer_read32() / U_BONGO_IDLE_MS) % U_BONGO_IDLE_FRAMES];
    } else if (wpm < 20) {
        art = u_bongo_prep[0];
    } else {
        const int16_t ms = (int16_t)(450 - (wpm * 5) / 2);
        if (timer_elapsed32(slap_at) >= (uint32_t)(ms > 100 ? ms : 100)) {
            slap    = (uint8_t)(slap ^ 1u);
            slap_at = timer_read32();
        }
        art = u_bongo_tap[slap];
    }

    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    u_blit(art, U_BONGO_W, U_BONGO_PAGES, 0, U_BONGO_Y);
    u_slave_wpm_corner(wpm);
    u_glitch_frame(u_sync_hard_stale());
    u_canvas_commit();
}

#endif     // OLED_ENABLE
