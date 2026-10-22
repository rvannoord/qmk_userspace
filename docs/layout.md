# Layout

The source of truth for my layout. A change goes here first, then into the firmware (both firmwares once the Halcyon exists).

## Basics

- Miryoku, 36 keys. The Kyria's other keys are unused (`XXX`) except the ones under Extra keys.
- Base: Colemak-DH, done in the firmware. The OS stays on a QWERTY-family layout.
- Thumb arc: five positions per half, counted from the outside edge. Position 1 is a rotary encoder on both halves (left volume, right screen brightness; push-click unassigned). `MIRYOKU_MAPPING = EXTENDED_THUMBS` puts the thumb keys on 3–5, leaving position 2 free.

  ```
  position:   1     2     3      4      5
  left:       ⟳     ·     Esc    Space  Tab
  right:      ⟳     ·     Del    Bksp   Enter
  ```
- Other Miryoku options are at their defaults: Extra layer QWERTY, Tap layer Colemak-DH, Nav with arrows on the right home row, layers not flipped. Clipboard is `WIN` (see Hosts).
- OS layout: undecided, see Open experiments. Today: US-International on regular keyboards.
- Mostly English. Swedish (å ä ö) is wanted later, not a priority.
- Goal: keyboard first, less mouse, vim-style.
- Hosts: Windows at work, Apple Silicon Mac at home. OS detection swaps Ctrl and Cmd on the Mac, so shortcuts use the same fingers on both. QMK also tells Linux and iOS apart (since 2026-10-21 the panel shows which): Linux behaves like Windows, and iOS like the Mac, swap included, because iPadOS shortcuts are Cmd-based. Android hosts report themselves as Linux, and nothing here tries to tell them apart. Side effect: on the Mac, Ctrl moves to the pinky, which you feel in the terminal and nvim. The alternative (mods stay put, only the clipboard keys adapt) stays open as a later switch.

## Extra keys

| Key | Where | Does |
|-----|-------|------|
| SYS (`U_SYS`) | left half, outer pinky column, top row | flips between the Mac and the detected non-Mac host (Windows, or Linux on a Linux machine) by hand when detection guesses wrong; the left OLED shows the result. Held two seconds, the stats page. Held while plugging in, it enters the bootloader (Bootmagic) |
| ANIM (`U_ANIM`) | right half, outer pinky column, top row, mirroring SYS | cycles the other half's animation: Bjorn, spaceship, Kyria logo, Monitor 1, Monitor 2. Persisted on both halves, so it follows the cable. See *Switching the slave's animation* |
| PAGE (`U_PAGE`) | left half, outer pinky column, middle row, under SYS (decided 2026-10-22: free on every layer, and the pinky reaches it while a thumb holds a layer) | cycles the master's pages, on release like SYS and ANIM. While a layer map is showing it switches the map between the big grid and the Kyria-shaped one instead. Persisted on both halves, so it follows the cable. See *The drashna round* under OLED; build order in `docs/oled-v3.md` |

## Home-row mods

GUI, Alt, Ctrl, Shift from the outside in, mirrored on both hands (on the Mac, Ctrl and Cmd swap).

Current plan, chosen to match the old bilateral-combinations patch as closely as stock QMK allows:

- `CHORDAL_HOLD`: a home-row mod plus a key on the same hand within the tapping term (200 ms) types letters.
- `FLOW_TAP_TERM 160`: a home-row mod pressed within 160 ms of the previous key is a letter. Shift is exempt.
- `get_chordal_hold()`: an opposite-hand key within 80 ms of the mod (120 ms for GUI) is a roll, so letters.
- Thumb layer keys are exempt from both.
- No `PERMISSIVE_HOLD` yet.

Known differences from the old patch:
- A same-hand chord held longer than 200 ms becomes a shortcut (the old patch waited 3 s). This one is welcome: an occasional one-handed shortcut with the mouse in the other hand works if the mod is held a moment first.
- Flow Tap only counts letters, Space and `. , ; /` as the key before. After Backspace, Enter, digits or arrows, a quick home-row press can still turn into a mod.
- GUI is no longer sent 120 ms late, so on Windows a GUI home-row key held a little too long flashes the Start menu.
- A left mod and a right mod now combine.
- A home-row key that settles as a letter key-repeats while held.

## Layers

### NUM (Miryoku default)

```
 [  7  8  9  ]
 ;  4  5  6  =
 `  1  2  3  \
       .  0  -
