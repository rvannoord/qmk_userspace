// The goat behind oled_slave_goat.h. A step is one render, 50 ms; every hold is in steps.
//
// It is only ever in one activity, and every activity ends on a frame boundary, so a key
// or a dead link never leaves it mid-stride: the frame it is in plays out, then it stands.

#include QMK_KEYBOARD_H
#include <string.h>
#include "oled_canvas.h"
#include "oled_slave_goat.h"
#include "oled_slave_director.h"
#include "art/goat_art.h"

#ifdef OLED_ENABLE

#    define U_GOAT_FEET   62        // the floor rule is y=63
#    define U_GOAT_X_MIN  66        // a little room from Bjorn's column
#    define U_GOAT_X_MAX  (127 - U_GOAT_W)   // fully in, with x=127 dark
#    define U_GOAT_X_OUT  127       // past the right edge: nothing of it drawn
#    define U_GOAT_STRIDE 2         // px a walk frame
#    define U_GOAT_WALK   4         // steps a walk frame: 200 ms, an amble
#    define U_GOAT_EAR    86        // the ear's clock, 4.3 s, twitched for 150 ms
#    define U_GOAT_FLICK  3

enum { G_ABSENT, G_WALK, G_STAND, G_GRAZE, G_LOOK, G_HOP, G_TURN, G_LIE, G_CURL, G_RISE };

// A beat: a frame, how many steps it holds, how high it is lifted, and how far it carries
// the goat forward. The animators' timings, rounded to the panel's 50 ms steps.
typedef struct {
    uint8_t frame, steps, lift, ahead;
} u_beat_t;

// Two quick nibbles and a long chew: the uneven rhythm is what reads as alive.
static const u_beat_t graze[] = {
    {U_GOAT_GRAZE_A, 12, 0, 0}, {U_GOAT_GRAZE_B, 4, 0, 0}, {U_GOAT_GRAZE_A, 6, 0, 0},
    {U_GOAT_GRAZE_B, 5, 0, 0},  {U_GOAT_GRAZE_A, 18, 0, 0},
};
// Anticipation; the passing frame as the push-off; up 3, 5 and 4 px carrying 2 px each; the
// passing frame again as the reach; the squash; the recovery.
static const u_beat_t hop[] = {
    {U_GOAT_HOP_CROUCH, 5, 0, 0}, {U_GOAT_WALK_B, 1, 0, 0},   {U_GOAT_HOP_AIR, 1, 3, 2},
    {U_GOAT_HOP_AIR, 2, 5, 2},    {U_GOAT_HOP_AIR, 1, 4, 2},  {U_GOAT_WALK_B, 1, 1, 0},
    {U_GOAT_HOP_LAND, 3, 0, 0},   {U_GOAT_HOP_CROUCH, 2, 0, 0},
};

// The tail: an uneven 3.75 s, a single flick, a long gap, then a double. In steps, from the
// start of the pattern: when each flick is up.
static bool tail_up(uint16_t t) {
    const uint16_t p = (uint16_t)(t % 75);
    return (p >= 28 && p < 31) || (p >= 68 && p < 71) || p >= 73;
}

// Asleep: three breaths in turn, 2.8, 3.05 and 3.45 s, never Bjorn's 3.6 s. Each a short rise
// and a long settle: steps settled, then steps risen.
static const uint8_t sleep_breaths[3][2] = {{39, 17}, {43, 18}, {50, 19}};

static bool breathing_in(uint16_t t) {
    uint16_t p = (uint16_t)(t % (39 + 17 + 43 + 18 + 50 + 19));
    for (uint8_t i = 0; i < 3; i++) {
        if (p < sleep_breaths[i][0]) return false;
        p = (uint16_t)(p - sleep_breaths[i][0]);
        if (p < sleep_breaths[i][1]) return true;
        p = (uint16_t)(p - sleep_breaths[i][1]);
    }
    return false;
}
static const u_beat_t kneel[] = {{U_GOAT_HOP_CROUCH, 6, 0, 0}};   // lying down, getting up
static const u_beat_t turn[]  = {{U_GOAT_TURN, 3, 0, 0}};         // a front view to pivot on

