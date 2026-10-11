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

u_split_state_t u_sync_build(void) { return drive; }
void            u_sync_note_record(uint16_t keycode, keyrecord_t *record) { (void)keycode; (void)record; }

bool transaction_rpc_send(uint8_t id, uint8_t size, const void *data) {
    (void)id; (void)size; (void)data;
    if (!send_ok) return false;
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

    return unit_done("split_sync");
}
