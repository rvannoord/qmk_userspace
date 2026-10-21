# OLED v3: plan

> **A work plan, not the state of the firmware.** Nothing in it is built yet. The design it
> builds is *The drashna round* in `docs/layout.md`; this file is only the order to build it in,
> what each stage touches, and the questions still open. It goes when the last stage is done.

The OLED ideas taken from drashna's userspace and decided on 2026-10-21: master pages behind a new
PAGE key, the live layer map on the half the keys are on, the OS logo and drashna's modifier icons
on the bands, the Ein font, bongo cat and a Mario, Yoshi and Bullet Bill scene as slave
animations, a bootloader
screen, encoder feedback, glitch bursts on link loss, and a screensaver step. Written 2026-10-21 on
the work PC, from a read of `main` at `ccac66d` and stock QMK 0.34.6 at `7a1bbf37c5`. The ideas were
chosen from host renders: the master frames came from `oled_master_zones.c` and Bjorn from
`oled_slave_bjorn.c`, compiled on the host, with each idea drawn over them.

Work it top to bottom, one stage at a time, stopping at every checkpoint. Keep the Progress table
current in the commit that finishes a stage. Every stage that changes a panel runs
`tests/run_unit.sh`. Every stage that changes the slave also runs `tests/preview/run_preview.sh`.
Every stage that is flashed gets its lines in `docs/flash-checklist.md`, the way the v2 stages did.

QMK line numbers refer to `7a1bbf37c5`.

## Progress

| Stage | Status |
|-------|--------|
| 0. Sources, licence, generators | open |
| 1. Bands: Ein, modifier icons, OS logo | open |
| 2. Bootloader screen | open |
| 3. PAGE key, page framework, the three bands | open |
| 4. Live layer map, master screen moves over | open |
| 5. Status sheet, layer map page, Typing, Event log | open |
| 6. Bongo cat and the Mario scene on the slave | open |
| 7. Encoder feedback | open |
| 8. Glitch bursts on link loss | open |
| 9. Screensaver | open |

## Decisions

Decided 2026-10-21, by picking from the renders:

- D1. The master gets pages, in this order: Bands, Bands with the small kitty, Bands with the QMK
  logo, Status sheet, Layer map, Typing, Stats, Event log.
- D2. The live layer map fills the panel on the half the layer's keys are on, Button on both
  halves, with no outlines. Both styles are kept, the big grid and the Kyria-shaped map, and PAGE
  switches between them while a map is showing.
- D3. The master's screen moves to the other half whenever a map covers the master's panel and not
  the slave's. Always on, not a setting.
- D4. Ein for all panel text. drashna's 12x8 icons in the mod row. The 12x16 OS logo replaces the
  badge.
- D5. Bongo cat and the Mario, Yoshi and Bullet Bill scene join the slave animations, the scene
  with its WPM at 2× (decided 2026-10-21, over a 1× one-line version). The bootloader screen gets
  the logo and a label. Encoder feedback as mocked. Glitch for link lost on both halves. Matrix rain as the
  screensaver.

Open, with a default, to answer before the stage that needs it:

- D6. **Licence (stage 0).** drashna's display code and art are GPL-3.0-or-later: the font file,
  `oled_assets.h` (kitty, header art, Mario, Yoshi and Bill) and the matrix rain. This repo is GPL-2.0, and
  its headers say "or later". Taking any of those in makes the firmware GPL-3.0. Every "or later"
  in here allows that, but it is a one-way move. Bongocats are GPL-2.0-or-later and fine either way.
  Three things on top of the licence:
  - The kitty is credited inside drashna's file to j-inc and pixelbenny.
  - TinyOLED-Fonts' own licence still has to be read.
  - Mario, Yoshi and Bullet Bill are Nintendo characters drawn as fan art, which matters in a
    public repo whatever the licence says.

  Default: move to GPL-3.0-or-later, and keep the Mario scene's art out of the public repo until
  that is settled.
