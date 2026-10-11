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
// A blink closes ballistically and opens slowly; the asymmetry is the most
// recognisable thing about it. Shut is a cut, the opening goes through a half lid.
#define BJORN_BLINK_SHUT      3                            // 150 ms shut
#define BJORN_BLINK_OPEN      5                            // then 100 ms half lid; over here
#define BJORN_BLINK_GAP       8                            // start-to-start for a double blink
#define BJORN_BLINK_PAIR_P   51                            // of 256, so about one in five
#define BJORN_BLINK_MIN     120                            // 6 s
#define BJORN_BLINK_SPREAD   80                            // ... to 10 s
// Awake, the breath is four states over 3.6 s. The easing is in the dwell, not in
// the shape: with one pixel of travel there is no curve to draw, so the rhythm has
// to carry it. Quiet breathing is roughly 1:2.4 in to out, with a short pause at
// the top and a long one at the bottom. The body leads and the head follows, both
// on the way in and on the way out, because the chest drives and the head is driven.
#define BJORN_BREATH_PERIOD  72                            // 3.6 s
#define BJORN_BREATH_SWELL   36                            // body fills, head still down
#define BJORN_BREATH_FULL    41                            // head arrives, held
#define BJORN_BREATH_SETTLE  57                            // body empties, head still up
#define BJORN_BREATH_REST    65                            // head comes down
#define BJORN_DOZE_PERIOD   120                            // 6 s in all
#define BJORN_DOZE_INHALE    30                            // 1.5 s in, 4.5 s out

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
static uint16_t bjorn_breath_origin;      // latched when he settles, so the first
                                          // thing he does is a full rest, then an inhale
static uint16_t bjorn_blink_at = BJORN_BLINK_MIN;
static bool     bjorn_blink_pair;         // the next blink is the second of a pair

// Falling asleep is the blink played slowly and forwards; waking is it played back.
#define BJORN_DOZE_HALF       6                            // 300 ms of half lid
#define BJORN_DOZE_SHUT      16                            // then shut until here, then the pose
#define BJORN_WAKE_SHUT       2                            // 100 ms shut on waking
#define BJORN_WAKE_OPEN       4                            // then half lid; eyes open here
#define BJORN_RENDER_GAP_MS 500                            // a longer gap means the panel slept

