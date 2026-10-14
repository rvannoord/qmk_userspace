# Copyright 2019 Manna Harbour
# https://github.com/manna-harbour/miryoku

OLED_ENABLE = yes
OLED_DRIVER = ssd1306
RGB_MATRIX_ENABLE = yes
RGBLIGHT_ENABLE = no
# LTO off, overriding the board. splitkb/kyria/info.json sets "lto": true, which is an
# AVR-era default: it saves about 11 KB of flash here, decisive on a 32 KB part and
# meaningless against the RP2040's 2 MB region at under 4% used. What it costs is every
# cross-module symbol — LTO inlines them out of the ELF, so `nm -S` can no longer answer
# "is this linked" or "how big is this", which is how this project verifies its builds.
LTO_ENABLE = no
WPM_ENABLE = yes
