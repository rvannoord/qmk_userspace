// The driver's write, mirrored exactly, so the tests see dirty blocks the way the panel
// does. This is what makes "an unchanged frame writes nothing" checkable on the host:
// the real oled_write_raw_byte returns early only when the byte already matches, and
// marks the block dirty otherwise (oled_driver.c:667-672).
#pragma once
#include "qmk_keyboard.h"

static inline void u_fake_write(const char data, uint16_t index) {
    if (index > OLED_MATRIX_SIZE) index = OLED_MATRIX_SIZE;   // the driver's clamp
    if (fake_oled[index] == (uint8_t)data) return;            // compare as uint8_t: char is signed here
    fake_oled[index] = (uint8_t)data;
    fake_dirty |= (uint16_t)(1u << (index / OLED_BLOCK_SIZE));
}
