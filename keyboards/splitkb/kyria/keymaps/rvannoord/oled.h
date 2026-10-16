// The OLED module's interface: what keymap.c calls, what the renderers implement, and
// which slave animations exist. oled.c is the dispatcher. The master half (the one with
// the USB cable) draws oled_master_zones.c; the other half draws one of the oled_slave_*
// animations, chosen at runtime with the U_ANIM key. Design in docs/layout.md, OLED.

#pragma once

#include <stdbool.h>
#include <stdint.h>

// --- the slave's animation --------------------------------------------------
// All five are compiled in and chosen at runtime, because switching between them
// is the point. OLED_SLAVE_ANIMATION_* in custom_config.h names only the *boot
// default* - the one you get on a fresh EEPROM. Defining none means Bjorn, and
// defining more than one takes the first in this order.

enum u_slave_anim {
    U_ANIM_BJORN,
    U_ANIM_SPACESHIP,
    U_ANIM_KYRIA,
    U_ANIM_MONITOR1,
    U_ANIM_MONITOR2,
    U_ANIM_COUNT,
};

#if defined(OLED_SLAVE_ANIMATION_BJORN)
#    define U_ANIM_DEFAULT U_ANIM_BJORN
#elif defined(OLED_SLAVE_ANIMATION_SPACESHIP)
#    define U_ANIM_DEFAULT U_ANIM_SPACESHIP
#elif defined(OLED_SLAVE_ANIMATION_KYRIA)
#    define U_ANIM_DEFAULT U_ANIM_KYRIA
#elif defined(OLED_SLAVE_ANIMATION_MONITOR1)
#    define U_ANIM_DEFAULT U_ANIM_MONITOR1
#elif defined(OLED_SLAVE_ANIMATION_MONITOR2)
#    define U_ANIM_DEFAULT U_ANIM_MONITOR2
#else
#    define U_ANIM_DEFAULT U_ANIM_BJORN
#endif

// Which one is live. Persisted, so it survives a replug - a setting you have to
// re-make every morning is not a setting.
enum u_slave_anim u_anim_get(void);
void              u_anim_next(void);

// Each animation's own entry points. The dispatcher in oled_slave_select.c picks
// between them; only Bjorn needs a per-loop task, and the rest are static or
// drive themselves from the render.
void u_slave_bjorn_render(void);
void u_slave_bjorn_task(void);
void u_slave_spaceship_render(void);
void u_slave_logo_render(enum u_slave_anim which);

// --- the two halves -----------------------------------------------------------
// oled_render_* draw, and run at the panel's update rate - 20 Hz, and not at all
// while it sleeps. oled_*_task run every main loop on their own half, so they are
// where state that must not miss an edge belongs.
void oled_render_master(void);
void oled_render_slave(void);
void oled_master_task(void);
void oled_slave_task(void);

// QMK's own display timeout follows oled_on(), which any dirty block calls, so a panel
// that keeps drawing can hold itself awake for ever. This reads the input clock instead,
// which no amount of drawing can re-arm. Pure, so the tests can pin the boundary.
static inline bool u_oled_should_sleep(bool on, uint32_t idle_ms) {
    return OLED_TIMEOUT > 0 && on && idle_ms >= OLED_TIMEOUT;
}
