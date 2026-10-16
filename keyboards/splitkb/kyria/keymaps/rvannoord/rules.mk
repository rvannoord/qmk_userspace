# The layers are Miryoku's. USER_NAME pulls in users/manna-harbour_miryoku/, whose
# manna-harbour_miryoku.c defines keymaps[] through the LAYOUT_miryoku wrapper in
# config.h. Miryoku's options are set here and never on the command line, so a plain
# `qmk compile` always builds the same firmware. See docs/layout.md.
USER_NAME := manna-harbour_miryoku
# Thumb keys on arc positions 3-5.
MIRYOKU_MAPPING = EXTENDED_THUMBS
# Ctrl-based clipboard keys; on the Mac host_os.c swaps Ctrl and GUI.
MIRYOKU_CLIPBOARD = WIN

# The rev3 PCB is wired for an Elite-C; these are Liatris (RP2040) controllers.
CONVERT_TO = liatris
# Win or Mac on plug-in, read by host_os.c.
OS_DETECTION_ENABLE = yes

# Everything beyond keymap.c, which QMK compiles on its own. Generated files live in
# art/ and are never edited by hand; their generators are under tools/.
SRC += host_os.c tap_hold.c
SRC += split_state.c split_sync.c split_health.c
SRC += oled.c oled_canvas.c oled_master_zones.c
SRC += oled_slave_select.c oled_slave_bjorn.c oled_slave_world.c oled_slave_spaceship.c oled_slave_logo.c
SRC += art/bjorn_art.c art/bjorn_poses.c art/world_art.c art/space_art.c
SRC += art/panel_font.c art/panel_glyphs.c art/panel_layer_icons.c
