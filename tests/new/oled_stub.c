// The scenario build has no panel. oled_slave_select.c, which owns the animation choice,
// needs EEPROM and the OLED driver, so the two calls split_state.c and the keymap make into
// it are answered here with the boot default instead.
#include "oled.h"

enum u_slave_anim u_anim_get(void) { return U_ANIM_DEFAULT; }
void              u_anim_next(void) {}
