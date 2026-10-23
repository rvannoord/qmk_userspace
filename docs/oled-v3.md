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
| 0. Sources, licence, generators | done 2026-10-21 |
| 1. Bands: Ein, modifier icons, OS logo | **done 2026-10-21**, flashed `f639c97`; the checkpoint passed on the board. Linux logo, the iOS "i" (D14) and drashna's locks added the same day |
| 2. Bootloader screen | **done 2026-10-22**, flashed `fd28d30`; the checkpoint passed on both halves |
| 3. PAGE key, page framework, the three bands | **done 2026-10-22**, flashed `657fb8e`, then `70f3bc0` so the page and the animation follow the cable; the checkpoint passed on the board |
| 4. Live layer map, master screen moves over | **done 2026-10-22**, on the board as `e839e27`; the early check passed on `12b236a`, and the screen moving over was tried on `d267d99` and taken out (D3) |
| 5. Status sheet, layer map page, Typing, Event log | **built 2026-10-22** in `636ba56`; all 18 of the mock's frames match byte for byte. Not flashed yet: the checkpoint waits for the next flash, with D16 |
| 6. Bongo cat and the Mario scene on the slave | **built 2026-10-22** in `42f140a`, not flashed yet; D13 decided (4 s) |
| 7. Encoder feedback | **built 2026-10-23** in `915f3b2`, not flashed yet; D11 decided (the same box, centred, 2 px rim) |
| 8. Glitch bursts on link loss | open |
| 9. Screensaver | open |

## Decisions

Decided 2026-10-21, by picking from the renders:

- D1. The master gets pages, in this order: Bands, Bands with the small kitty, Bands with the
  striped kitty, Bands with the QMK logo, Bands with the Monitor logo, Status sheet, Layer map,
  Typing, Stats, Event log. The striped kitty's stop was added on 2026-10-22, during stage 3. It
  is the kitty idea's variant A, beside the Kyria wordmark: both kitties, since the bands are
  stops anyway. The Monitor logo's stop came the same day, with the Monitor ERP screens on the
  slave (`docs/layout.md`, *The Monitor ERP screens*); it moves every later page's stored number
  up by one, so a stored Stats page reads as the bands once.
- D2. The live layer map fills the panel on the half the layer's keys are on, Button on both
  halves, with no outlines. Both styles are kept, the big grid and the Kyria-shaped map, and PAGE
  switches between them while a map is showing.
- D3. The master's screen moves to the other half whenever a map covers the master's panel and not
  the slave's. Always on, not a setting. **Reversed 2026-10-22** at stage 4's checkpoint: on the
  board the move was confusing, and the map's name tag already says which layer is held. It was
  built and flashed in `d267d99`, then taken out; the other half keeps its animation.
- D4. Ein for all panel text. drashna's 12x8 icons in the mod row. The 12x16 OS logo replaces the
  badge.
- D5. Bongo cat and the Mario, Yoshi and Bullet Bill scene join the slave animations, the scene
  with its WPM at 2× (decided 2026-10-21, over a 1× one-line version). The bootloader screen gets
  the logo and a label. Encoder feedback as mocked. Glitch for link lost on both halves. Matrix rain as the
  screensaver.
- D6. **Licence: GPL-3.0-or-later**, decided 2026-10-21 before stage 0. The root `LICENSE` is now
  the GPL-3.0 text, and other people's notices stay as they are. Two more answers came with it:
  - **The Mario scene's art is committed** like the rest of `art/`, knowingly, the way drashna
    publishes it. That was chosen over keeping it out of the public repo.
  - **Ein is used although its origin is untraced.** drashna credits TinyOLED-Fonts, which has no
    Ein; `VENDORED.md` has the details.

Open, with a default, to answer before the stage that needs it:

- D7. **PAGE key position (stage 3).** Decided 2026-10-22, the default: left half, outer column,
  middle row, under SYS. It's free on every layer, and the pinky reaches it while a thumb holds a
  layer, which is when it toggles the map style.
- D8. **Status sheet footer (stage 5).** The mock carried a keylogger, which was declined as a
  status-bar strip. Default: the layout name, `Colemak-DH`, or the default layer's name when that
  is not Base. **Decided 2026-10-22, the default.**
- D9. **Kitty thresholds (stage 3).** drashna's are 10, 40 and 60 WPM. Decided 2026-10-22, the
  default: Bjorn's tiers instead (10 / 30 / 50 up, 6 / 24 / 42 down, 800 ms dwell), so both
  characters read speed the same way, paw frames and all. Found at stage 0: the small kitty's paw frames are byte for byte its sleep frames. So on
  the bands it looks asleep in the paw tier, whichever WPM range that tier gets. The striped kitty
  has three paw frames of its own.
