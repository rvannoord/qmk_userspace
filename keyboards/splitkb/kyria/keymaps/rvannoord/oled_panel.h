// Which panel and which slave-side art get compiled in. This is the only place that
// decides; the art headers are pure data and the renderers only ask whether they are
// the chosen one. See docs/layout.md.
//
// Pick one of each in users/manna-harbour_miryoku/custom_config.h.

#pragma once

#include <stdbool.h>

// Counted the long way on purpose: `defined(X)` produced by a macro inside #if is
// undefined behaviour in ISO C, and GCC warns about it (-Wexpansion-to-defined).
#if defined(OLED_SLAVE_ANIMATION_SPACESHIP)
#    define U_OLED_SLAVE_N_SPACESHIP 1
#else
#    define U_OLED_SLAVE_N_SPACESHIP 0
#endif
#if defined(OLED_SLAVE_ANIMATION_MONITOR1)
#    define U_OLED_SLAVE_N_MONITOR1 1
#else
#    define U_OLED_SLAVE_N_MONITOR1 0
#endif
#if defined(OLED_SLAVE_ANIMATION_MONITOR2)
#    define U_OLED_SLAVE_N_MONITOR2 1
#else
#    define U_OLED_SLAVE_N_MONITOR2 0
#endif
#if defined(OLED_SLAVE_ANIMATION_KYRIA)
#    define U_OLED_SLAVE_N_KYRIA 1
#else
#    define U_OLED_SLAVE_N_KYRIA 0
#endif
#if defined(OLED_SLAVE_ANIMATION_BJORN)
#    define U_OLED_SLAVE_N_BJORN 1
#else
#    define U_OLED_SLAVE_N_BJORN 0
#endif

#define U_OLED_SLAVE_N                                                    \
    (U_OLED_SLAVE_N_SPACESHIP + U_OLED_SLAVE_N_MONITOR1 +                 \
     U_OLED_SLAVE_N_MONITOR2 + U_OLED_SLAVE_N_KYRIA + U_OLED_SLAVE_N_BJORN)

#if U_OLED_SLAVE_N == 0
#    define OLED_SLAVE_ANIMATION_SPACESHIP
#elif U_OLED_SLAVE_N > 1
#    error "Pick exactly one OLED_SLAVE_ANIMATION_* in custom_config.h"
#endif

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
