// The master send path. The GoogleTest scenarios build split_sync.c's
// non-SPLIT_KEYBOARD branch, where every function is a stub, so nothing there covers
// this — and a regression is silent and total: no sends at all, which on the keyboard
// looks like the other half's mascot simply going inert.
#include "unit.h"
#include "quantum.h"
#include "transactions.h"
#include "split_state.h"
#include "split_sync.h"
#include "split_health.h"

// Private to split_sync.c, mirrored here. Drift shows up as a failure rather than as a
// test that quietly asserts nothing: the first check is tight against the floor.
#define U_SYNC_MIN_MS        20
#define U_SYNC_HEARTBEAT_MS 250

uint32_t fake_now       = 0;
bool     fake_is_master = true;

static bool     send_ok   = true;
static uint32_t sends     = 0;
static uint32_t last_ok   = 0;

static u_split_state_t drive;
static uint8_t         build_noise;     // what a per-build counter would have put there
static uint8_t         sent_seq[64];
static uint8_t         sent_n;

// The build deliberately carries junk in seq, so the test proves the sender stamps it
// rather than passing through whatever the build happened to produce. The master builds
// twice per main loop, so a per-build counter advanced by 2L between two heartbeats L
// loops apart — and at L = 128 the slave saw the same number twice and aged itself into
// hard staleness while every send was succeeding.
u_split_state_t u_sync_build(void) {
    u_split_state_t s = drive;
    s.seq = build_noise++;
    return s;
}
void            u_sync_note_record(uint16_t keycode, keyrecord_t *record) { (void)keycode; (void)record; }

bool transaction_rpc_exec(uint8_t id, uint8_t m2s_size, const void *m2s,
                          uint8_t s2m_size, void *s2m) {
    (void)id;
    if (!send_ok) return false;
    if (m2s_size == sizeof(u_split_state_t) && sent_n < sizeof(sent_seq)) {
        sent_seq[sent_n++] = ((const u_split_state_t *)m2s)->seq;
    }
    // Hand back a plausible reply, so the health path is exercised rather than skipped.
    if (s2m_size == sizeof(u_health_slave_t)) {
        const u_health_slave_t r = {.uptime_s = 42, .build_id = 0x1234};
        memcpy(s2m, &r, sizeof(r));
    }
    sends++;
    last_ok = fake_now;
    return true;
}

static bool fake_connected = true;
bool is_transport_connected(void) { return fake_connected; }

void transaction_register_rpc(uint8_t id, void (*cb)(uint8_t, const void *, uint8_t, void *)) {
    (void)id; (void)cb;
}

static void run_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        fake_now++;
        u_health_task();
        u_sync_task();
    }
}

