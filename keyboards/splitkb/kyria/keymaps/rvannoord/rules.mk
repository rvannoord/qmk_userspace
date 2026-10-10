USER_NAME := manna-harbour_miryoku
CONVERT_TO = liatris

MIRYOKU_MAPPING = EXTENDED_THUMBS
MIRYOKU_CLIPBOARD = WIN

OS_DETECTION_ENABLE = yes
SRC += oled.c tap_hold.c host_os.c \
       oled_master_text.c \
       oled_slave_spaceship.c oled_slave_logo.c oled_slave_bjorn.c bjorn_art.c
