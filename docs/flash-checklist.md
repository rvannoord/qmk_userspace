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
| `2026-10-22-0130b8b.uf2` | `0130b8b` | 2026-10-22 | Both halves, loaded and verified with `picotool load -v`, built from a clean tree (label `0130b8`). Caps Word lights drashna's Caps Lock glyph; two lock slots. No full read-back, because the board had run `f639c97` since the night before; the right half was identified before loading by reading the start of its flash |
| `2026-10-21-f639c97.uf2` | `f639c97` | 2026-10-21 | Both halves, loaded and verified with `picotool load -v`, built from a clean tree (label `f639c9`, no `-dirty`). Stage 1 of `docs/oled-v3.md`: Ein, drashna's icons and locks, the host logos with the iOS "i", the 7-byte payload. Read-backs of both halves taken first, `readback-{left,right}-2026-10-21.uf2`: both were `27a1e24`, matching the table |

Before it: `27a1e24` (2026-10-16, the new space behind the ship). Before that, all on 2026-10-15 and all kept: `0c47a23` (the animation key), `1034f2e`,
`6c9b28f`, `994a874` (the world layer, lit-on-black at 255), `1008480` (read-backs taken
first, kept as `readback-{left,right}-2026-10-15c.uf2`). Before those: `a8dc5e6`,
`934e27c`, `91477af`, `c95356d`, and an unrecorded build — the read-backs of 2026-10-14
matched none of the kept `.uf2` files.

**This table was wrong for a day, and the fix is the point of the method.** It still read
`91477af` on 2026-10-15 while the board had been reflashed four times since, 21 commits
of drift. The read-back settled it without guessing: both halves carried the build date
`2026-10-15-10:43:35`, an exact match to `2026-10-15-a8dc5e6.uf2` and to nothing else.
That is what the compiled-in build date is *for*, and it is worth noting that the entry
it replaced was written by hand and was simply never updated — so prefer a read-back over
this table whenever the two could disagree.

From `91477af` on, the build date and the userspace commit are compiled in:

```sh
strings <file>.uf2 | grep -oE "20[0-9]{2}-[0-9]{2}-[0-9]{2}-[0-9]{2}:[0-9]{2}:[0-9]{2}" | sort -u
```