static uint8_t  bjorn_state;              // 0 awake, 1 falling asleep, 2 asleep
static uint16_t bjorn_state_at;           // tick the current state began
static uint16_t bjorn_wake_at;            // tick the waking gesture began
static uint32_t bjorn_last_render;
static uint16_t bjorn_doze_origin;        // latched on falling asleep
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
void oled_slave_task(void) {
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
    static const uint8_t half_of_tier[4]  = {BJORN_F_HALF_T0,  BJORN_F_HALF_T1,
                                             BJORN_F_HALF_T2,  BJORN_F_HALF_T3};

    bjorn_in_t in;
    bjorn_gather(&in);

    // The panel does not render while it sleeps, so a long gap between calls means it
    // has just come back. Waking from the timeout and waking from a doze are the same
    // event — a keypress — and get the same gesture.
    const bool panel_woke = timer_elapsed32(bjorn_last_render) > BJORN_RENDER_GAP_MS;
    bjorn_last_render = timer_read32();
    bjorn_ticks++;

    const bool settled = in.idle_ms >= BJORN_SETTLE_MS;
    const uint8_t tier = settled ? 1 : bjorn_tier;

    // Breath phase, latched to the moment he settles so his first act is a full rest.
    uint8_t breath = 0;                       // 0 rest, 1 swell, 2 full, 3 settle
    if (settled) {
        const uint16_t phase = (uint16_t)(bjorn_ticks - bjorn_breath_origin) % BJORN_BREATH_PERIOD;
        if (phase >= BJORN_BREATH_REST)        breath = 0;
        else if (phase >= BJORN_BREATH_SETTLE) breath = 3;
        else if (phase >= BJORN_BREATH_FULL)   breath = 2;
        else if (phase >= BJORN_BREATH_SWELL)  breath = 1;
    } else {
        bjorn_breath_origin = bjorn_ticks;
    }

    // --- awake / falling asleep / asleep ------------------------------------
    if (in.idle_ms < BJORN_DOZE_MS) {
        if (bjorn_state != 0) {               // a keypress; wake him
            bjorn_state   = 0;
            bjorn_wake_at = bjorn_ticks;
        }
    } else if (bjorn_state == 0) {
        // Only start closing his eyes at the bottom of a breath: the half-lid patch is
        // built over the resting body, so dropping it on a risen frame would snap the
        // body back down. Nobody nods off mid-inhale either.
        if (breath == 0) {
            bjorn_state    = 1;
            bjorn_state_at = bjorn_ticks;
        }
    } else if (bjorn_state == 1 &&
               (uint16_t)(bjorn_ticks - bjorn_state_at) >= BJORN_DOZE_SHUT) {
        bjorn_state       = 2;
        bjorn_doze_origin = bjorn_ticks;      // sleeping breath starts from rest
    }
    if (panel_woke && bjorn_state == 0) bjorn_wake_at = bjorn_ticks;
    if (bjorn_state == 1) breath = 0;         // hold him still while his eyes close

    // --- draw ---------------------------------------------------------------
    bjorn_blit_base();
    bjorn_blit(&bjorn_frames[bjorn_state == 2 ? BJORN_F_DOZE : pose_of_tier[tier]]);
    bjorn_blit(&bjorn_frames[BJORN_F_FLOOR_RIGHT]);

    if (bjorn_state == 2) {
        const uint16_t phase = (uint16_t)(bjorn_ticks - bjorn_doze_origin) % BJORN_DOZE_PERIOD;
        if (phase < BJORN_DOZE_INHALE) bjorn_blit(&bjorn_frames[BJORN_F_DOZE_BREATH]);
        return;                               // asleep: eyes already shut
    }

    if (breath == 1) bjorn_blit(&bjorn_frames[BJORN_F_BREATH_SWELL]);
    if (breath == 2) bjorn_blit(&bjorn_frames[BJORN_F_BREATH]);
    if (breath == 3) bjorn_blit(&bjorn_frames[BJORN_F_BREATH_SETTLE]);

    // Which eye frame, if any. Picked from this tick's breath state, not from the one
    // the gesture started in: a blink is five ticks long and can straddle a change, and
    // an unlifted patch over a risen body drags his horn tip and beard back down.
    const uint8_t shut = (breath >= 2) ? BJORN_F_BLINK_LIFT : blink_of_tier[tier];
    const uint8_t half = (breath >= 2) ? BJORN_F_HALF_LIFT  : half_of_tier[tier];

    if (bjorn_state == 1) {                   // falling asleep: slow, forwards
        const uint16_t d = bjorn_ticks - bjorn_state_at;
        bjorn_blit(&bjorn_frames[d < BJORN_DOZE_HALF ? half : shut]);
        return;
    }

    const uint16_t w = bjorn_ticks - bjorn_wake_at;
    if (w < BJORN_WAKE_OPEN) {                // waking: the same lids, played backwards
        bjorn_blit(&bjorn_frames[w < BJORN_WAKE_SHUT ? shut : half]);
        return;
    }

    const uint16_t d = bjorn_ticks - bjorn_blink_at;
    if (d < BJORN_BLINK_SHUT) {
        bjorn_blit(&bjorn_frames[shut]);
    } else if (d < BJORN_BLINK_OPEN) {
        bjorn_blit(&bjorn_frames[half]);
    } else if (d == BJORN_BLINK_OPEN) {
        bjorn_rng = bjorn_rng * 1664525u + 1013904223u;
        if (bjorn_blink_pair) {               // that was the second of the pair
            bjorn_blink_pair = false;
            bjorn_blink_at = bjorn_ticks + BJORN_BLINK_MIN + (bjorn_rng >> 24) % BJORN_BLINK_SPREAD;
        } else if ((bjorn_rng >> 24) < BJORN_BLINK_PAIR_P) {
            // Real blinks cluster. Keep the beat exact by counting from the last start.
            bjorn_blink_pair = true;
            bjorn_blink_at  += BJORN_BLINK_GAP;
        } else {
            // A fresh word for the interval, or "long gap" and "was a double" end up
            // perfectly correlated and the pattern becomes legible within a minute.
            bjorn_rng = bjorn_rng * 1664525u + 1013904223u;
            bjorn_blink_at = bjorn_ticks + BJORN_BLINK_MIN + (bjorn_rng >> 24) % BJORN_BLINK_SPREAD;
        }
    }
}

#endif     // OLED_SLAVE_ANIMATION_BJORN
