USER_NAME := manna-harbour_miryoku
CONVERT_TO = liatris

MIRYOKU_MAPPING = EXTENDED_THUMBS
MIRYOKU_CLIPBOARD = WIN

OS_DETECTION_ENABLE = yes
SRC += oled.c oled_canvas.c tap_hold.c host_os.c split_state.c split_sync.c split_health.c \
       oled_master_text.c oled_master_zones.c panel_font.c panel_glyphs.c \
       panel_layer_icons.c \
       oled_slave_spaceship.c oled_slave_logo.c oled_slave_bjorn.c bjorn_art.c bjorn_poses.c \
       oled_slave_world.c world_art.c oled_slave_select.c
