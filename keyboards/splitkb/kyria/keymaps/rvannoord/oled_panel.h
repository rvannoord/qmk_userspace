// Which panel and which slave-side art get compiled in. This is the only place that
// decides; the art headers are pure data and the renderers only ask whether they are
// the chosen one. See docs/layout.md.
//
// Pick one of each in users/manna-harbour_miryoku/custom_config.h.

#pragma once

#include <stdbool.h>

// Counted the long way on purpose: `defined(X)` produced by a macro inside #if is
// undefined behaviour in ISO C, and GCC warns about it (-Wexpansion-to-defined).
// --- the slave's animation --------------------------------------------------
// All five are compiled in and chosen at runtime, because switching between them
// is the point. OLED_SLAVE_ANIMATION_* in custom_config.h now names only the
// *boot default* - the one you get on a fresh EEPROM - rather than the only one
// that exists. It is no longer an error to define none, and defining more than
// one is resolved by taking the first in this order rather than failing a build.

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

#if defined(OLED_MASTER_PANEL_TEXT)
#    define U_OLED_MASTER_N_TEXT 1
#else
#    define U_OLED_MASTER_N_TEXT 0
#endif

#if defined(OLED_MASTER_PANEL_ZONES)
#    define U_OLED_MASTER_N_ZONES 1
#else
#    define U_OLED_MASTER_N_ZONES 0
#endif

#define U_OLED_MASTER_N (U_OLED_MASTER_N_TEXT + U_OLED_MASTER_N_ZONES)

#if U_OLED_MASTER_N == 0
#    define OLED_MASTER_PANEL_ZONES
#elif U_OLED_MASTER_N > 1
#    error "Pick exactly one OLED_MASTER_PANEL_* in custom_config.h"
#endif

// Implemented by whichever renderer was selected above. Each is a no-op in the
// builds where its own file compiled to nothing.
//
// oled_render_* draw, and run at the panel's update rate — 20 Hz, and not at all
// while it sleeps. oled_*_task run every main loop on their own half, so they are
// where state that must not miss an edge belongs.
// QMK's own display timeout follows oled_on(), which any dirty block calls, so a panel
// that keeps drawing can hold itself awake for ever. This reads the input clock instead,
// which no amount of drawing can re-arm. Pure, so the tests can pin the boundary.
static inline bool u_oled_should_sleep(bool on, uint32_t idle_ms) {
    return OLED_TIMEOUT > 0 && on && idle_ms >= OLED_TIMEOUT;
}

void oled_render_master(void);
void oled_render_slave(void);
void oled_master_task(void);
void oled_slave_task(void);
