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
| `2026-10-15-994a874.uf2` | `994a874` | 2026-10-15 | Both halves. The world layer, and the panel reverted to lit-on-black at brightness 255 |
| `2026-10-15-1008480.uf2` | `1008480` | 2026-10-15 | Both halves. Read-backs taken first, kept as `readback-{left,right}-2026-10-15c.uf2` |

Before it: `a8dc5e6`, `934e27c`, `91477af`, `c95356d`, and before those an unrecorded
build — the read-backs of 2026-10-14 matched none of the kept `.uf2` files.

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

matches a read-back against the kept builds. `c95356d` and the migration build predate
that and carry no date, so a read-back of either is identifiable only by byte comparison.

---

## Waiting for the next flash

**Both halves, same build.** More than usually this time: `f2dd88c` takes the split
transaction table from fifteen entries to sixteen, and that count is XORed into every
handshake. A mismatched pair fails *every* transaction including the matrix, so the half
that was not flashed simply stops typing.

| Check | What should happen | Why it matters | From |
|-------|--------------------|----------------|------|
| Black and white are swapped | Both panels show dark figures on a lit field, everywhere — badge, mod glyphs, the layer picture, Bjorn | The whole point of the change: Bjorn's art is dark ink on a white page, so this is the medium he was drawn for | `a6bd2f9` |
| ~~The alarm banner still shouts~~ | **Superseded**: there is no banner any more. See the band-inversion row below | The `a6bd2f9` check was written when the alarm was a 128x8 row on page 5. The three-band panel put the layer picture there and moved the alarm to the whole band | `a6bd2f9` |
| Caps Word still reads | Hold the Nav layer and tap the key under **M**. Bjorn's tile should still visibly flip against the rest of the panel | His tile inverts for Caps Word. With the panel already inverted, that flip now goes the other way — it should still be unmistakable, but check it is not now subtle | `a6bd2f9` |
| It is comfortable to look at | A lit panel at brightness 128 for two minutes at a time | Pixel wear goes from about a quarter of the pixels lit to about three quarters. The panels do now sleep after 120 s, which is what makes this tolerable, but it is a real change | `a6bd2f9` |
| Both halves idle together | Stop typing and watch both. Bjorn should settle at about 8 s and doze at about 75 s, in step with the master counting down to 120 s | He used to derive idle from how long the WPM reading had been *still*, and WPM keeps changing all the way down as it decays over its own five-second window — so that half started idling up to five seconds late | `f2dd88c` |
| The other half still types | Any key on the non-USB half | The transaction table changed. If this fails, the halves are on different builds | `f2dd88c` |
| Bjorn's own drawings appear | Stop typing. From about 20 s he should cut between six of infinition's frames, roughly every 18 s — two smiling, two with the goat, one in spectacles, one hooded | The first time anything beyond the four posture poses has been on the panel. **The goat arrives here**, because IDLE9 and IDLE10 contain it | `c15779c` |
| They stop the instant you type | Any key during a pose cuts straight back to the posture | This is also what keeps a drawn prop from being read as a signal: the axe means a modifier is held, and a pose can carry a sword | `c15779c` |
| A pose still looks alive | It should lift a pixel and settle, about every 3.6 s. It does **not** blink | A figure where nothing moves is this design's signal for a dead link. The lift is what keeps a pose out of that vocabulary — if it reads as frozen, that is a problem | `c15779c` |
| The master panel is three bands | Status bar, a big layer picture, then the mod row. No text anywhere but the badge | The whole rework. The layer is a picture now, which is read without being parsed | `panel` |
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
| `BLD` ends in `OK` | Hold SYS two seconds | Both halves agreeing on the build id. `!=` means the flash did not take on one of them, which otherwise looks exactly like a broken cable | `01174ee` |

### Only one season can be seen at a time

The world layer takes its season from `__DATE__`, so a build made in October shows
leaves and nothing else. Snow, rain and the star field cannot be looked at on the board
without either rebuilding with a faked date or adding a way to force the state. That is
a real gap in what can be checked, and it is an argument for the host daemon doing more
than supplying weather: a forced state is also how these get tested.

### If the inversion is not wanted

Comment out `U_OLED_INVERT` in `users/manna-harbour_miryoku/custom_config.h` and reflash.
Nothing else depends on it and no art changes either way — the dither produces the same
bits and the inversion is a hardware command.

---

## Settled

| Check | Verdict | Date | From |
|-------|---------|------|------|
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
