// The scenario build has no panel. oled_slave_select.c, which owns the animation choice,
// needs EEPROM and the OLED driver, so the calls split_state.c and the keymap make into the
// OLED module are answered here: the boot default, and a stored page byte.
#include "oled.h"
#include "oled_settings.h"
#include "oled_canvas.h"

enum u_slave_anim u_anim_get(void) { return U_ANIM_DEFAULT; }
void              u_anim_next(void) {}
void              u_page_next(void) {}

// The stored page byte the payload carries. Not zero, so a test can tell it arrived.
uint8_t u_setting_get(enum u_setting which) { return which == U_SETTING_VIEW ? 0x13 : U_ANIM_DEFAULT; }

// The Layer map page draws here; there is no driver to commit it to.
uint8_t u_canvas[OLED_MATRIX_SIZE];
