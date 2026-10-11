// Carrying the master's state to the other half. The payload is built by
// split_state.c; this file is only the transport. See docs/layout.md, "Split sync".

#pragma once

#include "split_state.h"

// Register the transaction. Call from keyboard_pre_init_user, unconditionally on both
// halves: which one is master is decided at runtime by which has the USB cable, and the
// slave's transport thread starts before keyboard_post_init_user runs.
void u_sync_init(void);

// Every main loop. Sends on the master, ages the clock on the slave.
void u_sync_task(void);

// Slave side. The state last received, and how long ago it last changed.
u_split_state_t u_sync_state(void);
uint32_t        u_sync_age_ms(void);

// Two levels, because they are different events. Soft: the transient state is no longer
// vouched for, so drop the props but keep him alive. Hard: the link is dead.
#define U_SYNC_SOFT_MS  600
#define U_SYNC_HARD_MS 1250

bool u_sync_soft_stale(void);
bool u_sync_hard_stale(void);
