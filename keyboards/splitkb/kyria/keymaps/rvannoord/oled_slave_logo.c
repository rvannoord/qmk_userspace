// Slave-side OLED: the static screens, the Kyria logo and the three Monitor ERP ones
// (art/monitor_art.c; docs/layout.md, "The Monitor ERP screens"). Whole panels, composed
// in the canvas like every other frame, so a lost link's glitch reaches them too. See oled.h.

#include QMK_KEYBOARD_H
#include "oled.h"
#include "oled_canvas.h"
#include "oled_draw.h"
#include "split_sync.h"

#include "art/oled_kyria.h"
#include "art/monitor_art.h"

void u_slave_logo_render(enum u_slave_anim which) {
    const char *src;
    switch (which) {
        case U_ANIM_KYRIA:           src = kyria_logo; break;
        case U_ANIM_MONITOR:         src = (const char *)u_monitor_screen[U_MONITOR_HORIZONTAL]; break;
        case U_ANIM_MONITOR_STACKED: src = (const char *)u_monitor_screen[U_MONITOR_STACKED]; break;
        default:                     src = (const char *)u_monitor_screen[U_MONITOR_FUTURE]; break;
    }
    for (uint16_t i = 0; i < OLED_MATRIX_SIZE; i++) u_canvas[i] = pgm_read_byte(&src[i]);
    u_glitch_frame(u_sync_hard_stale());
    u_canvas_commit();
}
