// Stands in for QMK_KEYBOARD_H when the panel is compiled on the host: the OLED
// surface it draws through, and a buffer laid out exactly as the driver's is.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "progmem.h"

#define OLED_DISPLAY_WIDTH  128
#define OLED_DISPLAY_HEIGHT 64

typedef struct { uint16_t dummy; } keyrecord_t;

extern uint8_t fake_oled[OLED_DISPLAY_WIDTH * 8];
void oled_write_raw_byte(const char data, uint16_t index);

// The panel latches edges against the clock, so a test has to be able to move it.
extern uint32_t fake_now;
static inline uint32_t timer_read32(void) { return fake_now; }
static inline uint32_t timer_elapsed32(uint32_t then) { return fake_now - then; }
