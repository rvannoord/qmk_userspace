// Slave-side OLED: Bjorn, the Tamagotchi Viking, reacting to typing speed.
//
// The design is in docs/layout.md, OLED section; the build order is docs/oled-v2.md.
// WPM crosses the link through SPLIT_WPM_ENABLE and drives the postures; everything
// the props need — held modifiers, Caps Word, misfires, the layers — arrives on the
// custom U_SYNC_STATE transaction. Built here: postures, blink, breath, doze and
// waking, the axe, the misfire raise, the Caps Word crown and tile inversion. Not
// built yet: the goat, Caps Lock, the defeated pose for a non-base default layer.

#include QMK_KEYBOARD_H
#include "oled_panel.h"

#ifdef OLED_SLAVE_ANIMATION_BJORN

#include "bjorn_art.h"
#include "bjorn_poses.h"
#include "split_sync.h"
#include "oled_canvas.h"

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

    // From the master, via the split transaction. Everything below is drawn, except
    // base_layer, which the defeated pose will use.
    uint8_t  layer;        // the active layer
    uint8_t  base_layer;   // the default layer — not U_BASE means silently on QWERTY
    uint8_t  fingers;      // modifiers by finger, outer to inner, both hands
    uint8_t  flags;        // locks, Caps Word, host OS
    uint8_t  anomaly;      // misfire count; only ever compared for change
    bool     soft_stale;   // state no longer vouched for: drop the props, keep him alive
    bool     hard_stale;   // link dead
} bjorn_in_t;

static uint8_t  bjorn_tier;              // never reinitialised: correct across a sleep
static uint32_t bjorn_tier_changed;
static uint16_t bjorn_ticks;
static uint16_t bjorn_breath_origin;      // latched when he settles, so the first
                                          // thing he does is a full rest, then an inhale
static uint16_t bjorn_blink_at = BJORN_BLINK_MIN;
static bool     bjorn_blink_pair;         // the next blink is the second of a pair

// Falling asleep is the blink played slowly and forwards; waking is it played back.
#define BJORN_DOZE_HALF       6                            // 300 ms of half lid
#define BJORN_DOZE_SHUT      16                            // then shut until here, then the pose
// A command modifier, released after it was used. 800 ms is the floor at which a pose
// change reads as static rather than as a flicker.
#define BJORN_AXE_TICKS      16                            // 800 ms

// A misfire: a command modifier held past half a second with nothing typed. The one
// thing in the design that animates, because the only thing that should move is the
// only thing that is wrong. The steps slow as the axe rises — a lifted mass
// decelerates toward the top of its arc, and that is what makes it read as heavy.
#define BJORN_MF_GRIP         2                            // 100 ms taking hold
#define BJORN_MF_R1           5                            // 150 ms, rising
#define BJORN_MF_R2           9                            // 200 ms, slower near the top
#define BJORN_MF_TOP         29                            // 1000 ms held overhead
#define BJORN_MF_D2          31                            // and back down, faster than up:
#define BJORN_MF_D1          33                            // lifting is effort, dropping is not
#define BJORN_MF_END         35

// Caps Word: a crown, and his tile flipped to dark-on-white. That is the polarity his
// art was drawn in — it is an e-paper original — so inverted he reads better, not worse.
#define BJORN_CAPS_FLIP       2                            // crown first, world 100 ms later

#define BJORN_WAKE_SHUT       2                            // 100 ms shut on waking
#define BJORN_WAKE_OPEN       4                            // then half lid; eyes open here
#define BJORN_RENDER_GAP_MS 500                            // a longer gap means the panel slept

static uint8_t  bjorn_state;              // 0 awake, 1 falling asleep, 2 asleep
static uint16_t bjorn_state_at;           // tick the current state began
static uint16_t bjorn_wake_at;            // tick the waking gesture began
static uint16_t bjorn_axe_at;             // tick the axe appeared
static bool     bjorn_axe_live;           // or tick 0 looks like a release at boot
static uint16_t bjorn_mf_at;              // tick the misfire began
static bool     bjorn_mf_live;
static uint8_t  bjorn_last_anomaly;
static uint16_t bjorn_caps_at;            // tick Caps Word began
static bool     bjorn_caps_was;
static bool     bjorn_invert;             // his tile only, never the other half
static uint32_t bjorn_last_render;
static uint16_t bjorn_doze_origin;        // latched on falling asleep
static uint32_t bjorn_rng = 1;
static bool     bjorn_was_hard_stale;

