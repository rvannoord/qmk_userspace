// Gathering the state the slave half needs. See split_state.h and docs/layout.md.

#include "quantum.h"
#include "split_state.h"
#include "host_os.h"
#include "oled.h"
#include "oled_settings.h"

#define U_ANOMALY_MS 500         // a hold this long with nothing typed is a misfire
#define U_HOLDS       4          // deepest chord worth tracking

// Which finger a modifier belongs to. The slot comes from the key's DECLARED mod,
// before mod_config() swaps anything, so a slot never moves when the OS changes —
// only its label does. Liveness is checked against the post-swap bits, below.
static uint8_t slot_of_mod(uint8_t mod) {
    switch (mod & 0x0F) {
        case MOD_LGUI: return U_FIN_L_PINKY;
        case MOD_LALT: return U_FIN_L_RING;
        case MOD_LCTL: return U_FIN_L_MIDDLE;
        case MOD_LSFT: return U_FIN_L_INDEX;
        default:       return 0;
    }
}

// QMK packs mods into 5 bits, bit 4 meaning "right hand". get_mods() returns the
// 8-bit form, so expand before testing. Mirrors quantum/keymap_common.c.
static uint8_t expand_mod(uint8_t mod) {
    return (mod & 0x10) ? ((uint8_t)(mod & 0x0F) << 4) : mod;
}

static uint8_t  tracked;             // finger bits we have seen pressed
static uint8_t  altgr;              // U_FLAG_ALTGR_L / _R
static uint8_t  anomalies;          // published count
static uint16_t last_payload_at;    // event.time of the last thing worth modifying
static bool     had_payload;

static struct {
    bool     active;
    uint8_t  slot;
    uint8_t  live;                  // the mod bit this key actually registered, expanded
    uint16_t at;                    // event.time of the press
} holds[U_HOLDS];

static uint8_t declared_mod(uint16_t keycode) {
    if (IS_QK_MOD_TAP(keycode)) return QK_MOD_TAP_GET_MODS(keycode);
    if (IS_MODIFIER_KEYCODE(keycode)) return MOD_BIT(keycode);
    return 0;
}

// A companion is something that was typed, i.e. something a modifier could modify.
// A mod-tap or layer-tap that resolved as a HOLD is not one: a pair of mods held
// with nothing typed between them is itself the misfire we are looking for.
static bool counts_as_payload(uint16_t keycode, keyrecord_t *record) {
    if (IS_QK_MOD_TAP(keycode) || IS_QK_LAYER_TAP(keycode)) return record->tap.count > 0;
    return true;
}

void u_sync_note_record(uint16_t keycode, keyrecord_t *record) {
    const uint8_t mod  = declared_mod(keycode);
    const bool    held = mod && (!IS_QK_MOD_TAP(keycode) || record->tap.count == 0);

    if (record->event.pressed && counts_as_payload(keycode, record)) {
        // Timestamped, not counted. process_record_user does not run at the physical press
        // of a mod-tap — it runs when the tap-or-hold decision is made — and when the
        // companion key is what forces that decision, the companion is processed first.
        // A counter snapshotted at hold registration therefore misses the very key that
        // caused the hold. Event times are correct whatever order the records arrive in.
        last_payload_at = record->event.time;
        had_payload     = true;
    }
    if (!mod) return;

    const bool right = chordal_hold_handedness(record->event.key) == 'R';

    // AltGr is MOD_RALT, which is MOD_LALT with the right-hand bit set — so it must be
    // tested on the whole value, not on the low nibble, or it reads as plain Alt and
    // lands in the ring-finger cell. It sits on the bottom row, not the home row, so it
    // gets its own flag rather than a finger slot.
    const bool    is_altgr = (mod & 0x10) && ((mod & 0x0F) == MOD_LALT);
    const uint8_t agr      = is_altgr ? (right ? U_FLAG_ALTGR_R : U_FLAG_ALTGR_L) : 0;
    const uint8_t bit      = is_altgr ? 0
                                      : (right ? (uint8_t)(slot_of_mod(mod) << 4) : slot_of_mod(mod));

    if (record->event.pressed) {
        if (!held) return;                       // a tap is not a held modifier
        tracked |= bit;
        altgr   |= agr;
        for (uint8_t i = 0; i < U_HOLDS; i++) {
            if (!holds[i].active) {
                // Record the bit this key actually registered, not the one it would
                // register now: mod_config() is applied at press time, so an OS change
                // while the key is held must not re-attribute it to another finger.
                holds[i].active  = true;
                holds[i].slot    = bit;
                holds[i].live    = expand_mod(mod_config(mod));
                holds[i].at      = record->event.time;
                break;
            }
        }
    } else {
        tracked &= (uint8_t)~bit;
        altgr   &= (uint8_t)~agr;
        for (uint8_t i = 0; i < U_HOLDS; i++) {
            if (holds[i].active && holds[i].slot == bit) {
                // Ordering cannot be trusted here — action_tapping.c processes a rolled
                // press of three tap-hold keys out of order — so ask whether anything was
                // typed during the hold, not whether a particular key came after it.
                const uint16_t held_for = TIMER_DIFF_16(record->event.time, holds[i].at);
                // Was anything typed between the press and the release? A payload from
                // before the hold gives a difference near 2^16, so it fails this on its own.
                const bool used = had_payload &&
                                  TIMER_DIFF_16(last_payload_at, holds[i].at) <= held_for;
                if (!used && held_for > U_ANOMALY_MS) {
                    // Held long, nothing typed: the signature of a home-row misfire.
                    anomalies++;
                }
                holds[i].active = false;
                break;
            }
        }
    }
}