- D7. **PAGE key position (stage 3).** Default: left half, outer column, middle row, under SYS.
  It's free on every layer, and the pinky reaches it while a thumb holds a layer, which is when it
  toggles the map style.
- D8. **Status sheet footer (stage 5).** The mock carried a keylogger, which was declined as a
  status-bar strip. Default: the layout name, `Colemak-DH`, or the default layer's name when that
  is not Base.
- D9. **Kitty thresholds (stage 3).** drashna's are 10, 40 and 60 WPM. Default: Bjorn's tiers
  instead (10 / 30 / 50 up, 6 / 24 / 42 down, 800 ms dwell), so both characters read speed the same
  way.
- D10. **Screensaver timing (stage 9).** The mock started at 60 s on both halves. Bjorn settles at
  8 s and dozes at 75 s, and v2's director owns the slave's idle time. Default: the master rains
  from 60 s, the slave keeps Bjorn's idle show and rains from 100 s, and both go dark at 120 s.
- D11. **Encoder feedback off the bands (stage 7).** Default: on any page it shows as the same box,
  centred.
- D12. **Order against `docs/oled-v2.md`.** Stages 0-7 here touch neither Bjorn's idle show nor the
  director, so they can go before v2's open stages. Stages 8 and 9 touch the slave's idle time and
  should follow v2 stage 4 (the director). Default: v3 0-7, then v2 resumes, then v3 8-9.
- D13. **When Yoshi curls up (stage 6).** The mock used 4 s after the last key. Default: at
  Bjorn's doze, 75 s, so the two slave scenes rest on the same clock.

---

## Stage 0: sources, licence, generators (no firmware change)

**Answer D6 first.** Then pin the sources:

| Repo | Commit | Files |
|------|--------|-------|
| `drashna/qmk_userspace` | `0903be5b3c56a394dbfb27b8095bc5ad747b8d98` (2026-10-17) | `users/drashna/display/oled/drashna_font.h`, `oled_assets.h`, `screensavers/matrix_scroll.h` |
| `drashna/qmk_modules` | `ee2c35bcbb7d8b8be108c93c5b8715ea1a2c020c` (2026-10-16) | `bongocats/bongocat_assets.c` |

Fetch outside the repo, the way `VENDORED.md` does for Bjorn and the Halcyon icons:

```sh
git clone --filter=blob:none --no-checkout https://github.com/drashna/qmk_userspace ~/drashna-src
cd ~/drashna-src && git sparse-checkout init --cone && git sparse-checkout set users/drashna/display/oled
git checkout 0903be5b3c56a394dbfb27b8095bc5ad747b8d98
git clone --filter=blob:none --no-checkout https://github.com/drashna/qmk_modules ~/drashna-modules
cd ~/drashna-modules && git sparse-checkout init --cone && git sparse-checkout set bongocats
git checkout ee2c35bcbb7d8b8be108c93c5b8715ea1a2c020c
```

