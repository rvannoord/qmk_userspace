// Slave-side OLED: one static logo. Only the selected bitmap is compiled in,
// because only its header is included. See oled_panel.h.

#include QMK_KEYBOARD_H
#include "oled_panel.h"

#if defined(OLED_SLAVE_ANIMATION_KYRIA) || defined(OLED_SLAVE_ANIMATION_MONITOR1) || \
    defined(OLED_SLAVE_ANIMATION_MONITOR2)

#if defined(OLED_SLAVE_ANIMATION_KYRIA)
#    include "art/oled_kyria.h"
#elif defined(OLED_SLAVE_ANIMATION_MONITOR1)
#    include "art/oled_monitor1.h"
#else
#    include "art/oled_monitor2.h"
#endif

void oled_render_slave(void) {
#if defined(OLED_SLAVE_ANIMATION_KYRIA)
    oled_write_raw_P(kyria_logo, sizeof(kyria_logo));
#elif defined(OLED_SLAVE_ANIMATION_MONITOR1)
    oled_write_raw_P(monitor_logo1, sizeof(monitor_logo1));
#else
    oled_write_raw_P(monitor_logo2, sizeof(monitor_logo2));
#endif
}

#endif
