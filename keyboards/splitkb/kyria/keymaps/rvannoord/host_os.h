// Which host the keyboard is plugged into. On the Mac, Ctrl and GUI swap so Cmd
// shortcuts use the same fingers as Ctrl shortcuts on Windows; nothing else may
// hardcode either. Detection sets it, SYS overrides it, the master OLED shows it.
#pragma once
#include <stdbool.h>
#include <stdint.h>

// What the panel shows. Behaviour knows two kinds only, Mac or not: iOS is a Mac
// (iPadOS shortcuts are Cmd-based) and Linux is not. QMK reports Android as Linux.
typedef enum {
    U_HOST_WINDOWS,     // also what an unsure detection gives
    U_HOST_MAC,
    U_HOST_IOS,
    U_HOST_LINUX,
} u_host_t;

bool     host_is_mac(void);
u_host_t host_kind(void);
void     host_os_toggle(void);
bool     host_os_is_manual(void);   // true once SYS has overridden detection
bool     host_os_detected(void);    // true once detection has reported, whatever it said
