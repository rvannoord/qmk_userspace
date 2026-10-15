// Slave-side OLED: the static logos. All three are compiled and chosen at
// runtime now, so all three headers are included - they are pure data with
// distinct names, so there is nothing to collide. See oled_panel.h.

#include QMK_KEYBOARD_H
#include "oled_panel.h"

#include "art/oled_kyria.h"
#include "art/oled_monitor1.h"
#include "art/oled_monitor2.h"

void u_slave_logo_render(enum u_slave_anim which) {
    switch (which) {
        case U_ANIM_KYRIA:    oled_write_raw_P(kyria_logo, sizeof(kyria_logo));       break;
        case U_ANIM_MONITOR1: oled_write_raw_P(monitor_logo1, sizeof(monitor_logo1)); break;
        default:              oled_write_raw_P(monitor_logo2, sizeof(monitor_logo2)); break;
    }
}
