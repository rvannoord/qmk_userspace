// The state the master gathers for the slave half: modifiers in finger order, the
// active and default layers, lock and Caps Word state, the host OS, and a count of
// home-row misfires. Built on the master every main loop; stage 2 ships it across
// the split link, and the master panel already renders from it. See docs/layout.md.

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t seq;       // advances on every build; only ever tested for inequality
    uint8_t fingers;   // see below
    uint8_t layers;    // low nibble active, high nibble default
    uint8_t flags;     // see below
    uint8_t anomaly;   // misfire count, wraps; only ever tested for change
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

// Call from process_record_user on the master. Tracks which finger holds which
// modifier, and counts misfires.
void u_sync_note_record(uint16_t keycode, keyrecord_t *record);

// Build the current state. Master only; cheap enough to call every main loop.
u_split_state_t u_sync_build(void);