static struct {
    uint8_t         mode;
    int16_t         x;          // the frame's left edge on the panel
    int8_t          dir;        // -1 facing left, toward Bjorn, as drawn; +1 mirrored
    int16_t         target;     // where a walk ends; U_GOAT_X_OUT to leave
    uint8_t         frame;
    uint8_t         lift;       // px off the floor, in a hop
    const u_beat_t *beats;      // the sequence the goat is in, and where
    uint8_t         n, beat;
    uint8_t         step;       // which foot, in a walk
    uint16_t        t;          // its own steps
    uint16_t        frame_end;  // the step the current frame ends on
    uint16_t        mode_end;   // and the activity
    bool            hopped;     // once an idle period
    bool            ambient;    // tail and ear may move: settled, and the link alive
    uint32_t        seen_at;    // the last step, on the keyboard's clock
    uint32_t        idle_was;   // and the idle time then
} g;

#    define U_GOAT_SLEPT_MS 500     // a longer gap between steps: the panel was off

static bool due(uint16_t at) { return (int16_t)(g.t - at) >= 0; }

static void frame(uint8_t f, uint16_t steps) {
    g.frame     = f;
    g.frame_end = (uint16_t)(g.t + steps);
}

static void play(uint8_t mode, const u_beat_t *beats, uint8_t n) {
    g.mode  = mode;
    g.beats = beats;
    g.n     = n;
    g.beat  = 0;
    g.lift  = beats[0].lift;
    frame(beats[0].frame, beats[0].steps);
}

// The next beat, carrying the goat forward as the beat says. False when the sequence is done.
static bool beat_on(void) {
    if (++g.beat >= g.n) return false;
    const u_beat_t *b = &g.beats[g.beat];
    g.x    = (int16_t)(g.x + g.dir * b->ahead);
    g.lift = b->lift;
    frame(b->frame, b->steps);
    return true;
}

static void stand(uint16_t steps) {
    g.mode     = G_STAND;
    g.lift     = 0;
    g.mode_end = (uint16_t)(g.t + steps);
    frame(U_GOAT_STAND, 1);
}

static void walk_to(int16_t x) {
    const int8_t dir = x < g.x ? -1 : 1;
    g.target = x;
    // Turning round on screen pivots on the front view; coming in from off it needs none.
    if (dir != g.dir && g.x < U_GOAT_X_OUT) {
        play(G_TURN, turn, 1);
        return;
    }
    g.dir  = dir;
    g.mode = G_WALK;
    g.step = 0;
    frame(U_GOAT_WALK_A, U_GOAT_WALK);
}

// What it does next, when it has stood long enough and nothing else is starting.
static void choose(void) {
    const uint16_t r    = u_director_rand(100);
    const bool     room = g.dir < 0 ? g.x - 6 >= U_GOAT_X_MIN : g.x + 6 <= U_GOAT_X_MAX;
    if (r < 40) {
        g.mode_end = (uint16_t)(g.t + 60 + u_director_rand(100));     // 3-8 s
        play(G_GRAZE, graze, sizeof graze / sizeof graze[0]);
    } else if (r < 58 && g.dir < 0) {
        g.mode     = G_LOOK;                                          // up at Bjorn
        g.mode_end = (uint16_t)(g.t + 32 + u_director_rand(16));      // 1.6-2.4 s
        frame(U_GOAT_LOOK_UP, 1);
    } else if (r < 63 && !g.hopped && room) {
        g.hopped = true;
        play(G_HOP, hop, sizeof hop / sizeof hop[0]);
    } else if (r < 90) {
        int16_t x = (int16_t)(U_GOAT_X_MIN + u_director_rand(U_GOAT_X_MAX - U_GOAT_X_MIN + 1));
        if (x > g.x - 6 && x < g.x + 6) x = x > (U_GOAT_X_MIN + U_GOAT_X_MAX) / 2 ? U_GOAT_X_MIN : U_GOAT_X_MAX;
        walk_to(x);
    } else {
        walk_to(U_GOAT_X_OUT);                                        // off it goes
    }
}

