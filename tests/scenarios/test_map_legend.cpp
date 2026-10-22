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
}

namespace {

// As manna-harbour_miryoku.c declares it, which a header does not export. Only the names
// matter: the tap dances sit on the half holding the layer, which the map does not draw.
enum {
    U_TD_BOOT,
#define MIRYOKU_X(LAYER, STRING) U_TD_U_##LAYER,
    MIRYOKU_LAYER_LIST
#undef MIRYOKU_X
};

// Miryoku's 40 slots: three rows of ten, then the thumb row. Each row runs left pinky to
// left inner, then right inner to right pinky; the thumbs used are slots 32-34 on the
// left and 35-37 on the right.
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
