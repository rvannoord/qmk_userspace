// Just enough QMK for split_sync.c. The clock is a variable, so a test can step
// through a heartbeat window without sleeping.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#define SPLIT_KEYBOARD 1

typedef struct { uint16_t dummy; } keyrecord_t;

extern uint32_t fake_now;
extern bool     fake_is_master;
extern bool     fake_is_left;

static inline uint32_t timer_read32(void) { return fake_now; }
static inline uint32_t timer_elapsed32(uint32_t then) { return fake_now - then; }
static inline bool     is_keyboard_master(void) { return fake_is_master; }
static inline bool     is_keyboard_left(void)   { return fake_is_left; }

// The panel test drives the driver through stub/driver.h, which models the buffer and
// its dirty blocks; power is not part of that model, so this is a no-op here. The real
// one comes from oled_driver.h via quantum.h in the firmware build.
static inline void     oled_off(void)            { }

// The panel's size, for the drawing files that include this header rather than
// qmk_keyboard.h; the same values as there.
#ifndef OLED_DISPLAY_WIDTH
#    define OLED_DISPLAY_WIDTH  128
#    define OLED_DISPLAY_HEIGHT 64
#    define OLED_MATRIX_SIZE    1024
#endif

// QMK's layer-tap encoding (quantum/keycodes.h, quantum_keycodes.h), for the Layer map page.
#define QK_LAYER_TAP               0x4000
#define QK_LAYER_TAP_MAX           0x4FFF
#define IS_QK_LAYER_TAP(code)      ((code) >= QK_LAYER_TAP && (code) <= QK_LAYER_TAP_MAX)
#define QK_LAYER_TAP_GET_LAYER(kc) (((kc) >> 8) & 0xF)
#define LT(layer, kc)              (QK_LAYER_TAP | (((layer) & 0xF) << 8) | ((kc) & 0xFF))
