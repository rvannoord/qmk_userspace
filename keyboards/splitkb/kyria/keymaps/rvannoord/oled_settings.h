// What the panels remember across a replug: one tagged word of user EEPROM, shared by
// the slave's animation and the master's page. Kept apart from both so neither can
// write the word without the other's byte, and so the tests can replug it.
//
// Each half has its own EEPROM. The master writes what you pick, and the other half
// writes what the master sends (u_settings_follow), so the settings follow the cable to
// whichever half has it next. See docs/layout.md, "Switching the slave's animation".

#pragma once

#include <stdint.h>

enum u_setting {
    U_SETTING_ANIM,     // byte 0: the slave's animation, an enum u_slave_anim
    U_SETTING_VIEW,     // byte 1: the master's page in bits 0-3, the map style in bit 4
};

#define U_VIEW_PAGE_MASK 0x0fu

uint8_t u_setting_get(enum u_setting which);
void    u_setting_set(enum u_setting which, uint8_t value);

// On the slave, with what a fresh state from the master carried. One write, and only when
// something differs.
void u_settings_follow(uint8_t anim, uint8_t view);
