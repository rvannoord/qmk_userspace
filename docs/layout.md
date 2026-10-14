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
- Hosts: Windows at work, Apple Silicon Mac at home. OS detection swaps Ctrl and Cmd on the Mac, so shortcuts use the same fingers on both. Side effect: on the Mac, Ctrl moves to the pinky, which you feel in the terminal and nvim. The alternative (mods stay put, only the clipboard keys adapt) stays open as a later switch.

## Extra keys

| Key | Where | Does |
|-----|-------|------|
| SYS (`U_SYS`) | left half, outer pinky column, top row | toggles Win/Mac by hand when detection guesses wrong; the left OLED shows the result. Held while plugging in, it enters the bootloader (Bootmagic) |

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
- **Lit on black**, not inverted. Inverted is more faithful to the e-paper original and measures
  77% lit against 24% — a lamp beside the keyboard — and the FS dots render worse inverted, since
  dark single pixels get eaten by bloom from the lit field. Inversion is reserved for alarms.
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
| WPM tier (0-19 / 20-39 / 40-59 / 60+, 8 WPM hysteresis) | horn posture, four static poses from IDLE8 / IDLE1 / IDLE3 / IDLE9. No speed lines: motion in peripheral vision taxes concentration, and WPM is the one signal with nothing to act on |
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

Dark at rest, so any light means something happened. Zones, all page-aligned:

| Zone | Origin | Size | Content |
|------|--------|------|---------|
| Status bar | (0,0) | 128x8 | `WIN`/`MAC` badge, underlined when SYS set it by hand. The right end is permanently dark, so the latched locks live there at x=100, 109, 118 — one row, so they cannot drift off a shared baseline |
| Mod row | (0,8) | 128x16 | four 32 px cells, one per finger, outer to inner. **Dark at rest**: a cell draws its 16x16 glyph only while that finger holds something, and nothing here ever inverts. Handedness is a 3x16 bar filling the padding on that hand's side |
| Layer band | (0,24) | 128x16 | layer name in DUNGEON.mode at 2x, so 16x16 a character, centred. Blank on base. Two rows under the name whenever the default layer — the high nibble of `layers` — is not base |
| Alarm row | (0,40) | 128x8 | rung 2 of the ladder, and the only thing on this panel that inverts. Dark otherwise, which is also what keeps the row below from reading as a fifth line of the layer band. The split link being down is its only occupant today |
| Trace strip | (0,48) | 128x16 | dark at rest. On a misfire, the offending finger's letter and hand at 2x for 1.5 s, then dark. The panel's only past tense |

**The font is DUNGEON.mode**, an 8x8 1-bit bitmap font, not QMK's built-in 6x8. It is the same
pack the rest of the panel's art is cut from, so the text and the glyphs share one hand. The sheet
is a 16x16 grid of 8x8 cells with its own character map; the build extracts only the characters
the panel actually draws. 8x8 gives 16 columns instead of 21, which is why the layer name is the
only text long enough to care and gets the whole width.

**There is no layer map.** The first design had one at (30,40), 60x24, one 6x8 legend a key,
erasing itself per layer once that layer had been used enough. Cut before it was built: it only
repeats what the finger already knows by the time it can read it, and it was the one zone whose
size forced the 6x8 font on everything else. The 24 px it held went to the event strip, which
shows the things the layer name cannot.

**Mod slots are fingers, not modifiers.** Outer to inner: pinky, ring, middle, index. On Windows
those keys give GUI, Alt, Ctrl, Shift; on macOS the Ctrl/GUI swap means the same fingers give
Ctrl, Option, Cmd, Shift — the order on a MacBook's bottom row. Only the glyph changes with the
OS. `get_mods()` returns post-swap bits (`mod_config()` runs at keycode-to-action translation,
`quantum/keymap_common.c:162`), which is why the display used to light the wrong slot on the
Mac. Fixed: the slot comes from the key's *declared* mod, so it never moves, and liveness is
checked against the bit the key actually registered — recorded when it was pressed, because
`mod_config()` is applied at press time and an OS change while a key is held must not
re-attribute it to another finger.

**Why the mod row is dark at rest.** The first build drew all four glyphs always and
inverted a cell to mark a held modifier. Measured, that put the most routine event on
the keyboard into the alarm's own register: rung 2 inverts 128x8 = 1024 px, and two
held modifiers inverted 2 x 512 = exactly 1024. Ctrl+Shift, many times a minute, spent
the whole rung — and three held modifiers outlit it. Drawing a cell only while its
finger holds something costs nothing in legibility, because the order never moves and
the signal was never the letter but the cell lighting up. It also makes "dark at rest"
literally true and leaves inversion entirely to the alarm row.

**The lock glyphs are 1 px and must never be inverted in place.** They are 7x7 inside an
8x8 cell — column 7 is a gutter so the 1 px channel between neighbours cannot close under
bloom, row 7 belongs to the SYS underline. At that size a 2 px stroke leaves no interior,
and 1 px is what the font's own symbol cells use anyway. Inverted they would become dark
hairlines in a lit field, which is the failure the mascot's work already measured. Nothing
inverts this row: the alarm is a different page. If that ever changes, this set has to be
redrawn, and 7x7 will not give one that survives it. All three carry ink on row 0 and none
of the six badge letters does, so the cluster sits a pixel proud of any text beside it.

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

