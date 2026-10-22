// The event ring behind oled_event_log.h.

#include "quantum.h"
#include <string.h>
#include "oled_event_log.h"
#include "oled_canvas.h"
#include "oled_draw.h"
#include "host_os.h"
#include "split_health.h"
#include "split_sync.h"

static char    lines[U_LOG_LINES][U_LOG_CHARS + 1];
static uint8_t next, count;

// Appends to `b` at `at`, never past a line. Returns where the next character goes.
static uint8_t put_s(char *b, uint8_t at, const char *s) {
    while (*s && at < U_LOG_CHARS) b[at++] = *s++;
    b[at] = 0;
    return at;
}

static uint8_t put_u(char *b, uint8_t at, uint32_t v) {
    char    tmp[10];
    uint8_t n = 0;
    do {
        tmp[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n && at < U_LOG_CHARS) b[at++] = tmp[--n];
    b[at] = 0;
    return at;
}

void u_log_add(const char *what) {
    const uint32_t m = timer_read32() / 60000u;
    char          *b = lines[next];
    uint8_t        n = put_s(b, 0, "+");
    n                = put_u(b, n, m / 60);
    n                = put_s(b, n, m % 60 < 10 ? ":0" : ":");
    n                = put_u(b, n, m % 60);
    n                = put_s(b, n, " ");
    put_s(b, n, what);
    next = (uint8_t)((next + 1) % U_LOG_LINES);
    if (count < U_LOG_LINES) count++;
}

uint8_t u_log_count(void) {
    return count;
}

const char *u_log_line(uint8_t i) {
    return lines[(next + U_LOG_LINES - count + i) % U_LOG_LINES];
}

// --- the events ----------------------------------------------------------------------

static const char *host_name(uint8_t host) {
    static const char *const names[] = {[U_HOST_WINDOWS] = "win", [U_HOST_MAC] = "mac", [U_HOST_IOS] = "ios",
                                        [U_HOST_LINUX] = "lnx"};
    return host < sizeof names / sizeof names[0] ? names[host] : "?";
}

void u_log_watch(const u_split_state_t *now) {
    static bool     primed, replied, down, detected_was, manual_was;
    static uint8_t  resets, host_was;
    static uint32_t down_at;
    char            b[U_LOG_CHARS + 1];

    if (!primed) {
        primed   = true;
        host_was = now->host;
        char label[7] = {0};
        strncpy(label, u_build_label(), 6);
        put_s(b, put_s(b, 0, "boot "), label);
        u_log_add(b);
    }

    // The first reply is the link coming up: before it, nothing has been sent long enough
    // to fail, so u_sync_link_down() has no edge to give.
    if (!replied) {
        if (u_health_have_reply()) {
            replied = true;
            down    = u_sync_link_down();
            resets  = u_health_slave_seen().resets;
            u_log_add("link up");
        }
    } else {
        // Before the link edges: the reply that carries a reset is the one that ends the
        // outage, and the log reads down, reset, up.
        const uint8_t r = u_health_slave_seen().resets;
        if (r > resets) u_log_add("heal reset");
        resets = r;

        const bool d = u_sync_link_down();
        if (d && !down) {
            down_at = timer_read32();
            u_log_add("link down");
        } else if (!d && down) {
            const uint32_t ds = timer_elapsed32(down_at) / 100u;
            uint8_t        n  = put_s(b, 0, "link up ");
            n                 = put_u(b, n, ds / 10);
            n                 = put_s(b, n, ".");
            n                 = put_u(b, n, ds % 10);
            put_s(b, n, "s");
            u_log_add(b);
        }
        down = d;
    }

    // A detection that says Windows changes nothing in the state, so detection is asked
    // whether it has reported.
    const bool detected = host_os_detected(), manual = now->flags & U_FLAG_OS_MANUAL;
    if (manual && (now->host != host_was || !manual_was)) {
        put_s(b, put_s(b, put_s(b, put_s(b, 0, "SYS "), host_name(host_was)), ">"), host_name(now->host));
        u_log_add(b);
    } else if (!manual && detected && (!detected_was || manual_was || now->host != host_was)) {
        put_s(b, put_s(b, put_s(b, 0, "os "), host_name(now->host)), " auto");
        u_log_add(b);
    }
    host_was     = now->host;
    manual_was   = manual;
    detected_was = detected;
}

void u_log_misfire(uint8_t cells, uint8_t left) {
    static const char *const fingers[4] = {"pnk", "rng", "mid", "idx"};
    for (uint8_t i = 0; i < 4; i++) {
        if (!(cells & (1u << i))) continue;
        char b[U_LOG_CHARS + 1];
        put_s(b, put_s(b, 0, (left & (1u << i)) ? "misfire L " : "misfire R "), fingers[i]);
        u_log_add(b);
        return;
    }
}

void u_log_draw(void) {
    memset(u_canvas, 0, OLED_MATRIX_SIZE);
    for (uint8_t i = 0; i < count; i++) u_text(0, (int16_t)(i * 8), u_log_line(i), 1, true);
}
