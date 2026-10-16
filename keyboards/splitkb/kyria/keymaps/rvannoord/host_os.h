// Which host the keyboard is plugged into. On the Mac, Ctrl and GUI swap so Cmd
// shortcuts use the same fingers as Ctrl shortcuts on Windows; nothing else may
// hardcode either. Detection sets it, SYS overrides it, the master OLED shows it.
#pragma once
#include <stdbool.h>

bool host_is_mac(void);
void host_os_toggle(void);
bool host_os_is_manual(void);   // true once SYS has overridden detection
