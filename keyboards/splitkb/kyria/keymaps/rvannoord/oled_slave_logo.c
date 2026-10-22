// Slave-side OLED: the static screens, the Kyria logo and the three Monitor ERP ones
// (art/monitor_art.c; docs/layout.md, "The Monitor ERP screens"). Whole panels, written
// straight to the driver, which only marks the bytes that change. See oled.h.

#include QMK_KEYBOARD_H
#include "oled.h"

#include "art/oled_kyria.h"
#include "art/monitor_art.h"

void u_slave_logo_render(enum u_slave_anim which) {
    switch (which) {
        case U_ANIM_KYRIA:           oled_write_raw_P(kyria_logo, sizeof(kyria_logo)); break;
        case U_ANIM_MONITOR:         oled_write_raw_P((const char *)u_monitor_screen[U_MONITOR_HORIZONTAL], 1024); break;
        case U_ANIM_MONITOR_STACKED: oled_write_raw_P((const char *)u_monitor_screen[U_MONITOR_STACKED], 1024); break;
        default:                     oled_write_raw_P((const char *)u_monitor_screen[U_MONITOR_FUTURE], 1024); break;
    }
}
