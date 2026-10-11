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

Two SSD1306 panels, 128x64, 1-bit, one per half. Specified 2026-10-10, not yet in the firmware.
The master half (whichever has the USB cable) is the instrument panel; the other half is the
mascot. Plug into the same half every time, or the two swap places.

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

Ambient clocks are non-harmonic so the scene never looks metronomic: breath 1.2 s, goat tail
1.9 s, goat ear 4.3 s, blink pseudo-random 6-10 s.

| Trigger | Response |
|---------|----------|
| WPM tier (0-19 / 20-39 / 40-59 / 60+, 8 WPM hysteresis) | horn posture, four static poses from IDLE8 / IDLE1 / IDLE3 / IDLE9. No speed lines: motion in peripheral vision taxes concentration, and WPM is the one signal with nothing to act on |
| Command modifier held | axe into his hand, and for 800 ms after it is released. Two dirty blocks, seated beside him inside the left column so it never crosses the dither seam and leaves the right half free for the goat. Disjoint from the blink window, so the two compose with no combined variants. **Shift is excluded** — here it is a home-row mod, part of typing a capital rather than an instruction, and it is held so often the axe would simply always be there. The trailing dwell is what stops a 100-300 ms hold strobing the prop. An earlier rule fired on a *completed* command instead, and was wrong twice over: with Chordal Hold most same-hand shortcuts resolve as letters, and Miryoku's clipboard keys are not modifiers at all, so the event barely happens |
| Misfire: a command modifier held past 500 ms with nothing typed | the axe raises overhead over three frames and holds, 1200 ms in all, with his horns drooping. The only thing in the design that animates, on the principle that the only thing which moves should be the only thing that is wrong. The steps *slow* as it rises — 100 ms taking hold, then 150, 200 and 750 held at the top — because a lifted mass decelerates toward the top of its arc, and that is what makes it read as heavy rather than as a sprite being repositioned. Measured 4, 5 and 4 blocks for the raise; 8 entering or leaving a non-drooped posture, 3 leaving to the drooped one. The whole figure cuts to the drooped posture rather than splicing horns into the current one: the tier progression is an arousal scale so that pose already exists, half a droop reads as damage where both falling reads as deflation, and since nothing was typed the typing-speed posture carries no information worth preserving. A misfire outranks the held axe and **cancels** it rather than suspending it — resuming a stale success indicator afterwards would read as a second event that never happened |
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
| Status bar | (0,0) | 128x8 | `Win`/`Mac` badge, underlined if set by hand with SYS; fault or surprising-lock glyph; alarm banner; `AGR` / `MOD!` / `LYR!` |
| Mod row | (0,8) | 128x16 | four 32 px cells, 16x16 glyph each, inverted when active, 3 px handedness tick in the padding |
| Layer band | (46,24) | 36x16 | layer name, 12x16, blank on base |
| Layer map | (30,40) | 60x24 | the layer's own legends, 10x3, hidden per layer once that layer has been used enough |

**Mod slots are fingers, not modifiers.** Outer to inner: pinky, ring, middle, index. On Windows
those keys give GUI, Alt, Ctrl, Shift; on macOS the Ctrl/GUI swap means the same fingers give
Ctrl, Option, Cmd, Shift — the order on a MacBook's bottom row. Only the glyph changes with the
OS. `get_mods()` returns post-swap bits (`mod_config()` runs at keycode-to-action translation,
`quantum/keymap_common.c:162`), which is why the display used to light the wrong slot on the
Mac. Fixed: the slot comes from the key's *declared* mod, so it never moves, and liveness is
checked against the bit the key actually registered — recorded when it was pressed, because
`mod_config()` is applied at press time and an OS change while a key is held must not
re-attribute it to another finger.

Alarms escalate by **scope**, not by brightness, so routine events never spend the loudest
register: normal, then an inverted status bar, then the 1 px edge frame, then full-panel inversion.
Entry blinks three times over 600 ms, which routine changes never do.

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
| Doze breath | 5.4 s: 1.8 s in, 3.6 s out | Asleep he breathes slower, and asymmetrically — an even alternation reads as a metronome. The gesture is his beard rising, since he has no chest: his own outline pushed down one pixel, not new art. No blink; his eyes are already shut |
| Breath | 3.6 s, four states | His head and helmet **lift 1 px** while the beard stretches beneath them and his base stays pinned to the floor; the beard also pushes 1 px sideways over rows 41-52, so he fills as well as rises. 1 px is a hard ceiling, not a preference: his horn apex sits on row 1, so 2 px clips it against the top of the panel. Body leads and head follows in both directions, because the chest drives and the head is driven: rest 1.8 s, body fills 0.25 s, risen and held 0.8 s, body empties 0.4 s, head settles 0.35 s — about 1:2.4 in to out |

**Idle is gated on how long the WPM reading has been still, not on it being zero.** If the
split link drops mid-word the slave's copy of WPM freezes non-zero for ever, and a
`wpm == 0` test would leave Bjorn sprinting until the board is replugged. The half's own
key activity is OR'd in, so a local keypress wakes him without waiting for a sync.

Measured costs, from the generated art: blink **1 block** on every tier, breath 7, tier hop
8, doze entry 8, doze breath 6, half lid 1 (2 while risen), floor 1. Art is 4672 bytes. A blink during a breath needs its own frame per swell state. The pixels do not
collide, but a patch is a whole 64-byte block, so a blink block built from the resting
pose carries un-swelled body edges either side of the eyes and flattens the swell where
they share page 4. Three blink variants — rest, half, full — cost 64 bytes each and make
the composition correct by construction. The host-side blitter harness caught this; it is
invisible in a pixel-level comparison. Stored as block-aligned patches rather than whole
frames, so a frame can be smaller than the column and the budget means something.

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
| `oled_master_text.c` | the text panel. Layer names now come from Miryoku's `MIRYOKU_LAYER_LIST` X-macro instead of a second copy of the layer order, so they are the short forms (`Base`, `Nav`, `Sym`). Override `MIRYOKU_LAYER_LIST` to change them |
| `oled_slave_bjorn.c` | the mascot: tier hysteresis, idle ladder, ambient clocks, blitter |
| `oled_slave_spaceship.c` | `render_space()`, moved byte-for-byte |
| `oled_slave_logo.c` | the three static logos |
| `bjorn_art.c` / `.h` | generated: base, patch table, frame index |
| `art/*.h` | one header per animation, pure data, no selection logic, one includer each |

Ambient timing counts render ticks rather than wall-clock milliseconds, so it freezes with
the panel and resumes where it stopped instead of jumping to a random phase on wake. The
tier and the idle ladder use the wall clock and are never reset, so both are already
correct the moment the screen comes back.

### Attribution

Bjorn's sprites are MIT licensed. The art files carry infinition's copyright notice and
`VENDORED.md` records where they came from. This is the only part of this repo that is not my own
work.

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
| OLED redesign in pixel art | Specified in full under OLED above, including the Bjorn mascot, the goat, the master panel zones and the split-sync design | specified, not yet built |

## Tuning log

| Date | Change | Why | Verdict after a week |
|------|--------|-----|----------------------|
