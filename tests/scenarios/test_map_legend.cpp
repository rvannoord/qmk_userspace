// Every key the live layer map draws has a legend, on both hosts. The scenario build's
// keymaps[] is the test framework's own, so the layers come from Miryoku's macros,
// expanded here the way the firmware expands them: through miryoku_layer_selection.h,
// with custom_config.h's shims and overrides, and RGB Matrix on, as the build has it.

#include "scenario.hpp"

#define RGB_MATRIX_ENABLE
#include "custom_config.h"

extern "C" {
#include "manna-harbour_miryoku.h"
#include "map_legend.h"
#include "art/drashna_icons.h"
#include "oled_layer_page.h"
#include "oled_canvas.h"
}
#include <cstdio>
#include <cstdlib>

namespace {

// As manna-harbour_miryoku.c declares it, which a header does not export. The tap dances
// sit on the half holding the layer: the live map leaves that half out, the Layer map page
// draws them as the layer's letter.
enum {
    U_TD_BOOT,
#define MIRYOKU_X(LAYER, STRING) U_TD_U_##LAYER,
    MIRYOKU_LAYER_LIST
#undef MIRYOKU_X
};

// Miryoku's 40 slots: three rows of ten, then the thumb row. Each row runs left pinky to
// left inner, then right inner to right pinky; the thumbs used are slots 32-34 on the
// left and 35-37 on the right.
const uint16_t base[]   = {MIRYOKU_LAYER_BASE};
const uint16_t extra[]  = {MIRYOKU_LAYER_EXTRA};
const uint16_t tap[]    = {MIRYOKU_LAYER_TAP};
const uint16_t nav[]    = {MIRYOKU_LAYER_NAV};
const uint16_t mouse[]  = {MIRYOKU_LAYER_MOUSE};
const uint16_t media[]  = {MIRYOKU_LAYER_MEDIA};
const uint16_t sym[]    = {MIRYOKU_LAYER_SYM};
const uint16_t num[]    = {MIRYOKU_LAYER_NUM};
const uint16_t fun[]    = {MIRYOKU_LAYER_FUN};
const uint16_t button[] = {MIRYOKU_LAYER_BUTTON};

struct DrawnLayer {
    const char     *name;
    const uint16_t *keys;
    bool            left, right;   // which halves the map draws (docs/oled-v3.md, stage 4)
};

const DrawnLayer layers[] = {
    {"NAV", nav, false, true},  {"MOUSE", mouse, false, true}, {"MEDIA", media, false, true},
    {"SYM", sym, true, false},  {"NUM", num, true, false},     {"FUN", fun, true, false},
    {"BUTTON", button, true, true},
};

// The Layer map page draws every layer, both halves: a cell for each of the 30 alpha keys,
// the full legend for the six thumbs.
const struct {
    const char     *name;
    const uint16_t *keys;
} every_layer[] = {
    {"BASE", base}, {"EXTRA", extra}, {"TAP", tap}, {"BUTTON", button}, {"NAV", nav},
    {"MOUSE", mouse}, {"MEDIA", media}, {"NUM", num}, {"SYM", sym}, {"FUN", fun},
};

bool same(u_legend_t a, u_legend_t b) {
    return memcmp(&a, &b, sizeof a) == 0;
}

u_legend_t cells(uint8_t a, uint8_t b = 0, uint8_t c = 0, uint8_t flags = 0) {
    return u_legend_t{{a, b, c}, flags};
}

}   // namespace

TEST(MapLegend, EveryDrawnKeyHasOne) {
    static const uint8_t left_slots[]  = {0, 1, 2, 3, 4, 10, 11, 12, 13, 14, 20, 21, 22, 23, 24, 32, 33, 34};
    static const uint8_t right_slots[] = {5, 6, 7, 8, 9, 15, 16, 17, 18, 19, 25, 26, 27, 28, 29, 35, 36, 37};
    for (const auto &l : layers) {
        for (int half = 0; half < 2; half++) {
            if (!(half == 0 ? l.left : l.right)) continue;
            for (uint8_t s : (half == 0 ? left_slots : right_slots)) {
                for (bool mac : {false, true}) {
                    const u_legend_t g = u_map_legend(l.keys[s], mac);
                    EXPECT_FALSE(g.flags & U_LEG_UNKNOWN)
                        << l.name << " slot " << int(s) << " keycode 0x" << std::hex << l.keys[s]
                        << (mac ? " on the Mac" : " on Windows");
                }
            }
        }
    }
}

