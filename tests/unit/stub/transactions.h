#pragma once
#include <stdint.h>
#include <stdbool.h>

#define U_SYNC_STATE 0

bool transaction_rpc_send(uint8_t id, uint8_t size, const void *data);
void transaction_register_rpc(uint8_t id, void (*cb)(uint8_t, const void *, uint8_t, void *));
