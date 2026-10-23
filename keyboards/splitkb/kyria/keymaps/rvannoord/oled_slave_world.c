// The world layer. Weather across the whole 128x64 slave panel, drawn behind
// Bjorn and behind the sky objects, so he stands in it rather than beside it.
//
// See docs/layout.md, "The world layer". Three things here are not obvious:
//
// 1. **It is drawn after Bjorn and cut around him**, not before. Drawing it
//    first would be simpler but `bjorn_blit_base()` assigns his whole column
//    rather than OR-ing it, so it would erase the field wherever he stands.
//
// 2. **His outline is computed from the canvas, not stored.** A morphological
//    close of whatever he happens to be, which costs nothing to keep correct:
//    it tracks breath, blink, doze, the axe, the crown, the misfire raise and
//    all six vignette poses automatically, and will track whatever is added
//    next. A table of masks would have to be regenerated every time he changes.
//
// 3. **Close, not a column fill.** Filling each column from its topmost to its
//    bottommost lit pixel is far cheaper and was tried first. It roofs over the
//    open sky under his horns, where the horn curves up and away from his head
//    - about two to four pixels a row, which measured as negligible and is
//    immediately visible, because a hole turning solid is exactly what the eye
//    catches. A per-column "fill gaps shorter than N" does not work either: his
//    own dither holes run up to 9 px, so any threshold that fills them also
//    roofs the horn, and any threshold that spares the horn punches weather
//    through his beard.

#include QMK_KEYBOARD_H
#include "oled.h"
#include "oled_canvas.h"
#include "oled_slave_world.h"
#include "oled_slave_goat.h"
#include "art/world_art.h"

#ifdef OLED_SLAVE_WORLD

#    define W 128
#    define H 64
#    define BJORN_W 64          // his half, and exactly the width of a uint64_t

// --- the season -------------------------------------------------------------
// From __DATE__, which is the *build* date: it drifts, and a firmware left
// unflashed for months will show the wrong season. That is accepted and is the
// reason the host daemon exists - but the panel must look right with no daemon
// at all, so this is the floor rather than a placeholder.

static uint8_t build_month(void) {
    // __DATE__ is "Mmm dd yyyy". Comparing three characters is smaller than any
    // table and runs once.
    const char *d = __DATE__;
    switch (d[0]) {
        case 'J': return d[1] == 'a' ? 1 : (d[2] == 'n' ? 6 : 7);   // Jan Jun Jul
        case 'F': return 2;
        case 'M': return d[2] == 'r' ? 3 : 5;                        // Mar May
        case 'A': return d[1] == 'p' ? 4 : 8;                        // Apr Aug
        case 'S': return 9;
        case 'O': return 10;
        case 'N': return 11;
        default:  return 12;
    }
}

static enum u_world_state season_state(void) {
    switch (build_month()) {
        case 12: case 1: case 2:  return U_WORLD_SNOW;
        case 9:  case 10: case 11: return U_WORLD_LEAVES;
        default:                   return U_WORLD_RAIN;
    }
}

static enum u_world_state world_state = U_WORLD_NONE;

enum u_world_state u_world_current(void) {
    return world_state;
}

// --- the fields -------------------------------------------------------------

static uint16_t scroll;         // rows the tile has fallen
static uint32_t last_step;

#    define U_WORLD_STEP_MS 100     // one scroll step; 2 rows at 10 Hz
#    define U_WORLD_FALL     2

static void put(uint8_t x, uint8_t y) {
    u_canvas[(uint16_t)(y >> 3) * W + x] |= (uint8_t)(1u << (y & 7));
}

static inline bool art_bit(const uint8_t *art, uint8_t stride, uint16_t row, uint8_t x) {
    return (pgm_read_byte(&art[(uint16_t)row * stride + (x >> 3)]) >> (7 - (x & 7))) & 1u;
}

// --- Bjorn's outline --------------------------------------------------------
// A row of his half is exactly 64 px, so it is one uint64_t and the whole
// morphology is shifts and ORs: a 7x7 close is four separable passes over 64
// words, a couple of thousand operations. Cheap enough to do every frame, so
// there is no cache to invalidate and no way for it to go stale.

#    define R 3      // close radius; 3 spans his widest dither hole

