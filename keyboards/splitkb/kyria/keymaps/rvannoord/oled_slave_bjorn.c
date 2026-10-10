// Slave-side OLED: Bjorn, the Tamagotchi Viking, reacting to typing speed.
//
// Stage 1 of the OLED redesign in docs/layout.md. Deliberately needs no new split
// traffic: WPM already crosses the link through SPLIT_WPM_ENABLE, and everything
// else here is local timing. The axe, Caps Word, the alarms and the goat all need
// the custom split transaction and arrive in stage 2.

#include QMK_KEYBOARD_H
#include "oled_panel.h"

#ifdef OLED_SLAVE_ANIMATION_BJORN

#include "bjorn_art.h"

// --- tuning -----------------------------------------------------------------
// WPM tiers. Deliberately low while the Colemak-DH migration is still settling;
// with higher boundaries the upper postures would simply never be seen.
static const uint8_t bjorn_tier_up[3]   = {10, 30, 50};
static const uint8_t bjorn_tier_down[3] = { 6, 24, 42};   // each strictly below its up
#define BJORN_TIER_DWELL_MS 800                            // a tier hop repaints the column

#define BJORN_SETTLE_MS  8000                              // drop to the resting posture
#define BJORN_DOZE_MS   75000                              // eyes shut; OLED_TIMEOUT is 120 s

// Ambient is counted in render ticks, not wall clock, so it freezes with the panel
// and resumes where it stopped instead of jumping to a random phase on wake.
#define BJORN_TICK_HZ        20                            // OLED_UPDATE_INTERVAL is 50 ms
#define BJORN_BLINK_HOLD      4                            // 200 ms
#define BJORN_BLINK_MIN     120                            // 6 s
#define BJORN_BLINK_SPREAD   80                            // ... to 10 s
#define BJORN_MICRO_PERIOD   72                            // 3.6 s, a breath
#define BJORN_MICRO_HOLD      6                            // 300 ms
// Asleep he breathes slower, and asymmetrically: a short draw in and a long let out.
// An even alternation reads as a metronome rather than as breathing.
#define BJORN_DOZE_PERIOD   108                            // 5.4 s in all
#define BJORN_DOZE_INHALE    36                            // 1.8 s in, 3.6 s out

// --- state ------------------------------------------------------------------
typedef struct {
    uint8_t  wpm;
    uint32_t idle_ms;
} bjorn_in_t;

static uint8_t  bjorn_tier;              // never reinitialised: correct across a sleep
static uint32_t bjorn_tier_changed;
static uint8_t  bjorn_last_wpm;
static uint32_t bjorn_wpm_changed;
static uint16_t bjorn_ticks;
static uint16_t bjorn_blink_at = BJORN_BLINK_MIN;
static uint32_t bjorn_rng = 1;

// Stage 2 adds layer, mods, locks and flags here, and nothing else in this file moves.
static void bjorn_gather(bjorn_in_t *in) {
    in->wpm = get_current_wpm();

    if (in->wpm != bjorn_last_wpm) {
        bjorn_last_wpm   = in->wpm;
        bjorn_wpm_changed = timer_read32();
    }
    // Gate on how long the reading has been STILL, not on it being zero. If the split
    // link drops mid-word the slave's copy of WPM freezes non-zero for ever, and a
    // wpm == 0 test would leave him sprinting until the board is replugged.
    const uint32_t since_wpm = timer_elapsed32(bjorn_wpm_changed);
    const uint32_t since_key = last_input_activity_elapsed();   // this half's own keys
    in->idle_ms = since_wpm < since_key ? since_wpm : since_key;
}

static void bjorn_update_tier(uint8_t wpm) {
    if (timer_elapsed32(bjorn_tier_changed) < BJORN_TIER_DWELL_MS) return;

    const uint8_t was = bjorn_tier;
    // while, not if: a jump from nothing to 80 WPM lands on the top tier in one tick.
    while (bjorn_tier < 3 && wpm >= bjorn_tier_up[bjorn_tier]) bjorn_tier++;
    while (bjorn_tier > 0 && wpm < bjorn_tier_down[bjorn_tier - 1]) bjorn_tier--;
    if (bjorn_tier != was) bjorn_tier_changed = timer_read32();
}

