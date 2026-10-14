// The master send path. The GoogleTest scenarios build split_sync.c's
// non-SPLIT_KEYBOARD branch, where every function is a stub, so nothing there covers
// this — and a regression is silent and total: no sends at all, which on the keyboard
// looks like the other half's mascot simply going inert.
#include "unit.h"
#include "quantum.h"
#include "transactions.h"
#include "split_state.h"
#include "split_sync.h"

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

bool transaction_rpc_send(uint8_t id, uint8_t size, const void *data) {
    (void)id;
    if (!send_ok) return false;
    if (size == sizeof(u_split_state_t) && sent_n < sizeof(sent_seq)) {
        sent_seq[sent_n++] = ((const u_split_state_t *)data)->seq;
    }
    sends++;
    last_ok = fake_now;
    return true;
}

void transaction_register_rpc(uint8_t id, void (*cb)(uint8_t, const void *, uint8_t, void *)) {
    (void)id; (void)cb;
}

static void run_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        fake_now++;
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

    return unit_done("split_sync");
}