Generators go in `tools/drashna/`, standard library only (v2's D2). The sources are C arrays behind
`#if` blocks, which is how the font file holds its nine faces. So read them through the
preprocessor first, `cc -E -P -DPROGMEM= -DOLED_FONT_EIN`, and then parse the initialisers. The
array dimensions can be enum names, so take the first dimension from the initialiser.
The outputs, all under the keymap's `art/`:

| Output | From | What |
|--------|------|------|
| `panel_font.c` | `drashna_font.h`, Ein | 95 glyphs through `tools/panel/gen_font.py`. **Trap:** drashna zeroed the font's upper rows, and `gen_font.py` assembles `u_qmk_logo` from rows 0x80/0xa0/0xc0. So the logo must still come from QMK's own `glcdfont.c`, which needs a second source option in the generator |
| `panel_glyphs.c` | `drashna_font.h`, 0xE4-0xED | the five mod glyphs, each 12x8 icon centred in its 16x16 slot (offset 2, 4) |
| `drashna_icons.c` | `drashna_font.h`, 0x95-0x98 / 0xB5-0xB8, 0xE0-0xE3, 0xF0-0xF1 | the 12x16 Windows and Apple logos; speaker, mute and bulb at 12x8 |
| `kitty_art.c` | `oled_assets.h`, `pet_animiations` | pets 0 (striped, status sheet) and 2 (small, bands): sleep, kaki, mati and awake, 32x32 |
| `status_frame.c` | `oled_assets.h` | `header_image`, `footer_image2`, `display_border` |
| `bongo_art.c` | `bongocat_assets.c` | 5 idle, 1 prep, 2 tap frames, 128x32 |
| `mario_art.c` | `oled_assets.h` | `mario_animation` (four 16x32), the sixteen Yoshi frames the scene uses (32x32) and `bullet_bill_logo` (32x24) |
| `matrix_art.c` | `matrix_scroll.h`, the `OLED_DISPLAY_128X64` branch | 72 frames of 1,024 bytes |

Add a section per source to `VENDORED.md`, with commit, licence, credit and the fetch recipe.

**Done when:** each generator reproduces its output byte for byte on a second run, and nothing
is compiled in yet.

## Stage 1: the bands, art first

Three changes to the bands as they are today, all art and drawing, no new state.

1. **Ein.** Regenerate `panel_font.c`. Check that the stats page still fits 16 characters, and that
   `u_qmk_logo` (the boot splash) is unchanged.
2. **Modifier icons.** Regenerate `panel_glyphs.c`. `render_mods()` does not change. Any panel test
   that counts glyph ink gets its numbers updated, and nothing else should move.
3. **OS logo.** `render_status()` stops drawing the badge text and draws the 12x16 logo at (0,0).
   Its bottom half lands in the band, and `render_band()` rewrites pages 1-5 every frame. So the
   logo must be drawn **after** the band, and the band art must keep x 0-13, y 8-17 dark: a test
   asserts that over all ten icons and every base variant. The SYS underline moves to y=16. **Two
   traps:**
   - During the link-down alarm the band is inverted, so the logo has to be knocked out of the lit
     band, not ORed into it.
   - The QMK version of the base band (stage 3) has to start below y=17.

Update *Master half* in `docs/layout.md` (the status bar row, the font paragraph, and the
`panel_glyphs.c` row of *Files*).

**Checkpoint, on the board:** from a normal seated position, can the five icons and the logo be
named without leaning in? The 12x8 icons are smaller than the 16x16 glyphs they replace.

## Stage 2: bootloader screen

`shutdown_user()` is called on the way to the bootloader: `reset_keyboard()` calls
`shutdown_quantum(true)` (`quantum.c:232`, `:208`), which calls `shutdown_kb()` and then
`shutdown_user()` (`:473`).

Define it once, in `oled.c`, beside the splash:
- Compose `u_qmk_logo` with an inverted `BOOTLOADER` label under it in the canvas, then commit.
- **Flush synchronously**, because there is no next main loop to send the dirty blocks.
- Return true.

Bootmagic is a boot-time matrix scan and never reaches this, which is correct: there is nothing on
the panel yet at that point.

**Checkpoint:** double-tap Boot on each half. The screen shows, and the flash still goes through.

## Stage 3: the PAGE key, the page framework, the three bands

- **Key.** `U_PAGE = QK_USER_2` in the keymap's `config.h`, at matrix [1,6] (`L12`), in **both**
  `LAYOUT_miryoku` variants. It acts on release, like SYS and ANIM. Update *Extra keys* from
  "planned" to the real entry.
- **Persisted** in the existing tagged EEPROM word (`oled_slave_select.c`). Byte 0 stays the
  animation; byte 1 takes the page in bits 0-2 (eight pages, exactly) and the map style in bit 3
  (stage 4). An old word without those bits reads page 0, grid style, so nothing needs migrating.
- **Pages.** A small dispatcher in front of `oled_render_master()`. The bands become one renderer
  with a base-band hook: the Kyria emblem (today), the small kitty (emblem area cleared, kitty at
  (18,12), wordmark kept), or the QMK logo (below y=17, see stage 1). The kitty's state comes from
  WPM (D9) on the master's own clock. The Stats page is the existing `render_stats()`.
- **Dots.** For 700 ms after a press, one dot per page in the middle of the status bar, the current
  one larger.

**Tests:**
- every page's second render writes nothing (the redraw-cost test, per page)
- each page stays under the alarm ratio
- the cycle wraps
- a replug keeps the page

**Checkpoint:** cycle through all four pages that exist so far (the three bands and Stats), then
replug.

## Stage 4: the live layer map, and the master screen moving over

**Which half draws.** It's a fixed table on the active layer, against `is_keyboard_left()`:

| Layer | Left panel | Right panel |
|-------|-----------|------------|
| Nav, Mouse, Media | — | map |
| Sym, Num, Fun | map | — |
| Button | map | map |

**Where the keys are**, from `keyboard.json` and the `LAYOUT_miryoku` wrapper, as [row, col]:

| | Pinky | Ring | Middle | Index | Inner |
|--|--|--|--|--|--|
| Left, top | [0,5] | [0,4] | [0,3] | [0,2] | [0,1] |
| Left, home | [1,5] | [1,4] | [1,3] | [1,2] | [1,1] |
| Left, bottom | [2,5] | [2,4] | [2,3] | [2,2] | [2,1] |
| Right, top | [4,5] | [4,4] | [4,3] | [4,2] | [4,1] |
| Right, home | [5,5] | [5,4] | [5,3] | [5,2] | [5,1] |
| Right, bottom | [6,5] | [6,4] | [6,3] | [6,2] | [6,1] |

The thumbs, from position 3 to 5: left [3,1] [3,5] [3,0] (Esc, Space, Tab); right [7,1] [7,5] [7,0]
(Del, Bksp, Enter). Position 1, the encoder click, is [3,4] and [7,4]. On screen the right half
runs inner to pinky, left to right.

**Legends.** `keymap_key_to_keycode(layer, pos)` (`keymap_common.c:198`), then a table from keycode
to legend: one glyph, or up to three characters in Ein. It has to cover:
- the clipboard keys
- arrows and paging
- the mouse and wheel keys
- RGB and media
- F1-F12, PrtSc, ScrLk, Pause, App
- Caps Word, the mouse buttons, `OU_AUTO`
- the four mods, as the 12x8 icons, following the OS like the mod row

An unknown keycode draws `·`. A host test preprocesses Miryoku's seven layer macros, through
`miryoku_layer_selection.h` so any override in `custom_config.h` counts, and fails on any keycode
the table does not cover. That's what keeps a layer change from quietly drawing dots.

**Pressed keys.** Each half reads its own rows with `matrix_is_on()`. The slave scans its own half,
so it needs nothing from the master for this; check that on the board early.

**Two renderers.** The big grid has columns centred at 13, 38, 64, 90 and 115, rows of 16, and
single glyphs and icons at 2×. The Kyria-shaped map uses 19 x 12 px per key unit, the stagger
0.75 / 0.25 / 0 / 0.25 / 0.5 (pinky to inner), and thumbs at (4.5, 3.5), (5.5, 4.0) and
(6.5, 4.25). Both draw the layer name inverted in the free thumb slots. A press lights a small
inverted box, never the whole cell.

**Style.**
- PAGE switches the style instead of the page whenever a map is showing.
- Persisted in byte 1 bit 3.
- Sent to the slave as `world` bit 3: `U_WORLD_MAP_SHAPED 0x08` in `split_state.h`. Rewrite the
  spare-bits comment there: bits 4-7 are left for the daemon.

**Master screen moves over.** While the map covers the master's panel and not the slave's, the
slave draws the bands page from `u_sync_state()` instead of Bjorn. That page reads only what the
six bytes carry (layers, fingers, flags, misfire count), and it already mirrors the mod row by
`is_keyboard_left()`. `oled_master_zones.c` needs to take its state from either source instead of
calling `u_sync_build()` itself. Bjorn's task does not run while he isn't drawn, which is the
existing rule, so he comes back awake.

**Tests:**
- the side table for every layer, on both halves
- legend coverage, as above
- the right half's column order
- a pressed key's box stays inside its cell
- the style toggle
- the slave's bands page matches the master's for the same state, mirrored

**Checkpoint, on the board:**
- Hold each of the six thumb layer keys and Z / slash: every map sits over its keys.
- Move the cable to the other half and repeat, since the master screen now moves the other way.
- Hold Enter and look at the right half: the layer icon is there.

## Stage 5: the other four pages

All four draw in Ein and go through the dispatcher from stage 3.

- **Status sheet.** The header and footer art, side borders, the title, the striped kitty at
  (4,9), WPM, the layer name (inverted off base), the OS logo with Win or Mac and auto or manual,
  CW / CAPS / SCRL inverted when on, and eight 12x8 modifier icons inverted while held. The footer
  is D8.
- **Layer map.** Both halves of the current layer at 1×, the title bar inverted, held keys
  inverted, and the held layer key inverted in the thumb row. It reuses stage 4's legend table.
  When a thumb layer is held, the live map still takes its half as usual.
- **Typing.** WPM at 2×, the session peak, a 62x28 graph fed from a ring of 62 samples taken every
  300 ms (18.6 s), and bongo cat along the bottom. That needs stage 6's art, so build stage 6 first
  or stub the cat.
- **Event log.** A RAM ring of eight `{uptime, event}`, RAM only like the rest of the link evidence.
  The events: boot; link up and down (the edges of `u_sync_link_down()`); a self-heal reset (the
  `resets` count in the slave's reply going up); the OS set by detection or by SYS; a misfire.
  Stamped `+1h04`, since there is no wall clock.

**Tests:** the redraw cost and the alarm ratio per page, and the event ring's wrap and order.

## Stage 6: bongo cat and the Mario scene on the slave

- `enum u_slave_anim` gains `U_ANIM_BONGO` and `U_ANIM_MARIO`: seven in all, still within
  `U_WORLD_ANIM_MASK`. ANIM's row in *Extra keys* lists them.
- **Bongo cat.** The cat at y 32-63, WPM at 2× top right. Idle frames at 300 ms below 10 WPM, the
  prep frame from 10 to 20, tap frames above that at max(100, 450 − 2.5 × WPM) ms.
- **Mario, Yoshi and Bullet Bill.** The scene in *The drashna round*, in `oled_slave_mario.c`.
  - **Art.** `mario_art.c` (stage 0) carries Mario's four frames, the sixteen Yoshi frames the scene
    uses (*idle*, *ready*, *fly*, *hide*, *run*; the empty slots in drashna's array are dropped) and
    Bill. drashna's frame orders are tables in the renderer, not baked into the art.
  - **State, on the slave's own clocks.**
    - The WPM tier, with the same hysteresis and 800 ms dwell as Bjorn's (`bjorn_tier_up` /
      `_down`); share the code rather than copy it.
    - The floor offset, as a fixed-point accumulator of `wpm/15` px per 50 ms tick.
    - Mario's and Yoshi's walk phases.
    - Bill's x, or none.
    - The next spawn time.
    - A jump per character: start tick and length.
    - Idle time, from `last_input_activity_elapsed()`, which `SPLIT_ACTIVITY_ENABLE` already shares.
  - **Bill's jumps.** Each tick, for each character still on the ground, work out when Bill's nose
    reaches its right edge (t_s) and when his tail clears its left edge (t_e), at his current speed.
    Then the jump length T = max(16, (t_e − t_s) / 0.5 + 2) ticks. Start once the overlap's
    midpoint, minus T/2, has arrived. The lift is 4 × 31 × t(1 − t). Yoshi's extent is his *fly*
    frames' (x 3-26 of the sprite), because those are what is in the air.
  - **Draw order:** floor, tufts, Mario, Yoshi, Bill, then the WPM at 2×, right-aligned at (90,1)
    with `wpm` at (108,18).
  - **D13, when Yoshi curls up.** Default: at Bjorn's doze, 75 s. He uncurls on the next key, back
    through *hide* reversed, then *idle*.
- **Tests.**
  - A host test runs the scene over a scripted WPM curve and asserts, for every tick, that Bill's
    lit pixels never meet Mario's or Yoshi's. The mock this was decided from passed that, 0 of 216
    frames.
  - The WPM corner (x 84-127, y 0-25) stays clear of every sprite.
  - Nothing reaches y < 0.
  - At WPM 0 with no Bill, an unchanged frame writes nothing.
- Add a scenario to `tests/preview/` for each animation, now that the preview builds again.

## Stage 7: encoder feedback

`encoder_update_user()` records the direction and the time. For 800 ms the master shows the speaker
or bulb icon at 2× with `VOL+`, `VOL−`, `BRI+` or `BRI−` (D11 for pages other than the bands). MUTE
shows only if a mute key exists; the encoder click stays an open experiment in `docs/layout.md`.
Measured on the renders at 2,663 px, so it goes through the alarm-ratio test like a page.

## Stage 8: glitch bursts on link loss

**Master.** While `u_sync_link_down()` holds, bursts of 3 to 7 frames, 40 to 60 ms each, every 1.5
to 3 s at random. Each burst frame is built from the alarm frame:
- a few row bands shifted 2-18 px with wrap
- an occasional 1-4 row inversion
- 40 random pixels
- now and then 1-3 rows cleared

It must not stop the alarm from being read.

**Slave.** The same, applied to Bjorn's frame while hard stale.

**Both.** Glitch frames are the only frames allowed to change with no new state, and only during the
alarm. A slave whose code has stopped shows nothing new either way, which is v2's D-6, and the
hardware watchdog still covers it.

## Stage 9: screensaver

The matrix rain, frame per render tick (50 ms on the split), on the timing D10 settles. The panel
still sleeps at `OLED_TIMEOUT` from the input clock, and nothing about drawing frames may re-arm it.
`u_oled_should_sleep()` already reads the input clock, not dirtiness, so this holds.

**The thing to watch:** this puts a full-panel repaint on both halves' I²C buses at once. That was
the one thing that changed before the link loss of 2026-10-13. Continuous I²C was shown not to be
enough on its own, but after flashing, keep an eye on the stats page's FAIL, OUT and DROP for a few
days.

---

## Appendix: numbers

Lit pixels on the host renders, the highest frame of each:

| Scene | Lit px |
|-------|--------|
| Master's alarm band (the ceiling) | ~4,200 |
| Encoder feedback | 2,663 |
| Status sheet | 2,519 |
| Today's bands, worst case (two held mods) | 1,947 |
| Layer map page | 1,943 |
| Typing page | 1,785 |
| Live map, big grid | 1,671 |
| Bands with the QMK logo | 1,663 |
| Glitch frame | 1,571 |
| Bands with the kitty | 1,404 |
| Live map, Kyria-shaped | 1,350 |
| Bongo cat | 1,288 |
| Bootloader screen | 1,183 |
| Matrix rain | 1,095 |
| Mario, Yoshi and Bill | 777 |

Flash, against 2 MB: matrix rain 72 KB, bongo cat 4 KB, two kitties about 4.6 KB, the Mario scene
about 2.4 KB (sixteen Yoshi frames of 128 bytes, Mario 256, Bill 96), the font 760 bytes, the icons
and frame art under 1 KB.