// Runs every main loop on this half, so it cannot miss an edge.
void housekeeping_task_user(void) {
    if (is_keyboard_master()) return;

    bjorn_in_t in;
    bjorn_gather(&in);
    bjorn_update_tier(in.wpm);
}

// --- drawing ----------------------------------------------------------------
static void bjorn_blit(const bjorn_frame_t *f) {
    for (uint8_t p = 0; p < f->count; p++) {
        // block * 64 IS the absolute buffer index the driver indexes by, and the mask
        // keeps us off block 16 — oled_write_raw_byte guards with > rather than >=.
        const uint16_t base = (uint16_t)(f->patches[p].block & 0x0F) * BJORN_PATCH_BYTES;
        const char    *src  = f->patches[p].data;
        for (uint8_t i = 0; i < BJORN_PATCH_BYTES; i++) {
            oled_write_raw_byte(pgm_read_byte(src + i), base + i);
        }
    }
}

static void bjorn_blit_base(void) {
    for (uint8_t page = 0; page < 8; page++) {
        for (uint8_t x = 0; x < 64; x++) {
            oled_write_raw_byte(pgm_read_byte(bjorn_base + page * 64 + x), page * 128 + x);
        }
    }
}

void oled_render_slave(void) {
    static const uint8_t pose_of_tier[4]  = {BJORN_F_POSE_T0,  BJORN_F_POSE_T1,
                                             BJORN_F_POSE_T2,  BJORN_F_POSE_T3};
    static const uint8_t blink_of_tier[4] = {BJORN_F_BLINK_T0, BJORN_F_BLINK_T1,
                                             BJORN_F_BLINK_T2, BJORN_F_BLINK_T3};

    bjorn_in_t in;
    bjorn_gather(&in);
    bjorn_ticks++;

    const bool dozing  = in.idle_ms >= BJORN_DOZE_MS;
    const bool settled = in.idle_ms >= BJORN_SETTLE_MS;
    // Rest at the default posture rather than the slow-typing one: IDLE2 is IDLE1's
    // partner frame, so this is the only pose with a breath to play. Settling to the
    // drooped-horn posture instead left him standing dead still for the whole idle.
    const uint8_t tier = settled ? 1 : bjorn_tier;

    // Every tick: base, then the pose on top, then at most one overlay. No bookkeeping
    // about what is already on screen — oled_write_raw_byte drops unchanged bytes, so
    // the driver is the dirty tracker and we cannot get out of step with it.
    bjorn_blit_base();
    bjorn_blit(&bjorn_frames[dozing ? BJORN_F_DOZE : pose_of_tier[tier]]);
    bjorn_blit(&bjorn_frames[BJORN_F_FLOOR_RIGHT]);

    if (dozing) {
        // No blink — his eyes are already shut. The breath is his beard rising, since
        // he has no chest; it is his own outline pushed down a pixel, not new art.
        if (bjorn_ticks % BJORN_DOZE_PERIOD < BJORN_DOZE_INHALE) {
            bjorn_blit(&bjorn_frames[BJORN_F_DOZE_BREATH]);
        }
        return;
    }

    if (bjorn_ticks - bjorn_blink_at < BJORN_BLINK_HOLD) {
        bjorn_blit(&bjorn_frames[blink_of_tier[tier]]);
        return;                               // one overlay at a time, never stacked
    }
    if (bjorn_ticks - bjorn_blink_at < BJORN_BLINK_HOLD + 1) {
        bjorn_rng = bjorn_rng * 1664525u + 1013904223u;
        bjorn_blink_at = bjorn_ticks + BJORN_BLINK_MIN + (bjorn_rng >> 24) % BJORN_BLINK_SPREAD;
    }
    // Breathing belongs to the idle, where attention is free; while he is typing the
    // panel stays still apart from the blink.
    if (settled && bjorn_ticks % BJORN_MICRO_PERIOD < BJORN_MICRO_HOLD) {
        bjorn_blit(&bjorn_frames[BJORN_F_MICRO]);
    }
}

#endif     // OLED_SLAVE_ANIMATION_BJORN