```

### SYM (Miryoku default, untouched)

The symbol layer is the number layer with Shift held. Every key is the shifted version of the key in the same spot on NUM: `7` → `&`, `[` → `{`, `0` → `)`, and so on. Learn NUM and you already know SYM.

```
 {  &  *  (  }
 :  $  %  ^  +
 ~  !  @  #  |
       (  )  _
```

`(` sits on both the top row and the thumb. The thumb copy is the easy one to reach.

## OLED

Two SSD1306 panels, 128x64, 1-bit, one per half. Specified 2026-10-10; what is built and what
is not is listed under **Built and still to do** below. The master half (whichever has the USB
cable) is the instrument panel; the other half is the mascot. Plug into the same half every time,
or the two swap places.

### Mascot: Bjorn

The character is **Bjorn**, from `github.com/infinition/Bjorn` — a Tamagotchi-like Viking that
lives on a Raspberry Pi e-paper display. Not an imitation: his own 78x78 sprites, box-filtered to
64x64 and dithered. He comes with a goat, which is already in his idle animation.

- **Floyd-Steinberg, region-locked.** Plain threshold is crisper and cheaper (275 changed px per
  frame step against 639) but loses the beard texture; the dithered look was chosen deliberately.
  Region locking dithers frame 0 normally and keeps those bits wherever the *source* greyscale did
  not change, so the dots stay put instead of re-rolling every frame.
- **Dither each 64-wide column separately.** FS diffuses error rightward, and a moving object sits
  outside the lock by definition, so with one 128-wide pass a move in one half dirties blocks in
  the other. Two passes, error never crossing x=63.
- **Lit on black.** The default, and where this ended up after trying the other way on the board.

  **A set bit is ink, always** — that never changed and is not what this setting is about.
  `U_OLED_INVERT` sends the SSD1306's own invert-display command, so it changes how the panel
  *reads* the buffer and nothing else. Every asset here works under either polarity with no
  regeneration; swapping is one define.

  The history, because the reasoning on both sides is still worth having. Inversion shipped in
  `a6bd2f9` on the argument that Bjorn's source is dark ink on a white page — an e-paper original,
  so dark-on-lit is the medium he was drawn for, and it is what makes the hooded poses legible as
  a solid cowl instead of a 33% blob. Against it: inverted measures about 77% lit against 24%,
  which is a lamp beside the keyboard. Looked at on the board on 2026-10-15, the lit field was not
  wanted, and it reverted.

  **The failure mode flips with the polarity, and that is the part that matters for new art.**
  Lit-on-black eats isolated *lit* pixels; inverted it eats *dark hairlines* in a lit field. The
  lock glyphs were drawn at 1 px for the lit-on-black regime and are correct again. Anything
  authored while the panel was inverted should be re-read against this — the weather sprites in
  particular were measured for hairline ink, which was the inverted-panel concern.

  One useful consequence: `OLED_BRIGHTNESS` is back to **255**. Full brightness is affordable
  precisely because lit-on-black lights about a quarter of the pixels, so 255 now is a lighter
  load than 128 was inverted.
- Row 63 of his sprite is a grey rule. Kept as a floor and run across both halves, so the two
  columns read as one scene. Static, therefore free.

### Slave half: Bjorn x=0-63, his world x=64-127

Each 64x64 column is exactly 8 of the driver's 16 dirty blocks, so an actor's full redraw spends
the whole scene budget. **8 blocks per animation step**, bid for: fault > alarm > one-shot
reaction > Bjorn pose > goat locomotion > ambient. Over budget defers a move by one step rather
than dropping it. At most one actor initiates per step, the other limited to 2 blocks of ambient.
**While typing the budget drops to 2 blocks** — ambient only — which makes "nothing loops while he
types" a number the compositor enforces rather than a rule to remember.

**What the budget is actually protecting, corrected 2026-10-14.** Not typing feel. The driver
sends at most one block per main loop (`OLED_UPDATE_PROCESS_LIMIT` is 1, `oled_driver.c:457`), so
the I2C cost of a frame is about 1.7 ms per loop whether one block changed or all sixteen — more
dirty blocks make the *panel* lag, not the keyboard. The evidence is the old spaceship, which
repainted every frame for months and never felt mushy, and Bug A, which did the same thing
accidentally for just as long without anyone noticing it in the typing. What the budget buys is
display latency — sixteen dirty blocks take sixteen loops to appear, so a frame that changes
everything is already a fifth of a second behind — and restraint, which is the design's real
concern. The number that would show typing being hurt is the main loop rate, which the stats page
reports.

Ambient clocks are non-harmonic so the scene never looks metronomic: breath 3.6 s, goat tail
1.9 s, goat ear 4.3 s, blink pseudo-random 6-10 s.

| Trigger | Response |
|---------|----------|
| WPM tier (up at 10 / 30 / 50 WPM, down at 6 / 24 / 42, in `speed_tier.h`, shared with the master's kitty) | horn posture, four static poses from IDLE8 / IDLE1 / IDLE3 / IDLE9. No speed lines: motion in peripheral vision taxes concentration, and WPM is the one signal with nothing to act on |
| Command modifier held | axe into his hand, and for 800 ms after it is released. Two dirty blocks, seated beside him inside the left column so it never crosses the dither seam and leaves the right half free for the goat. Disjoint from the blink window, so the two compose with no combined variants. **Shift is excluded** — here it is a home-row mod, part of typing a capital rather than an instruction, and it is held so often the axe would simply always be there. The trailing dwell is what stops a 100-300 ms hold strobing the prop. An earlier rule fired on a *completed* command instead, and was wrong twice over: with Chordal Hold most same-hand shortcuts resolve as letters, and Miryoku's clipboard keys are not modifiers at all, so the event barely happens |
| Misfire: a command modifier held past 500 ms with nothing typed | the axe raises overhead over three frames and holds, 1750 ms in all, with his horns drooping. The only thing in the design that animates, on the principle that the only thing which moves should be the only thing that is wrong. The steps *slow* as it rises — 100 ms taking hold, then 150 and 200 rising, 1000 ms held at the top, and three 100 ms steps back down — because a lifted mass decelerates toward the top of its arc, and that is what makes it read as heavy rather than as a sprite being repositioned. Measured 4, 5 and 4 blocks for the raise; 8 entering or leaving a non-drooped posture, 3 leaving to the drooped one. The whole figure cuts to the drooped posture rather than splicing horns into the current one: the tier progression is an arousal scale so that pose already exists, half a droop reads as damage where both falling reads as deflation, and since nothing was typed the typing-speed posture carries no information worth preserving. A misfire outranks the held axe and **cancels** it rather than suspending it — resuming a stale success indicator afterwards would read as a second event that never happened |
| Caps Word | a three-point crown, then his tile flips to dark-on-white 100 ms later. Inverted he reads **better**, not worse: his art is an e-paper original drawn as dark lines on a white page, so flipping restores the polarity the linework was made for. The bloom objection was measured rather than argued — 26 isolated pixels of 983, and inversion is a bijection so that count is identical in both polarities, bounding the loss at 2.6%. The earlier 77%-lit figure still stands but argued about *sustained* brightness, not legibility. Done at runtime as one XOR per byte, so every frame — blink, axe, misfire, breath, all four postures — composes unchanged and gets its inverted form free. Crown first and the flip a beat later makes the loudest event in the design feel authored; leaving is both at once, slow in and quick out. **Pages 0-6 invert with `0xFF`, page 7 with `0x7F`**, preserving row 63: the floor rule runs across both halves, and flipping only the left of it would put the two halves' dots a pixel out of phase and recreate the seam step the floor exists to remove. The right column never inverts — a white card beside black reads as deliberate, and it is a defined register one step below full-panel inversion |
| Caps Lock | crown plus a 1 px edge frame (y=0, y=63, x=0, x=127). Nothing else ever draws there, so lighting it is unambiguous. 380 lit px |
| Default layer != Base | helmet off, head bowed, axe dropped at his feet, plus the edge frame. A defeated bare-headed Bjorn reads instantly as "you are silently on QWERTY"; the emotion carries the meaning |
| Split link stale >1 s | frozen mid-pose, blinking stops. The absence of a blink is the error message |
| Idle 20 s | the goat may start moving: wanders in, grazes, looks up, wanders off |
| Idle 45 s | Bjorn pets the goat — once, ~1.5 s, settling into a held tableau. Requires the goat to have wandered within reach, so the wander is the setup and the pet is the payoff. Deliberately not guaranteed; a beat on a fixed timer is wallpaper within a week |

The goat moves only when idle. It carries no information at all, and a creature walking across the
panel is translation across a large area, which is the most attention-grabbing thing either screen
can do. A keypress mid-wander lets it finish the current step and stand — never freeze mid-stride,
because a raised leg keeps pulling the eye and frozen-with-no-blink already means the link died.

Deliberately excluded: per-layer expressions (a layer may change a prop, never an expression),
typing metrics beyond the WPM tier, lock states, and encoder reactions. The rule: an expression is
for something wrong, a prop is for something true, idle is for everything else.

### Master half

Three bands of pictures and no text, modelled on splitkb's own Halcyon TFT panel
(`hlc_tft_display.c`) and adapted to 128x64 monochrome. All page-aligned:

| Zone | Origin | Size | Content |
|------|--------|------|---------|
| Status bar | (0,0) | 128x8 | drashna's 12x16 host logo at the left (since 2026-10-21; it was `Win`/`Mac` text): Windows, Apple for the Mac, Tux for Linux. iOS is the Apple logo with a small lowercase "i" in front of it, in Apple's own lettering style, so it reads as an i-device (decided 2026-10-21). The "i" is 2 px wide and 10 rows tall, a row shorter than first drawn, with a 2 px gap before the logo, which moves 4 px right for it. The logo is twice the bar's height, so its lower half sits in the band's top-left corner, x 0-15 by y 8-17, which every band keeps dark (in those rows the nearest band pixel is the Kyria emblem at x=18). Underlined at y=16 when SYS set the host by hand, under the "i" too on iOS. The right end is permanently dark, so the latched locks live there at x=109 and 118 — one row, so they cannot drift off a shared baseline. Both are drashna's cells: Caps Lock, which Caps Word lights too, as on drashna's own screen, and Scroll Lock |
| Layer band | (0,8) | 128x40 | one picture per layer, from `panel_layer_icons.c`. Three rows under the icon when a held layer is hiding a non-base default. **This band is also the alarm**: the link being down inverts the whole of it and cuts a broken chain out of it. The OS logo's lower half and its underline are cut out of it then too, rather than lost in the lit field |
| Mod row | (0,48) | 128x16 | four 32 px cells, one per finger, each showing its glyph **always**: drashna's filled 12x8 icon (⊞ ⌥ ⌃ ⌘ ⇧, since 2026-10-21), centred in a 16x16 slot. A held finger fills its whole cell and the glyph is knocked out of it; a finger that just misfired fills only the glyph's own 16 px box. Both fills are chamfered, 3 px off each corner, so a lit cell reads as a key rather than a black rectangle. **The order reverses with the half the panel is on** |

**Why the layer is a picture now.** The name was text because text was what a 16 px band
could hold. Forty pixels holds a drawing, and a drawing is read without being parsed —
which is the whole point of a layer indicator you glance at mid-word. The icons are
splitkb's own, lifted out of the Halcyon module's QGF files and reduced; base is the
Kyria logo out of QMK's default keymap for this board. Nothing is drawn from scratch.
See `VENDORED.md` for the sources and `tools/panel/` for the reduction.

**The font is Ein** (since 2026-10-21), one of the faces in drashna's font file, transposed into
this panel's row-major form by `tools/panel/gen_font.py --drashna`. Its lowercase is small capitals.
Before it came QMK's own 6x8, the face `oled_write()` draws with (from 2026-10-15), and before that
DUNGEON.mode. Since the badge became a logo the font draws only the stats page, until the pages of
`docs/oled-v3.md` add more; the 2x and inverse-text paths went with the layer name that used them.
The QMK splash logo is still QMK's own, carried in the same generated file.

Glyphs sit **flush left** in an 8 px cell, not centred. That was load-bearing while the badge was
text, because the "no 1 px edge frame" argument below depends on x=0 being lit; both OS logos light
it now. Ein, like QMK's face, is 5 columns of ink plus a spacing column, so the cell carries 3 px of
tracking rather than its native 1. Cell width stays 8 because the stats page is 16 characters
across 128 px.

QMK itself offers nothing better: all 45 `glcdfont*.c` in the QMK tree are within 0.6% of the
default over printable ASCII, bar two. That is why the different face had to be vendored.

**There is no layer map.** The first design had one at (30,40), 60x24, one 6x8 legend a key,
erasing itself per layer once that layer had been used enough. Cut before it was built: it only
repeats what the finger already knows by the time it can read it, and it was the one zone whose
size forced the 6x8 font on everything else. The 24 px it held went to the event strip, which
shows the things the layer name cannot.

**Reversed on 2026-10-21 for a different kind of map.** The argument above is against a map that
sits in a corner all the time. The one decided in *The drashna round* below appears only while a
thumb layer key is held, takes a whole panel, and takes it on the half under the fingers that use
it, so it is read before the finger knows, not after. That matters while Colemak-DH and Miryoku's
layers are still being learned.

**The row reverses with the half.** Outer to inner is pinky, ring, middle, index, and on the
left half that runs left to right across the keys. On the right half the same fingers run the
other way — the index finger is on the *innermost* key, which is the leftmost one on that half —
so a row drawn pinky-first there sits mirrored over the keys and every glyph is above the wrong
finger. `is_keyboard_left()` on the master decides it. The cable moves between halves often
enough on this keyboard that this is a real case, not a hypothetical one.

**The misfire chip looks like a bug until you know it.** Verified on the board 2026-10-15 and
reported as a fault: *"the black outline doesn't disappear, but becomes narrower around the actual
icon and stays there."* That is an exact description of what it does — a 32 px hold replaced by a
16 px chip for 1.5 s — and the shrink reads as the held state failing to clear rather than as a
new statement. It is kept anyway, deliberately: the chip is half a hold so that pressing the same
finger again *grows* it back to a full cell instead of switching vocabulary, and that is worth
more than first-glance legibility for something seen a few times a day. A hollow outline and a
single blink on arrival were both considered and rejected — the blink because blinking belongs to
the alarm alone. If this is revisited, those are the two candidates.

**Handedness is not drawn.** A 3x16 bar used to fill the padding on the side of the hand that
held the key, cut out of a filled cell and drawn beside a chip. It went because four cells plus
two bars plus a chamfer is more marks than meaning in 16 px, and the bar was the least of them:
the cell already names the finger, and which hand is the one thing you can feel without looking
up. What is given up should be recorded honestly — Chordal Hold resolves on the *pair* of hands,
so a same-hand and a cross-hand hold are now drawn identically, and only the misfire chip reports
a bad one. `U_FIN_LEFT`/`U_FIN_RIGHT` stay on the wire; nothing on screen reads them.

**Mod slots are fingers, not modifiers.** Outer to inner: pinky, ring, middle, index. On Windows
those keys give GUI, Alt, Ctrl, Shift; on macOS the Ctrl/GUI swap means the same fingers give
Ctrl, Option, Cmd, Shift — the order on a MacBook's bottom row. Only the glyph changes with the
OS. `get_mods()` returns post-swap bits (`mod_config()` runs at keycode-to-action translation,
`quantum/keymap_common.c:162`), which is why the display used to light the wrong slot on the
Mac. Fixed: the slot comes from the key's *declared* mod, so it never moves, and liveness is
checked against the bit the key actually registered — recorded when it was pressed, because
`mod_config()` is applied at press time and an OS change while a key is held must not
re-attribute it to another finger.

**The mod row filled, went dark, and filled again — and the measurement is the reason
all three times.** The first build drew all four glyphs and filled a cell per held
modifier. Measured, that put the most routine event on the keyboard into the alarm's own
register: the alarm row inverted 128x8 = 1024 px, and two held modifiers filled 2 x 512 =
exactly 1024. Ctrl+Shift, many times a minute, spent the whole rung. So the row went dark
at rest, and inversion was left entirely to the alarm row.

The bands reopened it from the other end. The alarm is no longer a 128x8 row but the
whole 128x40 band, so the ceiling it sets is about five times what it was, and a filled
cell at 405 px no longer comes near it — four fingers of both hands at once is roughly
1600 against the band's 4200. The constraint that forced "dark at rest" is simply gone,
and with it the reason to hide a legend that is useful to read. So the row is a legend
again: all four glyphs always, a hold filling its cell.

What did **not** change is the rule. Nothing routine may light more of the panel than the
panel's own alarm, and `tests/unit/test_panel.c` asserts the ratio rather than the layout,
so the next person to move a band finds out immediately if they have spent the rung.

**The lock glyphs are 1 px and must never be inverted in place.** Caps Lock and Scroll Lock
are drashna's font cells since 2026-10-21 (0xC9, a ⇪ over a bar, and 0xCB), which replaced the
hand-drawn padlock and struck ring. Wherever this panel had a hand-drawn piece and drashna has
one, his is used.

**Caps Word lights the Caps Lock glyph.** drashna has no Caps Word glyph: his screen ORs Caps
Word into the Caps Lock indicator, and this panel does the same since 2026-10-21. That retired
the hand-drawn Caps Word glyph and the middle of three slots, and the two that are left
right-align at x=109 and 118. A word in caps and Caps Lock left on now look the same. Caps Word
ends at the end of the word, so the glyph going out by itself tells them apart.

Both sit in 8x8 cells and leave column 7 empty, so the 1 px channel between neighbours cannot
close under bloom. drashna's cells are 5 px wide and 7 tall. At that size a 2 px stroke
leaves no interior, and 1 px is what the font's own symbol cells use anyway. Inverted they would
become dark hairlines in a lit field, which is the failure the mascot's work already measured.
Nothing inverts this row: the alarm is a different page. If that ever changes, this set has to be
redrawn, and these cells will not survive it.

**Why AltGr is not shown.** It is held, not latched: you are holding it at the moment
it would be displayed, so it carries nothing — the same reason the mascot's axe excludes
Shift. It can also be shorter than the 100 ms a flag needs to survive the 20 Hz tick,
so it would both strobe and be missed.

**Why Num Lock is not shown.** `KC_NUM` appears on no layer, so it cannot be toggled
from this keyboard, and the Num layer sends `KC_7`/`KC_4`/`KC_1`/`KC_0` rather than
keypad codes, so its state cannot change anything this keyboard types. `U_FLAG_NUM_LOCK`
stays on the wire as a reserved bit. Scroll Lock is the opposite case and is shown:
`KC_SCRL` sits on the Fun layer's home row between `KC_PSCR` and `KC_PAUS`, so it gets
hit by accident, and it then silently changes what the arrow keys do in a spreadsheet.
That is Windows. **The Mac has no Scroll Lock:** it reads the three keys as F13, F14 and F15,
and F14 and F15 are its screen brightness down and up. So on the Mac `KC_SCRL` dims the
screen, `KC_PAUS` brightens it, and the Scroll Lock glyph never lights (found on the board,
2026-10-21). Caps Lock on the Mac also needs the key held a moment: macOS ignores a very
short Caps Lock press.

Alarms escalate by **scope**, not by brightness, so routine events never spend the loudest
register. The rungs are per panel, and the test is ordering inside one panel: nothing routine may
light more pixels than that panel's own alarm. On the master: normal, then the inverted layer
band, then full-panel inversion. The band keeps its layer icon while inverted — lit on dark — so
the alarm adds to what is on screen rather than replacing it; the one exception is base, whose
band is given over to the chain entirely, because base is where you already are and the Kyria
wordmark leaves no gutter to put a chain beside. The master has no 1 px edge frame — the frame needs
x=0, x=127, y=0 and y=63 kept clear and a status panel cannot promise that, since the OS logo itself
lights x=0. Entry blinks three times over 600 ms, which routine changes never do.

Miryoku uses the left-hand mod-tap variants on both hands (`LSFT_T(KC_T)` and `LSFT_T(KC_N)`), so
`get_mods()` cannot tell which hand fired. Handedness comes from the matrix row, the same test
`tap_hold.c` already uses. Four cells, not eight: the pair of hands is what Chordal Hold decides
on, and one side of that comparison is misleading on its own.

### Split sync

One custom 8-byte RPC, master to slave, on change plus a 250 ms heartbeat and a 20 ms minimum
interval. Eight bytes rather than a packed four because the wire cost is 13 bytes of fixed overhead
against 43 us per payload byte — width is about 3% of a send, so the bytes go on clarity
rather than packing. The seventh, `host`, came on 2026-10-21 with the Linux and iOS logos: the
flags byte had no bit left. `U_FLAG_IS_MAC` stays in flags, because the mod row's order follows
it and iOS counts as a Mac there. The eighth, `view`, came on 2026-10-22: the master's page and
map style as it stores them, so the other half can store the same (*Switching the slave's
animation*). Widening a payload adds no transaction, so the handshake
constant is unchanged. Not `SPLIT_LAYER_STATE_ENABLE` / `SPLIT_MODS_ENABLE` / `SPLIT_LED_STATE_ENABLE`: those
re-send every 100 ms whether or not anything changed, and they cannot carry Caps Word (QMK has no
sync for it) or the host OS (`SPLIT_DETECTED_OS_ENABLE` ignores the SYS override). Layout: layer,
finger-ordered mods, locks, flags, the `world` byte (the chosen animation, with spare bits for
weather and time of day), the `host` byte (Windows, Mac, iOS or Linux), the `view` byte, and a sequence counter
the slave uses as its staleness clock.
The slave's callback may only `memcpy` into a volatile struct — it runs in a HIGHPRIO thread
holding the shared-memory lock, and blocking there costs up to 200 ms of frozen scanning.

No callbacks fire on the slave, so state changes are found by diffing on the render tick. Edges
are captured in `process_record_user`, thresholds evaluated in `housekeeping_task_user`, and
`oled_task_user` only draws. The render tick is 20 Hz, so any transient flag must be sticky for at
least 100 ms or it can be missed entirely.

**The sequence number counts delivered states, not builds.** `u_sync_build()` is pure: two builds
with nothing in between are equal. The sender stamps `seq` just before the transaction and only
advances it on success. It used to advance per build, and the master builds twice per main loop —
once to decide whether anything changed, once for its own panel — so a heartbeat landing 128 loops
after the last one carried an unchanged number and the other half aged itself into hard staleness
while every send was succeeding.

### Settings this needs

| Setting | From | To | Why |
|---------|------|----|-----|
| `OLED_TIMEOUT` | 60000 (QMK default) | 120000 | Deep idle starts at 20 s and the petting beat at 45 s, so at 60 s the best scene is visible for 15 seconds |
| `OLED_BRIGHTNESS` | 255 (QMK default) | 128, then back to 255 on 2026-10-15 | OLED wear is strongly superlinear in drive current. 128 was set while the panel ran inverted, about three quarters lit; lit-on-black lights about a quarter, so 255 is the lighter load of the two |

### Keeping the old art

The spaceship and the three logos are still compiled in beside Bjorn, and the `U_ANIM` key
cycles through all five at runtime; see *Switching the slave's animation* below.
`OLED_SLAVE_ANIMATION_*` in `custom_config.h` names only the boot default. The old text
panel for the master half was removed on 2026-10-16 (`oled_master_text.c`, git has it): the
three-band panel is the only master renderer.

### Stage 1, as built

Shipped in stages because the custom RPC is the risky part and nothing in stage 1 needs
it: WPM already crosses the link, and everything else here is local timing. Stage 1 is
Bjorn reacting to typing speed and nothing else. It is also the experiment that decides
the rest: whether a dithered 78→64 downscale is legible at 50-70 cm, tilted, at
brightness 128. **The test when flashing: from a normal seated position, without leaning
in, can the posture tier be named?** If not, the silhouettes need exaggerating before
stage 2 commits to smaller details like the crown and the axe.

| Constant | Value | Why |
|----------|-------|-----|
| WPM tiers, up | 10 / 30 / 50 | Low on purpose while Colemak-DH is still being relearned; the usual boundaries would mean never seeing the upper postures. Raise once speed settles |
| WPM tiers, down | 6 / 24 / 42 | Each strictly below its up threshold, so the two hysteresis loops can never both fire |
| Tier dwell | 800 ms | A tier hop repaints the whole column, 8 blocks. Hysteresis alone does not stop a bursty WPM reading twitching the posture |
| Settle | 8 s | Drop to the resting posture |
| Doze | 75 s | Eyes shut. Leaves 45 s of doze before the 120 s timeout |
| Blink | shut 150 ms on a cut, then 100 ms of half lid | Real lids close ballistically and open slowly, and that asymmetry is the most recognisable thing about a blink. Closure stays a cut because there is nothing to animate on the way down. Interval 6-10 s from a small LCG, and about one blink in five is doubled 400 ms apart, because real blinks cluster and irregular grouping is a stronger liveness cue than anything else at this size. The pair decision and the interval are drawn from separately advanced words, or "long gap" and "was a double" correlate perfectly and the pattern becomes legible within a minute |
| Falling asleep | 300 ms half lid, 500 ms shut, then the pose cuts | The same three lid frames as a blink, played slowly and forwards instead of quickly and backwards. Deliberately far slower than a blink, or it reads as a blink that happened to be followed by a pose change. It waits for the bottom of a breath before starting: the unlifted lid patch is built over the resting body, so dropping it on a risen frame would snap the body back down — and nobody nods off mid-inhale |
| Waking | correct pose immediately, eyes open over 200 ms | Waking from the panel timeout and waking from a doze are the same event, a keypress, and get one gesture. Cut the information, animate the character: the right pose for the current tier is there from the first frame with the eyes shut, and only the lids are delayed. Detected from a long gap between render calls, since the panel does not render while it sleeps |
| Doze breath | 6 s: 1.5 s in, 4.5 s out | Asleep he breathes slower, and asymmetrically — an even alternation reads as a metronome. The gesture is his beard rising, since he has no chest: his own outline pushed down one pixel, not new art. No blink; his eyes are already shut |
| Breath | 3.6 s, four states | His head and helmet **lift 1 px** while the beard stretches beneath them and his base stays pinned to the floor; the beard also pushes 1 px sideways over rows 41-52, so he fills as well as rises. 1 px is a hard ceiling, not a preference: his horn apex sits on row 1, so 2 px clips it against the top of the panel. Body leads and head follows in both directions, because the chest drives and the head is driven: rest 1.8 s, body fills 0.25 s, risen and held 0.8 s, body empties 0.4 s, head settles 0.35 s — about 1:2.4 in to out |

**Idle is gated on how long the WPM reading has been still, not on it being zero.** If the
split link drops mid-word the slave's copy of WPM freezes non-zero for ever, and a
`wpm == 0` test would leave Bjorn sprinting until the board is replugged. The half's own
key activity is OR'd in, so a local keypress wakes him without waiting for a sync.

Measured costs, from the generated art: blink **1 block** on every tier, breath 7, tier hop
8, doze entry 8, doze breath 6, half lid 1 (2 while risen), floor 1. Art is 7,232 bytes: a 512-byte base plus 105 patches of 64 bytes. A blink during a breath needs its own frame per swell state. The pixels do not
collide, but a patch is a whole 64-byte block, so a blink block built from the resting
pose carries un-swelled body edges either side of the eyes and flattens the swell where
they share page 4. Three blink variants — rest, half, full — cost 64 bytes each and make
the composition correct by construction. The host-side blitter harness caught this; it is
invisible in a pixel-level comparison. Stored as block-aligned patches rather than whole
frames, so a frame can be smaller than the column and the budget means something.

**Drawing: compose, then commit.** Both renderers draw into `u_canvas`, a 1 KB frame buffer in
RAM, and call `u_canvas_commit()` once at the end, which writes only the bytes that differ from
what the panel already holds. Zones and layers may still clear themselves and redraw — they must,
since drawing text leaves whatever was to the right of it in place — but that now happens in RAM.
Writing an intermediate value straight into the driver's buffer marks its block dirty even when
the final value is identical, and a dirty block re-arms the display timeout, which is how both
panels came to stay lit for ever. An unchanged frame must write nothing, and the host tests fail
if it writes anything.

### Motion rules

Four attempts at the breath failed before one worked, each for a different reason. These
are what came out of that, and they apply to everything that moves, including the axe and
the goat later.

- **Anything that moves, moves as pre-dithered bits at integer offsets.** Never re-dither a
  moving object in place. Region-locked dithering keeps a pixel's old bits wherever the
  source grey barely changed — right for something standing still, wrong for something
  travelling, because part of the texture stays behind while the rest slides over it. The
  third attempt measured 16-29% of his body holding still while the rest of him moved: fine
  in a still pair, reads as crawling on hardware.
- **A gesture must differ in *place*, not only in size or timing.** The first attempt moved
  rows 30-37; the blink moves 32-37. Same magnitude, same place, so it was read as the blink
  being irregular.
- **Dilation is not motion.** Growing the outline adds a rim of new ink, which above the
  crown reads as a halo rather than as rising.
- **Easing lives in hold durations.** Below about 4 px of travel there is no curve to draw,
  so spend ticks on rhythm rather than frames on in-betweens.
- **Put motion in long coherent edges, not in texture.** A 1 px push along a 12-row contour
  is visible where the same pixels scattered through stipple are not.
- **Overlap and lag belong to soft masses only** — the beard, a tail. Never the horns or an
  axe head: at a couple of pixels of travel, lag on a rigid part is dither noise.
- **Cuts for identity, motion for position.** Which pose he is in is a cut; where something
  is, is animated. The four postures are different sprites with different dither lineages, so
  an in-between would be a cross-fade between unrelated stipples. A cut is also right on
  attention grounds: 150 ms of change across 8 blocks in peripheral vision pulls the eye,
  where a single discontinuity is discarded by change blindness while you are concentrating.
- **An overlay is a whole 64-byte block**, so it must carry the current state of everything
  else in that block. A blink during a breath needs a frame built over the breathing body, or
  it drags the body back each time it fires.

### Files

| File | Holds |
|------|-------|
| `oled.h` / `oled.c` | the module's interface and its dispatcher. The header holds the slave-animation enum and boot default, `u_anim_get` / `u_anim_next`, the master's page enum in D1's order with `U_PAGES_BUILT`, `u_page_get` / `u_page_next`, the four entry points `oled_render_*` / `oled_*_task` and the pure sleep predicate; the `.c` draws the boot splash and then branches on master or slave, and paints the bootloader screen in `shutdown_user()` on the way into the bootloader. The sleep check itself is in `keymap.c`'s `housekeeping_task_user`, so a stray dirty block can light the panel for one main loop at most |
| `oled_master_zones.c` | the master panel. Three bands, status bar, 128x40 layer picture and mod row, in raw page bytes composed in the canvas and committed once, never `oled_write`. Also the pages: which one is showing, the dots, the kitty and QMK versions of the base band, and the Stats stop. The bands also draw on the other half while a map covers the master's panel (`u_bands_slave_task`, `u_bands_render`), from the state that crossed the link. 1x and 2x glyph blitters; the 2x one doubles each source row, so a 16x16 is two page bytes a column |
| `oled_map.c` / `.h` | the live layer map: which halves a layer's map fills (`u_map_side`, the one answer both the drawing and PAGE use), and the two styles. A key's matrix position comes from `LAYOUT_miryoku` given slot numbers in place of keycodes, so the map and `keymaps[]` agree by construction; its legend from `keymap_key_to_keycode()` and `map_legend.c`; a press from this half's own matrix |
| `map_legend.c` / `.h` | keycode to legend, on both hosts: up to three cells of Ein or drashna's 6x8 icons, or one of his 12x8 mod icons, dimmed or not. The Mac's legends are D15's. Unknown keycodes are flagged, and the scenario tests fail on any key a map draws that has none |
| `art/panel_glyphs.c` | generated. The five mod-row glyphs — four-pane, option mark, caret, command, shift — as drashna's filled 12x8 icons, centred at (2, 4) in 16x16 cells so the cell model did not change (until 2026-10-21 they were hand-drawn at 16x16, in DUNGEON.mode's weight language; `tools/panel/glyphs.py` still holds those). The lock glyphs are drashna's Caps Lock and Scroll Lock cells (Caps Word lights the Caps Lock one), and the file also holds the 2x16 "i" that stands in front of the Apple logo on iOS. Eight slots are five drawings: shift is one drawing in both sets, and the caret is one drawing that changes cell when the OS does, so the Ctrl/GUI swap is visible as a hop from the middle finger to the pinky |
| `art/panel_font.c` | generated. Printable ASCII in Ein, transposed out of drashna's font file, one byte a row, flush left in an 8 px cell; plus the QMK splash logo out of QMK's own `glcdfont.c`. Regenerating it needs the drashna clone as well as the QMK checkout (`VENDORED.md`) |
| `art/panel_layer_icons.c` | generated. The ten 128x40 layer bands, reduced from splitkb's Halcyon icons and the Kyria logo; see `VENDORED.md` |
| `oled_slave_bjorn.c` | the mascot: idle ladder, ambient clocks, blitter; his speed tiers come from `speed_tier.h` |
| `speed_tier.h` | WPM as four tiers with hysteresis and an 800 ms dwell, shared by Bjorn and the master's kitty so both read speed the same way. Each keeps its own state |
| `oled_settings.c` / `.h` | the one tagged word of user EEPROM: byte 0 the slave's animation, byte 1 the master's page (bits 0-3) and map style (bit 4). Read-modify-write, so neither key's press resets the other's byte. `u_settings_follow` is the other half's side: it saves what the master sent, in one write, only when something differs |
| `oled_slave_world.c` | the weather behind Bjorn, across the whole panel, season from `__DATE__` until a host daemon supplies it |
| `oled_slave_select.c` | which slave animation is live: `oled_settings.c` on the master, the `world` byte of the sync on the slave, which also saves what the master sent once a fresh state has arrived. Ahead of the animation, from a fresh state only: this half's map, or the master's bands when a map covers the master's panel (`u_map_moves_master`); Bjorn's clocks pause while either is up |
| `split_state.c` / `.h` | the 8-byte payload the master gathers: modifiers in finger order, the active and default layers, locks and Caps Word, the host and whether it is a Mac, a misfire count, the chosen animation, and the stored page and map style. Built fresh every main loop; a modifier it reports is one that is held at that instant |
| `split_sync.c` / `.h` | the transport for that payload. Double-buffered on the slave because the callback runs on a HIGHPRIO thread; heartbeat 250 ms, minimum interval 20 ms, soft stale 600 ms, hard stale 1250 ms. `u_sync_link_down()` answers for both halves — the master from its own failed sends, the slave from the sequence number standing still |
| `split_health.c` / `.h` | link health: the stats page behind a long SYS press, self-reset of a half whose link stays dead, the hardware watchdog. Every guard is explained in the header |
| `oled_slave_spaceship.c` | the ship, JBaguley's and byte-identical, over three parallax planes of my own; speed from WPM |
| `oled_slave_logo.c` | the three static logos |
| `art/bjorn_art.c` / `.h` | generated: base, patch table, frame index |
| `art/bjorn_poses.c` / `.h` | generated: the idle poses, six of infinition's frames |
| `art/world_art.c` / `.h` | generated: the weather sprites |
| `art/space_art.c` / `.h` | generated: the three planes, their occlusion masks and the ship's derived mask |
| `art/oled_*.h` | the vendored ship and logos: pure data, no selection logic |
| `art/drashna_icons.c`, `kitty_art.c`, `status_frame.c`, `drashna_sprites.c`, `painter_art.c`, `painter_fonts.c`, `bongo_art.c`, `mario_art.c`, `matrix_art.c` | generated by `tools/drashna/gen_art.py` from drashna's art, each with its `.h`: every cell of his font with a name, three kitties, the status sheet's frame, his other sprites, his Quantum Painter graphics and fonts, bongo cat, the Mario scene's sprites, the matrix rain. *drashna's art, all of it* above says what each is for. Generated at stage 0 of `docs/oled-v3.md`, and each joins `SRC` at the stage that draws it. `drashna_icons.c` is in since stage 1, for the OS logo |

Ambient timing counts render ticks rather than wall-clock milliseconds, so it freezes with
the panel and resumes where it stopped instead of jumping to a random phase on wake. The
tier and the idle ladder use the wall clock and are never reset, so both are already
correct the moment the screen comes back.

### All seventeen sprite sets, decided 2026-10-14

Every one of infinition's seventeen sets lands on the keyboard somehow. They do **not**
have to be attached to an action or a keypress: a set may earn its place purely as idle
content — a costume he wears for a while, a prop he picks up, a mood that passes — with
nothing on the keyboard causing it and nothing being signalled by it.

That is the design's own third category rather than an exception to it: an expression is
for something wrong, a prop is for something true, and idle is for everything else. The
mistake would be the opposite one, inventing a meaning for a costume so that it looks
like a signal. Decoration must not read as information.

**Bjorn is taken as he was drawn. Nothing is recombined.** The seventeen folders hold 89
frames that reduce to **32 unique images**; the folders are packaging, the images are the
work. Each of those 32 is one authored drawing — a costume, a prop, a horn posture and an
expression that appear together stay together — and each is traced whole into its own
64x64 grid. They are never decomposed into a parts matrix of bodies and overlays and
recombined, even though that would be far cheaper: composition would let a smile that
belongs to one drawing appear on a pose that never wore it, which is inventing Bjorns the
original artist did not make.

Drawing all 32 is also what makes "every set lands" true, without any per-set scheme:
every set's frames are among the 32, so every set is represented by being drawn. Nine of
the seventeen own no unique frame at all — one hood-and-laptop image is shared by
thirteen of them — and under this approach that simply does not matter.

**What each pose does, decided on the board 2026-10-15.** A drawing without a goat
**breathes**: the head lifts a pixel and the beard's outer contour pushes one pixel out
on each side, which is the shipped gesture and both halves of it — a lift alone was
tried and reads as the picture being moved rather than as somebody breathing. A drawing
*with* a goat does **not** breathe: holding the goat out of the gesture was not enough
to make those two look right. They **blink** instead, with the lids the shipped blink
uses, so they are not frozen — a figure where nothing moves is how this panel says the
link is dead, and these poses cannot show that by accident.

The blink is the one gesture here that is drawn rather than taken from the source, since
there is no eyes-shut version of these frames upstream. It is drawn to match the blink
that already ships: 7x2 lids at rows 34-35, the half lid 5x2 at rows 33-34, on the same
6 to 10 second schedule with about one in five doubled. It lands correctly on every pose
because the framing is constant across the corpus — the eyes sit in rows 31-36 in all of
them.

**The life comes from motion on an authored pose, not from assembly.** The model is the
breath: one pixel of travel on the whole figure, the easing carried by the dwell, body
leading and head following. Blink, breath, small lifts, and cuts between authored states.

**All 32 are drawn, and every element in them.** Not a reduced set, not the legible ones:
the sword, the shield, the spectacles, the bottle, the laptop, the hood, the horned mask
and the bandit mask all get drawn as they appear. Where an element collides with
something the panel already uses as a signal — a sword at this size sits where the axe
sits, and the axe means a command modifier is held — the answer is *when that pose may
appear*, never removing the prop.

They are reduced by box filter and Floyd-Steinberg, which is how the shipped poses were
made and is what gives Bjorn his mass. The figure lands at 22-24% lit, right on the
shipped 23%. An attempt to hand-draw them instead produced clean outlines with none of
his weight and was rejected on sight; the shading is the style, not a texture on top of
it. Frames that have to move are hand-finished on top of the dither, which is what
`STAGE1.txt` already is.

The hooded poses are the one case the reduction does not solve: the cowl is a solid dark
mass in the source, so it comes out near 33% lit and reads as a blob. Six images are
affected. Inverting the panel would restore it exactly, which is a second reason to look
at the inversion experiment.

Of the eleven unique icons, seven are typeset words (`FTP`, `SSH`, `SMB`, `SQL`, `RDP`,
`Telnet`, `TEST`) and are not usable here — they would be illegible shrunk, and the panel
now carries almost no text at all. Four are pictorial: a longship with shields along the
hull, a longhouse, a horned mask, and a bandit mask he can wear.

**Assessed and decided on 2026-10-15.** All four were reduced and measured. Only the
longhouse survives: it re-dithers to 992 ink, which is Bjorn's own register, because it
is simple line art. The longship goes noisy at 1123 and loses its rigging; the bandit
mask lands at 1580, nearly twice Bjorn, and would swamp him. These are 28x28 sources, so
anything above that is an upscale — it buys a smoother dither and no new detail, which
simple shapes survive and detailed ones do not. The three are dropped. The longhouse is
being drawn fresh at 64 px through the greyscale-first pipeline instead, so the world
column's one permanent object is authored for the size it is shown at.

What constrains this: about 23% ink, the block budget, and the motion rules. The old
version of this paragraph also said "every frame drawn by hand, because the mechanical
reduction is not usable" — that was the 76%-ink error, where the white page was counted
as ink. It is corrected in `tools/README.md` and the rule is now **dither tone, threshold
line art**: reduction is the default, and hand work is finishing on top of it.

### Built and still to do

Built, newest first: dark at rest, the alarm row, the trace strip and the latched locks
(`c4c93b9`); the master's zones and the DUNGEON.mode font (`df9231f`); the Caps Word crown and
tile inversion (`43ef4ae`); the misfire raise (`7b78012`, `eafad3b`); the axe (`ef99af4`,
`fe95b61`, `c158c8c`); Bjorn reading the master's state (`162fb40`); the split sync (`d195f6a`,
`df5c23b`); the per-half tasks (`591a32f`); Bjorn's postures, blink, breath, doze and waking
(`4465d74`, `cefa650`, `4cae216`, `85ac2bc`, `f4dad75`, `9d066bb`). The old art and the text
panel are still selectable in `custom_config.h`.

Still to do: the goat (wander from 20 s, pet at 45 s); Caps Lock on the mascot (crown plus the
1 px edge frame — he reacts only to Caps Word today); the defeated helmet-off pose when the
default layer is not base (the master already draws a rule under the layer name, the mascot
draws nothing); the "invert the whole OLED" look; raising the WPM tiers once typing speed
settles. Beyond the original spec, `docs/oled-v2.md` adds the idle director, weather by season,
the doze extras, and a place for every one of Bjorn's seventeen sprite sets. *The drashna round*
below adds master pages, the live layer map and more; none of it is built, and its build order is
`docs/oled-v3.md`.

**Known defects, found 2026-10-14.** Each line goes when the commit that fixes it lands.
Bug A (neither panel could sleep), D-1 (he stopped blinking after a doze), D-2 (the slave
rewrote its overlay blocks every frame) and D-3 (the sequence number counted builds rather
than sends) were fixed on 2026-10-14 and their rows are gone.

| # | Defect | Fixed in |
|---|--------|----------|
| D-4 | A misfire that happened during an outage replays when the link returns, because soft staleness blanks the fingers and flags but not the misfire count | Stage 2 |
| D-5 | The slave's idle clock lags by up to ~5 s: it takes the smaller of "time since WPM changed" and the input clock, and WPM keeps changing while it decays over its 5 s window | Stage 4 |
| D-6 | A dead link leaves the slave's panel lit with a frozen frame, because the slave applies the last OLED state it received. A slave whose code has stopped does the same, since the SSD1306 holds its picture by itself | Stage 2 |
| D-7 | QMK's driver writes one byte past its buffer for `index >= 1024`. Keep every index below 1024; the host stubs carry a canary | n/a, guarded |

**The link loss of 2026-10-13** (Windows, cable in the right half, away ~20 minutes): the master
showed the `LINK DOWN` banner, Bjorn did not blink, **the other half's keys did not type** while
the master's did, and it cleared only after replugging. Keys dying on the far half is the
signature of the *standard* transport being down, not of an RPC-only failure: the master zeroes
the other half's matrix when the transport counts as disconnected (`quantum/matrix_common.c:101`),
and `transaction_rpc_send()` then refuses immediately, which is the banner. QMK retries once per
500 ms while disconnected, so ~2,400 attempts went unanswered over those twenty minutes — the
other half was wedged or unpowered rather than suffering a line glitch, and replugging the USB
half power-cycles it over TRRS, which is why that fixed it. What wedged it is not established.
Bug A was first written up here as an aggravating factor, on the grounds that it kept that half
pushing an I²C block every main loop for the whole period instead of going dark at 120 s. **That
is weak, and Roel supplied the counter-evidence on 2026-10-14:** the old spaceship repainted every
frame for months, which both kept the panel from ever sleeping and kept the same I²C traffic
running, and nothing ever wedged. Continuous I²C on this half is therefore not sufficient on its
own. The one thing that genuinely changed with the zones panel is that the *master* began doing it
too, so both halves were saturating their buses rather than one — a difference, but not an
explanation. What wedged it is not established, and nothing here should be read as if it were.

A software self-reset cannot help a half whose code has stopped, so the hardware watchdog in
`docs/oled-v2.md` (Stage 2) is the fix that matches this evidence regardless of the cause — which is
the point of preferring it: it does not depend on guessing right.

### Attribution

`VENDORED.md` records where the parts of this repo that are not my own work came from, at which
commit, and under which licence. In short:
- Miryoku's userspace (Manna Harbour)
- Bjorn's sprites (infinition, MIT)
- the spaceship (JBaguley)
- the layer bands: splitkb's Halcyon icons, which are mostly Google's Material Symbols, and
  splitkb's Kyria logo
- from the drashna round: drashna's Ein face, icons, kitties, status-sheet frame, the Mario, Yoshi
  and Bullet Bill sprites and the matrix rain, plus HorrorTroll's bongo cat
- the slave's three logos, the Kyria logo and the two Monitor logos (`art/oled_*.h`), which
  `VENDORED.md` does not cover yet

`bjorn_art.c` and the drashna round's files carry their sources' copyright notices.

The panel's font is **QMK's own 6x8**, `drivers/oled/glcdfont.c`, read out of the QMK checkout
at generation time, until stage 1 of `docs/oled-v3.md` swaps in Ein. It replaced **DUNGEON.mode**
by datagoblin (CC0) on 2026-10-15.

Taking drashna's art in moved the repo from GPL-2.0 to **GPL-3.0-or-later** on 2026-10-21
(`docs/oled-v3.md`, D6). Everything already here was "or later" or permissive, so the move was
allowed, and the other authors' notices stay as they are.

### Switching the slave's animation

All five slave animations — Bjorn, the spaceship, the Kyria logo and the two Monitor logos —
are compiled in and chosen at runtime. `OLED_SLAVE_ANIMATION_*` in `custom_config.h` now names
only the **boot default**, the one you get on a fresh EEPROM, rather than the only one that
exists. It costs about 11 KB of flash, which on 16 MB is not a consideration.

**`U_ANIM`, right outer column, top row** — mirroring `U_SYS` on the left, so the two system
keys sit in the same place on their own halves. It acts on release, like SYS: a key that acts
on press fires while you are still deciding.

Two things made this less trivial than it looks, and both are worth knowing before touching it.

**The half that is told is not the half that draws.** Keypresses are processed on the master,
so the choice has to cross the link. It rides in a new `world` byte on the existing per-loop
state sync. Widening `u_split_state_t` from 5 bytes to 6 is safe where adding a transaction
would not be: `NUM_TOTAL_TRANSACTIONS` is the last member of an enum in QMK's
`transaction_id_define.h`, so it counts transaction *ids* and not bytes, and it is the thing
XORed into every handshake — change it and a mismatched pair stops typing entirely. The upper
bits of `world` are deliberately spare, because the weather and time-of-day a host daemon
would send belong in the same byte.

**It is persisted on both halves, and the master's copy wins.** A setting you re-make every
morning is not a setting, and neither is one that changes when the cable moves to the other half.
So both halves keep it (decided 2026-10-22, with the master's page, after the replug check showed
each half bringing back its own):
- The master saves what you pick, as before.
- The other half saves what the master sends: the animation from `world`, and the page and map
  style from a `view` byte that carries the master's stored byte as it is. Whichever half gets
  the cable next shows the same animation and page.
- It writes only when a fresh state from the master differs from what it holds. Nothing is saved
  before the first state since boot arrives, and nothing from a stale one, because a zeroed or
  frozen state would read as the first animation and the first page.
- So it writes on the same presses the master does, and once when two halves that disagree are
  joined. Then the half with the cable wins.
- A change made while the halves are apart stays with the half that had the cable, until they
  are joined again.

The stored word carries a tag, because `eeconfig_read_user()` returns
zero on a fresh or cleared EEPROM and zero is a legitimate animation index — an untagged word
means "never set" rather than "the first one".

Bjorn's per-loop task runs only while Bjorn is the one drawn. Letting his idle clocks advance
unseen would mean he came back mid-doze; this way he always returns awake, which is the honest
reading — you just arrived.

### The spaceship's space

JBaguley's ship, mask and mechanism — `wpm/4` slides the ship right, `wpm/15` sets the scroll
speed, 50 ms a tick — are unchanged since April. The space behind the ship was redrawn on
2026-10-16, because the original was composed for a **32-row Corne panel**: only pages 2-5 were
ever his, and padding that to 64 rows left 32 rows of stray stars with the Earth and moon clipped
at the old band's edge. A 64-row space has to be composed as 64 rows from the start.

**Three parallax planes off one counter.** Far stars at `state >> 2`, a mid plane at `state >> 1`,
a near plane at `state`, with periods 128, 256 and 512. Each nearer plane carries an occlusion
mask — its silhouette, holes filled, dilated 1 px — so far stars do not shine through a planet's
dark continents and do not fuse with its rim. Masks are stored complemented, the ship mask's own
convention, so all four reads are the same AND-then-OR. 18 432 bytes of planes.

**The loop doubled, from 256 to 512, and the reason is arithmetic.** A plane at speed `1/n` needs
a period dividing `loop/n`. Under 256 the far plane would be a 64-px tile shown twice across the
128-px screen, and anything on the mid plane would have period 128 — exactly the screen width —
so Saturn could never leave: its copy enters right as it exits left. Under 512 the periods are
128/256/512, every object has time off screen, and at the wrap all three offsets are 128, 256 and
512, each ≡ 0 modulo its period, so the seam is invisible. The cost is a longer loop: **25.6 s at
rest instead of 12.8, 6.4 s at wpm 50 instead of 3.2.** `state` was already a `uint16_t`.

**Parallax does not make it cheaper**, and should not be sold as if it did. A full-width scroll
dirties both blocks of every row it touches, so this animation is 16 blocks a tick by nature and
always was. Measured over a loop: mean 10.9 at rest (sparse planes leave some blocks untouched on a
given tick), 15.5 at wpm 50, ceiling 16. Budget for 16.

**Lit-on-black is the natural polarity for space** and the bloom risk is an isolated lit pixel.
The objects were drawn to a 2x2 star floor, derived from JBaguley's own smallest star on this panel
(7 lit px) and a bloom simulation; the planes have zero isolated pixels. Lit area is 15.9% against
the old 9.2%, because the planets are solid discs like the original Earth — that is the chosen
style, and the brighter panel is the price.

**The ship's outline is one pixel, and it cannot be zero.** JBaguley's mask left up to 4 px of
dark margin. Tried at zero: lit-on-black, a lit ship over lit Earth has no edge and simply
vanishes, leaving its window. One pixel keeps the edge and halves the halo (213 → 102 px). The
mask is now derived from the ship's own silhouette — holes filled so the window stays opaque,
dilated once, complemented — by the generator, so the ship stays byte-identical and the mask
cannot drift from it. `test_space.c` asserts the ring around every ship pixel.

**Authoring** lives in `tools/space/`: the objects and the placement script they were composed from
under `authoring/` (the script needs Pillow and is provenance, not build), the five planes as grids
in `planes.txt`, and the stdlib generator that appends each strip's 128-column tail from its head —
appended, never authored, because an authored tail can drift from its head and then the seam shows
once a loop. `tests/unit/test_space.c` asserts closure on the compiled bytes at rest and at speed,
and that the ship is bit-exact inside its own mask.

### The drashna round, decided 2026-10-21

drashna's userspace (`drashna/qmk_userspace` at `0903be5`, with the bongocats module from
`drashna/qmk_modules` at `ee2c35b`) was read through for OLED ideas. Each one was rendered on this
keyboard's own panels before it was decided. The master frames came from `oled_master_zones.c` and
Bjorn from `oled_slave_bjorn.c`, both compiled on the host, with everything new drawn over them.
This section is what was picked. `docs/oled-v3.md` is the build order, says which stages are
built, and holds the questions still open.

**The master gets pages, and a key to turn them.** `U_PAGE` (left outer column, middle row, under
SYS) cycles what the master shows, the way `U_ANIM` cycles the slave. It acts on release, is
persisted in the same tagged EEPROM word, and only the master draws pages, so nothing new crosses
the link for them. For 700 ms after a press, a row of dots in the middle of the status bar says
which page you landed on. In order:

| Page | What it shows |
|------|---------------|
| Bands | today's three bands, with the changes below |
| Bands, small kitty | the same, with drashna's small kitty in place of the Kyria emblem on the base layer, the wordmark kept. It reacts to WPM on Bjorn's own tiers (up at 10, 30 and 50, down at 6, 24 and 42, 800 ms dwell; decided 2026-10-22), so both characters read speed the same way: sleeps, paws, watches, bounces. Her paw frames are her sleep frames, so in the first tier she looks asleep |
| Bands, striped kitty | the same again with drashna's bigger striped kitty, in the same place, on the same tiers and clock. She has paw frames of her own, so all four tiers look different (added 2026-10-22, from the kitty idea's variant A: both kitties, since the bands are stops anyway) |
| Bands, QMK | the same, with the QMK splash logo across the base layer's band |
| Status sheet | drashna's left screen rebuilt with this keyboard's data: header and border art, the striped kitty top left, WPM, the layer name (inverted off base), the OS logo with auto or manual, CW / CAPS / SCRL inverted when on, and eight modifier icons for both hands |
| Layer map | the whole current layer, both halves at 1×, held keys inverted, the held layer key inverted in the thumb row |
| Typing | WPM at 2×, the session peak, a history graph of one column per 300 ms, bongo cat along the bottom |
| Stats | the SYS page as it is; holding SYS still opens it from any page |
| Event log | the last eight events with uptime stamps (there is no clock): boot, link up and down, a self-heal reset, the OS set by detection or by SYS, a misfire |

The four versions of the bands are four stops rather than a setting. That settles the two kitties
and the QMK logo all wanting the base band: pick by pressing. Every other layer looks the same in
all four. Nine pages need four bits, so byte 1 of the EEPROM word holds the page in bits 0-3 and
the map style in bit 4.

**The bands page loses its last text.** The `Win`/`Mac` badge becomes drashna's 12x16 Windows or
Apple logo at the top left. It borrows the band's empty top-left corner: x 0-15, y 8-17 is dark on
all ten layers, and in those rows the nearest band pixel is the Kyria emblem at x=18 (measured on
the art). A hand-set OS keeps its underline, at y=16 under the logo. The mod row's glyphs
become drashna's filled 12x8 icons (⊞ ⌥ ⌃ ⌘ ⇧), centred in the existing 16x16 slots, so the cell
model does not change: holds, the Mac order, the mirroring and the misfire chip work exactly as
now. Rendered against the real panel, the cell model matched the firmware byte for byte.

**The font becomes Ein**, one of the faces in drashna's font file. drashna credits datacute's
TinyOLED-Fonts for the extra faces, but that set has no Ein, so its origin is untraced (see
`VENDORED.md`). It draws whatever text the panels still carry: the stats page, the new pages and
the layer maps. Its lowercase is small capitals.

**Three of Ein's glyphs come from elsewhere** (decided 2026-10-22, after the first board look at
the Sym map). The map draws a single character at 2×, which is where these three failed:
- Ein's `@` is missing its left side, so it read as a 2 with a hook. It is **QMK's `@`** now, which
  is Ein's with that column back, so it fits as it is.
- Its `|` is a broken bar, two short strokes, and its `*` is a dense blob that reads as the
  brightness sun. QMK's font has both the same way, so these two are **ZXPIX's**, another face in
  drashna's file.

ZXPIX's glyphs are six columns by six rows where Ein's are five by seven, so the two are placed in
Ein's slot without being redrawn: the `|` moves to the middle column, so it centres like every
other glyph, and the `*` drops its sixth column, one stray pixel that made it lopsided.

**The live layer map.** It shows only while a thumb layer key is held, and fills a whole panel on
the half under the fingers that use it:

- Space, Tab and Esc (Nav, Mouse, Media) fill the **right** panel; Enter, Bksp and Del (Sym, Num,
  Fun) fill the **left**. Miryoku keeps the holding hand for mods and layer locks, so the half with
  the layer key has nothing worth drawing.
- Button, held on the pinky's Z or /, has keys on both hands, so both panels show their own half.
- The physical half decides, not master or slave. The keymap is compiled into both halves and the
  slave already receives the active layer, so either half can work out its own legends with
  `keymap_key_to_keycode()`, and its own pressed keys from its own matrix.
- Keys are drawn where they sit: on the left half pinky to inner, left to right; on the right half
  inner to pinky. The thumbs sit under the inner columns (Ent, Bsp, Del from the inside out on the
  right). There are no outlines. A pressed key lights as a small inverted box, and the layer's name
  sits inverted in the two free thumb slots. Button's modifiers use the mod row's 12x8 icons.
- Two styles, both kept, switched by **PAGE while a map is showing**. A page change would never be
  wanted at that moment, so the key is free for it. The **big grid** has five columns of 25.6 px,
  rows of 16 and single symbols at 2×. The **Kyria-shaped** map uses 1× legends on the board's own
  column stagger (0.75, 0.25, 0, 0.25 and 0.5 units, pinky to inner) and thumb arc, both from
  `keyboard.json`. The style is persisted with the page and crosses the link with it, in bit 4 of
  the `view` byte, which leaves bits 3-7 of `world` for the weather and the time of day.
- **On the Mac, a key that is something else there says so** (D15, decided 2026-10-22 from renders
  of the Fun and Nav maps on both hosts). Print Screen draws `F13`. Scroll Lock and Pause draw
  drashna's sun with ▼ and ▲, because the Mac reads them as F14 and F15, its screen brightness
  down and up. App is dimmed to every other pixel, because it does nothing there. Insert draws
  `Hlp`, because the Mac reads it as Help. AltGr draws drashna's ⌥, because it is right Option.
  Everywhere else the legends are the same on both hosts, and the mods follow the host the way the
  mod row does.
- **The master's screen moves over.** While a map covers the master's panel and not the slave's,
  the master's own bands page takes Bjorn's place on the other half: the OS logo, the layer icon
  and the mod row, mirrored for that hand by the code that already mirrors it. So the layer icon
  never disappears. With the cable on the left that happens for Sym, Num and Fun; with it on the
  right, for Nav, Mouse and Media. Button covers both panels, so for that one layer there is
  nowhere for it to go. The eight bytes the slave already receives carry everything that page draws.
- **Never over the alarm** (decided 2026-10-22, while building stage 4). The master shows its map
  only while the link is up. With the link down it shows the bands, so holding Button can't hide
  the alarm on both panels at once. The other half shows a map, or the master's screen moved over,
  only from a fresh state, and its animation otherwise. So a layer that was held when the cable
  came out doesn't stay frozen on screen.

**Two more slave animations** behind `U_ANIM`, after today's five. **Bongo cat** is drashna's
bongocats module (HorrorTroll's art): 128x32 along the bottom, WPM at 2× top right, and it slaps
faster as WPM climbs, with a frame time of max(100, 450 − 2.5 × WPM) ms. **Mario, Yoshi and
Bullet Bill** is a small scene rather than a loop, described below. That makes seven, which still
fits the three bits of `world` the animation index has.

**Mario, Yoshi and Bullet Bill, driven by WPM.** All of it is drashna's art from `oled_assets.h`:
- Mario: four 16x32 frames, two standing, a step and a jump.
- Yoshi: five sets of 32x32, with drashna's own frame orders from the comments there. *Idle*
  (0 0 1 2 1 2 1 2 1 0), *ready* (5 6 7 8 9 8 7 6), *fly*, a flutter (10 11 12 11), *hide*, curling
  up (15 16 17), and *run* (20 21).
- Bill: one 32x24 frame, nose left, so he flies right to left. A flying object needs no frames; his
  motion is position.

The layout:
- Both face right with their feet on y=62: Mario at x=14, Yoshi in front at x=40.
- Under them is a dotted floor rule at y=63, like Bjorn's, with a few grass tufts on y 61-62. Floor
  and tufts scroll left at `wpm/15` px a tick, the spaceship's own rate.
- **WPM at 2× in the top-right corner**, as bongo cat has it, with `wpm` small underneath (x 90-125,
  y 1-25). Nobody ever reaches that corner, so nothing has to avoid it.

The steps are Bjorn's WPM tiers (up at 10 / 30 / 50, down at 6 / 24 / 42), so the board reads speed
one way:

| WPM | Mario | Yoshi | Floor |
|-----|-------|-------|-------|
| 0 | standing | *idle* | still |
| typing, below 10 | stepping (frames 1 and 2) | *ready* | creeping |
| 10 and up | walking, max(100, 450 − 2.5 × WPM) ms a frame, bongo cat's formula | *run*, max(80, 300 − 3 × WPM) ms a frame | scrolling, faster with WPM |
| stopped for a while | standing | *hide*, then stays curled | still |

**Bill** comes in from the right along the ground (y 41-61), only while typing at 10 WPM or more.
After each pass, the next one waits about 5 s, or about 2.8 s from 30 WPM. He flies at 4 px a tick
plus the floor's speed. Yoshi flutters over him on *fly* and Mario jumps him on the jump frame, each
on a 31 px arc. **The jump is timed to Bill, not fixed.** For each character, the jump lasts at
least 16 ticks and long enough that the whole time Bill overlaps it horizontally falls in the middle
half of the arc, where the lift clears Bill's top. So it works at every speed. Checked on the host
renders: Bill touched neither of them in any frame. Yoshi's flutter apex reaches y=0 exactly.

When Yoshi curls up is open (D13 in `docs/oled-v3.md`). The demo used 4 s, and on the board it
would follow Bjorn's doze. This scene moves while you type, which Bjorn's never does. That is the
point of it, and the spaceship already works the same way; ANIM is how you get the quiet one back.

**A screen for the bootloader.** `reset_keyboard()` calls `shutdown_quantum()` (`quantum.c:208`),
which calls `shutdown_user()`. There the panel paints the QMK splash logo with an inverted
BOOTLOADER label under it, so a successful Boot double-tap is confirmed instead of the last frame
freezing. Only the half that resets shows it.

**Encoder feedback.** Turning the left encoder flashes VOL+ or VOL− in the band for under a second,
with drashna's speaker icon at 2×. The right encoder shows BRI+ or BRI− with the bulb. A mute key,
if the left encoder's click ever gets one, shows MUTE. Only the direction is known; the host's
volume is not.

**Glitch bursts, for the alarms only.** While the link is down, the master breaks up in short
random bursts (row shifts, noise, a sliced inversion) over its alarm frame. Bjorn does the same
while his link is dead, on top of the blink that already stops. They are procedural, with no
stored art. Motion stays reserved for things that are wrong.

**A screensaver step.** drashna's matrix rain (72 frames, 72 KB) runs between idle and the panels
switching off. At the split's 50 ms update interval it loops every 3.6 s rather than drashna's
2.2 s. When it starts, and on which half, is open.

**Everything stays under the alarm.** Measured on the host renders: the status sheet is the
brightest routine page at 2,519 lit px, the encoder flash 2,663, the rain 1,095 at most, bongo cat
1,288 and the big-grid map 1,671. That is against roughly 4,200 for the master's inverted band.
When each page is built, `test_panel.c`'s ratio test should cover it.

**Declined, so they are not proposed again:**
- Game of Life seeded by typing
- a WPM graph anywhere but the Typing page
- the autocorrect notice
- a keylogger strip in the status bar
- the event log as a second SYS page (it is a PAGE stop instead)
- the glitch on an OS switch
- the kitty on every layer, and the outline kitty
- OS logos or 2× icons in the mod row
- the 12x8 OS glyph as the badge
- drashna's own two screens as they are
- the train, the small running cat, the full-screen logos and the small sprites
- the 128x128 arrangement as it was mocked

### drashna's art, all of it, decided 2026-10-21

The first round rendered what drashna's screens use. A second pass rendered everything else in his
userspace that a 128x64 panel can draw: every non-text cell of his font, the sprites his screens
never show, the kitties' last two moods, his Quantum Painter graphics and fonts, and the
display_menu module. All of it is taken in, generated into `art/` under the names below,
**whether or not anything draws it yet**, so a later idea starts from named art rather than from
another search. Two rules came with it:
- Where this panel has a hand-drawn piece and drashna has one for the same thing, his is used:
  the mod glyphs and the badge at stage 1, and now the Caps Lock and Scroll Lock glyphs. Caps
  Word, which drashna shows as Caps Lock, went the same way.
- The first round's declined pieces stay out: the train, the small running cat, the full-screen
  logos, the Witcher wolf, the big digits and the plain small QMK chip.

"—" below means the art is there and nothing draws it yet. Nothing that draws with it is decided
just by being in this table. The stages that use art are in `docs/oled-v3.md`.

**Font cells**, `art/drashna_icons.c`, out of `drashna_font.h`. The cells outside the nine faces
are the same in every face. Page format, one byte a column.

| Name | Cells | Size | Where it goes |
|------|-------|------|---------------|
| `u_os_logo[WINDOWS]` | 97 98 / b7 b8 | 12x16 | status bar, on Windows (stage 1) |
| `u_os_logo[APPLE]` | 95 96 / b5 b6 | 12x16 | status bar, on the Mac (stage 1); on iOS, 4 px to the right with the hand-drawn "i" in front |
| `u_os_logo[LINUX]` | 99 9a / b9 ba | 12x16 | status bar, on Linux |
| `u_os_logo[ANDROID]` | 9b 9c / bb bc | 12x16 | — : QMK reports Android as Linux, so nothing can select it |
| `u_icon12[CTRL, ALT, SHIFT, WIN, CMD]` | e4-ed | 12x8 | mod row (stage 1, through `panel_glyphs.c`); Button's layer map (stage 4); the status sheet (stage 5) |
| `u_icon12[MUTE, SPEAKER]` | e0-e3 | 12x8 | encoder feedback (stage 7) |
| `u_icon12[BULB]` | f0 f1 | 12x8 | encoder feedback (stage 7) |
| `u_icon12[BULB_OFF, MOUSE, CLICK_OFF, CLICK, CAT]` | ee ef, f2-f7, f9 fa | 12x8 | — |
| `u_icon6[CAPS_LOCK, SCROLL_LOCK]` | c9, cb | 6x8 | status bar locks, in place of the hand-drawn ones (2026-10-21). Caps Word lights the Caps Lock cell too, as on drashna's screen |
| `u_icon6[NUM_LOCK]` | ca | 6x8 | — : Num Lock is not shown (see *Master half*) |
| `u_icon6[RADIO_FILLED, ARROW_UP, ARROW_DOWN, TRI_RIGHT]` | c2, 18, 19, 10 | 6x8 | what display_menu draws, if it comes |
| the other `u_icon6` cells, listed below | | 6x8 | — |

The other single cells:
- 0x00-0x1f: `QMK_PSI` (00, a tiny Ψ), `FACE_FROWN`, `FACE_SMILE`, `HEART`, `DIAMOND`, `CLUB`, `SPADE`,
  `BULLET`, `BULLET_INV`, `CIRCLE`, `CIRCLE_INV`, `MALE`, `FEMALE`, `NOTE`, `NOTES`, `SUN`,
  `TRI_RIGHT`, `TRI_LEFT`, `ARROW_UP_DOWN`, `DOUBLE_BANG`, `PILCROW`, `SECTION`, `BAR`,
  `ARROW_UP_DOWN_BASE`, `ARROW_UP`, `ARROW_DOWN`, `ARROW_RIGHT`, `ARROW_LEFT`, `RIGHT_ANGLE`,
  `ARROW_LEFT_RIGHT`, `TRI_UP`, `TRI_DOWN`: the old code-page-437 symbols, in that order.
- `MOUSE_IDLE`, `MOUSE_LEFT`, `MOUSE_RIGHT` (80-82); `APPLE` and `WINDOWS` at 6x8 (9e, 9f); `BLUETOOTH`,
  `PADLOCK_OPEN`, `PADLOCK_CLOSED` (bd-bf); `RADIO_EMPTY`, `CHECKBOX` (c1, c3); `MENU` (c8, ≡);
  `KEYBOARD` (cc).
- `KEY_MUTE`, `KEY_VOL_UP`, `KEY_VOL_DOWN`, `KEY_NEXT`, `KEY_PREV`, `KEY_STOP`, `KEY_PLAY` (cd-d3, each
  in a keycap frame).
- `F1` … `F11` (d5-df) and `F12` (fc); `DEGREE` (f8), `BOMB` (fb), `IRONY` (fd, ⸮), `INTERROBANG` (fe, ‽).
- Named by shape, because drashna's code never names them: `OVERBAR_DIAMOND` (c4),
  `STEM_DIAMOND` (c5), `TO_START` (c6, `|<`), `TO_END` (c7, `>|`), `BOXED_PLAY` (d4).

Candidates only, nothing decided: the arrows, the triangles, the F-key cells and the media
keycaps fit the legend table of the live layer map (stage 4).

**Kitties**, `art/kitty_art.c`, out of `oled_assets.h`'s `pet_animiations`, credited there to j-inc
and pixelbenny. All three kitties with all six moods, 32x32. The moods are `SLEEP`, `PAW`
(drashna's *kaki*), `WATCH` (*mati*), `BOUNCE` (*awake*), `RTOGI` and `RUN`; drashna's code never
says what *rtogi* means, so it keeps his name.

| Name | Where it goes |
|------|---------------|
| `u_kitty_striped` (tora) | the status sheet (stage 5) |
| `u_kitty_small` (luna) | the bands' kitty page (stage 3). Her paw frames are her sleep frames |
| `u_kitty_outline` (neko) | — : declined as the master's kitty in the first round, so it is kept as art only |
| `RTOGI` and `RUN`, all three | — |

**The status sheet's frame**, `art/status_frame.c`, out of `oled_assets.h`.
- `u_status_header`, `u_status_footer` (the filled `footer_image2`) and `u_status_border`: the
  status sheet (stage 5).
- `u_status_footer_outline` (`footer_image`) and `u_status_row2` (`row_2_image`): —.

**Sprites**, `art/drashna_sprites.c`, out of `oled_assets.h`, all —:
- `u_mouse16[MOUSE, CROSSHAIR, DRAG_HAND]`, 16x16
- `u_triangle`, 32x32
- `u_qmk_chip`, 32x24 (`qmk_logo_small_g`)
- `u_qmk_wordmark`, 128x24 (`qmk_logo`)
- `u_arasaka_small`, 32x32
- `u_wow_tall`, 32x128, which only a 128-row panel can show

**Quantum Painter graphics**, `art/painter_art.c`, out of `painter/graphics.qgf.c`. They were drawn for
drashna's colour TFTs and are cut to 1 bit at half brightness. All —:
- `APPLE16`, `WINDOWS16`, `LINUX16`, 16x16
- `GAMEPAD24`, `MOUSE24`, 24x24
- `MOUSE_ICON`, 11x14
- `ALT9`, `CMD9`, `CTRL9`, `SHIFT9`, `WIN9`, 9x9

**Quantum Painter fonts**, `art/painter_fonts.c`, out of `painter/fonts`: `font_oled`, Proggy
Clean, Proggy Tiny and Thintel 15. They are proportional, cut to 1 bit, and cover ASCII plus the
few extra glyphs each one carries. All —.

**Already placed**, from stage 0: `art/bongo_art.c` (a slave animation, stage 6), `art/mario_art.c`
(the Mario scene, stage 6) and `art/matrix_art.c` (the screensaver, stage 9).

**Not taken:**
- the two 240x320 colour pictures in `painter/graphics/`, which are TFT art
- display_menu, which is code rather than art, and waits under *Open experiments*

## Open experiments

| Idea | Why | Status |
|------|-----|--------|
| Encoder push-clicks | Position 1 clicks are unassigned. Candidates: mute (left), play/pause (right) | later |
| OS layout: plain US instead of US-International | US-Intl's dead keys (`' " ` ~ ^`) clash with vim (`"a` gives ä, `'a` gives á) and with the symbol layer | to decide |
| å ä ö | Swedish later. Options: EurKEY (US base, letters on AltGr), or switching the OS layout when writing Swedish | later |
| Vim on Colemak-DH | `hjkl` are scattered on Colemak-DH. Miryoku's Nav layer has arrows on the right home row | later |
| OS-aware navigation | Word jump is Ctrl+Arrow on Windows but Option+Arrow on macOS; line start/end is Home/End vs Cmd+Arrow. OS detection could drive these too, but what works depends on the app (a Mac text field wants Cmd+Arrow, a terminal and nvim want Home), so a blanket translation would break the terminal. **Leaning nvim-style instead (2026-10-22):** learn vim's motions (`0`, `$`, `w`, `b`, `gg`, `G`), which behave the same on every host, and leave Home, End and the word jumps untranslated | later, nvim-style |
| Brightness on Scroll Lock and Pause, on the Mac | The Mac reads them as F14 and F15, its own brightness down and up. Not Miryoku's doing, and it is wanted: keep it. Only the live layer map has to show it (`docs/oled-v3.md`, D15) | keep, decided 2026-10-22 |
| Print Screen on the Mac | The Mac reads it as F13, and nothing on Roel's Mac is bound to F13 (checked in its shortcut table), so it does nothing there. On the Mac it could send Cmd+Shift+5, the screenshot toolbar, which is the closest match to Windows 11's Print Screen opening the Snipping Tool. Only the Mac setting would change | later |
| Flow Tap after any key | If mods misfire right after Backspace, Enter or digits, widen `is_flow_tap_key()` to match the old typing streak | only if it happens |
| `PERMISSIVE_HOLD` | QMK's recommended pairing with Chordal Hold. Makes quick deliberate shortcuts register before 200 ms | after the migration settles |
| OLED redesign in pixel art | Specified in full under OLED above, including the Bjorn mascot, the goat, the master panel zones and the split-sync design | partly built, see Built and still to do |
| drashna's display_menu | His on-screen settings menu, as a community module (`drashna/qmk_modules`, `display_menu`), with an OLED renderer. Mocked on this panel on 2026-10-21: an inverted title line, the radio cell on the selected entry, ► after a submenu. It is code, so it goes with the question of community modules rather than with the art | later |
| Invert the whole OLED | **Tried in `a6bd2f9` on 2026-10-14, reverted 2026-10-15.** The case for it and what flips with the polarity are under the OLED section above, in the polarity note | done, and reverted |

## Tuning log

| Date | Change | Why | Verdict after a week |
|------|--------|-----|----------------------|
| 2026-10-22 | Flashed `70f3bc0` to both halves. The other half now saves the page and the animation the master sends, so they follow the cable; the payload is eight bytes | The replug check showed each half bringing back its own page | Passed on the board the same day: the cable moved both ways and the choice came along |
| 2026-10-22 | Flashed `657fb8e` to both halves: stage 3 of `docs/oled-v3.md`. PAGE under SYS turns five master pages, the bands, the small kitty, the striped kitty, the QMK logo and Stats, with dots after each press | D1's pages start here, and the kitties read speed the way Bjorn does | Checkpoint passed the same day |
| 2026-10-22 | Flashed `fd28d30` to both halves: stage 2 of `docs/oled-v3.md`. A Boot double-tap now shows the QMK logo over an inverted BOOTLOADER on the half with the cable | A Boot that froze the last frame looked the same as one that did nothing | Checkpoint passed on both halves the same night. Boot is fussy: Space has to be held until Nav shows, and the two taps of Q must land within 0.2 s |
| 2026-10-22 | Flashed `0130b8b` to both halves. Caps Word lights drashna's Caps Lock glyph, as on his screen, and the status bar has two lock slots instead of three | drashna has no Caps Word glyph, and where he has a piece, his is used | Open |
| 2026-10-21 | Flashed `f639c97` to both halves: stage 1 of `docs/oled-v3.md`. The panel font is Ein, the mod row and the Caps Lock and Scroll Lock glyphs are drashna's, and the Win/Mac text became a 12x16 host logo: Windows, Apple, Tux, and the Apple logo with an "i" for iOS. The link payload is 7 bytes | The drashna round's first stage, and the first flash since `27a1e24`. Both halves were read back first and both were `27a1e24` | Checkpoint passed the same day: from a normal seated position the five mod icons and the logo can be named without leaning in. The week's verdict and the other `v3-1` rows in `docs/flash-checklist.md` are still open |
| 2026-10-14 | Flashed `91477af` to both halves. The misfire no longer replays after an outage; the master counts failed sends, outages, transport drops and host sleeps, and main loops per second; the other half returns its uptime, stale episodes and build id on the transaction the master already makes; SYS held two seconds shows all of it, which moves the Win/Mac toggle to the release of a short press; the other half restarts itself after five seconds of silence, with backoff and a cap of three, and a two second hardware watchdog catches the case where its code has stopped | The link died for twenty minutes on 2026-10-13 and the keyboard recorded nothing, so the cause is still unknown. Both halves were verified as running `c95356d` before this flash. None of the recovery depends on knowing the cause: the software reset covers a half that is running but silent, the watchdog one that has stopped | Open. Watch for `OUT`, `RST`, or the other half's `UP` falling far below the master's. The counters are RAM only, so look before replugging. **Main loop baseline measured on the board the same day: 924 per second, worst second 875** — a scan every 1.08 ms, against a 1 ms USB polling interval and a 5 ms debounce window, so the loop is already matched to the hardware downstream of it and oversamples debounce about five times over. The 5% spread between typical and worst is the part that says there are no stalls. Future stages are compared against this |
| 2026-10-14 | Flashed `c95356d` to both halves. Frames are composed in RAM and committed once; the master switches both panels off 120 s after the last key whatever is drawn; Bjorn reschedules his blink after a doze, the waking lids and a dead link; the link's sequence counts delivered states; LTO off | Neither panel had ever slept, because every zone cleared and redrew itself straight into the driver's buffer and any changed byte re-arms the display timeout. The same flaw made Bjorn stop blinking after every doze, which is this design's signal that the link is dead — so he reported a fault that was not there, every time Roel came back | Checked on the board the same day: both panels go dark at about 120 s, a key wakes both, he blinks within about ten seconds and keeps blinking after a doze, the master is unchanged and typing feels the same. Week's verdict still open |