int main(void) {
    drive = (u_split_state_t){0};
    u_sync_init();

    // Ordered deliberately: the module's state cannot be reset from out here, so each
    // step has to leave the clock somewhere the next one can use.
    check("quiet is not yet a fault at boot", !u_sync_link_down());

    // A half that was never there at all is reported, rather than excused for never
    // having primed. last_sent_at starts at zero, so the age reads as uptime.
    send_ok = false;
    run_ms(U_SYNC_HARD_MS + 1);
    check("a half that never answered is reported down", u_sync_link_down());
    check("and nothing was counted as sent", sends == 0);

    // The guard is age >= U_SYNC_MIN_MS against that same clock, so the first success
    // lands on the next loop. Anything that reseeds the clock while unprimed deadlocks
    // the guard against itself and nothing is ever sent.
    send_ok = true;
    run_ms(1);
    check("the first send lands as soon as it can", sends == 1);
    check("and the link comes up with it", !u_sync_link_down());

    const uint32_t before = sends;
    run_ms(U_SYNC_HEARTBEAT_MS + 1);
    check("a quiet master still heartbeats", sends > before);
    check("the link stays up while sends land", !u_sync_link_down());

    // Timed from the last send that landed, not from here: where in the heartbeat cycle
    // the failure starts is arbitrary.
    send_ok = false;
    const uint32_t ok_at = last_ok;
    while (!u_sync_link_down() && fake_now - ok_at < U_SYNC_HARD_MS + 50) run_ms(1);
    check("down exactly when the hard window passes", fake_now - ok_at == U_SYNC_HARD_MS);
    check("and nothing went out while it was failing", last_ok == ok_at);

    // Recovery is a send that lands, nothing more.
    send_ok = true;
    run_ms(U_SYNC_HEARTBEAT_MS + 1);
    check("back up once a send lands again", !u_sync_link_down());

    // --- the sequence number ------------------------------------------------
    // One step per delivered state. Consecutive sends must differ however many builds
    // happened in between, and a failed send must not burn a number.
    sent_n = 0;
    send_ok = true;
    for (int i = 0; i < 6; i++) {
        drive.fingers = (uint8_t)(i + 1);      // something changed, so it sends
        run_ms(U_SYNC_MIN_MS + 1);
    }
    bool consecutive = sent_n >= 6;
    for (uint8_t i = 1; i < sent_n; i++) {
        if ((uint8_t)(sent_seq[i] - sent_seq[i - 1]) != 1) consecutive = false;
    }
    check("every delivered state steps the sequence by one", consecutive);

    // 128 builds between two sends is the case that used to alias back to the same
    // number. Nothing the build does can reach the wire any more.
    const uint8_t before_gap = sent_seq[sent_n - 1];
    for (int i = 0; i < 128; i++) (void)u_sync_build();
    drive.fingers = 0x7f;
    run_ms(U_SYNC_MIN_MS + 1);
    check("128 builds in between change nothing",
          sent_n > 0 && (uint8_t)(sent_seq[sent_n - 1] - before_gap) == 1);

    // A send that fails repeats the same number next time rather than skipping one.
    const uint8_t last_good = sent_seq[sent_n - 1];
    send_ok = false;
    drive.fingers = 0x11;
    run_ms(U_SYNC_HEARTBEAT_MS + 1);
    send_ok = true;
    run_ms(U_SYNC_MIN_MS + 1);
    check("a failed send does not burn a sequence number",
          (uint8_t)(sent_seq[sent_n - 1] - last_good) == 1);

    // --- the health counters ------------------------------------------------
    // These exist because the link went down for twenty minutes on 2026-10-13 and the
    // keyboard recorded nothing. What they count has to be right, or the next outage
    // is another memory test.
    {
        const u_health_master_t before = u_health_master();

        send_ok = false;
        const uint32_t t0 = fake_now;
        while (!u_sync_link_down()) run_ms(1);      // 1.25 s of failing
        run_ms(2000);                               // then two seconds down
        send_ok = true;
        run_ms(U_SYNC_MIN_MS + 1);                  // and back

        const u_health_master_t after = u_health_master();
        check("an outage is counted once", after.outages == before.outages + 1);
        check("its length is remembered", after.longest_outage_ds >= 20);
        check("the failed sends are counted", after.failed_sends > before.failed_sends);
        check("and the worst streak with them", after.worst_fail_streak > 0);
        check("when it began is remembered",
              after.last_outage_start_s >= (t0 / 1000) && after.last_outage_start_s <= after.uptime_s);
        check("the link is up again", !u_sync_link_down());
    }

    // The transport dropping is a different and louder event than our own sends
    // failing: it is what kills the other half's keys.
    {
        const uint16_t before = u_health_master().transport_drops;
        fake_connected = false;
        run_ms(50);
        fake_connected = true;
        run_ms(50);
        check("a transport drop is counted", u_health_master().transport_drops == before + 1);
    }

    // The loop counter is what answers "is any of this costing me typing".
    run_ms(2500);
    check("the loop rate is measured", u_health_master().loops_per_s > 0);

    check("the build id is stable and non-zero", u_build_id() != 0 && u_build_id() == u_build_id());

    // The reply the other half sends back rides the transaction the master already makes.
    check("the other half's reply arrived", u_health_have_reply());
    check("and carries its uptime", u_health_slave_seen().uptime_s == 42);

    return unit_done("split_sync");
}
