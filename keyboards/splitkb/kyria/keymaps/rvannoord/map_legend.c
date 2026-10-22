// The keycode-to-legend tables behind map_legend.h.

#include "quantum.h"
#include "manna-harbour_miryoku.h"
#include "map_legend.h"
#include "art/drashna_icons.h"

typedef struct {
    uint16_t   keycode;
    u_legend_t legend;
} u_legend_entry_t;

#define T(...)  {{__VA_ARGS__}, 0}
#define I6(i)   U_LEG_ICON6(U_ICON6_##i)
#define I12(i)  {{U_ICON12_##i}, U_LEG_ICON12}

// Searched in order, and KC_NO first: Miryoku's U_NA, U_NU and U_NP are all KC_NO, and so
// are the RGB keys in a build without RGB, so they draw nothing rather than matching
// whatever entry comes first.
static const u_legend_entry_t legends[] = {
    {KC_NO, T(0)},

    // Clipboard, as Miryoku's MIRYOKU_CLIPBOARD makes them.
    {U_RDO, T('R', 'd', 'o')}, {U_PST, T('P', 's', 't')}, {U_CPY, T('C', 'p', 'y')},
    {U_CUT, T('C', 'u', 't')}, {U_UND, T('U', 'n', 'd')},

    // Nav.
    {CW_TOGG, T('C', 'W')},
    {KC_LEFT, T(I6(ARROW_LEFT))}, {KC_DOWN, T(I6(ARROW_DOWN))},
    {KC_UP, T(I6(ARROW_UP))},     {KC_RGHT, T(I6(ARROW_RIGHT))},
    {KC_INS, T('I', 'n', 's')},   {KC_HOME, T('H', 'o', 'm')}, {KC_PGDN, T('P', 'g', 'D')},
    {KC_PGUP, T('P', 'g', 'U')},  {KC_END, T('E', 'n', 'd')},

    // Thumbs.
    {KC_ENT, T('E', 'n', 't')}, {KC_BSPC, T('B', 's', 'p')}, {KC_DEL, T('D', 'e', 'l')},
    {KC_SPC, T('S', 'p', 'c')}, {KC_TAB, T('T', 'a', 'b')},  {KC_ESC, T('E', 's', 'c')},

    // Mouse: the pointer as triangles, the wheel as W and a triangle. QMK's current names,
    // which custom_config.h's shims turn Miryoku's KC_MS_L and friends into.
    {MS_LEFT, T(I6(TRI_LEFT))}, {MS_DOWN, T(I6(TRI_DOWN))},
    {MS_UP, T(I6(TRI_UP))},     {MS_RGHT, T(I6(TRI_RIGHT))},
    {MS_WHLL, T('W', I6(TRI_LEFT))}, {MS_WHLD, T('W', I6(TRI_DOWN))},
    {MS_WHLU, T('W', I6(TRI_UP))},   {MS_WHLR, T('W', I6(TRI_RIGHT))},
    {MS_BTN1, T('L', 'M', 'B')}, {MS_BTN2, T('R', 'M', 'B')}, {MS_BTN3, T('M', 'M', 'B')},

    // Media. Both RGB families, because custom_config.h maps Miryoku's RGB_* keys to RGB
    // Matrix or RGB Light by what the build has, and this file is built without either.
    {RM_TOGG, T('R', 'G', 'B')}, {RM_NEXT, T('M', 'o', 'd')}, {RM_HUEU, T('H', 'u', 'e')},
    {RM_SATU, T('S', 'a', 't')}, {RM_VALU, T('V', 'a', 'l')},
    {UG_TOGG, T('R', 'G', 'B')}, {UG_NEXT, T('M', 'o', 'd')}, {UG_HUEU, T('H', 'u', 'e')},
    {UG_SATU, T('S', 'a', 't')}, {UG_VALU, T('V', 'a', 'l')},
    {KC_MPRV, T('P', 'r', 'v')}, {KC_VOLD, T('V', 'l', '-')}, {KC_VOLU, T('V', 'l', '+')},
    {KC_MNXT, T('N', 'x', 't')}, {OU_AUTO, T('O', 'u', 't')},
    {KC_MSTP, T('S', 't', 'p')}, {KC_MPLY, T('P', 'l', 'y')}, {KC_MUTE, T('M', 'u', 't')},

    // Num.
    {KC_1, T('1')}, {KC_2, T('2')}, {KC_3, T('3')}, {KC_4, T('4')}, {KC_5, T('5')},
    {KC_6, T('6')}, {KC_7, T('7')}, {KC_8, T('8')}, {KC_9, T('9')}, {KC_0, T('0')},
    {KC_LBRC, T('[')}, {KC_RBRC, T(']')}, {KC_SCLN, T(';')}, {KC_EQL, T('=')},
    {KC_GRV, T('`')},  {KC_BSLS, T('\\')}, {KC_DOT, T('.')}, {KC_MINS, T('-')},

    // Sym.
    {KC_LCBR, T('{')}, {KC_AMPR, T('&')}, {KC_ASTR, T('*')}, {KC_LPRN, T('(')}, {KC_RCBR, T('}')},
    {KC_COLN, T(':')}, {KC_DLR, T('$')},  {KC_PERC, T('%')}, {KC_CIRC, T('^')}, {KC_PLUS, T('+')},
    {KC_TILD, T('~')}, {KC_EXLM, T('!')}, {KC_AT, T('@')},   {KC_HASH, T('#')}, {KC_PIPE, T('|')},
    {KC_RPRN, T(')')}, {KC_UNDS, T('_')},

    // Fun.
    {KC_F1, T('F', '1')},  {KC_F2, T('F', '2')},  {KC_F3, T('F', '3')},  {KC_F4, T('F', '4')},
    {KC_F5, T('F', '5')},  {KC_F6, T('F', '6')},  {KC_F7, T('F', '7')},  {KC_F8, T('F', '8')},
    {KC_F9, T('F', '9')},  {KC_F10, T('F', '1', '0')}, {KC_F11, T('F', '1', '1')},
    {KC_F12, T('F', '1', '2')},
    {KC_PSCR, T('P', 'S', 'c')}, {KC_SCRL, T('S', 'c', 'L')}, {KC_PAUS, T('P', 'a', 'u')},
    {KC_APP, T('A', 'p', 'p')},

    // Button's modifiers, as the mod row draws them on Windows.
    {KC_LGUI, I12(WIN)}, {KC_LALT, I12(ALT)}, {KC_LCTL, I12(CTRL)}, {KC_LSFT, I12(SHIFT)},
    {KC_ALGR, T('A', 'G', 'r')},
};

