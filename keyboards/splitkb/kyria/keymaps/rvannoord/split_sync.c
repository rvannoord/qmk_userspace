// Transport for the state in split_state.h. See docs/layout.md, "Split sync".

#include "quantum.h"
#include "split_sync.h"

#ifdef SPLIT_KEYBOARD

#    include "transactions.h"

#    define U_SYNC_MIN_MS        20    // floor between sends: caps cost during a flurry
#    define U_SYNC_HEARTBEAT_MS 250    // so a quiet master is distinguishable from a dead one

// Two slots and an index, rather than one struct. The callback runs in a HIGHPRIO
// thread and can land between any two instructions of the reader, so a single struct
// yields torn reads — one frame composed from a mix of old and new state. Eleven bytes
// makes that impossible by construction.
static volatile u_split_state_t slots[2];
static volatile uint8_t         slot_live;

// File scope so the panel can ask about the link. On the master the only evidence the
// other half is there is transaction_rpc_send returning true, so "when a send last
// succeeded" is the whole health signal. It starts at zero, which reads as uptime until
// the first success — so a half that was never there is reported, rather than excused.
static uint32_t last_sent_at;
static bool     primed;

static uint32_t last_seen_at;          // when the sequence number last changed
static uint8_t  last_seen_seq;

static void u_sync_slave_cb(uint8_t m2s_size, const void *m2s, uint8_t s2m_size, void *s2m) {
    // Length first, always. It arrives off the wire protected only by a CRC8 over three
    // bytes, so a collision can deliver a wrong one — and while the source is bounded by
    // the RPC buffer, the destination is not.
    if (m2s_size != sizeof(u_split_state_t)) return;

    const uint8_t next = slot_live ^ 1u;
    memcpy((void *)&slots[next], m2s, sizeof(u_split_state_t));
    __atomic_signal_fence(__ATOMIC_RELEASE);     // never publish a half-written slot
    slot_live = next;
}

void u_sync_init(void) {
    transaction_register_rpc(U_SYNC_STATE, u_sync_slave_cb);
}

u_split_state_t u_sync_state(void) {
    const uint8_t i = slot_live;                 // read the index once, then the slot
    return (u_split_state_t){slots[i].seq, slots[i].fingers, slots[i].layers,
                             slots[i].flags, slots[i].anomaly};
}

uint32_t u_sync_age_ms(void) {
    return timer_elapsed32(last_seen_at);
}

bool u_sync_soft_stale(void) {
    return u_sync_age_ms() >= U_SYNC_SOFT_MS;
}

bool u_sync_hard_stale(void) {
    return u_sync_age_ms() >= U_SYNC_HARD_MS;
}

// Asked by both panels, and it means the same thing on each: this half has not heard
// from the other one for long enough to stop pretending. The evidence differs — the
// master watches its own sends, the slave watches the sequence number.
bool u_sync_link_down(void) {
    if (!is_keyboard_master()) return u_sync_hard_stale();
    return timer_elapsed32(last_sent_at) >= U_SYNC_HARD_MS;
}

static void u_sync_master_task(void) {
    static u_split_state_t last_sent;

    u_split_state_t next = u_sync_build();
    const uint32_t  age  = timer_elapsed32(last_sent_at);

    // Compare everything but seq, which the build leaves at zero and this function
    // stamps on the way out.
    const bool changed = !primed || memcmp(&next.fingers, &last_sent.fingers,
                                           sizeof(u_split_state_t) - offsetof(u_split_state_t, fingers)) != 0;
    if (!((changed && age >= U_SYNC_MIN_MS) || age >= U_SYNC_HEARTBEAT_MS)) return;

    // One step per delivered state, so the slave's staleness clock measures what
    // actually arrived. last_sent only advances on success, so a failed send repeats
    // the same number rather than burning one.
    next.seq = (uint8_t)(last_sent.seq + 1);

    if (!transaction_rpc_send(U_SYNC_STATE, sizeof(next), &next)) {
        // There is no retry inside transaction_rpc_send, so leaving the timestamp and the
        // payload untouched is the retry: the next main loop is microseconds away. Waiting
        // for the heartbeat window would turn one noisy moment into a 250 ms gap.
        return;
    }
    last_sent    = next;
    last_sent_at = timer_read32();
    primed       = true;
}

static void u_sync_slave_task(void) {
    const uint8_t seq = slots[slot_live].seq;
    if (seq != last_seen_seq) {
        last_seen_seq = seq;
        last_seen_at  = timer_read32();
    }
}

void u_sync_task(void) {
    if (is_keyboard_master()) {
        u_sync_master_task();
    } else {
        u_sync_slave_task();
    }
}

#else     // not a split build (the scenario tests); the state still builds, it just goes nowhere

void            u_sync_init(void) {}
void            u_sync_task(void) {}
u_split_state_t u_sync_state(void) { return u_sync_build(); }
uint32_t        u_sync_age_ms(void) { return 0; }
bool            u_sync_soft_stale(void) { return false; }
bool            u_sync_hard_stale(void) { return false; }
bool            u_sync_link_down(void) { return false; }

#endif