// The keys D15 decided, and the thumbs and arrows the D15 renders showed.
TEST(MapLegend, TheMacSaysWhatTheKeyIsThere) {
    EXPECT_TRUE(same(u_map_legend(KC_PSCR, false), cells('P', 'S', 'c')));
    EXPECT_TRUE(same(u_map_legend(KC_PSCR, true), cells('F', '1', '3')));
    EXPECT_TRUE(same(u_map_legend(KC_SCRL, false), cells('S', 'c', 'L')));
    EXPECT_TRUE(same(u_map_legend(KC_SCRL, true), cells(U_LEG_ICON6(U_ICON6_SUN), U_LEG_ICON6(U_ICON6_TRI_DOWN))));
    EXPECT_TRUE(same(u_map_legend(KC_PAUS, true), cells(U_LEG_ICON6(U_ICON6_SUN), U_LEG_ICON6(U_ICON6_TRI_UP))));
    EXPECT_TRUE(same(u_map_legend(KC_APP, false), cells('A', 'p', 'p')));
    EXPECT_TRUE(same(u_map_legend(KC_APP, true), cells('A', 'p', 'p', U_LEG_DIM)));
    EXPECT_TRUE(same(u_map_legend(KC_INS, false), cells('I', 'n', 's')));
    EXPECT_TRUE(same(u_map_legend(KC_INS, true), cells('H', 'l', 'p')));
    EXPECT_TRUE(same(u_map_legend(KC_ALGR, true), cells(U_ICON12_ALT, 0, 0, U_LEG_ICON12)));
    EXPECT_TRUE(same(u_map_legend(KC_LEFT, true), cells(U_LEG_ICON6(U_ICON6_ARROW_LEFT))));
    EXPECT_TRUE(same(u_map_legend(KC_ENT, true), cells('E', 'n', 't')));
}

// The mods follow the host as the mod row does (oled_master_zones.c, render_mods: the pinky
// shows the four panes on Windows and the caret on the Mac, the middle finger the caret on
// Windows and Cmd on the Mac), because the Ctrl/GUI swap is what the key does there.
TEST(MapLegend, TheModsFollowTheHost) {
    EXPECT_TRUE(same(u_map_legend(KC_LGUI, false), cells(U_ICON12_WIN, 0, 0, U_LEG_ICON12)));
    EXPECT_TRUE(same(u_map_legend(KC_LGUI, true), cells(U_ICON12_CTRL, 0, 0, U_LEG_ICON12)));
    EXPECT_TRUE(same(u_map_legend(KC_LCTL, false), cells(U_ICON12_CTRL, 0, 0, U_LEG_ICON12)));
    EXPECT_TRUE(same(u_map_legend(KC_LCTL, true), cells(U_ICON12_CMD, 0, 0, U_LEG_ICON12)));
    EXPECT_TRUE(same(u_map_legend(KC_LALT, true), cells(U_ICON12_ALT, 0, 0, U_LEG_ICON12)));
    EXPECT_TRUE(same(u_map_legend(KC_LSFT, true), cells(U_ICON12_SHIFT, 0, 0, U_LEG_ICON12)));
}

// What draws nothing, and what an unknown key draws.
TEST(MapLegend, BlanksAndUnknowns) {
    EXPECT_TRUE(same(u_map_legend(KC_NO, false), cells(0)));
    EXPECT_TRUE(same(u_map_legend(KC_NO, true), cells(0)));
    EXPECT_TRUE(u_map_legend(KC_F24, false).flags & U_LEG_UNKNOWN);
    EXPECT_TRUE(u_map_legend(KC_F24, true).flags & U_LEG_UNKNOWN);
}