matches a read-back against the kept builds. **It can miss one:** a `.uf2` is cut into 256-byte blocks, and a date that falls across a block boundary comes out of `strings` as two pieces (`0130b8b`'s shows only `2026-10-22-0`). If nothing matches, search the read-back for the commit label instead, which is shorter and less likely to be split. `c95356d` and the migration build predate
that and carry no date, so a read-back of either is identifiable only by byte comparison.

---

## Waiting for the next flash

**Both halves, same build.** More than usually this time: `f2dd88c` takes the split
transaction table from fifteen entries to sixteen, and that count is XORed into every
handshake. A mismatched pair fails *every* transaction including the matrix, so the half
that was not flashed simply stops typing.

| Check | What should happen | Why it matters | From |
|-------|--------------------|----------------|------|
| ~~Black and white are swapped~~ | **Superseded**: the inversion from `a6bd2f9` was reverted in `994a874`, and the panel is lit-on-black again | The check as written can no longer be made. Kept because rows move here, they do not vanish | `a6bd2f9` |
| ~~The alarm banner still shouts~~ | **Superseded**: there is no banner any more. See the band-inversion row below | The `a6bd2f9` check was written when the alarm was a 128x8 row on page 5. The three-band panel put the layer picture there and moved the alarm to the whole band | `a6bd2f9` |
| ~~Caps Word still reads~~ | **Superseded**: the inversion from `a6bd2f9` was reverted in `994a874`, and the panel is lit-on-black again | The check as written can no longer be made. Kept because rows move here, they do not vanish | `a6bd2f9` |
| ~~It is comfortable to look at~~ | **Superseded**: the inversion from `a6bd2f9` was reverted in `994a874`, and the panel is lit-on-black again | The check as written can no longer be made. Kept because rows move here, they do not vanish | `a6bd2f9` |
| Both halves idle together | Stop typing and watch both. Bjorn should settle at about 8 s and doze at about 75 s, in step with the master counting down to 120 s | He used to derive idle from how long the WPM reading had been *still*, and WPM keeps changing all the way down as it decays over its own five-second window — so that half started idling up to five seconds late | `f2dd88c` |
| The other half still types | Any key on the non-USB half | The transaction table changed. If this fails, the halves are on different builds | `f2dd88c` |
| Bjorn's own drawings appear | Stop typing. From about 20 s he should cut between six of infinition's frames, roughly every 18 s — two smiling, two with the goat, one in spectacles, one hooded | The first time anything beyond the four posture poses has been on the panel. **The goat arrives here**, because IDLE9 and IDLE10 contain it | `c15779c` |
| They stop the instant you type | Any key during a pose cuts straight back to the posture | This is also what keeps a drawn prop from being read as a signal: the axe means a modifier is held, and a pose can carry a sword | `c15779c` |
| A pose still looks alive | It should lift a pixel and settle, about every 3.6 s. It does **not** blink | A figure where nothing moves is this design's signal for a dead link. The lift is what keeps a pose out of that vocabulary — if it reads as frozen, that is a problem | `c15779c` |
| The master panel is three bands | Status bar, a big layer picture, then the mod row. No text anywhere: the badge became a logo at stage 1 of `docs/oled-v3.md` | The whole rework. The layer is a picture now, which is read without being parsed | `panel` |
| Base shows the Kyria logo | Rest on base | splitkb's own art out of QMK's default keymap for this board, and the only art here native to this panel | `panel` |
| Every layer has its own icon | Hold each thumb key in turn | Nine of them are splitkb's Halcyon icons reduced 120->40. If one is unreadable at arm's length that is a reduction artefact, and `tools/panel/icon_overrides.txt` is where it gets fixed | `panel` |
| A held mod fills its cell | Hold each home-row mod | The glyph is knocked out of the fill and the hand bar is cut out of it. All four glyphs are now drawn at rest, which they were not before | `panel` |
| `LINK DOWN` inverts the whole band | Unplug the TRRS for two seconds | It is no longer a banner: a 128x8 row had fallen to about two held cells. The layer icon should stay visible, lit on dark, with a broken chain beside it. On base the chain takes the band alone | `panel` |
| The default-layer rule still shows | Set a non-base default, then hold a thumb | Three rows under the icon. It must *not* appear when resting on that default, nor ever when the default is base | `panel` |
| The mod row matches your fingers | Plug the cable into the **right** half and hold each home-row mod | The row reverses with the half. On a right-hand master, pinky must be the rightmost cell. This is the one check that needs the cable moved | `panel` |
| A held cell looks like a key | Hold any home-row mod | Filled with 3 px chamfered corners, glyph knocked out. No hand bar any more — that was removed deliberately | `panel` |
| Both panels go dark when the PC sleeps | Sleep the host and watch | They used to stay lit for the full 120 s. The non-USB half should follow the master | `panel` |
| Bjorn does not blink in his glasses | Idle until the spectacled pose comes round | The lids are fixed patches and were closing over the lenses. He should still breathe, which is what keeps him out of the "link is dead" vocabulary | `panel` |
| Lit on black at brightness 255 | Comfortable to look at, and the lock glyphs still read | The inversion was tried and reverted. 255 is affordable now only because lit-on-black lights about a quarter of the pixels where inverted lit three quarters | `994a874` |
| 1 px rain still reads | Needs a season override to see at all — see below | It was measured as 95% hairline ink, which was the *inverted* panel's failure mode. Lit-on-black eats isolated lit pixels instead, so the risk moved rather than went away | `994a874` |
| QMK Firmware splash at boot | Both panels, about a second, then the normal panels | It is 63 characters of the panel's own font rather than new art. Also the cheapest confirmation a flash took | `a86f9db` |
| The animation key cycles | Right outer column top row, mirroring SYS. Bjorn, spaceship, Kyria, Monitor 1, Monitor 2, round again | Verified on a host harness including persistence and the slave taking it over the link, but never pressed on a board | `U_ANIM` |
| And it survives a replug | Pick a non-default one, unplug, plug back in | Only the master persists it; the slave takes what it is told, so they cannot disagree | `U_ANIM` |
| The spaceship flies through new space | Cycle to the spaceship. Three parallax planes - far stars slow, Saturn and the Dipper mid, Earth, the giant and the comet near - and the ship untouched | Loop closure and ship integrity are asserted on the compiled bytes by `tests/unit/test_space.c`; what the host cannot see is bloom on the 2x2 stars and whether 15.9% lit area is comfortable | `space` |
| The ship's outline is one dark pixel | Watch the ship cross Earth | Was up to 4 px. Zero made the ship vanish into the planet on the host; one is the floor. Bloom on glass could eat a one-pixel rim - if the ship loses its edge over Earth, the margin goes to 2 | `space` |
| The loop is seamless on glass | Watch one full loop at rest, 25.6 s | Arithmetic says the seam cannot show. The test says it does not. The panel is the only thing that can disagree | `space` |
| `BLD` ends in `OK` | Hold SYS two seconds | Both halves agreeing on the build id. `!=` means the flash did not take on one of them, which otherwise looks exactly like a broken cable | `01174ee` |
| The OS logo reads and follows the OS | Tap SYS: it flips between the Windows and the Apple logo, with a line under it. That the logo reads from the seat was settled on 2026-10-21 | It replaces the `Win`/`Mac` text, and its lower half sits in the band's empty top-left corner. Part of the same checkpoint | `v3-1` |
| The logo survives the alarm | Unplug the TRRS for two seconds. The logo, and the line if SYS set the OS, show dark, cut out of the lit band | Under the alarm that corner is lit with the rest of the band, so the logo is knocked out of it instead of vanishing into it | `v3-1` |
| The stats page in Ein | Hold SYS two seconds. All eight lines read | The font changed to drashna's Ein, and the stats page is the only text left on the master | `v3-1` |
| The boot splash is unchanged | `QMK Firmware` on both panels at boot, as before | The logo still comes from QMK's own font, because drashna's does not carry it | `v3-1` |
| Caps Lock and Scroll Lock read | Turn on Caps Word, then Caps Lock (Shift + the Caps Word key, held a moment on the Mac), then Scroll Lock (Fun layer) **on Windows**. Caps Word and Caps Lock light the same ⇪ at x=109; Scroll Lock sits at 118. On the Mac, Scroll Lock is brightness down and never lights | Both are drashna's 6x8 cells now (⇪ over a bar, and his Scroll Lock), replacing the hand-drawn padlock and ring. Caps Word lights the ⇪ as on his screen, which retired its own glyph. They must still read at 1 px without inverting. On the board since `0130b8b` | `v3-1` |
| iOS shows "i" and the Apple logo | Plug into an iPad or iPhone, if one is at hand. Tap SYS: Windows, with a line; again: the "i" and Apple logo back, with the line under both | The "i" is hand-drawn, 2 px wide and 10 rows tall, with a 2 px gap that moves the logo 4 px right; iOS swaps Ctrl and GUI like the Mac | `v3-1` |
| Linux shows Tux | Plug into a Linux machine, if one is at hand | QMK reports Linux, and the logo follows the new host byte. Behaviour stays Windows-like | `v3-1` |
| SYS on Linux flips Linux and Mac | On that Linux machine, tap SYS twice | It should go to the Apple logo with a line under it, then back to Tux with the line, not to Windows | `v3-1` |
| The other half still types | Any key on the non-USB half | The payload grew from six bytes to seven. The handshake is unchanged, but a half on the old build would drop every state as the wrong size | `v3-1` |

### Only one season can be seen at a time

The world layer takes its season from `__DATE__`, so a build made in October shows
leaves and nothing else. Snow, rain and the star field cannot be looked at on the board
without either rebuilding with a faked date or adding a way to force the state. That is
a real gap in what can be checked, and it is an argument for the host daemon doing more
than supplying weather: a forced state is also how these get tested.

---

## Settled

| Check | Verdict | Date | From |
|-------|---------|------|------|
| The mod icons and the host logo read from the seat (stage 1's checkpoint) | Passed. From a normal seated position the five 12x8 icons (⊞ ⌥ ⌃ ⌘ ⇧) and the 12x16 logo can be named without leaning in, although the icons are smaller than the 16x16 glyphs they replaced | 2026-10-21 | `f639c97` |
| Falling leaves on the slave half | Works, first look 2026-10-15. Six leaves, independent speed and tumble, passing behind Bjorn and reappearing | 2026-10-15 | `994a874` |
| A misfire chips one cell | Works, and reads correctly once known. Reported first as a fault — "the black outline doesn't disappear, but becomes narrower around the actual icon and stays there" — which is an exact description of the 32 px hold being replaced by the 16 px chip. Kept as drawn; see `docs/layout.md` | 2026-10-15 | `1008480` |
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