static void dilate(uint64_t *rows) {
    uint64_t tmp[H];
    for (uint8_t y = 0; y < H; y++) {
        uint64_t v = rows[y];
        for (uint8_t k = 1; k <= R; k++) v |= (rows[y] << k) | (rows[y] >> k);
        tmp[y] = v;
    }
    for (uint8_t y = 0; y < H; y++) {
        uint64_t v = tmp[y];
        for (uint8_t k = 1; k <= R; k++) {
            if (y >= k)     v |= tmp[y - k];
            if (y + k < H)  v |= tmp[y + k];
        }
        rows[y] = v;
    }
}

static void bjorn_hull(uint64_t *hull) {
    for (uint8_t y = 0; y < H; y++) {
        uint64_t v = 0;
        for (uint8_t x = 0; x < BJORN_W; x++) {
            if ((u_canvas[(uint16_t)(y >> 3) * W + x] >> (y & 7)) & 1u) v |= (uint64_t)1 << x;
        }
        hull[y] = v;
    }
    // Close = dilate then erode. Erode is the dilate of the complement,
    // complemented - so the same routine runs twice and nothing else is needed.
    dilate(hull);
    for (uint8_t y = 0; y < H; y++) hull[y] = ~hull[y];
    dilate(hull);
    for (uint8_t y = 0; y < H; y++) hull[y] = ~hull[y];
}

// --- leaves -----------------------------------------------------------------
// Particles rather than a scrolling tile, because leaves are sparse and
// individuated: you watch one. Rain is the opposite and is a tile.
//
// Everything about a leaf is its own. The first version gave each one a phase of
// `i * TICKS / N` while their x ran left to right in index order, so phase and x
// were correlated by construction and six leaves necessarily drew a diagonal
// across the panel. They now carry independent phase, fall speed, sway and turn
// rate, drawn from a small generator rather than from the index.
//
// The periods are deliberately *not* chosen to divide anything. The design study
// had to make every period divide its 504-tick preview loop or the animation
// would not close; that is a property of a GIF and not of a keyboard, and
// inheriting it here would quietly re-impose the regularity we just removed.

// Nine, which is the ceiling: above that a 13 px leaf stops reading as weather
// and starts reading as confetti. Six was the density of the design study's
// 63-wide field preserved at double width; on the board it looked sparse,
// because a third of them are behind him at any moment.
#    define U_LEAVES 9

// 1/256 px per step, and a step is U_WORLD_STEP_MS. The spread matters more than
// the floor: leaves at the same speed read as a curtain however fast it falls.
#    define U_LEAF_FALL_MIN 32
#    define U_LEAF_FALL_SPAN 48

typedef struct {
    int16_t  x16, y16;      // 1/256 px
    uint16_t fall;          // 1/256 px per tick
    uint16_t sway_len;
    int16_t  sway_amp16;
    uint16_t sway_at;
    uint16_t turn;          // ticks per tumble frame
    uint16_t turn_at;
    uint8_t  frame;
} leaf_t;

static leaf_t   leaves[U_LEAVES];
static uint32_t leaf_rng = 0x9e3779b9u;
static bool     leaves_ready;

static uint16_t rnd(uint16_t span) {
    leaf_rng = leaf_rng * 1664525u + 1013904223u;
    return span ? (uint16_t)((leaf_rng >> 16) % span) : 0;
}

static void leaf_spawn(leaf_t *l, bool anywhere) {
    l->x16       = (int16_t)(rnd(W - U_LEAF_W) << 8);
    // Negate after the shift, never before: shifting a negative signed value is
    // undefined behaviour, and it is the kind that works until a compiler flag
    // changes.
    l->y16       = anywhere ? (int16_t)(rnd(H) << 8) : (int16_t)(-(int32_t)(U_LEAF_H << 8));
    l->fall      = (uint16_t)(U_LEAF_FALL_MIN + rnd(U_LEAF_FALL_SPAN));
    l->sway_len  = (uint16_t)(70 + rnd(90));
    l->sway_amp16 = (int16_t)((2 + rnd(4)) << 8);
    l->sway_at   = rnd(l->sway_len);
    l->turn      = (uint16_t)(14 + rnd(26));
    l->turn_at   = rnd(l->turn);
    l->frame     = (uint8_t)rnd(U_LEAF_FRAMES);
}

