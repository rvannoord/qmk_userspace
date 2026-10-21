#!/usr/bin/env bash
# Render the idle show on the host, so it can be watched before it is flashed.
#
#   tests/preview/run_preview.sh [scenario] [--invert]
#
# Scenarios: idle (type, settle, doze, switch off) · wake · tiers · props.
# Output lands in tests/preview/out/<scenario>/ — strip.png, anim.gif, frames.bin,
# events.csv — which is git-ignored.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
repo=$(dirname "$(dirname "$here")")/$(basename "$(dirname "$here")")
repo=$(cd "$here/../.." && pwd)
keymap=$repo/keyboards/splitkb/kyria/keymaps/rvannoord
scenario=${1:-idle}
invert=${2:-}

out=$here/out/$scenario
mkdir -p "$out"
bin=$(mktemp -d)/preview
trap 'rm -rf "$(dirname "$bin")"' EXIT

# OLED_SLAVE_WORLD as in custom_config.h, so the weather he stands in on the board is in
# the preview too.
cc -std=c11 -Wall -Wextra -Werror \
   -I "$repo/tests/unit" -I "$repo/tests/unit/stub" -I "$keymap" \
   -DQMK_KEYBOARD_H='"qmk_keyboard.h"' -DOLED_ENABLE -DOLED_SLAVE_ANIMATION_BJORN -DOLED_SLAVE_WORLD \
   -o "$bin" "$here/preview.c" "$keymap/art/bjorn_art.c" "$keymap/art/bjorn_poses.c" "$keymap/oled_canvas.c" \
   "$keymap/oled_slave_world.c" "$keymap/art/world_art.c"

"$bin" "$scenario" "$out"
python3 -I "$repo/tools/preview.py" "$out" $invert
