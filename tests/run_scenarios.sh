#!/usr/bin/env bash
# Runs the scenario tests (MIGRATION.md step 7) against a qmk_firmware tree:
#   tests/run_scenarios.sh new [tree]   current QMK with this userspace's keymap code (default ~/qmk_firmware)
#   tests/run_scenarios.sh old [tree]   the old fork at archive/miryoku-with-latest-qmk (default ~/miryoku_qmk_old)
# QMK only finds tests inside its own tests/ folder, so the files are staged into
# <tree>/tests/rvannoord_hrm/ for the run and removed afterwards.
set -euo pipefail

usage="usage: $0 new|old [qmk_firmware tree]"
mode=${1:?$usage}
here=$(cd "$(dirname "$0")" && pwd)
repo=$(dirname "$here")
case $mode in
    new) tree=${2:-$HOME/qmk_firmware} ;;
    old) tree=${2:-$HOME/miryoku_qmk_old} ;;
    *) echo "$usage" >&2; exit 2 ;;
esac

stage=$tree/tests/rvannoord_hrm
rm -rf "$stage"
mkdir -p "$stage"
trap 'rm -rf "$stage"' EXIT

cp "$here/$mode"/* "$here/scenarios/scenario.hpp" "$here/scenarios/test_home_row_mods.cpp" "$stage/"
if [ "$mode" = new ]; then
    keymap=$repo/keyboards/splitkb/kyria/keymaps/rvannoord
    miryoku=$repo/users/manna-harbour_miryoku
    cp "$here/scenarios/test_host_os.cpp" "$keymap/tap_hold.c" "$keymap/host_os.c" "$keymap/host_os.h" "$stage/"
    # The test framework has its own keymap.c and config.h, so these get other names
    cp "$keymap/keymap.c" "$stage/rvannoord_keymap.c"
    cp "$keymap/config.h" "$stage/rvannoord_keymap_config.h"
    cp -r "$miryoku/manna-harbour_miryoku.h" "$miryoku/miryoku_babel" "$stage/"
fi

cd "$tree"
if [ "$mode" = old ]; then
    # The old Makefile runs the QMK CLI, and the 2023 tree's Python needs appdirs, which the
    # installed CLI lacks. A throwaway uv environment with the tree's requirements leaves it alone.
    uv run --no-project --with qmk==1.2.0 --with-requirements requirements.txt -- make test:rvannoord_hrm
else
    make test:rvannoord_hrm
fi