TEST(MapShort, EveryKeyOfEveryLayerHasOne) {
    for (const auto &l : every_layer) {
        for (uint8_t s = 0; s < 38; s++) {
            if (s == 30 || s == 31) continue;   // U_NP
            for (bool mac : {false, true}) {
                const bool       thumb = s >= 32;
                const u_legend_t g     = thumb ? u_map_legend(l.keys[s], mac) : u_map_short(l.keys[s], mac);
                const auto       where = [&] {
                    std::ostringstream o;
                    o << l.name << " slot " << int(s) << " keycode 0x" << std::hex << l.keys[s]
                      << (mac ? " on the Mac" : " on Windows");
                    return o.str();
                };
                EXPECT_FALSE(g.flags & U_LEG_UNKNOWN) << where();
                if (!thumb) EXPECT_TRUE(!g.cell[1] || (g.flags & U_LEG_TIGHT)) << "more than a cell: " << where();
            }
        }
    }
}

// docs/layout.md, "The Layer map page's legends".
TEST(MapShort, OneCellEach) {
    EXPECT_TRUE(same(u_map_short(LGUI_T(KC_A), false), cells('A')));
    EXPECT_TRUE(same(u_map_short(KC_QUOT, true), cells('\'')));
    EXPECT_TRUE(same(u_map_short(LT(U_FUN, KC_Z), false), cells('Z')));
    EXPECT_TRUE(same(u_map_short(KC_0, false), cells('0')));
    EXPECT_TRUE(same(u_map_short(KC_F7, false), cells('7')));
    EXPECT_TRUE(same(u_map_short(KC_F10, false), cells('1', '0', 0, U_LEG_TIGHT)));
    EXPECT_TRUE(same(u_map_short(KC_F12, true), cells('1', '2', 0, U_LEG_TIGHT)));
    EXPECT_TRUE(same(u_map_short(TD(U_TD_BOOT), false), cells('!')));
    EXPECT_TRUE(same(u_map_short(TD(U_TD_U_BASE), false), cells('B')));
    EXPECT_TRUE(same(u_map_short(TD(U_TD_U_BUTTON), false), cells('U')));
    EXPECT_TRUE(same(u_map_short(TD(U_TD_U_MEDIA), false), cells('D')));
    EXPECT_TRUE(same(u_map_short(TD(U_TD_U_FUN), false), cells('F')));
    EXPECT_TRUE(same(u_map_short(U_CUT, false), cells('X')));
    EXPECT_TRUE(same(u_map_short(KC_HOME, false), cells(U_LEG_ICON6(U_ICON6_TRI_LEFT))));
    EXPECT_TRUE(same(u_map_short(MS_WHLU, false), cells(U_LEG_ICON6(U_ICON6_ARROW_UP))));
    EXPECT_TRUE(same(u_map_short(KC_LEFT, false), cells(U_LEG_ICON6(U_ICON6_ARROW_LEFT))));
    EXPECT_TRUE(same(u_map_short(KC_LCBR, false), cells('{')));
    EXPECT_TRUE(same(u_map_short(KC_LCTL, true), cells(U_ICON12_CMD, 0, 0, U_LEG_ICON12)));
    EXPECT_TRUE(same(u_map_short(KC_ALGR, false), cells(U_ICON12_ALT, 0, 0, U_LEG_ICON12)));
    EXPECT_TRUE(same(u_map_short(KC_NO, false), cells(0)));
    EXPECT_TRUE(u_map_short(KC_F24, false).flags & U_LEG_UNKNOWN);
    EXPECT_TRUE(u_map_short(KC_APP, false).flags & U_LEG_UNKNOWN);   // three cells: thumbs only
}

