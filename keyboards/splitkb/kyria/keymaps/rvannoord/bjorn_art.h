// Bjorn, generated from his own sprites. Do not edit by hand.
//
// Artwork: Copyright (c) infinition, from the Bjorn project
// <https://github.com/infinition/Bjorn>, MIT licensed. Box-filtered from 78x78
// to 64x64 and dithered; see docs/layout.md and VENDORED.md.

#pragma once

#include <stdint.h>
#include "progmem.h"

#define BJORN_PATCH_BYTES 64      // one dirty block: 64 columns of one page

// `block` is the driver's own dirty-block index, buffer_index / 64. Bjorn owns the
// left column, so his blocks are the even ones; block 15 is the right column's page 7.
typedef struct {
    uint8_t     block;
    const char *data;
} bjorn_patch_t;

typedef struct {
    const bjorn_patch_t *patches;
    uint8_t              count;
} bjorn_frame_t;

enum bjorn_frame_id {
    BJORN_F_POSE_T0,
    BJORN_F_POSE_T1,
    BJORN_F_POSE_T2,
    BJORN_F_POSE_T3,
    BJORN_F_BLINK_T0,
    BJORN_F_BLINK_T1,
    BJORN_F_BLINK_T2,
    BJORN_F_BLINK_T3,
    BJORN_F_BLINK_HALF,
    BJORN_F_BLINK_FULL,
    BJORN_F_BREATH_HALF,
    BJORN_F_BREATH,
    BJORN_F_DOZE,
    BJORN_F_DOZE_BREATH,
    BJORN_F_FLOOR_RIGHT,
    BJORN_FRAME_COUNT
};

extern const char          PROGMEM bjorn_base[512];
extern const bjorn_frame_t         bjorn_frames[BJORN_FRAME_COUNT];