Alarms escalate by **scope**, not by brightness, so routine events never spend the loudest
register. The rungs are per panel, and the test is ordering inside one panel: nothing routine may
light more pixels than that panel's own alarm. On the master: normal, then the inverted banner on
the alarm row, then full-panel inversion. The master has no 1 px edge frame — the frame needs
x=0, x=127, y=0 and y=63 kept clear and a status panel cannot promise that, since the badge itself
lights x=0. Entry blinks three times over 600 ms, which routine changes never do.

Miryoku uses the left-hand mod-tap variants on both hands (`LSFT_T(KC_T)` and `LSFT_T(KC_N)`), so
`get_mods()` cannot tell which hand fired. Handedness comes from the matrix row, the same test
`tap_hold.c` already uses. Four cells, not eight: the pair of hands is what Chordal Hold decides
on, and one side of that comparison is misleading on its own.

### Split sync

One custom 5-byte RPC, master to slave, on change plus a 250 ms heartbeat and a 20 ms minimum
interval. Five bytes rather than four because the wire cost is 13 bytes of fixed overhead
against 43 us per payload byte — width is about 3% of a send, so the bytes go on clarity
rather than packing. Not `SPLIT_LAYER_STATE_ENABLE` / `SPLIT_MODS_ENABLE` / `SPLIT_LED_STATE_ENABLE`: those
re-send every 100 ms whether or not anything changed, and they cannot carry Caps Word (QMK has no
sync for it) or the host OS (`SPLIT_DETECTED_OS_ENABLE` ignores the SYS override). Layout: layer,
finger-ordered mods, locks, flags, and a sequence counter the slave uses as its staleness clock.
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
| `OLED_BRIGHTNESS` | 255 (QMK default) | 128 | Pairs with the above: OLED wear is strongly superlinear in drive current, so the two together are less total wear than today |

### Keeping the old art

The existing slave art stays selectable exactly as it is, through the
`OLED_SLAVE_ANIMATION_SPACESHIP` / `_MONITOR1` / `_MONITOR2` / `_KYRIA` flags in
`custom_config.h`. Bjorn is an additional option, not a replacement. A matching selector is added
for the master panel so today's text layout stays available the same way.

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
| `oled_panel.h` | the only place that decides which panel and which art compile in, with an `#error` on picking two |
| `oled.c` | dispatcher: sleep check, master/slave branch, nothing else |
| `oled_master_zones.c` | the panel. Four zones, raw page bytes rather than `oled_write`, because a zone has to clear exactly its own area. 1x and 2x glyph blitters; the 2x one doubles each source row, so a 16x16 is two page bytes a column |
| `panel_glyphs.c` | generated. The five mod-row glyphs — four-pane, option mark, caret, command, shift — hand-drawn at 16x16 in DUNGEON.mode's weight language, in page format. Eight slots are five drawings: shift is one drawing in both sets, and the caret is one drawing that changes cell when the OS does, so the Ctrl/GUI swap is visible as a hop from the middle finger to the pinky |
| `panel_font.c` | generated. Printable ASCII cut out of the DUNGEON.mode sheet, one byte a row, plus the symbol cells the event strip draws. Neither the sheet nor the generator is in the repo — the table is, and the table is what builds. Regenerating means fetching the pack again (see Attribution) |
| `oled_master_text.c` | the text panel. Layer names now come from Miryoku's `MIRYOKU_LAYER_LIST` X-macro instead of a second copy of the layer order, so they are the short forms (`Base`, `Nav`, `Sym`). Override `MIRYOKU_LAYER_LIST` to change them |
| `oled_slave_bjorn.c` | the mascot: tier hysteresis, idle ladder, ambient clocks, blitter |
| `split_state.c` / `.h` | the 5-byte payload the master gathers: modifiers in finger order, the active and default layers, locks and Caps Word, the host OS, and a misfire count. Built fresh every main loop; a modifier it reports is one that is held at that instant |
| `split_sync.c` / `.h` | the transport for that payload. Double-buffered on the slave because the callback runs on a HIGHPRIO thread; heartbeat 250 ms, minimum interval 20 ms, soft stale 600 ms, hard stale 1250 ms. `u_sync_link_down()` answers for both halves — the master from its own failed sends, the slave from the sequence number standing still |
| `oled_slave_spaceship.c` | `render_space()`, moved byte-for-byte |
| `oled_slave_logo.c` | the three static logos |
| `bjorn_art.c` / `.h` | generated: base, patch table, frame index |
| `art/*.h` | one header per animation, pure data, no selection logic, one includer each |

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
`Telnet`, `TEST`) and are not usable here — they would be illegible shrunk and would
clash with DUNGEON.mode. Four are pictorial and are worth drawing: a longship with shields
along the hull, a longhouse, a horned mask, and a bandit mask he can wear.

