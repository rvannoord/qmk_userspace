// Which animation the slave half draws, and the switch that changes it.
//
// Two things make this less trivial than it looks.
//
// **The half that is told is not the half that draws.** Keypresses are processed
// on the master, so pressing the key there has to reach the other side. It rides
// in `world` on the existing per-loop state sync rather than in a transaction of
// its own - see split_state.h for why widening the payload is safe where adding
// a transaction would not be.
//
// **A setting you re-make every morning is not a setting**, so it is persisted.
// Only the master persists it: the slave has no opinion and takes what it is
// told, which also means the two halves cannot disagree after a replug.

#include QMK_KEYBOARD_H
#include "oled.h"
#include "oled_settings.h"
#include "split_state.h"
#include "split_sync.h"

#ifdef OLED_ENABLE

// The stored byte lives in oled_settings.c, which falls back to the build's default on
// a fresh EEPROM.
static uint8_t anim_local = U_ANIM_DEFAULT;
static bool    anim_loaded;

static void anim_load(void) {
    if (anim_loaded) return;
    anim_loaded = true;
    const uint8_t v = u_setting_get(U_SETTING_ANIM);
    if (v < U_ANIM_COUNT) anim_local = v;
}

enum u_slave_anim u_anim_get(void) {
    if (is_keyboard_master()) {
        anim_load();
        return (enum u_slave_anim)anim_local;
    }
    // The slave takes what crossed the link. A stale or zeroed sync reads as
    // Bjorn, which is the default anyway, so a dead link shows the mascot rather
    // than nothing - and the mascot not blinking is already this design's way of
    // saying the link is dead.
    const uint8_t w = U_WORLD_ANIM(u_sync_state().world);
    return (enum u_slave_anim)(w < U_ANIM_COUNT ? w : U_ANIM_BJORN);
}

void u_anim_next(void) {
    if (!is_keyboard_master()) return;
    anim_load();
    anim_local = (uint8_t)((anim_local + 1) % U_ANIM_COUNT);
    u_setting_set(U_SETTING_ANIM, anim_local);
}

// --- dispatch ---------------------------------------------------------------

void oled_render_slave(void) {
    switch (u_anim_get()) {
        case U_ANIM_SPACESHIP: u_slave_spaceship_render(); break;
        case U_ANIM_KYRIA:
        case U_ANIM_MONITOR1:
        case U_ANIM_MONITOR2:  u_slave_logo_render(u_anim_get()); break;
        default:               u_slave_bjorn_render(); break;
    }
}

void oled_slave_task(void) {
    // Only Bjorn keeps state between frames. Running his task while another
    // animation is showing would let his idle clocks advance unseen, so he would
    // come back mid-doze; letting it run only while he is drawn means he always
    // returns awake, which is the honest reading - you just arrived.
    if (u_anim_get() == U_ANIM_BJORN) u_slave_bjorn_task();
}

#endif     // OLED_ENABLE