// Checked first on the Mac: the keys that are something else there (D15), and the mods,
// which follow the host the way the mod row does: the Ctrl/GUI swap puts Ctrl on the
// pinky's GUI key and Cmd on the middle finger's Ctrl key.
static const u_legend_entry_t mac_legends[] = {
    {KC_PSCR, T('F', '1', '3')},                 // read as F13
    {KC_SCRL, T(I6(SUN), I6(TRI_DOWN))},         // F14, screen brightness down
    {KC_PAUS, T(I6(SUN), I6(TRI_UP))},           // F15, screen brightness up
    {KC_APP, {{'A', 'p', 'p'}, U_LEG_DIM}},      // does nothing
    {KC_INS, T('H', 'l', 'p')},                  // read as Help
    {KC_ALGR, I12(ALT)},                         // right Option
    {KC_LGUI, I12(CTRL)},
    {KC_LCTL, I12(CMD)},
};

static bool find(const u_legend_entry_t *table, uint8_t n, uint16_t keycode, u_legend_t *out) {
    for (uint8_t i = 0; i < n; i++) {
        if (table[i].keycode == keycode) {
            *out = table[i].legend;
            return true;
        }
    }
    return false;
}

// What a key types: a key that also holds a modifier or a layer is drawn as its tap.
static uint16_t tap_of(uint16_t keycode) {
    if (IS_QK_MOD_TAP(keycode)) return QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
    if (IS_QK_LAYER_TAP(keycode)) return QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    return keycode;
}

static const u_legend_t unknown = {{U_LEG_ICON6(U_ICON6_BULLET)}, U_LEG_UNKNOWN};

u_legend_t u_map_legend(uint16_t keycode, bool mac) {
    u_legend_t l;
    keycode = tap_of(keycode);
    if (mac && find(mac_legends, ARRAY_SIZE(mac_legends), keycode, &l)) return l;
    if (find(legends, ARRAY_SIZE(legends), keycode, &l)) return l;
    return unknown;
}