TEST(MapShort, TheMacSaysWhatTheKeyIsThere) {
    EXPECT_TRUE(same(u_map_short(KC_INS, false), cells('I')));
    EXPECT_TRUE(same(u_map_short(KC_INS, true), cells('H')));
    EXPECT_TRUE(same(u_map_short(KC_PSCR, false), cells('P')));
    EXPECT_TRUE(same(u_map_short(KC_PSCR, true), cells('F')));
    EXPECT_TRUE(same(u_map_short(KC_SCRL, false), cells('S')));
    EXPECT_TRUE(same(u_map_short(KC_SCRL, true), cells(U_LEG_ICON6(U_ICON6_TRI_DOWN))));
    EXPECT_TRUE(same(u_map_short(KC_PAUS, false), cells('B')));
    EXPECT_TRUE(same(u_map_short(KC_PAUS, true), cells(U_LEG_ICON6(U_ICON6_TRI_UP))));
}

// --- the Layer map page -------------------------------------------------------------

namespace {

int lit(int x0, int y0, int x1, int y1) {
    int n = 0;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) n += (u_canvas[(y >> 3) * 128 + x] >> (y & 7)) & 1;
    return n;
}

uint64_t bit(int s) { return 1ull << s; }

void page(const char *name, const uint16_t *keys, uint64_t down, bool mac) {
    uint8_t layer = 0;
    for (const auto &l : every_layer)
        if (l.keys == keys) break; else layer++;
    u_layer_page_draw(layer, name, keys, base, down, mac);
}

}   // namespace

// The held thumb shows the base layer's key, cut out of its box, and the holding side's
// other thumbs are blank.
TEST(LayerPage, TheHeldKeyShowsItsBaseLegend) {
    page("NAV", nav, bit(33), false);
    EXPECT_GT(lit(23, 51, 42, 61), 150) << "Space's box under the second left thumb";
    EXPECT_EQ(lit(3, 51, 22, 61), 0) << "the outer left thumb is U_NA on Nav";
    EXPECT_GT(lit(66, 53, 82, 60), 0) << "the right thumbs keep their legends";
    page("NAV", nav, 0, false);
    EXPECT_EQ(lit(3, 51, 62, 61), 0) << "nothing held, nothing drawn on the holding side";
}

// Every layer draws, and none outshines the alarm frame's 4,584 pixels.
TEST(LayerPage, NoPageOutshinesTheAlarm) {
    for (const auto &l : every_layer) {
        for (bool mac : {false, true}) {
            page(l.name, l.keys, bit(32) | bit(33) | bit(16) | bit(17), mac);
            EXPECT_LT(lit(0, 0, 127, 63), 4584) << l.name;
            EXPECT_GT(lit(0, 10, 127, 46), 0) << l.name << " draws its keys";
        }
    }
}

// U_LAYER_PAGES=<file> writes the frames the stage 5 mock showed, in its order, for a byte
// compare against it.
TEST(LayerPage, TheMocksFrames) {
    const char *out = std::getenv("U_LAYER_PAGES");
    if (!out) GTEST_SKIP() << "set U_LAYER_PAGES to write the frames";
    struct { const char *name; const uint16_t *keys; uint64_t down; bool mac; } shots[] = {
        {"BASE", base, 0, false},
        {"NAV", nav, bit(33) | bit(16) | bit(17), false},
        {"NAV", nav, bit(33), true},
        {"MOUSE", mouse, bit(34), false},
        {"MEDIA", media, bit(32), false},
        {"NUM", num, bit(36), false},
        {"SYM", sym, bit(35), false},
        {"FUN", fun, bit(37), false},
        {"FUN", fun, bit(37), true},
        {"BUTTON", button, bit(20), false},
        {"EXTRA", extra, 0, false},
        {"TAP", tap, 0, false},
    };
    FILE *f = std::fopen(out, "wb");
    ASSERT_NE(f, nullptr);
    for (const auto &s : shots) {
        page(s.name, s.keys, s.down, s.mac);
        std::fwrite(u_canvas, 1, sizeof u_canvas, f);
    }
    std::fclose(f);
}