// A build is pure: two builds with nothing in between are equal. The sequence number
// is stamped by the sender instead, because the master builds twice per main loop (once
// to decide whether anything changed, once for the panel) and a counter that advances
// per build makes the number say how often we looked rather than how often we sent.
u_split_state_t u_sync_build(void) {
    const uint8_t mods = get_mods();

    // Publish only what is both tracked and actually held. A tracker bit that leaked —
    // a release swallowed by the tap-hold queue — can then never light a cell for a
    // modifier that is not down.
    uint8_t fingers = 0;
    uint8_t covered = 0;
    for (uint8_t i = 0; i < U_HOLDS; i++) {
        if (holds[i].active && (mods & holds[i].live)) {
            fingers |= holds[i].slot;
            covered |= holds[i].live;
        }
    }

    // A modifier that is held but that the tracker missed still lights its cell, with no
    // hand attributed. "Something is held, hand unknown" beats showing nothing.
    static const uint8_t declared[4] = {MOD_LGUI, MOD_LALT, MOD_LCTL, MOD_LSFT};
    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t live = expand_mod(mod_config(declared[i]));
        if ((mods & live) && !(covered & live)) fingers |= (uint8_t)(1u << i);
    }

    const led_t led = host_keyboard_led_state();
    uint8_t     flags = altgr;
    if (led.num_lock)       flags |= U_FLAG_NUM_LOCK;
    if (led.caps_lock)      flags |= U_FLAG_CAPS_LOCK;
    if (led.scroll_lock)    flags |= U_FLAG_SCROLL_LOCK;
#ifdef CAPS_WORD_ENABLE
    // Polled, never read from process_record_user: process_caps_word runs thirty lines
    // after process_record_kb in the same chain, so anything read there is the value
    // from before the key.
    if (is_caps_word_on())  flags |= U_FLAG_CAPS_WORD;
#endif
    if (host_is_mac())      flags |= U_FLAG_IS_MAC;
    if (host_os_is_manual())flags |= U_FLAG_OS_MANUAL;

    return (u_split_state_t){
        .seq     = 0,               // stamped by u_sync_master_task on the way out
        .fingers = fingers,
        .layers  = (uint8_t)(get_highest_layer(layer_state | default_layer_state) |
                             (get_highest_layer(default_layer_state) << 4)),
        .flags   = flags,
        .anomaly = anomalies,
        // The slave cannot see keypresses, so the chosen animation crosses here.
        .world   = (uint8_t)u_anim_get(),
        .host    = (uint8_t)host_kind(),
        // As stored, not as shown, so the other half stores exactly what this one does.
        .view    = u_setting_get(U_SETTING_VIEW),
    };
}
