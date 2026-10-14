#pragma once
#include <stdbool.h>
// QMK's own: false while the standard transport has given up, which is also when the
// master zeroes the other half's matrix and its keys stop working.
bool is_transport_connected(void);
