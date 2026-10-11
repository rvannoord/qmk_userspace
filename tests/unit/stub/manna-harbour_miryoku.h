// Miryoku's layer list, copied rather than included: pulling in the real header drags
// in all of QMK. If the real list changes, the panel's own X-macro use still compiles
// against the real one in the firmware build, and these tests only need the names.
#pragma once

#define MIRYOKU_LAYER_LIST \
MIRYOKU_X(BASE,   "Base") \
MIRYOKU_X(EXTRA,  "Extra") \
MIRYOKU_X(TAP,    "Tap") \
MIRYOKU_X(BUTTON, "Button") \
MIRYOKU_X(NAV,    "Nav") \
MIRYOKU_X(MOUSE,  "Mouse") \
MIRYOKU_X(MEDIA,  "Media") \
MIRYOKU_X(NUM,    "Num") \
MIRYOKU_X(SYM,    "Sym") \
MIRYOKU_X(FUN,    "Fun")

enum layers {
#define MIRYOKU_X(LAYER, STRING) U_##LAYER,
    MIRYOKU_LAYER_LIST
#undef MIRYOKU_X
};