What constrains this is unchanged: about 23% lit, the block budget, the motion rules,
and every frame drawn by hand, because the mechanical reduction is not usable
(`tools/README.md`).

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
the doze extras, and a place for every one of Bjorn's seventeen sprite sets.

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
`docs/oled-v2.md` §2.4 is the fix that matches this evidence regardless of the cause — which is
the point of preferring it: it does not depend on guessing right.

### Attribution

Bjorn's sprites are MIT licensed. The art files carry infinition's copyright notice and
`VENDORED.md` records where they came from.

The panel's font is **DUNGEON.mode** by datagoblin, released CC0 — public domain, no attribution
required. Credited anyway, and recorded in `VENDORED.md`. Only the extracted glyph table is in the
repo, not the pack.

These two are the only parts of this repo that are not my own work.

## Open experiments

| Idea | Why | Status |
|------|-----|--------|
| Encoder push-clicks | Position 1 clicks are unassigned. Candidates: mute (left), play/pause (right) | later |
| OS layout: plain US instead of US-International | US-Intl's dead keys (`' " ` ~ ^`) clash with vim (`"a` gives ä, `'a` gives á) and with the symbol layer | to decide |
| å ä ö | Swedish later. Options: EurKEY (US base, letters on AltGr), or switching the OS layout when writing Swedish | later |
| Vim on Colemak-DH | `hjkl` are scattered on Colemak-DH. Miryoku's Nav layer has arrows on the right home row | later |
| OS-aware navigation | Word jump is Ctrl+Arrow on Windows but Option+Arrow on macOS; line start/end is Home/End vs Cmd+Arrow. OS detection can drive these too | later |
| Flow Tap after any key | If mods misfire right after Backspace, Enter or digits, widen `is_flow_tap_key()` to match the old typing streak | only if it happens |
| `PERMISSIVE_HOLD` | QMK's recommended pairing with Chordal Hold. Makes quick deliberate shortcuts register before 200 ms | after the migration settles |
| OLED redesign in pixel art | Specified in full under OLED above, including the Bjorn mascot, the goat, the master panel zones and the split-sync design | partly built, see Built and still to do |
| Invert the whole OLED | **Wanted, 2026-10-14.** Bjorn's source art is dark ink on a white page — it is an e-paper original — so inverting is not a style choice but a return to the medium he was drawn for. It also dissolves two reduction problems rather than working around them: the hooded poses, whose solid cowl blobs at 33% lit on a dark panel and is simply a solid cowl on a light one, and the pupil catchlight, a dark speck in a lit mass that the panel eats and an inverted one would not. It blocks nothing: the dither produces the same bits either way and inversion is a hardware flip, so all 32 can be reduced before deciding. The costs are unchanged — the alarm ladder's top rung *is* full-panel inversion and an already-inverted panel cannot escalate to it; "dark at rest" becomes "lit at rest"; and wear goes from about 23% of pixels lit to about 77%, which is the one that needs thinking about even though the panels now sleep |

## Tuning log

| Date | Change | Why | Verdict after a week |
|------|--------|-----|----------------------|
| 2026-10-14 | Flashed `91477af` to both halves. The misfire no longer replays after an outage; the master counts failed sends, outages, transport drops and host sleeps, and main loops per second; the other half returns its uptime, stale episodes and build id on the transaction the master already makes; SYS held two seconds shows all of it, which moves the Win/Mac toggle to the release of a short press; the other half restarts itself after five seconds of silence, with backoff and a cap of three, and a two second hardware watchdog catches the case where its code has stopped | The link died for twenty minutes on 2026-10-13 and the keyboard recorded nothing, so the cause is still unknown. Both halves were verified as running `c95356d` before this flash. None of the recovery depends on knowing the cause: the software reset covers a half that is running but silent, the watchdog one that has stopped | Open. Watch for `OUT`, `RST`, or the other half's `UP` falling far below the master's. The counters are RAM only, so look before replugging. **Main loop baseline measured on the board the same day: 924 per second, worst second 875** — a scan every 1.08 ms, against a 1 ms USB polling interval and a 5 ms debounce window, so the loop is already matched to the hardware downstream of it and oversamples debounce about five times over. The 5% spread between typical and worst is the part that says there are no stalls. Future stages are compared against this |
| 2026-10-14 | Flashed `c95356d` to both halves. Frames are composed in RAM and committed once; the master switches both panels off 120 s after the last key whatever is drawn; Bjorn reschedules his blink after a doze, the waking lids and a dead link; the link's sequence counts delivered states; LTO off | Neither panel had ever slept, because every zone cleared and redrew itself straight into the driver's buffer and any changed byte re-arms the display timeout. The same flaw made Bjorn stop blinking after every doze, which is this design's signal that the link is dead — so he reported a fault that was not there, every time Roel came back | Checked on the board the same day: both panels go dark at about 120 s, a key wakes both, he blinks within about ten seconds and keeps blinking after a doze, the master is unchanged and typing feels the same. Week's verdict still open |
