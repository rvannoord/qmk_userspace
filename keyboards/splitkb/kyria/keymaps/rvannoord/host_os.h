#pragma once
#include <stdbool.h>

bool host_is_mac(void);
void host_os_toggle(void);
bool host_os_is_manual(void);   // true once SYS has overridden detection
