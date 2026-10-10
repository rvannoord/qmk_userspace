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
| Modifier released | axe into his hand, held 800 ms, matching the master's mod trace. Not during the hold: a mod is held 100-300 ms, so tracking the hold strobes at 3 Hz. A static pose needs ~800 ms of dwell to read as static |
| Mod anomaly (held >500 ms, no companion key) | axe raised overhead, 3 frames, 1200 ms, plus drooping horns. The misfire indicator — the only thing that animates is the only thing that is wrong |
| Caps Word | spiked crown, and his 64x64 tile renders inverted: dark Bjorn on a white page, his native e-paper look, spent at the one moment it carries information |
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
`quantum/keymap_common.c:162`), which is why the current display lights the wrong slot on the Mac.

Alarms escalate by **scope**, not by brightness, so routine events never spend the loudest
register: normal, then an inverted status bar, then the 1 px edge frame, then full-panel inversion.
Entry blinks three times over 600 ms, which routine changes never do.

Miryoku uses the left-hand mod-tap variants on both hands (`LSFT_T(KC_T)` and `LSFT_T(KC_N)`), so
`get_mods()` cannot tell which hand fired. Handedness comes from the matrix row, the same test
`tap_hold.c` already uses. Four cells, not eight: the pair of hands is what Chordal Hold decides
on, and one side of that comparison is misleading on its own.

### Split sync

One custom 4-byte RPC, master to slave, on change plus a 250 ms heartbeat and a 20 ms minimum
interval. Not `SPLIT_LAYER_STATE_ENABLE` / `SPLIT_MODS_ENABLE` / `SPLIT_LED_STATE_ENABLE`: those
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
| Blink | every 6-10 s, held 200 ms | Pseudo-random from a small LCG, not `rand()`. 200 ms because at brightness 128 a shorter closure may not register |
| Breath | 3.6 s, held 300 ms | A human breath, roughly. Resting posture only: IDLE2 is IDLE1's partner frame and the other postures have no equivalent, so elsewhere the blink carries the ambient |

**Idle is gated on how long the WPM reading has been still, not on it being zero.** If the
split link drops mid-word the slave's copy of WPM freezes non-zero for ever, and a
`wpm == 0` test would leave Bjorn sprinting until the board is replugged. The half's own
key activity is OR'd in, so a local keypress wakes him without waiting for a sync.

Measured costs, from the generated art: blink **1 block** on every tier, breath 2, tier hop
8, doze entry 8, floor 1. Art is 2880 bytes as block-aligned patches rather than whole
frames, so a frame can be smaller than the column and the budget means something.

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
