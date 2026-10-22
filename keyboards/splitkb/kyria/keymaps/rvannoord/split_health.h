// Evidence about the split link, kept on both halves.
//
// On 2026-10-13 the link went down for twenty minutes: the master showed its banner,
// the other half's keys stopped working, and only replugging fixed it. Keys dying on
// the far half means the standard transport was down rather than our own RPC, and QMK
// retries that twice a second for ever — so roughly 2,400 attempts went unanswered.
// That is a half which had stopped answering, not a line glitch. Nothing on the
// keyboard recorded any of it; this file is so that the next one is not a memory test.
//
// RAM only, by decision: look before replugging, because a replug clears it.

#pragma once

#include <stdint.h>
#include <stdbool.h>

// What the other half sends back on every state transaction. Nine bytes, well inside
// the 32-byte reply buffer, and GET_RPC_RESP_DATA already runs on every send with zero
// bytes, so carrying these adds no new failure point — only about 0.35 ms on the wire.
typedef struct __attribute__((packed)) {
    uint16_t uptime_s;          // the giveaway: far below the master's means it restarted
    uint16_t longest_stale_ds;  // deciseconds, saturating
    uint16_t build_id;          // halves that disagree here are running different builds
    uint8_t  stale_episodes;    // saturating
    uint8_t  resets;            // self-resets since power-on
    uint8_t  last_reset_cause;  // u_reset_cause
} u_health_slave_t;

enum u_reset_cause {
    U_RESET_NONE = 0,
    U_RESET_SILENT_LINK,        // no state for long enough, with the panel lit
};

// The master's own view. Counters saturate rather than wrap: "at least this many" is
// the honest reading of a counter you cannot see overflow.
typedef struct {
    uint32_t uptime_s;
    uint16_t failed_sends;
    uint16_t worst_fail_streak;
    uint16_t outages;             // rising edges of u_sync_link_down()
    uint16_t longest_outage_ds;
    uint32_t last_outage_start_s; // uptime when it began; 0 means none yet
    uint16_t last_outage_ds;
    uint16_t transport_drops;     // is_transport_connected() going false
    uint16_t suspends;            // host sleeps, counted once each
    uint16_t loops_per_s;         // last full second
    uint16_t worst_loops_per_s;   // the slowest second since boot, ignoring the first
} u_health_master_t;

// A number that identifies this firmware, from the build date and the userspace commit.
// Referencing them also puts both strings in the binary, so a flash read back off a
// board can finally be identified — the read-backs taken on 2026-10-14 could not be.
uint16_t u_build_id(void);
// The userspace commit the build was made from, as `git describe` gave it: "e839e2".
const char *u_build_label(void);

// Every main loop, on both halves.
void u_health_task(void);

// From the master's send path, once per attempt.
void u_health_note_send(bool ok);

// The master's copy of the other half's reply, and whether one has ever arrived.
void u_health_note_reply(const void *data, uint8_t size);
bool u_health_have_reply(void);

u_health_master_t u_health_master(void);
u_health_slave_t  u_health_slave_seen(void);   // master: what the other half last said
u_health_slave_t  u_health_slave_local(void);  // slave: what it is about to say

// Called from the slave's RPC callback, which runs on a HIGHPRIO thread holding the
// shared-memory lock: it copies out of a double buffer and does nothing else. Nine
// bytes is not an atomic write, so a single struct would hand back frames composed of
// half old and half new state.
void u_health_fill_reply(void *dst);

// --- the stats page ---------------------------------------------------------
// Dark at rest is kept: the page exists only while it is asked for. SYS held for two
// seconds opens it, which moves the Win/Mac toggle to the release of a *short* press.
// Bootmagic is unaffected — that is a boot-time scan, not a keypress.

#define U_STATS_HOLD_MS   2000
#define U_STATS_LINGER_MS 15000

void u_stats_note_sys(bool pressed);   // from process_record_user, for U_SYS
bool u_stats_sys_was_tap(void);        // at release: short enough to mean "toggle"
void u_stats_note_key(void);           // any other key closes the page
bool u_stats_page_open(void);

// --- self-heal --------------------------------------------------------------
// If the other half stops answering, let it restart itself. On 2026-10-13 it stayed
// dead for twenty minutes and only a replug fixed it, so the alternative to this is
// reaching for the cable.
//
// Every condition below exists to stop it firing when it would do harm:
//
//  - A reset within 500 ms of a boot lands in the USB bootloader, not the firmware
//    (RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT is 500U in this build), which would
//    take the half off the keyboard entirely. Hence the uptime floor, with margin.
//  - A reset costs about a second of that half's keys, so never while typing.
//  - During host sleep the master sends "panel off" every 100 ms and housekeeping stops,
//    so the state legitimately goes stale. Requiring the panel to have been lit
//    throughout disarms the whole thing while the computer is away.
//  - Backoff, and a hard cap: a half that will not stay up should end up visibly broken
//    rather than rebooting for ever, because a reboot loop hides the fault it is caused
//    by. After the cap it stays dead, which is the honest state, and the stats page says
//    how many times it tried.

#define U_HEAL_MAX_RESETS 3
#define U_HEAL_MIN_UPTIME_MS 10000   // far outside the 500 ms double-tap window
#define U_HEAL_MIN_PANEL_MS   5000
#define U_HEAL_MIN_IDLE_MS    2000

typedef struct {
    uint32_t state_age_ms;   // u_sync_age_ms(): how long since the master was heard
    uint32_t panel_on_ms;    // how long the panel has been continuously lit
    uint32_t uptime_ms;
    uint32_t idle_ms;        // last_input_activity_elapsed(), both halves' keys
    uint8_t  resets_done;    // this power-on
} u_heal_in_t;

// Pure, so the whole decision can be pinned down on the host rather than by rebooting
// a keyboard and hoping.
bool u_heal_should_reset(const u_heal_in_t *in);
