// The whole framework. These tests exist to cover two things the GoogleTest scenarios
// cannot reach: the split transport's master half, which only compiles under
// SPLIT_KEYBOARD, and the panel's drawing, which needs a buffer to draw into.
#pragma once
#include <stdio.h>

static int unit_fails;

static void check(const char *what, int ok) {
    printf("%s %s\n", ok ? "  ok  " : "  FAIL", what);
    if (!ok) unit_fails++;
}

static int unit_done(const char *name) {
    printf("%s: %s\n", name, unit_fails ? "FAILED" : "all good");
    return unit_fails != 0;
}