- D10. **Screensaver timing (stage 9).** The mock started at 60 s on both halves. Bjorn settles at
  8 s and dozes at 75 s, and v2's director owns the slave's idle time. Default: the master rains
  from 60 s, the slave keeps Bjorn's idle show and rains from 100 s, and both go dark at 120 s.
- D11. **Encoder feedback off the bands (stage 7).** Default: on any page it shows as the same box,
  centred. **Decided 2026-10-23, the default**, from renders of the box over the Status sheet, the
  Layer map, Typing and the Event log.
- D12. **Order against `docs/oled-v2.md`.** Stages 0-7 here touch neither Bjorn's idle show nor the
  director, so they can go before v2's open stages. Stages 8 and 9 touch the slave's idle time and
  should follow v2 stage 4 (the director). Default: v3 0-7, then v2 resumes, then v3 8-9.
  **Decided 2026-10-23, the default:** v2's director (stage 4) and the goat (stage 5) next, then
  v3's stages 8 and 9.
- D13. **When Yoshi curls up (stage 6).** The mock used 4 s after the last key. Default: at
  Bjorn's doze, 75 s, so the two slave scenes rest on the same clock. **Decided 2026-10-22: 4 s
  after the last key**, as the mock had it.
- D14. **How iOS is told from the Mac (stage 1).** Decided 2026-10-21: a small lowercase "i" in
  front of the Apple logo, in Apple's lettering style, so it reads as an i-device. Roel's own
  design, after these four were rendered over the real Mac frames:
  - A: a tablet glyph beside the logo. It read as an O at panel size.
  - B: drashna's 6x8 Apple. It lost the set-by-hand line.
  - C: the logo inside a device outline. It broke up under the alarm.
  - D: a home-indicator bar under the logo.

  The "i" is drawn here, because drashna's fonts have only a 1 px one and Ein's is a small capital.
- D15. **Mac legends on the live layer map (stage 4).** The Mac has no Print Screen, Scroll Lock or
  Pause: it reads the Fun layer's three keys as F13, F14 and F15, and F14 and F15 are its screen
  brightness down and up (found on the board, 2026-10-21). Default: on the Mac the map draws what
  those keys do there, `F13` and drashna's sun cell with ▼ or ▲, and `PSc`, `ScL` and `Pau`
  everywhere else, the way the mods already follow the host.

  Every keycode on every layer was checked the same day for other keys that behave differently
  on the Mac. Three more have a legend that would mislead there:
  - `KC_INS` (Nav): the Mac has no Insert and reads the key as Help.
  - `KC_APP` (Fun thumb): it does nothing on the Mac.
  - `KC_ALGR` and the AltGr holds on X and dot: on the Mac it is right Option, which types
    Option characters, so its legend there is drashna's ⌥ icon.

  Default for these: `Hlp`, a dimmed `App`, and ⌥ on the Mac.

  **Decided 2026-10-22, the default**, after renders of the Fun and Nav maps in both styles on
  both hosts, against a words-only version (`Br-` and `Br+`, App blank). The sun reads best at 2×
  in the big grid; at 1× in the Kyria-shaped map words would read a little better.

  Two more differ in behaviour, not in what the key is, so their legends stay:
  - Home, End, PgUp and PgDn scroll the view on the Mac rather than moving the cursor. That is
    the *OS-aware navigation* experiment in `docs/layout.md`.
  - Media Stop probably does nothing on the Mac.
- D16. **The Status sheet's layer line at rest on a default that is not Base (stage 5).** The
  line names the active layer, which at rest is the default, so it says `EXTRA` or `TAP` there; the
  mock's `BASE` cannot be produced. Either it is inverted like any layer that is not Base, as
  built, or it is inverted only while a layer is held, which keeps the plain look of the mock at
  rest. Default: as built. A one-line change either way; asked at stage 5's checkpoint.

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
| `panel_font.c` | `drashna_font.h`, Ein | 95 glyphs through `tools/panel/gen_font.py`. **Trap:** `gen_font.py` assembles `u_qmk_logo` from rows 0x80/0xa0/0xc0, and drashna's font does not hold the logo there (those rows are mostly empty, with a few other glyphs). So the logo must still come from QMK's own `glcdfont.c`, which needs a second source option in the generator |
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

