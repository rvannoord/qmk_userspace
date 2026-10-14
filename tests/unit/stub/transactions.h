#pragma once
#include <stdint.h>
#include <stdbool.h>

#define U_SYNC_STATE 0

// The master now asks for a reply as well, so the state transaction carries the other
// half's health counters back. GET_RPC_RESP_DATA already ran on every send with zero
// bytes, so this is wire time and no new failure point.
bool transaction_rpc_exec(uint8_t id, uint8_t m2s_size, const void *m2s,
                          uint8_t s2m_size, void *s2m);
void transaction_register_rpc(uint8_t id, void (*cb)(uint8_t, const void *, uint8_t, void *));
