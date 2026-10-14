#pragma once
#include <stddef.h>   // bjorn_art.c uses NULL; the firmware gets it indirectly
#include <stdint.h>
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