**As built, 2026-10-21.**
- **Extended the same day.** The second pass took in the rest of drashna's art:
  - `drashna_icons.c` now names every lit non-text cell of his font, and `gen_art.py` stops on a
    lit cell with no name.
  - Three files are new: `drashna_sprites.c`, `painter_art.c` and `painter_fonts.c`.
  - `kitty_art.c` carries all three kitties with all six moods, and `status_frame.c` has both
    footers and row 2.
  - The clone now needs `painter/fonts` as well, so take the fetch recipe from `VENDORED.md` rather
    than the one above.
  - All of it came out byte for byte the same on a second run, and nothing new is in `SRC`.
    `tests/run_unit.sh` compiles every file in `art/` with `-Werror`, so art that nothing draws
    cannot rot unnoticed.
- **Licence.** D6 answered and `LICENSE` replaced. The two clones are at `~/drashna-src` and
  `~/drashna-modules`, at the pinned commits.
- **Generators.** `tools/drashna/csource.py` reads the sources through the preprocessor.
  `tools/drashna/gen_art.py` writes the six new art files. `gen_font.py` gained `--drashna … --face
  EIN` and `gen_glyphs.py` gained `--drashna …`.
- **Not written to `art/` yet.** `panel_font.c` and `panel_glyphs.c` stay as they are until stage 1
  regenerates them.
- **Verified:**
  - all eight outputs came out byte for byte the same on a second run
  - without the option, the committed `panel_font.*` and `panel_glyphs.*` still regenerate
    unchanged
  - the Ein build of `panel_font.c` keeps `u_qmk_logo` byte for byte, and the lock glyphs are
    untouched
  - every output compiles on the host with `-Wall -Wextra -pedantic -Werror`
  - none of it is in `SRC`, and the firmware builds as before

  A contact sheet rendered from the generated files was checked by eye.
- **What the sources hold** that the later stages need is in `VENDORED.md`:
  - the logo rows
  - the small kitty's two paw frames, which are its sleep frames (D9)
  - Yoshi's set sizes, 3 / 5 / 3 / 3 / 2
  - Mario lit at y 5-31 standing and 3-28 jumping, and Bill at 28x21 from (2,2), as the mock had them

## Stage 1: the bands, art first

Three changes to the bands as they are today, all art and drawing, no new state.

1. **Ein.** Regenerate `panel_font.c`. Check that the stats page still fits 16 characters, and that
   `u_qmk_logo` (the boot splash) is unchanged.
2. **Modifier icons.** Regenerate `panel_glyphs.c`. `render_mods()` does not change. Any panel test
   that counts glyph ink gets its numbers updated, and nothing else should move.
3. **OS logo.** `render_status()` stops drawing the badge text and draws the 12x16 logo at (0,0).
   Its bottom half lands in the band, and `render_band()` rewrites pages 1-5 every frame. So the
   logo must be drawn **after** the band, and the band art must keep x 0-15, y 8-17 dark: a test
   asserts that over all ten icons and every base variant. The SYS underline moves to y=16. **Two
   traps:**
   - During the link-down alarm the band is inverted, so the logo has to be knocked out of the lit
     band, not ORed into it.
   - The QMK version of the base band (stage 3) has to start below y=17.

Update *Master half* in `docs/layout.md` (the status bar row, the font paragraph, and the
`panel_glyphs.c` row of *Files*).

**Checkpoint, on the board:** from a normal seated position, can the five icons and the logo be
named without leaning in? The 12x8 icons are smaller than the 16x16 glyphs they replace.

**As built, 2026-10-21.**
- `panel_font.c` and `panel_glyphs.c` were regenerated with `--drashna`. `u_qmk_logo` is unchanged
  byte for byte, and so are the lock glyphs.
- `render_band()` now returns whether the alarm lit the band this frame, and `render_os_logo()`
  takes that. It runs after the band: the logo is ORed onto the dark corner, or cut out of the lit
  band, and the underline likewise. `drashna_icons.c` is in `SRC`, adding 48 bytes.
- `test_panel.c` now checks:
  - the logo's exact bytes on Windows and on the Mac
  - the underline at y=16, a row clear of the logo
  - the cut-out under the alarm, on a held layer and on base, with and without the underline
  - the corner dark on all ten bands' art
  - two more redraws that must cost nothing

  The badge's two checks became logo checks. The inverted-row probes start just past the
  logo's corner, and the centring check ignores the corner.
- Making the knockout an OR, or drawing the logo before the band, fails the tests (both tried and
  reverted).
- On the host renders: the master at rest lights 1,091 px, and the mod row 99 of them. A held
  Nav with two mods lights 1,359. The alarm frame lights 4,612.

