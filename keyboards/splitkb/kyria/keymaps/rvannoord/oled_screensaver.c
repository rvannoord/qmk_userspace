// The screensaver: drashna's matrix rain, over whatever each half shows from U_RAIN_AFTER_MS
// of idle until the panels switch off (docs/layout.md, "A screensaver step"). Its frame comes
// from the idle clock both halves share, so the two fall in step without saying so.

#include QMK_KEYBOARD_H
#include "oled.h"
#include "oled_canvas.h"
#include "art/matrix_art.h"

#ifdef OLED_ENABLE

#    define U_RAIN_FRAME_MS 50      // the render tick: 3.6 s a loop, against drashna's 2.2 s

void u_rain_draw(uint32_t idle_ms) {
    const uint8_t f = (uint8_t)(((idle_ms - U_RAIN_AFTER_MS) / U_RAIN_FRAME_MS) % U_MATRIX_FRAMES);
    for (uint16_t i = 0; i < OLED_MATRIX_SIZE; i++) u_canvas[i] = pgm_read_byte(&u_matrix[f][i]);
}

#endif     // OLED_ENABLE
