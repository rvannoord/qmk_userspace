---
paths:
  - "keyboards/**/oled*"
  - "keyboards/**/*logo*"
---

# OLED (SSD1306 on the Kyria rev3)

## The panel

- 128×64 per half over I²C, driven by a Liatris (RP2040). `OLED_ROTATION_180` keeps it 128 wide, so text is 21 columns × 8 lines in the 6×8 font. Rotating 90/270 gives 10 columns × 16 lines.
- The buffer is 1024 bytes: 8 pages of 128 columns. Each byte is a vertical strip of 8 pixels with bit 0 at the top. Raw art has to be in this page format.
- `OLED_DRIVER = ssd1306` is lowercase. `oled_task_user` returns `bool`. Older tutorials get both wrong.
- Rotation is set by `kyria.c`'s `oled_init_kb`, which returns 180 without calling `oled_init_user`. An `oled_init_user` in the keymap does nothing.

## What each half shows

- Master (USB side): logo, "Kyria rev3", current layer, held mods, lock state, host OS (`Win` / `Mac` from `host_is_mac()`).
- Other half: the WPM-driven spaceship animation, or one of the static logos picked in `custom_config.h`.

## Drawing

- `oled_write` / `oled_write_P` writes text at the cursor and doesn't clear what comes after. Give every field a fixed width and a blank of the same length: `"CTRL "` / `"     "`, not `"CTRL"` / `"   "`. Mismatched widths shift everything after them and leave stale characters on screen.
- `oled_write_raw` / `oled_write_raw_P` writes bytes from the cursor. `oled_set_cursor(0, page)` followed by 128 bytes fills one page row.
- The driver tracks dirty blocks (16 blocks of 64 bytes) and by default sends one block per scan loop (`OLED_UPDATE_PROCESS_LIMIT`). Redrawing a static screen costs little. A full-frame change takes about 16 loops to reach the panel, which is why the frame interval is throttled (50 ms now). Keep the throttle. On the RP2040 this I²C time is the real limit, not flash.
- The master-side logo is font glyphs 0x80–0xD4 from QMK's default `glcdfont.c`. A different logo means a raw bitmap or a custom font through `OLED_FONT_H`.
- Bitmaps live in `PROGMEM` and are read with `pgm_read_byte`. Both are no-ops on ARM, but keep them so the code stays portable. Convert images with image2cpp at 128×64, draw mode "Vertical - 1 bit per pixel".
- Return early when `!is_oled_on()`. Don't render to a sleeping screen.

## Split halves

- Only the master half knows layer, mods, lock state and host OS. The other half sees layer, mods and locks only with the sync flags: `SPLIT_LAYER_STATE_ENABLE`, `SPLIT_MODS_ENABLE`, `SPLIT_LED_STATE_ENABLE`. `SPLIT_WPM_ENABLE` is on because the spaceship needs it.
- OLED sleep sync is already on through the Kyria rev3's `keyboard.json` (`split.transport.sync.oled`): the other screen sleeps and wakes with the master.
- Each flag adds split traffic. Only add one when the slave screen actually shows that data.
- `is_keyboard_master()` is true on the half with USB plugged in.

## Burn-in

- Leave `OLED_TIMEOUT` on. Static logos burn in.
