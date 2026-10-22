// The Layer map page: every key of one layer on one panel, a cell a key and the thumbs with
// their full legends. Design in docs/layout.md, "The Layer map page's legends". It draws from
// keycodes alone, so the scenario tests can put every Miryoku layer through it.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define U_PAGE_SLOTS 40     // Miryoku's: three rows of ten, then the thumb row

// The whole frame, into the canvas; the caller commits. `keys` is the layer's keycodes in
// slot order, `base` the default layer's, and bit s of `down` says slot s is pressed. The
// held layer key, a pressed slot whose default keycode is a layer-tap to `layer`, shows its
// default legend.
void u_layer_page_draw(uint8_t layer, const char *name, const uint16_t keys[U_PAGE_SLOTS],
                       const uint16_t base[U_PAGE_SLOTS], uint64_t down, bool mac);
