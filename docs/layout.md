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

## Tuning log

| Date | Change | Why | Verdict after a week |
|------|--------|-----|----------------------|
