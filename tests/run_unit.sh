#!/usr/bin/env bash
# Host tests for the two things the GoogleTest scenarios cannot reach: the split
# transport's master half, which only exists under SPLIT_KEYBOARD, and the panel's
# drawing, which needs a buffer to draw into. No QMK tree and no toolchain beyond cc.
#
#   tests/run_unit.sh            build and run both
#   tests/run_unit.sh --dump     print the panel's frames instead of checking them
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
keymap=$(dirname "$here")/keyboards/splitkb/kyria/keymaps/rvannoord
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

cc_flags=(-std=c11 -Wall -Wextra -Werror -I "$here/unit" -I "$here/unit/stub" -I "$keymap")

# -Wno-unused-parameter only for the transport: QMK's RPC callback signature has two
# parameters a master-to-slave transaction never uses, and that is QMK's shape, not ours.
cc "${cc_flags[@]}" -Wno-unused-parameter -o "$out/split_sync" \
   "$here/unit/test_split_sync.c" "$keymap/split_sync.c"
cc "${cc_flags[@]}" -DOLED_MASTER_PANEL_ZONES -o "$out/panel" \
   "$here/unit/test_panel.c" "$keymap/panel_font.c" "$keymap/panel_glyphs.c"

case "${1:-}" in
    --dump) exec "$out/panel" --dump ;;   # the frames as text
    --raw)  exec "$out/panel" --raw  ;;   # three 1024-byte frames, for a renderer
esac

fails=0
"$out/split_sync" || fails=1
echo
"$out/panel" || fails=1
exit $fails
