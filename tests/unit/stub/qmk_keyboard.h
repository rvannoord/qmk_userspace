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

// The few QMK calls the master's pages make: user EEPROM, one word, and the WPM. The
// tests own both, which is how a replug and a typing speed are staged.
uint32_t eeconfig_read_user(void);
void     eeconfig_update_user(uint32_t val);
uint8_t  get_current_wpm(void);

// What the live layer map asks of QMK: where a key is, what it does on a layer, and
// whether it is down. Each test that draws a map answers the last two itself.
typedef struct {
    uint8_t col;
    uint8_t row;
} keypos_t;

#define KC_NO     0x0000
#define QK_USER_0 0x7E40
#define QK_USER_1 0x7E41
#define QK_USER_2 0x7E42

#include "kyria_layout.h"
#include "config.h"          // the keymap's: LAYOUT_miryoku, as the build force-includes it

uint16_t keymap_key_to_keycode(uint8_t layer, keypos_t key);
bool     matrix_is_on(uint8_t row, uint8_t col);
