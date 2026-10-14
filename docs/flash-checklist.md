# Flash checklist

What is on the board, what is waiting to go on it, and what to look at when it does.

Host tests cannot see I2C timing, the real split transport, or how anything looks. Those
are the only things that need eyes, so this file is the list of them. A row moves to
**Settled** once it has been looked at on the board, with the date.

Keep this current in the same commit that adds the behaviour, so the list is written
while the reason for it is still obvious.

---

## On the board now

| Build | Commit | Flashed | Notes |
|-------|--------|---------|-------|
| `2026-10-14-91477af.uf2` | `91477af` | 2026-10-14 | Both halves. Read-backs taken first and kept as `readback-{left,right}-2026-10-14b.uf2` |

Before it, `c95356d` (stage 1) and, before that, an unrecorded build — the read-backs of
2026-10-14 matched none of the kept `.uf2` files. From `91477af` on the build date and
the userspace commit are compiled in, so `strings` on a read-back identifies it.

---

## Waiting for the next flash

**Both halves, same build.** More than usually this time: `f2dd88c` takes the split
transaction table from fifteen entries to sixteen, and that count is XORed into every
handshake. A mismatched pair fails *every* transaction including the matrix, so the half
that was not flashed simply stops typing.

| Check | What should happen | Why it matters | From |
|-------|--------------------|----------------|------|
| Black and white are swapped | Both panels show dark figures on a lit field, everywhere — badge, mod glyphs, layer name, Bjorn | The whole point of the change: Bjorn's art is dark ink on a white page, so this is the medium he was drawn for | `a6bd2f9` |
| The alarm banner still shouts | Unplug the TRRS for two seconds. `LINK DOWN` should still read as a banner, now dark-on-light, and still blink three times on entry | It worked by inverting a row against a dark panel. Inverted, it is a dark row on a lit one. It should still contrast, but this is the thing most likely to have quietly stopped working | `a6bd2f9` |
| Caps Word still reads | Hold the Nav layer and tap the key under **M**. Bjorn's tile should still visibly flip against the rest of the panel | His tile inverts for Caps Word. With the panel already inverted, that flip now goes the other way — it should still be unmistakable, but check it is not now subtle | `a6bd2f9` |
| It is comfortable to look at | A lit panel at brightness 128 for two minutes at a time | Pixel wear goes from about a quarter of the pixels lit to about three quarters. The panels do now sleep after 120 s, which is what makes this tolerable, but it is a real change | `a6bd2f9` |
| Both halves idle together | Stop typing and watch both. Bjorn should settle at about 8 s and doze at about 75 s, in step with the master counting down to 120 s | He used to derive idle from how long the WPM reading had been *still*, and WPM keeps changing all the way down as it decays over its own five-second window — so that half started idling up to five seconds late | `f2dd88c` |
| The other half still types | Any key on the non-USB half | The transaction table changed. If this fails, the halves are on different builds | `f2dd88c` |
| `BLD` ends in `OK` | Hold SYS two seconds | Both halves agreeing on the build id. `!=` means the flash did not take on one of them, which otherwise looks exactly like a broken cable | `01174ee` |

### If the inversion is not wanted

Comment out `U_OLED_INVERT` in `users/manna-harbour_miryoku/custom_config.h` and reflash.
Nothing else depends on it and no art changes either way — the dither produces the same
bits and the inversion is a hardware command.

---

## Settled

| Check | Verdict | Date | From |
|-------|---------|------|------|
| Both panels go dark at about 120 s | Works. They had never done this | 2026-10-14 | `0dcfa5b`, `93a2210` |
| Bjorn blinks within ~10 s of waking, and keeps blinking after a doze | Works. Previously he stopped for up to 55 minutes, which is this design's "link is dead" signal | 2026-10-14 | `422f3d9` |
| The master panel is unchanged by the canvas rework | Works | 2026-10-14 | `93a2210` |
| Typing feels the same on both halves | Works. Main loop measured at 924/s, worst second 875 | 2026-10-14 | `f2dd88c` baseline |
| A short SYS tap still toggles Win/Mac | Works, now on release rather than press | 2026-10-14 | `01174ee` |

---

## The procedure itself

1. Build from a clean, committed tree, so the flash can be named by its commit.
2. Per half: USB into that half, then **double-tap its reset button**. Not Bootmagic —
   holding SYS while plugging in clears EEPROM.
3. If the build on the board is unknown, read it back first:
   `picotool save -a -v ~/kyria-firmware-backup/readback-<half>-<date>.uf2`
4. `picotool load -v -x <file>.uf2`, or `qmk flash` on the Mac.
5. Repeat for the other half. They cannot talk until both run the same build.
6. Keep the flashed file as `~/kyria-firmware-backup/<date>-<shortsha>.uf2`, add a
   Tuning-log row in `docs/layout.md`, and move this file's pending rows to Settled.