// --- one cell ----------------------------------------------------------------------------

// Miryoku's tap dances are Boot, then one lock per layer in MIRYOKU_LAYER_LIST order
// (manna-harbour_miryoku.c), so a tap dance's index past Boot is the layer it locks.
static const char letters[] = {[U_BASE] = 'B', [U_EXTRA] = 'E', [U_TAP] = 'T', [U_BUTTON] = 'U', [U_NAV] = 'V',
                                 [U_MOUSE] = 'M', [U_MEDIA] = 'D', [U_NUM] = 'N', [U_SYM] = 'Y', [U_FUN] = 'F'};

static const u_legend_entry_t shorts[] = {
    {KC_QUOT, T('\'')}, {KC_COMM, T(',')}, {KC_SLSH, T('/')},
    {U_RDO, T('R')}, {U_PST, T('P')}, {U_CPY, T('C')}, {U_CUT, T('X')}, {U_UND, T('U')},
    {CW_TOGG, T('W')}, {KC_INS, T('I')},
    {KC_HOME, T(I6(TRI_LEFT))}, {KC_PGDN, T(I6(TRI_DOWN))}, {KC_PGUP, T(I6(TRI_UP))}, {KC_END, T(I6(TRI_RIGHT))},
    {MS_WHLL, T(I6(ARROW_LEFT))}, {MS_WHLD, T(I6(ARROW_DOWN))}, {MS_WHLU, T(I6(ARROW_UP))}, {MS_WHLR, T(I6(ARROW_RIGHT))},
    {RM_TOGG, T('R')}, {RM_NEXT, T('M')}, {RM_HUEU, T('H')}, {RM_SATU, T('S')}, {RM_VALU, T('V')},
    {UG_TOGG, T('R')}, {UG_NEXT, T('M')}, {UG_HUEU, T('H')}, {UG_SATU, T('S')}, {UG_VALU, T('V')},
    {KC_MPRV, T(I6(TRI_LEFT))}, {KC_VOLD, T('-')}, {KC_VOLU, T('+')}, {KC_MNXT, T(I6(TRI_RIGHT))}, {OU_AUTO, T('O')},
    {KC_PSCR, T('P')}, {KC_SCRL, T('S')}, {KC_PAUS, T('B')},
    {KC_ALGR, I12(ALT)},
};

static const u_legend_entry_t mac_shorts[] = {
    {KC_INS, T('H')},                   // Help
    {KC_PSCR, T('F')},                  // F13
    {KC_SCRL, T(I6(TRI_DOWN))},         // F14, brightness down
    {KC_PAUS, T(I6(TRI_UP))},           // F15, brightness up
};

u_legend_t u_map_short(uint16_t keycode, bool mac) {
    keycode = tap_of(keycode);
    if (keycode == KC_NO) return (u_legend_t){{0}, 0};
    if (IS_QK_TAP_DANCE(keycode)) {
        const uint8_t i = QK_TAP_DANCE_GET_INDEX(keycode);
        if (i == 0) return (u_legend_t)T('!');
        return i - 1 < (int)sizeof letters ? (u_legend_t)T((uint8_t)letters[i - 1]) : unknown;
    }
    if (keycode >= KC_A && keycode <= KC_Z) return (u_legend_t)T((uint8_t)('A' + keycode - KC_A));
    if (keycode >= KC_1 && keycode <= KC_9) return (u_legend_t)T((uint8_t)('1' + keycode - KC_1));
    if (keycode == KC_0) return (u_legend_t)T('0');
    if (keycode >= KC_F1 && keycode <= KC_F12) {
        const uint8_t n = (uint8_t)(1 + keycode - KC_F1);
        if (n < 10) return (u_legend_t)T((uint8_t)('0' + n));
        return (u_legend_t){{'1', (uint8_t)('0' + n - 10)}, U_LEG_TIGHT};
    }
    u_legend_t l;
    if (mac && find(mac_shorts, ARRAY_SIZE(mac_shorts), keycode, &l)) return l;
    if (find(shorts, ARRAY_SIZE(shorts), keycode, &l)) return l;
    // Single characters, arrows, the pointer and the modifiers are the full legend already.
    l = u_map_legend(keycode, mac);
    if (l.flags & (U_LEG_ICON12 | U_LEG_UNKNOWN)) return l;
    return l.cell[1] ? unknown : l;
}