static void bjorn_gather(bjorn_in_t *in) {
    in->wpm = get_current_wpm();

    // One clock, shared with the master through SPLIT_ACTIVITY_ENABLE, so this half
    // starts idling at the same instant the master does and the panels agree about when
    // the show ends. It used to take the smaller of this and "how long the WPM reading
    // has been still", which lagged by up to five seconds: WPM keeps changing all the
    // way down as it decays over its own five-second window, so the reading is never
    // still until long after the typing stopped.
    //
    // WPM is still what picks the posture tier. It is just no longer asked what time it
    // is. The old concern it answered — that a dropped link freezes WPM non-zero and
    // leaves him sprinting for ever — is covered by the tier being forced to its
    // resting value once settled, which this clock now decides honestly.
    in->idle_ms = last_input_activity_elapsed();

    const u_split_state_t s = u_sync_state();
    in->layer      = s.layers & 0x0F;
    in->base_layer = s.layers >> 4;
    in->fingers    = s.fingers;
    in->flags      = s.flags;
    in->anomaly    = s.anomaly;
    in->soft_stale = u_sync_soft_stale();
    in->hard_stale = u_sync_hard_stale();

    // Soft stale means the master has gone quiet for longer than two heartbeats, so the
    // transient state is no longer worth believing. Blanking it here means no prop added
    // later can show something stale — the props read these fields and nothing else.
    if (in->soft_stale) {
        in->fingers = 0;
        in->flags  &= (uint8_t)~(U_FLAG_CAPS_WORD | U_FLAG_NUM_LOCK | U_FLAG_CAPS_LOCK | U_FLAG_SCROLL_LOCK);
    }
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
// His tile inverts; the other half never does. Row 63 is preserved — it is the floor
// rule, and it runs across both halves, so flipping only the left of it would put the
// two halves' dots a pixel out of phase and recreate the seam step the floor exists to
// avoid. Row 63 is bit 7 of page 7, hence 0x7f.
static uint8_t bjorn_mask(uint8_t block) {
    if (!bjorn_invert || (block & 1u)) return 0x00;        // odd blocks are the right column
    return (block == 14) ? 0x7f : 0xff;                    // block 14 is page 7, left
}

// Both of these draw into u_canvas, never into the driver's buffer. Laying the base
// down and then patching poses over it means the blocks an overlay covers are written
// twice a frame; in the driver that marked them dirty every frame and kept this half's
// I2C busy for ever. In RAM it costs nothing, and u_canvas_commit decides what is
// actually different.
// --- authored poses ---------------------------------------------------------
// Infinition's own frames, reduced whole and never recombined. They are shown only in
// deep idle, and only as a cut: a pose change repaints the whole column, which is the
// entire per-step block budget, so it may happen when nothing else wants to move.
//
// They do not blink. The shipped blink and breath are patches aimed at the four tier
// drawings and would land on the wrong pixels over a different figure. What a pose does
// instead is breathe, by lifting the whole drawing one pixel — which is the dominant
// half of the shipped breath anyway, the body leading and the head following. That is
// enough to stay out of the fault vocabulary: the signal for a dead link is a figure
// where *nothing* changes, and this one is still moving.

#define BJORN_VIG_START_MS 20000     // deep idle; the settle at 8 s is long past
#define BJORN_VIG_HOLD       360     // 18 s a pose, off a clock that is not the breath's

static uint8_t  bjorn_vig = U_POSE_COUNT;    // U_POSE_COUNT means "none, show the tier"
static uint16_t bjorn_vig_at;
// The cursor is separate from what is showing, and survives the pose being cleared.
// Folded together, every idle period would restart at the same drawing and the rest of
// the cast would be seen about never.
static uint8_t  bjorn_vig_next;

// The breath the shipped poses have: the head rises and the beard swells beneath it.
//
// A lift on its own is not that. It was tried and it reads as the drawing being moved
// rather than as somebody breathing, however carefully the seam is placed — because
// half of what makes the shipped breath legible is the beard pushing sideways as the
// chest fills, and a translation cannot do that.
//
// Both halves are generatable, so these poses get the same treatment as the hand-drawn
// ones without being hand-drawn:
//
//   above the seam   lift one pixel: helmet, horns, face and moustache together
//   seam to row 52   swell: the outer contour moves one pixel out on each side. Only
//                    the silhouette's edge, never the interior — dilating the whole
//                    row would thicken the dither everywhere and he would just get fat
//   row 53 down      still: body, feet, and the floor he stands on
//
// A goat, where there is one, is held out of all of it. It is a separate animal on the
// same ground and does not rise or swell with his chest. Everything else in a frame is
// something he carries.

#define BJORN_POSE_SWELL_END 52     // docs/layout.md: the beard pushes over rows 41-52

// The blink, as a state rather than as a frame: 0 open, 1 shut, 2 half lid. Same
// arithmetic and the same schedule as the tier path — 150 ms shut, 100 ms half, 6 to 10
// seconds apart with about one in five doubled — because it is the same blink. The
// poses need it as a state because they cannot use the tier's eyes-shut frames: those
// are drawn over the tier figure's face and would paste its pixels onto another
// drawing.
// Six to ten seconds from now. Called when a blink finishes, and whenever he comes
// back from somewhere the blink code does not run — the doze, the waking lids, a dead
// link — so the length of that gap can never matter.
static void bjorn_schedule_blink(void) {
    bjorn_rng        = bjorn_rng * 1664525u + 1013904223u;
    bjorn_blink_at   = bjorn_ticks + BJORN_BLINK_MIN + (bjorn_rng >> 24) % BJORN_BLINK_SPREAD;
    bjorn_blink_pair = false;
}

// It also advances the schedule, because it is the only place that does. Exactly one
// of the two draw paths runs per frame, so it is called exactly once per tick.
static uint8_t bjorn_blink_state(void) {
    const int16_t d = (int16_t)(bjorn_ticks - bjorn_blink_at);
    if (d < 0) return 0;
    if (d < BJORN_BLINK_SHUT) return 1;
    if (d < BJORN_BLINK_OPEN) return 2;

    if (d == BJORN_BLINK_OPEN) {
        bjorn_rng = bjorn_rng * 1664525u + 1013904223u;
        if (bjorn_blink_pair) {               // that was the second of the pair
            bjorn_schedule_blink();
        } else if ((bjorn_rng >> 24) < BJORN_BLINK_PAIR_P) {
            // Real blinks cluster. Keep the beat exact by counting from the last start.
            bjorn_blink_pair = true;
            bjorn_blink_at += BJORN_BLINK_GAP;
        } else {
            // A fresh word for the interval, or "long gap" and "was a double" end up
            // perfectly correlated and the pattern becomes legible within a minute.
            bjorn_schedule_blink();
        }
    } else {
        // The schedule went by while he was asleep, waking or stale. Start again from
        // now rather than waiting out the counter.
        bjorn_schedule_blink();
    }
    return 0;
}

// Lids where the shipped blink puts them. Measured: in every one of these drawings the
// eyes sit in rows 31-36, left around x21-27 and right around x37-43, which is where
// the hand-drawn lids already are — the framing is constant across the corpus, so the
// same bars land on the same eyes.
//
// Unlike the breath, this is drawn rather than taken from the source: there is no
// eyes-shut version of these frames upstream. It is the one gesture here that is
// authored, and it is authored to match the blink that already ships.
static void bjorn_pose_lids(uint64_t *row, uint8_t state) {
    static const uint8_t shut_x[2] = {21, 37}, shut_w = 7;
    static const uint8_t half_x[2] = {22, 38}, half_w = 5;
    const uint8_t *bx = (state == 1) ? shut_x : half_x;
    const uint8_t  bw = (state == 1) ? shut_w : half_w;
    const uint8_t  y0 = (state == 1) ? 34 : 33;

    for (uint8_t e = 0; e < 2; e++) {
        const uint64_t clear = (((uint64_t)1 << 7) - 1) << shut_x[e];
        for (uint8_t y = 31; y <= 35; y++) row[y] &= ~clear;
        const uint64_t bar = (((uint64_t)1 << bw) - 1) << bx[e];
        row[y0] |= bar;
        row[y0 + 1] |= bar;
    }
}

static void bjorn_blit_pose(uint8_t which, bool breathing, uint8_t lids) {
    const uint8_t *src  = u_poses[which];
    const uint8_t  seam = pgm_read_byte(&u_pose_seam[which]);
    const uint8_t  gx0  = pgm_read_byte(&u_pose_still_x0[which]);
    const uint8_t  gx1  = pgm_read_byte(&u_pose_still_x1[which]);
    const uint8_t  gy0  = pgm_read_byte(&u_pose_still_y0[which]);

    // By row rather than by column, because the swell is a horizontal move. 64 rows of
    // 64 bits is half a kilobyte of stack, built and written once a frame.
    uint64_t row[64];
    for (uint8_t y = 0; y < 64; y++) {
        uint64_t r = 0;
        for (uint8_t x = 0; x < 64; x++) {
            if ((pgm_read_byte(&src[(y / 8) * 64 + x]) >> (y % 8)) & 1u) {
                r |= (uint64_t)1 << x;
            }
        }
        row[y] = r;
    }

    if (breathing) {
        const uint64_t goat =
            (gx0 == 255) ? 0
                         : ((gx1 - gx0 >= 63) ? ~(uint64_t)0
                                              : ((((uint64_t)1 << (gx1 - gx0 + 1)) - 1) << gx0));

        for (uint8_t y = 0; y + 1u < seam; y++) {
            const uint64_t keep = (gy0 != 255 && y >= gy0) ? goat : 0;
            row[y] = (row[y + 1] & ~keep) | (row[y] & keep);
        }

        for (uint8_t y = seam; y <= BJORN_POSE_SWELL_END; y++) {
            uint64_t him = row[y];
            if (gy0 != 255 && y >= gy0) him &= ~goat;     // his contour, not the goat's
            if (!him) continue;

            uint8_t lo = 0, hi = 63;
            while (!((him >> lo) & 1u)) lo++;
            while (!((him >> hi) & 1u)) hi--;

            uint64_t add = 0;
            if (lo > 0) add |= (uint64_t)1 << (lo - 1);
            if (hi < 63) add |= (uint64_t)1 << (hi + 1);
            if (gy0 != 255 && y >= gy0) add &= ~goat;     // never swell into him
            row[y] |= add;
        }
    }

    if (lids) bjorn_pose_lids(row, lids);

    for (uint8_t page = 0; page < 8; page++) {
        const uint8_t m = bjorn_mask((uint8_t)(page * 2));
        for (uint8_t x = 0; x < 64; x++) {
            uint8_t byte = 0;
            for (uint8_t k = 0; k < 8; k++) {
                if ((row[page * 8 + k] >> x) & 1u) byte |= 1u << k;
            }
            u_canvas[page * 128 + x] = (uint8_t)(byte ^ m);
        }
    }
}

static void bjorn_blit(const bjorn_frame_t *f) {
    for (uint8_t p = 0; p < f->count; p++) {
        // block * 64 IS the absolute buffer index, and the & 0x0F keeps us inside the
        // canvas no matter what the art says.
        const uint16_t base = (uint16_t)(f->patches[p].block & 0x0F) * BJORN_PATCH_BYTES;
        const char    *src  = f->patches[p].data;
        const uint8_t  m    = bjorn_mask(f->patches[p].block);
        for (uint8_t i = 0; i < BJORN_PATCH_BYTES; i++) {
            u_canvas[base + i] = (uint8_t)(pgm_read_byte(src + i) ^ m);
        }
    }
}

static void bjorn_blit_base(void) {
    for (uint8_t page = 0; page < 8; page++) {
        const uint8_t m = bjorn_mask((uint8_t)(page * 2));
        for (uint8_t x = 0; x < 64; x++) {
            u_canvas[page * 128 + x] = (uint8_t)(pgm_read_byte(bjorn_base + page * 64 + x) ^ m);
        }
    }
}

static void bjorn_compose(void) {
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

    // The tick the waking gesture ends on, so the reschedule happens exactly once.
    const bool woke_from_lids =
        bjorn_state == 0 && (uint16_t)(bjorn_ticks - bjorn_wake_at) == BJORN_WAKE_OPEN;

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

    // A pose runs only in deep idle. Typing ends it at once — the cut on a key event is
    // the shipped wake behaviour, and it is also what keeps an authored prop from being
    // read as a signal: a sword and the axe are twenty seconds apart by construction,
    // and this closes the one instant where they could overlap.
    if (in.idle_ms < BJORN_VIG_START_MS || bjorn_state != 0) {
        bjorn_vig = U_POSE_COUNT;
    } else if (bjorn_vig == U_POSE_COUNT ||
               (uint16_t)(bjorn_ticks - bjorn_vig_at) >= BJORN_VIG_HOLD) {
        // A cursor, not a shuffle. Random selection repeats a drawing back to back and
        // takes a dozen idle periods to work through the set; a cursor guarantees the
        // whole cast is seen and lets the order mean something later.
        bjorn_vig      = bjorn_vig_next;
        bjorn_vig_next = (uint8_t)((bjorn_vig_next + 1u) % U_POSE_COUNT);
        bjorn_vig_at   = bjorn_ticks;
    }

    // He blinks again after anywhere he has been where the blink code below does not
    // run. Without this he comes back from a doze and never blinks — and a figure that
    // moves but never blinks is this design's signal that the link is dead, so it would
    // report a fault that is not there.
    if (bjorn_state != 0 || woke_from_lids) bjorn_schedule_blink();

    const bool hard = u_sync_hard_stale();
    if (bjorn_was_hard_stale && !hard) {
        bjorn_schedule_blink();
        // Adopt the misfire count without animating. A misfire that happened while the
        // link was dead is over by the time the link returns, and the axe code's own
        // rule is that a stale indicator must not replay: raising it now would report a
        // second event that never happened. This runs before the comparison below, so
        // the first fresh state only ever starts an animation for a misfire that
        // happened while we were watching.
        bjorn_last_anomaly = u_sync_state().anomaly;
    }
    bjorn_was_hard_stale = hard;
    if (bjorn_state == 1) breath = 0;         // hold him still while his eyes close

    // --- draw ---------------------------------------------------------------
    // He puts the crown on, and the world flips 100 ms later — one beat of sequencing
    // that makes the loudest event in the design feel authored rather than like a glitch.
    // Leaving is both at once: slow in, quick out, as the blink and the axe are.
    const bool caps = (in.flags & U_FLAG_CAPS_WORD) && bjorn_state != 2;
    if (caps && !bjorn_caps_was) bjorn_caps_at = bjorn_ticks;
    bjorn_caps_was = caps;
    bjorn_invert   = caps && (uint16_t)(bjorn_ticks - bjorn_caps_at) >= BJORN_CAPS_FLIP;

    if (bjorn_vig != U_POSE_COUNT) {
        // One authored drawing, whole. It breathes by lifting; it does not blink,
        // because the blink patches belong to the four tier drawings.
        // A drawing with a goat in it does not breathe. Roel's call, after watching
        // both on the board: holding the goat out of the gesture is not enough to stop
        // the result looking wrong on those two, so they are shown still.
        //
        // The cost is real and is recorded in docs/layout.md: these poses do not blink
        // either, and a figure where nothing moves is this design's way of saying the
        // link is dead. Two of six, for eighteen seconds each, against a goat that
        // looks right.
        // A drawing with a goat in it does not breathe — holding the goat out of the
        // gesture was not enough to make it look right on those two, so they are still.
        // They blink instead, which is where their liveness comes from: a figure that
        // neither blinks nor breathes is this design's way of saying the link is dead.
        const bool has_goat = pgm_read_byte(&u_pose_still_x0[bjorn_vig]) != 255;
        bjorn_blit_pose(bjorn_vig, !has_goat && breath >= 1 && breath <= 2,
                        bjorn_blink_state());
        bjorn_blit(&bjorn_frames[BJORN_F_FLOOR_RIGHT]);
        return;
    }

    bjorn_blit_base();
    const uint8_t draw_tier = bjorn_mf_live ? 0 : tier;   // drooped for the misfire
    bjorn_blit(&bjorn_frames[bjorn_state == 2 ? BJORN_F_DOZE : pose_of_tier[draw_tier]]);
    bjorn_blit(&bjorn_frames[BJORN_F_FLOOR_RIGHT]);

    if (bjorn_state == 2) {
        const uint16_t phase = (uint16_t)(bjorn_ticks - bjorn_doze_origin) % BJORN_DOZE_PERIOD;
        if (phase < BJORN_DOZE_INHALE) bjorn_blit(&bjorn_frames[BJORN_F_DOZE_BREATH]);
        return;                               // asleep: eyes already shut
    }

    // A misfire outranks the axe and cancels it rather than suspending it: resuming a
    // stale success indicator afterwards would read as a second event that never happened.
    if (in.anomaly != bjorn_last_anomaly) {
        bjorn_last_anomaly = in.anomaly;
        bjorn_mf_at        = bjorn_ticks;
        bjorn_mf_live      = true;
        bjorn_axe_live     = false;
    }
    const uint16_t mf = bjorn_ticks - bjorn_mf_at;
    if (bjorn_mf_live && mf >= BJORN_MF_END) bjorn_mf_live = false;

    // The axe while a command modifier is held, and for 800 ms after it is released.
    // Holding is the event worth marking: it is what he actually does, where a completed
    // shortcut mostly is not — Chordal Hold turns his same-hand ones into letters, and
    // Miryoku's clipboard keys are not modifiers at all. The trailing dwell is what stops
    // a 100-300 ms hold strobing the prop on and off.
    //
    // Shift is excluded: here it is a home-row mod, part of typing a capital rather than
    // an instruction, and it is held so often that the axe would simply always be there.
    static const uint8_t axe_of_tier[4] = {BJORN_F_AXE_T0, BJORN_F_AXE_T1,
                                           BJORN_F_AXE_T2, BJORN_F_AXE_T3};
    if (in.fingers & (uint8_t)~(U_FIN_L_INDEX | U_FIN_R_INDEX)) {
        bjorn_axe_at   = bjorn_ticks;         // refreshed while held
        bjorn_axe_live = true;
    }
    if (bjorn_axe_live && (uint16_t)(bjorn_ticks - bjorn_axe_at) >= BJORN_AXE_TICKS) {
        bjorn_axe_live = false;
    }

    if (caps) {
        static const uint8_t crown_of_tier[4] = {BJORN_F_CROWN_T0, BJORN_F_CROWN_T1,
                                                 BJORN_F_CROWN_T2, BJORN_F_CROWN_T3};
        bjorn_blit(&bjorn_frames[crown_of_tier[draw_tier]]);
    }

    if (bjorn_mf_live) {
        // His horns droop for the duration. The whole figure cuts to the drooped posture
        // rather than splicing horns into the current one: the tier progression is an
        // arousal scale, so that pose already exists, and half a droop reads as damage
        // where both falling reads as deflation. Nothing was typed, so the typing-speed
        // posture is carrying no information to preserve.
        // Up, held, and back down. Cutting from the raised pose straight to the bare
        // posture reads as the axe vanishing rather than as a gesture finishing.
        bjorn_blit(&bjorn_frames[mf < BJORN_MF_GRIP ? BJORN_F_AXE_T0
                                 : mf < BJORN_MF_R1 ? BJORN_F_MISFIRE_R1
                                 : mf < BJORN_MF_R2 ? BJORN_F_MISFIRE_R2
                                 : mf < BJORN_MF_TOP ? BJORN_F_MISFIRE_R3
                                 : mf < BJORN_MF_D2  ? BJORN_F_MISFIRE_R2
                                 : mf < BJORN_MF_D1  ? BJORN_F_MISFIRE_R1
                                                     : BJORN_F_AXE_T0]);
    } else if (bjorn_axe_live && !settled) {
        bjorn_blit(&bjorn_frames[axe_of_tier[tier]]);
    }

    if (breath == 1) bjorn_blit(&bjorn_frames[BJORN_F_BREATH_SWELL]);
    if (breath == 2) bjorn_blit(&bjorn_frames[BJORN_F_BREATH]);
    if (breath == 3) bjorn_blit(&bjorn_frames[BJORN_F_BREATH_SETTLE]);

    // Which eye frame, if any. Picked from this tick's breath state, not from the one
    // the gesture started in: a blink is five ticks long and can straddle a change, and
    // an unlifted patch over a risen body drags his horn tip and beard back down.
    const uint8_t shut = (breath >= 2) ? BJORN_F_BLINK_LIFT : blink_of_tier[draw_tier];
    const uint8_t half = (breath >= 2) ? BJORN_F_HALF_LIFT  : half_of_tier[draw_tier];

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

    // Frozen mid-pose with no blink is the fault vocabulary: the absence of a blink is
    // the error message. Nothing else changes, so a dead link is unmistakable but quiet.
    if (u_sync_hard_stale()) return;

    const uint8_t blink = bjorn_blink_state();
    if (blink == 1) {
        bjorn_blit(&bjorn_frames[shut]);
    } else if (blink == 2) {
        bjorn_blit(&bjorn_frames[half]);
    }
}

void oled_render_slave(void) {
    // Compose first, commit once. Every early return in bjorn_compose is a frame that
    // still has to reach the panel, which is why the commit lives out here.
    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    bjorn_compose();
    u_canvas_commit();
}

#endif     // OLED_SLAVE_ANIMATION_BJORN