**Added the same day, after the second pass over drashna's art** (`docs/layout.md`, *drashna's
art, all of it*):
- **Locks.** Caps Lock and Scroll Lock are drashna's cells 0xc9 and 0xcb, through `gen_glyphs.py
  --drashna`. After the checkpoint, Caps Word was changed to light the Caps Lock cell, the way
  drashna's screen does, because he has no Caps Word glyph. The hand-drawn one and the middle slot
  went, and the two locks right-align at x=109 and 118. The firmware is 113,312 bytes of text
  with it, from a clean tree. Flashed as `0130b8b` on 2026-10-22.
- **Hosts.** The logo follows the host: Windows, the Apple logo for the Mac, Tux for Linux, and
  for iOS the Apple logo with the "i" in front (D14).
  - `host_os.c` keeps the detected host, and `host_kind()` reports it.
  - SYS still only flips Mac and not-Mac. Each side shows the detected flavour when the detected
    host is on it, so a Linux machine flips Linux↔Mac and an iPad iOS↔Windows.
  - An unsure detection is Windows, as before. QMK reports Android as Linux, so the Android logo
    is art only.
- **The link.** The payload grew to 7 bytes with a `host` byte, because the flags byte had no bit
  left. `U_FLAG_IS_MAC` still drives the mod row's order. The transaction count, and so the
  handshake, is unchanged.
- **Tests.**
  - The scenario `S10_HostKind` covers detection and SYS for Linux, iOS, macOS and unsure.
  - `test_split_state` checks the host byte.
  - `test_panel.c` checks Tux and the iOS "i" (drawn, set by hand, cut out under the alarm), and
    redraws on Linux, on iOS and with all three locks lit.
  - The corner every band keeps dark grew to x 0-15, because the "i" and its 2 px gap move the
    logo 4 px right. In rows 8-17 the nearest band pixel is the Kyria emblem at x=18.
- **Size.** The firmware is 113,288 bytes of text, 240 more than before these additions.

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

**As built, 2026-10-22.**
- `shutdown_user()` in `oled.c`, beside the splash. It draws only when `jump_to_bootloader` is
  true: `soft_reset_keyboard()` comes through the same hook with false. The link's self-heal
  calls `mcu_reset()` directly and never reaches it.
- **The screen.** The splash's QMK logo, now drawn by a shared `draw_logo()`, at y=12.
  BOOTLOADER is in Ein, centred on its ink and cut out of an 83x11 bar at y=42. It lights
  1,341 px.
- **The flush.** The canvas is committed, then `oled_render_dirty(true)` sends every dirty block,
  because nothing runs after `shutdown_user()` to send them. The SSD1306 keeps showing the frame
  while the bootloader runs, since the panel stays powered.
- **Only the master** shows it: keys are processed there, so a Boot double-tap resets the half
  with the cable. Each half needs the cable for its own check.
- **Tests.** `tests/unit/test_boot.c` compiles the real `oled.c` and records every flush. It
  checks the logo pixel for pixel, the bar's edges, each letter cut out, a single full flush
  after the commit, and that a soft reset draws and sends nothing. Leaving out the flush, or
  drawing on a soft reset, fails it (both tried and reverted).
- **Firmware.** Builds with no warnings at 113,540 bytes of text from a clean tree, 228 more than
  stage 1. The 113,544 in the commit message was measured on a dirty tree.
  `shutdown_user` links as a strong symbol over QMK's weak default.

## Stage 3: the PAGE key, the page framework, the three bands

- **Key.** `U_PAGE = QK_USER_2` in the keymap's `config.h`, at matrix [1,6] (`L12`), in **both**
  `LAYOUT_miryoku` variants. It acts on release, like SYS and ANIM. Update *Extra keys* from
  "planned" to the real entry.
- **Persisted** in the existing tagged EEPROM word (`oled_slave_select.c`). Byte 0 stays the
  animation; byte 1 takes the page in bits 0-3 (nine pages since D1 grew) and the map style in bit 4
  (stage 4). An old word without those bits reads page 0, grid style, so nothing needs migrating.
- **Pages.** A small dispatcher in front of `oled_render_master()`. The bands become one renderer
  with a base-band hook: the Kyria emblem (today), the small kitty (emblem area cleared, kitty at
  (18,12), wordmark kept), the striped kitty the same way, or the QMK logo (below y=17, see
  stage 1). The kitty's state comes from
  WPM (D9) on the master's own clock. The Stats page is the existing `render_stats()`.
- **Dots.** For 700 ms after a press, one dot per page in the middle of the status bar, the current
  one larger.

**Tests:**
- every page's second render writes nothing (the redraw-cost test, per page)
- each page stays under the alarm ratio
- the cycle wraps
- a replug keeps the page

**Checkpoint:** cycle through all five pages that exist so far (the four bands and Stats), then
replug.

**As built, 2026-10-22.**
- **The key.** `U_PAGE = QK_USER_2` at the start of row 1 in both `LAYOUT_miryoku` variants, acting
  on release. The *Extra keys* row in `docs/layout.md` was already the real entry once D7 was
  decided.
- **The settings word** moved out of `oled_slave_select.c` into `oled_settings.c` / `.h`, which
  both the animation and the page use, so the replug can be tested. Three things it does:
  - It keeps the tag `0x5ea1`, so the flash that adds the page keeps the stored animation.
  - Every write is read-modify-write of the whole word. An ANIM press keeps the page, and a PAGE
    press keeps the animation.
  - A first PAGE press on a fresh EEPROM stores the build's animation default beside it.
- **The stored page.** It is the page's number in D1's order (`enum u_master_page` in `oled.h`),
  not its place in the cycle, so building a page later doesn't move the others. `U_PAGES_BUILT`
  says which pages exist, and PAGE skips the rest: for now the cycle is Bands, small kitty, striped
  kitty, QMK, Stats. A stored page this build hasn't got reads as the bands. Byte 1 holds the page
  in bits 0-3, since D1 grew to nine pages, and PAGE never touches bit 4, the map style's.
- **Both halves keep it**, since 2026-10-22, after the replug check. The page came back on a
  replug into the same half, but the other half brought back its own, the same as the animation
  always had.
  - The payload grew to eight bytes. Its `view` byte carries the master's stored byte 1.
  - The other half saves that and the animation through `u_settings_follow()`, once a fresh state
    has arrived and only when something differs.
  - The design is in `docs/layout.md`, *Switching the slave's animation*.
  - Tests: the new `test_sync_slave.c` checks, from boot, that nothing is fresh before a state
    arrives, even inside the first 600 ms. `test_panel.c` checks the save: one write when
    something differs, none when nothing does, the map-style bit kept, and the page showing after
    a replug. The scenarios check that `view` carries the stored byte. Five breakages each failed
    them. The one line in `oled_slave_select.c` that saves only from a fresh state isn't compiled
    by any host test, so the board checks it.
- **The kitties.** Each clears x 0-57 of the base band and sits at (18,12), with the wordmark
  kept. One renderer draws both, given the art and its mood table.
  - The striped one has three paw frames of her own, so her four tiers all look different.
  - Frames change every 500 ms, or every 300 ms in the top tier.
  - The tier runs in `oled_master_task` on every main loop, whatever the page, so a kitty shows
    your current speed the moment you turn to her.
  - `speed_tier.h` holds the tier code Bjorn had. He now uses it too, his test passes unedited,
    and the `tiers` preview renders byte for byte as before.
- **The QMK logo** sits at (1,21), centred in y 18-47. Its ink starts two rows into the art, so
  y=16 would also have cleared the corner. The test checks the corner itself, which fails from
  y=12 up.
- **Both versions are drawn before the alarm**, which still fills the base band whichever version
  is showing.
- **The dots** use the mock's geometry: one per page PAGE can reach: six after the Monitor stop,
  at x 37-89, and ten after stage 5, at x 21-105, the current one 5x4 and the others 3x2, for 700 ms. They are drawn
  last, over any page, including the Stats page's first line.
- **The Stats stop** calls `render_stats()` directly, so no key or timer closes it. Holding SYS
  still opens the overlay from any page, and a key still closes the overlay.
- **Neither shows the alarm**, the same as the SYS page never has. While Stats is up, a lost link
  shows only in its FAIL and OUT lines. Whether stage 8's glitch bursts should reach every page is
  open.
- **Tests** in `test_panel.c`:
  - the cycle, the skip, the wrap and the replug
  - the old word, an unbuilt page, the map-style bit, and that the slave never turns the page
  - the dots, where they sit, that they are free to redraw, and that they go
  - the small kitty, pixel for pixel in all three looks, with Bjorn's hysteresis
  - the striped kitty asleep, her three paw frames, and bouncing
  - the QMK logo, and the OS corner with iOS set by hand on all three versions
  - every other layer, and the alarm, identical to the bands page
  - Stats as a page and as the overlay
  - on every page, a free redraw for five busy states, and staying under the alarm

  Seven breakages each failed it: no read-modify-write, the kitty drawn over the alarm, a narrower
  clear, no skipping, no fast frames, the striped stop drawn with the small kitty, and a three-bit
  page mask. All were tried and reverted. `--dump` and `--raw` now
  include the new pages.
- **Lit pixels**, the most over the busy states and three speeds: Bands 2,935, small kitty 2,392,
  striped kitty 2,486, QMK 2,651, Stats 994. The alarm frame lights 4,584.
- **Firmware.** It builds with no warnings at 117,468 bytes of text from a clean tree, 3,928 more
  than stage 2. Most of that is the kitties' frames: ten small and thirteen striped, 2,944 bytes.

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
- Persisted in byte 1 bit 4.
- It already crosses to the slave in `view` bit 4, the stored byte 1 sent since 2026-10-22, so
  `world` keeps bits 3-7 for the daemon.

**Master screen moves over.** *Built, then taken out on 2026-10-22: see D3.* While the map covers the master's panel and not the slave's, the
slave draws the bands page from `u_sync_state()` instead of Bjorn. That page reads only what the
eight bytes carry (layers, fingers, flags, misfire count, host), and it already mirrors the mod row by
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

The last two were for the screen moving over, which D3 took out. What remains is the first, in both
cable positions.

**As built, 2026-10-22.**
- **Files.** `oled_map.c` / `.h` hold the side table and both styles.
  `map_legend.c` / `.h` hold the keycode-to-legend table. Both are in `SRC`.
- **Where the keys are.** The keymap's own `LAYOUT_miryoku`, given the slot numbers 1-40 in place
  of keycodes, builds a matrix of slot numbers, which is inverted once. So the map and `keymaps[]`
  agree by construction, whichever thumb mapping is built. The shaped map's thumb arc is keyed by
  matrix column (`keyboard.json`), so it follows the thumb mapping too. The test checks every key
  against the position table above.
- **Legends.** Each is up to three cells of Ein or drashna's 6x8 icons, or one 12x8 mod icon, and
  can be dimmed.
  - The big grid draws single glyphs and icon-only legends at 2x. The Mac's sun and triangle count
    as one glyph, as approved.
  - A pressed key's box is capped at its cell: 25 x 16 in the grid, 19 x 12 in the shaped map.
  - Unknown keycodes are flagged, and drawn as a dot.
- **Coverage.** As planned, the test preprocesses Miryoku's layer macros. A scenario test does it,
  because the scenario build's `keymaps[]` is the framework's own, so `keymap_key_to_keycode()`
  there checks nothing. It includes `custom_config.h` and defines `RGB_MATRIX_ENABLE`, so the RGB
  keys are the `RM_*` codes the firmware has. The table lists both RGB families and QMK's current
  mouse names, so it needs no shim of its own. Removing any of three legends failed the test.
- **D15.** The firmware's Fun and Nav maps, both styles on both hosts, match the render D15 was
  decided from, byte for byte.
- **Never over the alarm**, decided while building and written into `docs/layout.md`. The master
  shows its map only while the link is up. The other half shows a map, or the master's screen,
  only from a fresh state.
- **Three glyphs**, after the early board check. Ein's `@` lacked a column and is QMK's now; its
  `|` and `*` are ZXPIX's, placed in Ein's slot (`docs/layout.md`, under *The font becomes Ein*).
- **The early check** passed on `12b236a`: the half without the cable lights its own presses.
- **The master's screen moving over** was built and flashed in `d267d99`, then taken out the same
  day (D3): on the board it confused more than it helped. The other half keeps its animation while
  the master shows a map. Bjorn's clocks still pause while a map covers his own panel.
- **Tests.**
  - `test_map.c`: positions, sides, the right half's order, boxes inside their cells, the glyphs, a
    free redraw, and staying under the alarm.
  - `test_panel.c`: the master's map, the style toggle and the alarm rule.
  - `test_map_legend.cpp`: coverage, D15's legends and the mods following the host.
  - Deliberate breakages each failed them.
- **The brightest map** lights 2,004 px, with every legend three letters wide and four keys held,
  against an alarm frame of 4,584.
- **Firmware.** It builds with no warnings at 120,964 bytes of text from a clean tree, 3,384 more
  than `70f3bc0`.

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
  or stub the cat. Decided 2026-10-22: stub it; the cat joins in stage 6.
- **Event log.** A RAM ring of eight `{uptime, event}`, RAM only like the rest of the link evidence.
  The events: boot; link up and down (the edges of `u_sync_link_down()`); a self-heal reset (the
  `resets` count in the slave's reply going up); the OS set by detection or by SYS; a misfire.
  Stamped `+1:04`, since there is no wall clock.

**Tests:** the redraw cost and the alarm ratio per page, and the event ring's wrap and order.

**Checkpoint, on the board,** on both hosts:
- PAGE through all ten pages: ten dots, and each page as mocked.
- Status sheet: WPM and the kitty while typing; a held thumb layer inverts its name; Caps Word,
  Caps Lock and Scroll Lock invert; each held home-row mod inverts its icon over its finger; SYS
  shows the other host and `hand`.
- Layer map: Base at rest, a pressed key inverted; hold Space and the page shows Nav with Space
  inverted. A layer whose live map is on the master's half still shows the live map there.
- Typing: the graph moves and the peak holds.
- Event log: the boot line with the build's label, the link up, the OS; then SYS, a misfire, and
  the TRRS out for a few seconds and back, which logs down and up with the seconds it was gone.
- With the TRRS out, the Status sheet, Layer map and Typing show the bands and the alarm; the Event
  log and Stats keep showing.

**As built, 2026-10-22.**
- **Files.** `oled_draw.c` / `.h` draw at any row: boxes, art, legends, text. The live map moved
  onto them, and its frames did not change by a byte. `oled_layer_page.c` / `.h` draw the Layer map
  page from keycodes alone, `oled_pages.c` / `.h` the Status sheet and Typing, and
  `oled_event_log.c` / `.h` the Event log. `oled_master_zones.c` picks the page.
- **Legends.** `u_map_short` in `map_legend.c` gives the page's one cell a key; the thumbs keep
  `u_map_legend`. Both unwrap mod-taps and layer-taps. The F-keys 10 to 12 are `U_LEG_TIGHT`.
- **The held layer key** is found on the page itself: a pressed slot whose default-layer keycode is
  a layer-tap to the layer shown. The master's matrix covers both halves.
- **Two signals the state did not carry.** The first link up is the first reply
  (`u_health_have_reply()`), because before any send has failed `u_sync_link_down()` has no edge to
  give. A Windows detection changes nothing in the state, so `host_os.c` now says whether detection
  has reported (`host_os_detected()`). The build label comes from `u_build_label()` in
  `split_health.c`.
- **The mock match.** A scenario test draws the twelve Layer map frames from Miryoku's real layers,
  and `tests/run_unit.sh --raw-pages` the four Status sheets, Typing and the Event log. All 18
  match the mock byte for byte. The mock's third Status sheet was redrawn with Extra as the layer,
  the state the firmware can reach (`docs/layout.md`), and its Typing series as whole WPM.
- **Tests.**
  - `test_panel.c`: per page, a free redraw, under the alarm, and the link-down rule; the striped
    kitty; Typing sampling on every page; the ring's wrap, order, stamp and cut; each event.
  - `test_map_legend.cpp`: every key of every layer on both hosts, the one-cell legends, the Mac's,
    the held key's base legend, and no page over the alarm.
  - Breaking a legend entry failed them.
- **The busiest mock frame** lights 2,594 px, a Status sheet, against an alarm frame of 4,584.
- **Firmware.** It builds with no warnings at 127,028 bytes of text, 4,772 more than `e839e27`.

## Stage 6: bongo cat and the Mario scene on the slave

- `enum u_slave_anim` gains `U_ANIM_BONGO` and `U_ANIM_MARIO`: eight in all with the Monitor
  screens, exactly `U_WORLD_ANIM_MASK`. ANIM's row in *Extra keys* lists them.
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
  - **D13, when Yoshi curls up:** 4 s after the last key (decided 2026-10-22). He uncurls on the
    next key, back through *hide* reversed, then *idle*.
- **Tests.**
  - A host test runs the scene over a scripted WPM curve and asserts, for every tick, that Bill's
    lit pixels never meet Mario's or Yoshi's. The mock this was decided from passed that, 0 of 216
    frames.
  - The WPM corner (x 84-127, y 0-25) stays clear of every sprite.
  - Nothing reaches y < 0.
  - At WPM 0 with no Bill, an unchanged frame writes nothing.
- Add a scenario to `tests/preview/` for each animation, now that the preview builds again.

**Checkpoint, on the board:**
- ANIM through all eight on the other half: bongo cat and the Mario scene come after the Monitor
  screens.
- Bongo cat idles at rest, holds his paws up typing slowly, and slaps faster as you speed up.
- The Mario scene: at rest Mario stands and Yoshi idles; typing slowly Mario steps and Yoshi is
  ready; from 10 WPM they walk and run, the floor scrolls, and Bill comes by every few seconds and
  both jump him. Stop typing: Yoshi curls up after 4 s and uncurls on the next key.

**As built, 2026-10-22.**
- **Files.** `oled_slave_bongo.c` (with the WPM corner both scenes share) and
  `oled_slave_mario.c`; `oled_draw.c` gained a clipped sprite blit for Bill and the jumpers.
  `oled_slave_select.c` runs the Mario scene's task only while it is shown, as it does Bjorn's.
- **Measured from the art,** and checked against it by the test: both stand with their feet on
  y=62 at y0 = 31, so a 31 px lift puts Yoshi's apex at y=0 exactly; Bill's ink is x 2-29 and,
  at y0 = 39, y 41-61. Yoshi's fly frames ink x 2-26 of the sprite (frame 10 starts at 2, so the
  plan's 3 became 2), Mario's jump frame all 16 columns.
- **The jump** is planned in quarter ticks at Bill's speed, 60 + WPM fifteenths of a pixel a tick:
  when his nose reaches a character's right edge (rounded down) and his tail clears its left edge
  (rounded up). Over about a hundred passes Bill never touched either of them; a jump 1.5 ticks
  early or late does.
- **Resting** waits for Bill: Yoshi curls up only once nobody is in the air or on the way, and Bill
  stops coming 4 s after the last key, so a pass in flight finishes first.
- **Not in the plan, and chosen while building:** Yoshi's *idle* and *ready* sets at 300 ms a
  frame, his flutter at 100 ms, a curl step every 150 ms, Mario's step at bongo cat's rate from the
  first key, and three tufts at x 9, 51 and 94 of the scrolling floor. Uncurling goes back through
  *hide* and then to whatever the WPM says: *idle* at 0, *ready* if the key left any WPM.
- **Previews:** `tests/preview/run_preview.sh bongo` and `mario`, from `tests/preview/scenes.c`.
- **Firmware.** It builds with no warnings at 135,256 bytes of text, 8,228 more than `636ba56`,
  most of it the two scenes' art.

## Stage 7: encoder feedback

`encoder_update_user()` records the direction and the time. For 800 ms the master shows the speaker
or bulb icon at 2× with `VOL+`, `VOL−`, `BRI+` or `BRI−` (D11 for pages other than the bands). MUTE
shows only if a mute key exists; the encoder click stays an open experiment in `docs/layout.md`.
Measured on the renders at 2,663 px, so it goes through the alarm-ratio test like a page.

**Checkpoint, on the board,** with the cable on either half:
- Turn the left encoder on the bands: VOL+ or VOL− in the band, the OS logo whole, gone 800 ms
  after the last turn. The right encoder: BRI+ or BRI−. Media's Mute: MUTE.
- On the Status sheet, Layer map, Typing, Stats and the Event log: the same box, centred.
- With the TRRS out on the bands, the alarm and no box. Holding a layer whose map is on the
  master's half, no box over the map.

**As built, 2026-10-23.**
- **Where.** `encoder_update_user()` in `keymap.c` calls `u_encoder_note()`, which latches what
  and when; `oled_master_zones.c` draws it. QMK runs both encoders' callbacks on the master, so the
  other half's encoder shows there too.
- **The box,** from the October mock, now in Ein: 90x30 with the mod row's chamfer, the icon and
  the four-letter label cut out at 2×. The mock's chamfer helper also notched the box's sides
  halfway down, an artefact of reusing a 16-row helper; the firmware chamfers the corners only.
  The OS logo stays whole: the band is cleared and the toast drawn before the logo, which the D11
  render had the other way round.
- **D11.** Over any other page the same box at y=17 with a two-pixel dark rim. It matches the D11
  render byte for byte over the Status sheet, Typing and the Event log
  (`tests/run_unit.sh --raw-pages`).
- **Never over the alarm,** following *Pages give way to the alarm*: on the bands with the link
  down there is no box, and the pages that give way show the bands. Stats and the Event log keep
  it, having no alarm to hide. It is not drawn over a live map.
- **The alarm ratio.** The busiest Status sheet with the box lights about 2% more than the alarm
  frame; kept that way on 2026-10-23 over a full-width strip (`docs/layout.md`). Every other page
  and state stays under.
- **MUTE** shows for Miryoku's Mute on the Media layer (`KC_MUTE`, noted in
  `process_record_user()` on press), with drashna's struck-through speaker. Added on 2026-10-23
  after the first build had tied it to the encoder's unassigned click only.
- **Firmware.** It builds with no warnings at 135,668 bytes of text, 412 more than `42f140a`.

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
