// The other half's side of the transport: whether what arrived may be acted on, not only
// drawn. The slave saves the master's page and animation from it, so a state that is not
// really there yet must never count. Its own program, because the window that matters is
// the first 600 ms after boot, and test_split_sync.c has long left it by the time it could
// switch halves.
#include "unit.h"
#include "quantum.h"
#include "transactions.h"
#include "split_state.h"
#include "split_sync.h"
#include "split_health.h"

uint32_t fake_now       = 0;
bool     fake_is_master = false;
bool     fake_is_left   = false;

u_split_state_t u_sync_build(void) { return (u_split_state_t){0}; }
void            u_sync_note_record(uint16_t keycode, keyrecord_t *record) { (void)keycode; (void)record; }
bool            is_transport_connected(void) { return true; }

bool transaction_rpc_exec(uint8_t id, uint8_t m2s_size, const void *m2s, uint8_t s2m_size, void *s2m) {
    (void)id; (void)m2s_size; (void)m2s; (void)s2m_size; (void)s2m;
    return false;
}

// Kept, so the test can deliver states the way the transport's thread would.
static void (*slave_cb)(uint8_t, const void *, uint8_t, void *);
void transaction_register_rpc(uint8_t id, void (*cb)(uint8_t, const void *, uint8_t, void *)) {
    (void)id;
    slave_cb = cb;
}

static void deliver(const u_split_state_t *s, uint8_t size) {
    u_health_slave_t reply;
    slave_cb(size, s, sizeof(reply), &reply);
}

static void run_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        fake_now++;
        u_sync_task();
    }
}

int main(void) {
    u_sync_init();

    // Inside the first 600 ms the clock alone says "not stale", and the slots are zeros
    // that read as the first animation and the first page.
    run_ms(10);
    check("at boot, before anything arrives, nothing is fresh", !u_sync_fresh());

    const u_split_state_t zeros = {0};
    deliver(&zeros, sizeof(zeros));
    run_ms(10);
    check("a state the slots already held is not news", !u_sync_fresh());

    const u_split_state_t first = {.seq = 1, .world = 3, .view = 0x13};
    deliver(&first, sizeof(first));
    run_ms(1);
    check("the first real state is fresh", u_sync_fresh());
    check("and carries the stored page byte", u_sync_state().view == 0x13 && u_sync_state().world == 3);

    const u_split_state_t wrong = {.seq = 2, .view = 0x02};
    deliver(&wrong, sizeof(wrong) - 1);
    run_ms(1);
    check("a state of the wrong size is dropped", u_sync_state().view == 0x13);

    run_ms(U_SYNC_SOFT_MS);
    check("it goes stale with the master's silence", !u_sync_fresh());
    const u_split_state_t next = {.seq = 2, .view = 0x13};
    deliver(&next, sizeof(next));
    run_ms(1);
    check("and is fresh again with the next state", u_sync_fresh());

    return unit_done("sync_slave");
}