// A frame has ended: the next one in the activity, or the end of the activity. `settle`
// is a key or a dead link: whatever it is doing stops at this boundary and it stands. A
// hop and a turn are one movement each and always finish; a hop stopped in the air is the
// one thing worse than a stride stopped mid-way.
static void next_frame(bool settle, bool may_start) {
    switch (g.mode) {
        case G_WALK: {
            if (settle) {
                stand(40);
                return;
            }
            g.x = (int16_t)(g.x + g.dir * U_GOAT_STRIDE);
            if (g.target == U_GOAT_X_OUT && g.x >= U_GOAT_X_OUT) {
                g.mode = G_ABSENT;
                return;
            }
            if (g.target != U_GOAT_X_OUT && (g.dir < 0 ? g.x <= g.target : g.x >= g.target)) {
                stand((uint16_t)(40 + u_director_rand(60)));         // 2-5 s
                return;
            }
            g.step ^= 1u;   // contact, then passing with the bob
            frame(g.step ? U_GOAT_WALK_B : U_GOAT_WALK_A, U_GOAT_WALK);
            return;
        }
        case G_GRAZE:
            if (beat_on()) {
                if (settle) stand(40);
                return;
            }
            if (settle || due(g.mode_end)) {
                stand((uint16_t)(30 + u_director_rand(40)));
                return;
            }
            play(G_GRAZE, graze, sizeof graze / sizeof graze[0]);
            return;
        case G_LOOK:
            if (settle || due(g.mode_end)) stand((uint16_t)(30 + u_director_rand(40)));
            else frame(U_GOAT_LOOK_UP, 1);
            return;
        case G_HOP:
            if (!beat_on()) stand((uint16_t)(30 + u_director_rand(40)));
            return;
        case G_TURN:
            g.dir = (int8_t)-g.dir;
            if (settle) {
                stand(40);
                return;
            }
            walk_to(g.target);
            return;
        case G_LIE:
            if (settle) {
                stand(40);
                return;
            }
            g.mode = G_CURL;
            frame(U_GOAT_CURLED, 1);
            return;
        case G_RISE:
            stand(40);
            return;
        case G_STAND:
            if (!settle && may_start && due(g.mode_end) && u_director_claim()) {
                choose();
                return;
            }
            frame(U_GOAT_STAND, 1);
            return;
        default:
            return;
    }
}

void u_goat_step(uint32_t idle_ms, bool dead, bool goat_pose) {
    // While the panel was off it wandered away: the screen comes back without it, and
    // Bjorn's own goat drawings get their turn again. Off means the show had run to its
    // end before the gap; a map over this half also stops the steps, and that is no reason
    // for it to vanish.
    if (timer_elapsed32(g.seen_at) > U_GOAT_SLEPT_MS && g.idle_was >= U_IDLE_FREEZE_MS) g.mode = G_ABSENT;
    g.seen_at  = timer_read32();
    g.idle_was = idle_ms;

    // Still from here on: the panel goes off on a frame where nothing is changing.
    if (idle_ms >= U_IDLE_FREEZE_MS) return;
    g.t++;

    const bool quiet     = idle_ms < U_IDLE_ACTORS_MS;     // a key, not long ago
    const bool dozing    = idle_ms >= U_IDLE_DOZE_MS;
    const bool may_start = !quiet && !dead && !dozing && idle_ms < U_IDLE_LAST_START_MS;
    if (quiet) g.hopped = false;
    g.ambient = idle_ms >= U_IDLE_SETTLE_MS && !dead;

    if (g.mode == G_ABSENT) {
        // A chance a second, never while Bjorn shows his own goat.
        if (may_start && !goat_pose && g.t % 20 == 0 && u_director_rand(4) == 0 && u_director_claim()) {
            g.x   = U_GOAT_X_OUT;
            g.dir = -1;
            walk_to((int16_t)(U_GOAT_X_MIN + 8 + u_director_rand(U_GOAT_X_MAX - U_GOAT_X_MIN - 7)));
        }
        return;
    }

    if (g.mode == G_CURL) {
        if (!dozing) {                                     // a key: up it gets, by a kneel
            play(G_RISE, kneel, 1);
            return;
        }
        if (dead) return;
        g.frame = breathing_in(g.t) ? U_GOAT_CURLED_BREATH : U_GOAT_CURLED;
        return;
    }

    if (!due(g.frame_end)) return;                         // the frame plays out
    // Down where it stands at the doze, by a kneel; a walk out and a hop finish first.
    const bool leaving = g.mode == G_WALK && g.target == U_GOAT_X_OUT;
    if (dozing && !dead && !leaving && g.mode != G_HOP && g.mode != G_TURN && g.mode != G_LIE && g.mode != G_RISE) {
        g.lift = 0;
        play(G_LIE, kneel, 1);
        return;
    }
    next_frame((quiet || dead) && !(leaving && dozing), may_start);
}

