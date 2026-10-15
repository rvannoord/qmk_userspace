// Stands in for QMK_KEYBOARD_H when the panel is compiled on the host: the OLED
// surface it draws through, and a buffer laid out exactly as the driver's is.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "progmem.h"

#define OLED_DISPLAY_WIDTH  128
#define OLED_DISPLAY_HEIGHT 64
#define OLED_MATRIX_SIZE    1024
#define OLED_BLOCK_SIZE     64
#define OLED_TIMEOUT        120000

typedef struct { uint16_t dummy; } keyrecord_t;

// One byte longer than the panel: the driver clamps index >= OLED_MATRIX_SIZE to
// OLED_MATRIX_SIZE and then writes there (oled_driver.c:668), one past the end of its
// own buffer. The extra byte is a canary, and the tests assert it never changes.
extern uint8_t  fake_oled[OLED_MATRIX_SIZE + 1];
extern uint16_t fake_dirty;

void oled_write_raw_byte(const char data, uint16_t index);

typedef struct {
    const uint8_t *current_element;
    uint16_t       remaining_element_count;
} oled_buffer_reader_t;

static inline oled_buffer_reader_t oled_read_raw(uint16_t start_index) {
    if (start_index > OLED_MATRIX_SIZE) start_index = OLED_MATRIX_SIZE;
    return (oled_buffer_reader_t){&fake_oled[start_index],
                                  (uint16_t)(OLED_MATRIX_SIZE - start_index)};
}

// The panel latches edges against the clock, so a test has to be able to move it.
// Which half this panel is on. The mod row reverses with it, because outer-to-inner
// runs left-to-right on the left half and right-to-left on the right one.
extern bool fake_is_left;
static inline bool is_keyboard_left(void) { return fake_is_left; }

extern uint32_t fake_now;
static inline uint32_t timer_read32(void) { return fake_now; }
static inline uint32_t timer_elapsed32(uint32_t then) { return fake_now - then; }