static void leaves_step(void) {
    if (!leaves_ready) {
        for (uint8_t i = 0; i < U_LEAVES; i++) leaf_spawn(&leaves[i], true);
        leaves_ready = true;
    }
    for (uint8_t i = 0; i < U_LEAVES; i++) {
        leaf_t *l = &leaves[i];
        l->y16 = (int16_t)(l->y16 + l->fall);
        if (++l->sway_at >= l->sway_len) l->sway_at = 0;
        if (++l->turn_at >= l->turn) {
            l->turn_at = 0;
            l->frame   = (uint8_t)((l->frame + 1) % U_LEAF_FRAMES);
        }
        if ((l->y16 >> 8) >= (int16_t)H) leaf_spawn(l, false);
    }
}

// A triangle rather than a sine: no table, no multiply, and at this amplitude
// the difference is under a pixel.
static int16_t sway_of(const leaf_t *l) {
    const uint16_t half = l->sway_len / 2;
    const int32_t  t    = l->sway_at < half ? l->sway_at : (l->sway_len - l->sway_at);
    return (int16_t)((t * 2 * l->sway_amp16) / (half ? half : 1) - l->sway_amp16);
}

// Behind Bjorn, in his column, or behind the goat, in its.
static bool blocked(const uint64_t *hull, const uint64_t *goat, int16_t x, int16_t y) {
    return x < BJORN_W ? (hull[y] >> x) & 1u : (goat[y] >> (x - BJORN_W)) & 1u;
}

static void leaves_draw(const uint64_t *hull, const uint64_t *goat) {
    for (uint8_t i = 0; i < U_LEAVES; i++) {
        const leaf_t *l  = &leaves[i];
        const int16_t ox = (int16_t)((l->x16 + sway_of(l)) >> 8);
        const int16_t oy = (int16_t)(l->y16 >> 8);
        for (uint8_t ly = 0; ly < U_LEAF_H; ly++) {
            const int16_t y = (int16_t)(oy + ly);
            if (y < 0 || y >= (int16_t)H) continue;
            for (uint8_t lx = 0; lx < U_LEAF_W; lx++) {
                const int16_t x = (int16_t)(ox + lx);
                if (x < 0 || x >= (int16_t)W) continue;
                if (blocked(hull, goat, x, y)) continue;
                if (art_bit(u_leaf[l->frame], U_LEAF_STRIDE, ly, lx)) put((uint8_t)x, (uint8_t)y);
            }
        }
    }
}

// --- drawing ----------------------------------------------------------------

void u_world_draw(void) {
    if (world_state == U_WORLD_NONE) return;

    uint64_t hull[H], goat[H];
    bjorn_hull(hull);
    u_goat_silhouette(goat);

    const uint8_t *art;
    uint16_t       tile_h;
    bool           scrolls;
    switch (world_state) {
        case U_WORLD_RAIN:  art = u_rain;  tile_h = U_RAIN_H;  scrolls = true;  break;
        case U_WORLD_SNOW:  art = u_snow;  tile_h = U_SNOW_H;  scrolls = true;  break;
        case U_WORLD_STARS: art = u_stars; tile_h = U_STARS_H; scrolls = false; break;
        case U_WORLD_LEAVES: leaves_draw(hull, goat); return;
        default: return;
    }

    for (uint8_t y = 0; y < H; y++) {
        const uint16_t row = scrolls ? (uint16_t)((y + scroll) % tile_h) : y;
        for (uint8_t x = 0; x < W; x++) {
            if (blocked(hull, goat, x, y)) continue;
            if (art_bit(art, U_WORLD_ROW_BYTES, row, x)) put(x, y);
        }
    }
}

void u_world_task(void) {
    if (world_state == U_WORLD_NONE) world_state = season_state();

    const uint32_t now = timer_read32();
    if (timer_elapsed32(last_step) >= U_WORLD_STEP_MS) {
        last_step = now;
        scroll    = (uint16_t)(scroll + U_WORLD_FALL);
        if (world_state == U_WORLD_LEAVES) leaves_step();
    }
}

#endif     // OLED_SLAVE_WORLD
