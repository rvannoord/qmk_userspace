#!/usr/bin/env bash
# Host tests for the two things the GoogleTest scenarios cannot reach: the split
# transport's master half, which only exists under SPLIT_KEYBOARD, and the panel's
# drawing, which needs a buffer to draw into. No QMK tree and no toolchain beyond cc.
#
#   tests/run_unit.sh            build and run all eight
#   tests/run_unit.sh --dump     print the panel's frames instead of checking them
#   tests/run_unit.sh --raw      the panel's frames as raw buffers; --raw-map the live maps
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
keymap=$(dirname "$here")/keyboards/splitkb/kyria/keymaps/rvannoord
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

# QMK_KEYBOARD_H and OLED_ENABLE on the command line, because oled_canvas.c is its own
# translation unit here and never sees the test file that would otherwise define them.
cc_flags=(-std=c11 -Wall -Wextra -Werror -I "$here/unit" -I "$here/unit/stub" -I "$keymap"
          -DQMK_KEYBOARD_H='"qmk_keyboard.h"' -DOLED_ENABLE -DMIRYOKU_MAPPING_EXTENDED_THUMBS)

# -Wno-unused-parameter only for the transport: QMK's RPC callback signature has two
# parameters a master-to-slave transaction never uses, and that is QMK's shape, not ours.
cc "${cc_flags[@]}" -Wno-unused-parameter -o "$out/split_sync" \
   "$here/unit/test_split_sync.c" "$keymap/split_sync.c" "$keymap/split_health.c"
cc "${cc_flags[@]}" -o "$out/panel" \
   "$here/unit/test_panel.c" "$keymap/art/panel_font.c" "$keymap/art/panel_glyphs.c" "$keymap/art/panel_layer_icons.c" "$keymap/art/drashna_icons.c" \
   "$keymap/art/kitty_art.c" "$keymap/art/monitor_art.c" "$keymap/art/status_frame.c" \
   "$keymap/oled_canvas.c" "$keymap/oled_draw.c" "$keymap/split_health.c" "$keymap/oled_map.c" "$keymap/oled_layer_page.c" \
   "$keymap/oled_pages.c" "$keymap/oled_event_log.c"

cc "${cc_flags[@]}" -DOLED_SLAVE_ANIMATION_BJORN -o "$out/bjorn" \
   "$here/unit/test_bjorn.c" "$keymap/art/bjorn_art.c" "$keymap/art/bjorn_poses.c" "$keymap/oled_canvas.c"

# The other half's side of the transport, from boot.
cc "${cc_flags[@]}" -Wno-unused-parameter -o "$out/sync_slave" \
   "$here/unit/test_sync_slave.c" "$keymap/split_sync.c" "$keymap/split_health.c"

# The live layer map, from oled_map.c itself.
cc "${cc_flags[@]}" -o "$out/map" \
   "$here/unit/test_map.c" "$keymap/art/panel_font.c" "$keymap/art/drashna_icons.c" "$keymap/oled_canvas.c" "$keymap/oled_draw.c" "$keymap/oled_layer_page.c"

# Bongo cat and the Mario scene, from their own files.
cc "${cc_flags[@]}" -o "$out/scenes" \
   "$here/unit/test_scenes.c" "$keymap/art/bongo_art.c" "$keymap/art/mario_art.c" "$keymap/art/panel_font.c" \
   "$keymap/art/drashna_icons.c" "$keymap/oled_canvas.c" "$keymap/oled_draw.c"

# The bootloader screen, from oled.c itself.
cc "${cc_flags[@]}" -o "$out/boot" \
   "$here/unit/test_boot.c" "$keymap/art/panel_font.c" "$keymap/oled_canvas.c"

# -Wno-unused-parameter: the spaceship's oled_set_cursor stub takes QMK's (col, line).
cc "${cc_flags[@]}" -Wno-unused-parameter -o "$out/space" \
   "$here/unit/test_space.c" "$keymap/art/space_art.c"

case "${1:-}" in
    --dump) exec "$out/panel" --dump ;;   # the frames as text
    --raw)  exec "$out/panel" --raw  ;;   # three 1024-byte frames, for a renderer
    --raw-map) exec "$out/map" --raw ;;   # the live maps D15 was decided from
    --raw-pages) exec "$out/panel" --raw-pages "$2" ;;   # stage 5's mock frames, fed a WPM series
esac

fails=0

# Every art file compiles on its own, including the ones nothing draws yet: drashna's
# catalogue is generated ahead of use (docs/layout.md, "drashna's art, all of it"), and
# art that only compiles once something includes it would rot unnoticed.
art_ok=1
for f in "$keymap"/art/*.c; do
    cc "${cc_flags[@]}" -I "$keymap/art" -c "$f" -o "$out/art.o" || { echo "  FAIL $f"; art_ok=0; }
done
if [ $art_ok = 1 ]; then echo "art: all good"; else echo "art: FAILED"; fails=1; fi
echo
"$out/split_sync" || fails=1
echo
"$out/sync_slave" || fails=1
echo
"$out/panel" || fails=1
echo
"$out/map" || fails=1
echo
"$out/space" || fails=1
echo
"$out/bjorn" || fails=1
echo
"$out/scenes" || fails=1
echo
"$out/boot" || fails=1
exit $fails
