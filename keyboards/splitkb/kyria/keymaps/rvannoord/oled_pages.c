// The Status sheet and Typing pages behind oled_pages.h, at the geometry Roel approved from
// the stage 5 mock.

#include "quantum.h"
#include <string.h>
#include "manna-harbour_miryoku.h"
#include "oled_pages.h"
#include "oled_canvas.h"
#include "oled_draw.h"
#include "oled_map.h"
#include "host_os.h"
#include "art/drashna_icons.h"
#include "art/kitty_art.h"
#include "art/panel_glyphs.h"
#include "art/status_frame.h"

// Right-aligned in three places, the way WPM is printed.
static void num3(char *b, uint8_t v) {
    b[0] = v >= 100 ? (char)('0' + v / 100) : ' ';
    b[1] = v >= 10 ? (char)('0' + v / 10 % 10) : ' ';
    b[2] = (char)('0' + v % 10);
    b[3] = 0;
}

// --- the Status sheet ----------------------------------------------------------------

static const char *const host_names[] = {[U_HOST_WINDOWS] = "Win", [U_HOST_MAC] = "Mac", [U_HOST_IOS] = "iOS",
                                         [U_HOST_LINUX] = "Lnx"};
static const uint8_t     host_logos[] = {[U_HOST_WINDOWS] = U_OS_LOGO_WINDOWS, [U_HOST_MAC] = U_OS_LOGO_APPLE,
                                         [U_HOST_IOS] = U_OS_LOGO_APPLE, [U_HOST_LINUX] = U_OS_LOGO_LINUX};

static void frame_art(void) {
    u_blit(u_status_header, U_STATUS_FRAME_W, 1, 0, 0);
    for (uint8_t p = 1; p < 7; p++) {
        for (uint8_t i = 0; i < U_STATUS_BORDER_W; i++) {
            const uint8_t b = pgm_read_byte(&u_status_border[i]);
            u_canvas[p * OLED_DISPLAY_WIDTH + i]                          = b;
            u_canvas[p * OLED_DISPLAY_WIDTH + OLED_DISPLAY_WIDTH - 1 - i] = b;
        }
    }
    u_blit(u_status_footer, U_STATUS_FRAME_W, 1, 0, 56);
}

// Eight modifier icons, the left hand's four then the right's mirrored, each over its
// finger: pinky outermost on both sides.
static void mods(const u_split_state_t *s) {
    static const uint8_t win[4] = {U_ICON12_WIN, U_ICON12_ALT, U_ICON12_CTRL, U_ICON12_SHIFT};
    static const uint8_t mac[4] = {U_ICON12_CTRL, U_ICON12_ALT, U_ICON12_CMD, U_ICON12_SHIFT};
    const uint8_t       *set    = (s->flags & U_FLAG_IS_MAC) ? mac : win;
    for (uint8_t i = 0; i < 8; i++) {
        const int16_t    x   = (int16_t)(8 + i * 14 + (i >= 4 ? 4 : 0));
        const uint8_t    fin = i < 4 ? i : (uint8_t)(7 - i);
        const bool       on  = s->fingers & (i < 4 ? 1u << fin : 0x10u << fin);
        const u_legend_t g   = {{set[fin]}, U_LEG_ICON12};
        if (on) u_rect((int16_t)(x - 1), 46, 14, 9, true);
        u_legend_at(g, x, 47, 1, !on);
    }
}

