#include QMK_KEYBOARD_H
#include "oled_canvas.h"

#ifdef OLED_ENABLE

// The geometry the renderers assume. If a future panel changes either of these, the
// block arithmetic in the art and the tests is wrong, and it should fail to build
// rather than draw nonsense.
_Static_assert(OLED_MATRIX_SIZE == 1024, "the canvas assumes a 128x64 panel");
_Static_assert(OLED_BLOCK_SIZE == 64, "the canvas assumes 16 blocks of 64 bytes");

uint8_t u_canvas[OLED_MATRIX_SIZE];

uint16_t u_canvas_commit(void) {
    // One reader for the whole buffer: oled_read_raw hands back a pointer into the
    // driver's own array, so this compares against what is really there rather than
    // against a second copy that could drift out of step with it.
    const oled_buffer_reader_t reader = oled_read_raw(0);
    const uint8_t             *have   = reader.current_element;
    uint16_t                   wrote  = 0;

    for (uint16_t i = 0; i < OLED_MATRIX_SIZE; i++) {
        if (u_canvas[i] == have[i]) continue;
        oled_write_raw_byte((char)u_canvas[i], i);
        wrote |= (uint16_t)(1u << (i / OLED_BLOCK_SIZE));
    }
    return wrote;
}

#endif     // OLED_ENABLE
