// The state the master gathers for the slave half: modifiers in finger order, the
// active and default layers, lock and Caps Word state, the host OS, and a count of
// home-row misfires. Built on the master every main loop, shipped across the
// split link by split_sync.c, and rendered from on both halves. See docs/layout.md.

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t seq;       // advances on every build; only ever tested for inequality
    uint8_t fingers;   // see below
    uint8_t layers;    // low nibble active, high nibble default
    uint8_t flags;     // see below
    uint8_t anomaly;   // misfire count, wraps; only ever tested for change
    uint8_t world;     // see below
} u_split_state_t;

// fingers: outer to inner, the order the panel's cells are in.
#define U_FIN_L_PINKY  0x01
#define U_FIN_L_RING   0x02
#define U_FIN_L_MIDDLE 0x04
#define U_FIN_L_INDEX  0x08
#define U_FIN_R_PINKY  0x10
#define U_FIN_R_RING   0x20
#define U_FIN_R_MIDDLE 0x40
#define U_FIN_R_INDEX  0x80
#define U_FIN_CELLS(f) (((f) | ((f) >> 4)) & 0x0F)   // the four display cells
#define U_FIN_LEFT(f)  ((f) & 0x0F)
#define U_FIN_RIGHT(f) (((f) >> 4) & 0x0F)

#define U_FLAG_NUM_LOCK    0x01
#define U_FLAG_CAPS_LOCK   0x02
#define U_FLAG_SCROLL_LOCK 0x04
#define U_FLAG_CAPS_WORD   0x08
#define U_FLAG_IS_MAC      0x10
#define U_FLAG_OS_MANUAL   0x20
#define U_FLAG_ALTGR_L     0x40
#define U_FLAG_ALTGR_R     0x80

// `world` carries what the slave half needs to know but cannot see. Keypresses
// are processed on the master, so the chosen animation has to cross the link -
// the half that is told is not the half that draws.
//
// This widened the struct from 5 bytes to 6 rather than adding a transaction.
// NUM_TOTAL_TRANSACTIONS is the last member of an enum in QMK's
// transaction_id_define.h, so it counts transaction *ids*, not bytes: widening
// a payload adds no enum member and the handshake constant is untouched. That
// matters because the constant is XORed into every handshake, so changing it
// makes a mismatched pair stop typing entirely. RPC_M2S_BUFFER_SIZE is 32, so
// there is room to grow.
//
// The upper bits are deliberately spare: the weather and time-of-day the host
// daemon will send belong here too, and this is the byte they go in.
#define U_WORLD_ANIM_MASK  0x07     // which slave animation, 0-4
#define U_WORLD_ANIM(w)    ((w) & U_WORLD_ANIM_MASK)

// Call from process_record_user on the master. Tracks which finger holds which
// modifier, and counts misfires.
void u_sync_note_record(uint16_t keycode, keyrecord_t *record);

// Build the current state. Master only; cheap enough to call every main loop.
u_split_state_t u_sync_build(void);