void u_status_draw(const u_split_state_t *s, uint8_t wpm, uint8_t kitty) {
    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    frame_art();
    u_text_bar(25, 0, " Kyria rev3 ");
    u_blit(u_kitty_striped[kitty], U_KITTY_W, U_KITTY_BYTES / U_KITTY_W, 4, 9);

    char b[8] = "WPM ";
    num3(b + 4, wpm);
    u_text(40, 10, b, 1, true);

    const uint8_t active = s->layers & 0x0F, deflt = s->layers >> 4;
    if (active == U_BASE) {
        u_text(40, 20, u_layer_name(active), 1, true);
    } else {
        u_text_bar(40, 20, u_layer_name(active));
    }

    const uint8_t host = s->host < sizeof host_logos ? s->host : U_HOST_WINDOWS;
    uint8_t       lx   = 40;
    if (host == U_HOST_IOS) {
        u_blit(u_ios_i, U_IOS_I_W, 2, 40, 29);
        lx = 44;
    }
    u_blit(u_os_logo[host_logos[host]], U_OS_LOGO_W, U_OS_LOGO_PAGES, lx, 29);
    u_text(58, 29, host_names[host], 1, true);
    u_text(58, 37, (s->flags & U_FLAG_OS_MANUAL) ? "hand" : "auto", 1, true);

    static const char *const locks[3] = {"CW", "CAPS", "SCRL"};
    static const uint8_t     bits[3]  = {U_FLAG_CAPS_WORD, U_FLAG_CAPS_LOCK, U_FLAG_SCROLL_LOCK};
    for (uint8_t i = 0; i < 3; i++) {
        const int16_t x = (int16_t)(88 + (36 - u_text_w(locks[i])) / 2), y = (int16_t)(10 + i * 10);
        if (s->flags & bits[i]) {
            u_text_bar(x, y, locks[i]);
        } else {
            u_text(x, y, locks[i], 1, true);
        }
    }

    mods(s);

    // D8: the layout, or the default layer when that is not Base.
    const char *foot = deflt == U_BASE ? "Colemak-DH" : u_layer_name(deflt);
    u_rect(22, 56, 84, 8, true);
    u_text((int16_t)(22 + (84 - u_text_w(foot)) / 2), 56, foot, 1, false);
}

// --- Typing --------------------------------------------------------------------------

static uint8_t  samples[U_TYPING_SAMPLES];   // oldest at `head`
static uint8_t  head, peak;
static uint32_t sampled_at;
static bool     sampling;

void u_typing_task(uint8_t wpm) {
    if (sampling && timer_elapsed32(sampled_at) < U_TYPING_MS) return;
    sampling        = true;
    sampled_at      = timer_read32();
    samples[head]   = wpm;
    head            = (uint8_t)((head + 1) % U_TYPING_SAMPLES);
    if (wpm > peak) peak = wpm;
}

#define U_GRAPH_X    66
#define U_GRAPH_Y    1
#define U_GRAPH_H    28
#define U_GRAPH_VMAX 110

void u_typing_draw(uint8_t wpm) {
    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    char b[12];
    num3(b, wpm);
    u_text(2, 2, b, 2, true);
    u_text(2, 20, "wpm", 1, true);
    num3(b, peak);
    const char *digits = b[0] != ' ' ? b : b[1] != ' ' ? b + 1 : b + 2;
    char        line[8] = "max ";
    strcpy(line + 4, digits);
    u_text(24, 20, line, 1, true);

    // A two-pixel line, and every third row under it, so the area reads as filled
    // without lighting it.
    for (uint8_t i = 0; i < U_TYPING_SAMPLES; i++) {
        const uint8_t v   = samples[(head + i) % U_TYPING_SAMPLES];
        const int16_t x   = (int16_t)(U_GRAPH_X + i);
        const int16_t top = (int16_t)(U_GRAPH_Y + U_GRAPH_H - 1 - (v < U_GRAPH_VMAX ? v : U_GRAPH_VMAX) * (U_GRAPH_H - 1) / U_GRAPH_VMAX);
        u_put(x, top, true);
        u_put(x, (int16_t)(top + 1), true);
        for (int16_t y = (int16_t)(top + 2); y < U_GRAPH_Y + U_GRAPH_H; y++) {
            if ((y - U_GRAPH_Y) % 3 == 0) u_put(x, y, true);
        }
    }
}
