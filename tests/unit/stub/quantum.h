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