bool u_goat_present(void) {
    return g.mode != G_ABSENT;
}

// --- drawing -------------------------------------------------------------------------

static bool bit(const uint8_t (*art)[U_GOAT_BYTES], uint8_t f, uint8_t x, uint8_t y) {
    return (pgm_read_byte(&art[f][(y >> 3) * U_GOAT_W + x]) >> (y & 7)) & 1u;
}

// The tail and the ear are patches of their own: where a flick frame differs from the frame
// it was drawn on, it wins. Their regions do not overlap, so both combine.
static bool ink(uint8_t x, uint8_t y, bool *masked) {
    bool on = bit(u_goat, g.frame, x, y);
    *masked = bit(u_goat_mask, g.frame, x, y);
    if (!g.ambient) return on;
    // Standing, the tail and the ear; grazing, the tail, from its own frame over either jaw.
    static const struct {
        uint8_t base, patch;
    } patches[3] = {{U_GOAT_STAND, U_GOAT_STAND_TAIL}, {U_GOAT_STAND, U_GOAT_STAND_EAR},
                    {U_GOAT_GRAZE_A, U_GOAT_GRAZE_TAIL}};
    const bool grazing = g.frame == U_GOAT_GRAZE_A || g.frame == U_GOAT_GRAZE_B;
    const bool tail = tail_up(g.t), ear = (uint16_t)(g.t % U_GOAT_EAR) < U_GOAT_FLICK;
    for (uint8_t i = 0; i < 3; i++) {
        if (patches[i].base == U_GOAT_STAND ? g.frame != U_GOAT_STAND : !grazing) continue;
        if (!(patches[i].patch == U_GOAT_STAND_EAR ? ear : tail)) continue;
        if (bit(u_goat, patches[i].patch, x, y) == bit(u_goat, patches[i].base, x, y)) continue;
        on      = bit(u_goat, patches[i].patch, x, y);
        *masked = *masked || bit(u_goat_mask, patches[i].patch, x, y);
    }
    return on;
}

// Each pixel of the goat as it stands now: f(x, y on the panel, ink, inside its silhouette).
static void each(void (*f)(int16_t, int16_t, bool, bool, void *), void *ctx) {
    if (g.mode == G_ABSENT) return;
    const int16_t top = (int16_t)(U_GOAT_FEET - (U_GOAT_H - 1) - g.lift);
    for (uint8_t y = 0; y < U_GOAT_H; y++) {
        for (uint8_t x = 0; x < U_GOAT_W; x++) {
            const int16_t px = (int16_t)(g.x + x), py = (int16_t)(top + y);
            if (px < 64 || px > 126 || py < 1 || py > U_GOAT_FEET) continue;   // x=127, y=0 dark
            bool          masked;
            const bool    on = ink(g.dir < 0 ? x : (uint8_t)(U_GOAT_W - 1 - x), y, &masked);
            f(px, py, on, masked, ctx);
        }
    }
}

static void put(int16_t x, int16_t y, bool on, bool masked, void *ctx) {
    (void)masked;
    (void)ctx;
    if (on) u_canvas[(uint16_t)(y >> 3) * OLED_DISPLAY_WIDTH + (uint16_t)x] |= (uint8_t)(1u << (y & 7));
}

void u_goat_draw(void) {
    each(put, NULL);
}

static void hull(int16_t x, int16_t y, bool on, bool masked, void *ctx) {
    if (on || masked) ((uint64_t *)ctx)[y] |= (uint64_t)1 << (x - 64);
}

void u_goat_silhouette(uint64_t rows[64]) {
    memset(rows, 0, 64 * sizeof rows[0]);
    each(hull, rows);
}

#endif     // OLED_ENABLE
