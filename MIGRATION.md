# Migration: miryoku_qmk fork → qmk_userspace

Handoff from the planning session on 2026-10-09, reviewed the same day against qmk_firmware master @ 7a1bbf37c5 (2026-10-02). It lives in the repo so the migration can continue on either machine (work PC or home Mac). Keep the Progress section current, and delete this file once the migration is done.

## Progress

| Step | Status |
|------|--------|
| 0. Save today's firmware | Done 2026-10-09 on both machines. Work PC: rebuilt from `b12c2bb7c8` with `CONVERT_TO=liatris MIRYOKU_MAPPING=EXTENDED_THUMBS`, verified in the build's `cflags.txt`, in `C:\src\personal\kyria-firmware-backup\`. Mac: `~/kyria-firmware-backup/`, holding the 2026-05-28 `.uf2` that was sitting in `~/miryoku_qmk` plus the uncommitted OLED sources that exist nowhere else (see Environment (Mac)). The `grep MIRYOKU` question is settled: the only option ever passed was `MIRYOKU_MAPPING=EXTENDED_THUMBS`, and the converter was `CONVERT_TO=rp2040_ce`, which is the same build as `liatris` (`elite_c_to_liatris/pre_converter.mk` only points at `elite_c_to_rp2040_ce`, in the 2023 tree and in current QMK; the Mac build defines both `CONVERT_TO_LIATRIS` and `CONVERT_TO_RP2040_CE`). So the work PC's rebuild is converter-equivalent to what was flashed. **Settled 2026-10-09 by reading both halves' flash back** with `picotool save -a -v` (`readback-left.uf2`, `readback-right.uf2` in the Mac backup): both halves ran the committed `b12c2bb7c8` build — the `GUI`/`CTRL`/`SHIFT` strings are there, the `icon_cmd` bitmap is not — and they are byte-identical apart from 512 bytes at `0x101ff000`, the EEPROM-emulation area holding each half's settings. So the 2026-05-28 `.uf2` sitting in `~/miryoku_qmk` was built from that clone's working tree and **never flashed**; it has never run on hardware. The two read-backs are now the fallback: bit-exact, per half, settings included. Restore with `picotool load readback-<half>.uf2`, matching the file to the half. Open: attach a `.uf2` to a GitHub release on the archive tag so there is a copy reachable from both machines |
| 1. Tag the old work | Done. `archive/miryoku-with-latest-qmk` → `b12c2bb7c8`, pushed to GitHub |
| 2. Clone the userspace | Done. The fork's `main` was even with `qmk/qmk_userspace` at `940d6f5` |
| 3. Copy in | Done. Miryoku userspace, keymap as `rvannoord`, fixes 1–3, `VENDORED.md`, `.gitattributes`, docs, `qmk.json`. The keymap's `rules.mk` doesn't have `MIRYOKU_CLIPBOARD = WIN` yet, so the step 4 build is today's layout; it's added with the host OS work in step 6 |
| 4. First build | Done. `qmk compile -kb splitkb/kyria/rev3 -km rvannoord` builds clean (no warnings), and `cflags.txt` shows `EXTENDED_THUMBS`, Liatris and all of Miryoku's features. The keymap rename to `rvannoord` worked. Not for flashing: the `BILATERAL_COMBINATIONS*` defines do nothing in stock QMK, so home-row mods are plain QMK until step 5 |
| 5. Tap-hold | Done. `BILATERAL_COMBINATIONS*` and `DEFERRED_EXEC_ENABLE` removed, `CHORDAL_HOLD` + `FLOW_TAP_TERM 160` in `custom_config.h`, callbacks in the keymap's `tap_hold.c` (spaces, per the repo's `.editorconfig`/`.clang-format`). Confirmed by preprocessing with the build's flags: `CHORDAL_HOLD`, `FLOW_TAP_TERM 160`, `TAPPING_TERM 200`, `QUICK_TAP_TERM 0`, no `PERMISSIVE_HOLD`. `LTO_ENABLE` left in `custom_rules.mk` (redundant, harmless). Behaviour unverified until step 7 |
| 6. Host OS | Done. `MIRYOKU_CLIPBOARD = WIN` and `OS_DETECTION_ENABLE` in the keymap's `rules.mk`, `OS_DETECTION_SINGLE_REPORT` in `custom_config.h`, `host_os.c`/`.h`, `process_record_user` in `keymap.c` (SYS toggle, Mac redo), and `Win`/`Mac` at the right of the "Kyria rev3" line on the master OLED. The compiled `keymaps` array has `U_SYS` (`0x7E40`) at `[0,6]`, the Bootmagic key, on all 10 layers. Behaviour unverified until step 7 (scenario 10) |
| 7. Scenario tests | Done. `tests/run_scenarios.sh new\|old`, see `tests/README.md`. New 13/13 (scenarios 1–10, with 8 split into a/b, plus crossover boundaries). Old 10/12: only S2 and S4 fail, which are the known differences. A timing sweep showed both trees switch from roll to shortcut at exactly 81 ms (Shift) and 121 ms (GUI). Each test first taps an unrelated key and waits 500 ms, because the old code keeps its last-key time in static state while the test clock restarts at 0 for every test, which faked a GUI difference before that was added |
| 8. Flash both halves | In progress. Read-backs **done** 2026-10-09 for both halves (see step 0), so the fallback is real and nothing on the keyboard is at risk; the flashing itself is still to do. Roel flashes. The read-back rule, for any future flash of an unknown build, in this order per half: put the half in the bootloader, run the read-back, only then flash. The `RPI-RP2` drive accepts UF2 writes but offers no way to read flash out (it holds only `INDEX.HTM` and `INFO_UF2.TXT`), so the read-back goes over the boot ROM's PICOBOOT interface instead: `picotool save -a -v ~/kyria-firmware-backup/readback-<half>.uf2` (picotool 2.3.1 installed on the Mac 2026-10-09; `-a` saves all of flash, so the wear-levelling area comes along, and `-v` verifies what was read). That gives the bit-exact fallback step 0 never had and settles which build is on the board. Do both halves: only the USB half renders the mod line. Easiest on the Mac, where `qmk flash` can write the drive directly; on Windows picotool may want a USB driver. Fallback: the step 0 `.uf2` |

Work-PC-only things the later steps need:
- `user.overlay_dir` is set to `/mnt/c/src/personal/qmk_userspace` in WSL (`/root/.config/qmk/qmk.ini`).
- The old tree for step 7's comparison tests is a native WSL clone at `/root/miryoku_qmk_old` (archive tag, submodules initialised, built once). The 2023 tree needs `appdirs`, which the installed QMK CLI lacks. Run it as `uv tool run --from qmk==1.2.0 --with-requirements requirements.txt qmk ...` from inside that tree. On the Mac it would need its own clone of the archive tag.
- Git for `C:/src/personal/` comes from `~/.gitconfig-personal` (via `includeIf`): personal identity, 1Password signing, and GitHub over SSH on `ssh.github.com:443` (port 22 is blocked on the work network). The Mac has its own setup.

## Where things stand

- Working branch: `miryoku-with-latest-qmk` in `rvannoord/miryoku_qmk`. The local clone at `C:\src\personal\miryoku_qmk` is checked out on the old `miryoku` branch, not the working one. No upstream remote is configured, and no git submodules are initialised.
- That branch is QMK from October 2023 (the `user-keymaps-still-present` snapshot) with sunaku's `miryoku_bilateral` branch (QMK 0.20.0) merged in. The ~250 lines of bilateral code in `quantum/action.c` are what made the fork unsyncable. There is also a dead one-liner in `quantum/action_tapping.c` (an `#error` about `IGNORE_MOD_TAP_INTERRUPT` commented out; nothing in this build defines it).
- Upstream Miryoku QMK is still on QMK 0.20 (February 2023), last commit 2025-02-28. There is nothing to sync from.
- drashna (QMK collaborator) maintains a Miryoku port for external userspace: `drashna/qmk_userspace`, branch `miryoku` ([discussion](https://github.com/manna-harbour/miryoku/discussions/287)). Checked 2026-10-09 at commit `b954f4bb` (2026-06-27). Diffed against my branch, its Miryoku code is identical apart from exactly the fixes in step 3 (plus AVR-only size savings, inert on the RP2040). Its Kyria `config.h` is identical to mine apart from `RGBLIGHT_LIMIT_VAL`. Used only as a cross-check that step 3's fix list is complete. No dependency on it.
- Other repos: `rvannoord/qmk_userspace` (fork of the template, synced 2026-10-09), `rvannoord/qmk_firmware` (fork, 2024-09), `rvannoord/miryoku` (2024-10). About 20 stale upstream branches are mirrored in `rvannoord/miryoku_qmk`.
- My own files on the branch:
  - `keyboards/splitkb/kyria/keymaps/manna-harbour_miryoku/`: `config.h`, `keymap.c`, `oled.c`, `oled_frames.h`, `oled_logos.h`, `rules.mk`
  - `users/manna-harbour_miryoku/custom_config.h`, `custom_rules.mk`

## Environment (work PC, checked 2026-10-09)

- No QMK CLI on Windows and no QMK MSYS. No `gh` either, so GitHub tasks (sync fork, releases, archiving) happen in the browser.
- **QMK is already installed in WSL Ubuntu** (26.04, done by the planning session on 2026-10-09). Don't reinstall it; run `qmk doctor` to confirm it still works.
  - The distro has only the `root` user, so everything lives under `/root`.
  - QMK CLI 1.2.0 at `/root/.local/bin/qmk` (via `uv`), and toolchains at `/root/.local/share/qmk` (arm-none-eabi-gcc 15.2.0).
  - Stock `qmk_firmware` at `/root/qmk_firmware`, master 0.34.6 (7a1bbf37c5, 2026-10-02), submodules initialised.
  - A test build of the stock `splitkb/kyria/rev3` keymap with `CONVERT_TO=liatris` produced a `.uf2`. The test file was deleted.
- From Git Bash, run WSL commands as `wsl.exe -d Ubuntu --exec bash -lc '...'` with `MSYS_NO_PATHCONV=1`. Without `--exec`, the default shell expands `$VARS` before bash sees them.
- The userspace repo lives at `C:\src\personal\qmk_userspace`, with `user.overlay_dir` pointing to `/mnt/c/src/personal/qmk_userspace`. The scenario tests run in the same WSL setup.
- Flashing: the Liatris shows up as an `RPI-RP2` drive in Windows. WSL doesn't auto-mount it, so `qmk flash` inside WSL can't find it. Copy the `.uf2` to the drive from Explorer (or `sudo mount -t drvfs E: /mnt/e` and `cp`).
- Podman is installed, but its machine also runs the Monitor Postgres container for work. If a QMK container is needed, never reset or restart the podman machine.

## Environment (Mac, checked and set up 2026-10-09)

Apple Silicon, macOS 26.6.1. Builds and flashes natively; no WSL, no container.

- **Git**: the global identity is the personal one, `rvannoord <rvannoord@icloud.com>`, with `gpg.format=ssh`, `commit.gpgsign=true` and the signer at `/Applications/1Password.app/Contents/MacOS/op-ssh-sign`. No `includeIf` split is needed here, unlike the work PC. Commits in both repos do carry a `gpgsig`; `git log --show-signature` cannot verify them locally only because `gpg.ssh.allowedSignersFile` is unset, which is cosmetic. `origin` for the userspace is HTTPS, so pushes do not use the SSH key.
- **`~/qmk_userspace`**: this repo. `qmk compile` drops a `.uf2` here as well as in the QMK tree; `.gitignore` covers it.
- **`~/qmk_firmware`**: stock `qmk/qmk_firmware`, cloned 2026-10-09, master at `7a1bbf37c5` (0.34.6) — the same commit the work PC built against — submodules initialised. The 2024 clone that used to be at this path (0.26.11, 50 commits ahead of upstream with sunaku's bilateral branch merged, and a `sunaku` remote) was **moved** to `~/qmk_firmware_sunaku_2024`, not deleted.
- **`~/miryoku_qmk`**: the old clone, left exactly as it was, including the uncommitted mod-icon work. Three remotes (`origin`, `miryoku_qmk` = manna-harbour, `sunaku`), which is why lazygit asks which base repo to use for pull requests. To rebuild the old firmware from it, name the tree explicitly rather than relying on the cwd: `QMK_HOME=~/miryoku_qmk qmk compile -kb splitkb/kyria/rev3 -km manna-harbour_miryoku -e CONVERT_TO=rp2040_ce -e MIRYOKU_MAPPING=EXTENDED_THUMBS`.
- **QMK CLI** 1.1.8 from Homebrew (`qmk/qmk/qmk`); the tap has nothing newer, and the work PC's 1.2.0 came from `uv`. 1.1.8 builds master 0.34.6 and runs the tests without complaint. `user.qmk_home=/Users/roelvannoord/qmk_firmware` and `user.overlay_dir=/Users/roelvannoord/qmk_userspace` are set, in `~/Library/Application Support/qmk/qmk.ini`, so `qmk compile` works from any directory.
- **Toolchain**: `arm-none-eabi-gcc@8` and `avr-gcc@8` were both broken — `cc1` could not load `libisl.23.dylib` because `isl`, `mpfr` and `libmpc` had been pruned off the system (`brew missing` listed them for both compilers and for `qmk`). Fixed with `brew install isl mpfr libmpc`. `make` was missing too, and macOS only ships GNU Make 3.81, so `brew install make` as well (`gmake` 4.4.1; the plain `make` 3.81 still drives the test build fine). The AVR and DFU flashing tools the `qmk` formula wants are still missing on purpose: nothing on the RP2040 path uses them.
- **No `uv`**, so step 7's `old` comparison tests cannot be run here with the recipe in `tests/README.md`. Their results are already recorded; the `old` tree itself only exists in WSL on the work PC.
- **Flashing** can use `qmk flash` here, because macOS mounts the `RPI-RP2` volume. On the work PC the `.uf2` has to be copied in Explorer.
- Verified on 2026-10-09 after the setup: `qmk compile -kb splitkb/kyria/rev3 -km rvannoord` builds clean with no warnings; `cflags.txt` shows `MIRYOKU_MAPPING_EXTENDED_THUMBS`, `MIRYOKU_CLIPBOARD_WIN`, `OS_DETECTION_ENABLE` and the Liatris/RP2040 target; preprocessing with the build's own flags gives `CHORDAL_HOLD`, `FLOW_TAP_TERM 160`, `TAPPING_TERM 200`, `QUICK_TAP_TERM 0`, `OS_DETECTION_SINGLE_REPORT`, `U_SYS`, and no `PERMISSIVE_HOLD` or `BILATERAL_*`; `tests/run_scenarios.sh new` passes 13/13.

## Git identity and signing (work PC)

- This PC's global git config is the **work** identity, `Roel van Noord <roel.vannoord@monitor.se>`, with no signing. My personal commits are `rvannoord <rvannoord@icloud.com>` and SSH-signed with a key kept in 1Password.
- 1Password's signer is installed at `C:/Users/roel.van.noord/AppData/Local/Microsoft/WindowsApps/op-ssh-sign.exe`.
- **Before the first commit**, personal repos need their own identity, without touching the work repos. Proposed: an `includeIf` in the global config that applies to everything under `C:/src/personal/`. Changing the global config needs Roel's okay.

```ini
# added to ~/.gitconfig
[includeIf "gitdir/i:C:/src/personal/"]
    path = ~/.gitconfig-personal
```

```ini
# ~/.gitconfig-personal
[user]
    name = rvannoord
    email = rvannoord@icloud.com
    signingkey = <public SSH key from 1Password>
[gpg]
    format = ssh
[gpg "ssh"]
    program = C:/Users/roel.van.noord/AppData/Local/Microsoft/WindowsApps/op-ssh-sign.exe
[commit]
    gpgsign = true
```

- The public key comes from the SSH key item in 1Password; its "Configure Commit Signing" option shows the exact lines. It's a public key, so pasting it is fine. Never handle the private key.
- 1Password has to be unlocked when committing, and may ask Roel to approve each signature. Claude can't approve it.
- Verify before the first commit: `git -C C:/src/personal/qmk_userspace config user.email` must print `rvannoord@icloud.com`. After it, `git log --show-signature -1` should show a good signature (that needs `gpg.ssh.allowedSignersFile` to verify locally; GitHub verifies it anyway if the key is registered there as a signing key).

## Decisions

| Topic | Decision |
|-------|----------|
| Repo | QMK external userspace in `rvannoord/qmk_userspace`, built against stock QMK master. No core patches |
| Miryoku source | My own branch's `users/manna-harbour_miryoku/` (commit `b12c2bb7c8`), with the fixes from step 3 applied by us. The only upstream dependencies are `qmk/qmk_userspace` (the template) and `qmk/qmk_firmware` (the build). drashna's port is a cross-check, not a source |
| Controller | Liatris (RP2040). `CONVERT_TO = liatris` in the keymap's `rules.mk` (Kyria rev3 is `development_board: elite_c`, and `elite_c` → `liatris` exists) |
| Build | QMK CLI in WSL Ubuntu |
| Base | Colemak-DH |
| Miryoku options | `MIRYOKU_MAPPING = EXTENDED_THUMBS` (same as today, staying) and `MIRYOKU_CLIPBOARD = WIN` in the keymap's `rules.mk`; everything else default |
| Tap-hold | Chordal Hold + Flow Tap, set up to match the old bilateral settings (below). No `PERMISSIVE_HOLD` |
| Host OS | OS detection; swap Ctrl/GUI on the Mac; SYS key toggles by hand; Apple Silicon, so `OS_DETECTION_SINGLE_REPORT` |
| OS layout | Not decided, and not part of the migration. Recommendation: plain US |
| Halcyon | Later. ZMK, separate decision |

## Steps

0. **Save the firmware that works today.** Nothing gets flashed until this file exists.
   - First choice: if the April 2026 build was made on the Mac, the `.uf2` is probably still in that clone's root (`splitkb_kyria_rev3_*.uf2`). Only use it if it matches what's flashed now.
   - Otherwise rebuild it in WSL. In the old clone, check out `miryoku-with-latest-qmk`, initialise the submodules (`qmk git-submodule`, or `make git-submodule`; large download), then `qmk compile -kb splitkb/kyria/rev3 -km manna-harbour_miryoku -e CONVERT_TO=liatris`, plus the Miryoku options of the build that's on the keyboard now. The old build took them from the command line and nothing in the repo records them. Confirmed with Roel: today's firmware uses `-e MIRYOKU_MAPPING=EXTENDED_THUMBS` (Space on thumb position 4). If the Mac's shell history shows other options (`grep MIRYOKU ~/.zsh_history`), add those too. A rebuild with different options is not a fallback. Unverified: whether the 2023 tree builds with the 2026 toolchain and Python 3.14. If it doesn't, try the old tree's `util/docker_build.sh` with `RUNTIME=podman`.
   - Keep the `.uf2` outside both repos, or attach it to a GitHub release on the archive tag.
   - A failing step 0 isn't dangerous, because the keyboard keeps today's firmware until something is flashed.
1. **Tag the old work:** `git -C C:/src/personal/miryoku_qmk tag archive/miryoku-with-latest-qmk origin/miryoku-with-latest-qmk`. Ask before pushing the tag.
2. **Clone the userspace.** The fork is already synced; clone it to `C:\src\personal\qmk_userspace`. The stock `qmk_firmware` is already at `/root/qmk_firmware` in WSL.
3. **Copy in** (each change to vendored Miryoku files gets a line in `VENDORED.md`):
   - `users/manna-harbour_miryoku/` from the working branch (`git archive origin/miryoku-with-latest-qmk users/manna-harbour_miryoku`). It carries my `custom_config.h` and `custom_rules.mk`.
   - Fix 1: in its `rules.mk`, change the two cwd-relative includes to `include $(USER_PATH)/custom_rules.mk` and `include $(USER_PATH)/post_rules.mk`. They fail in an external userspace, because make runs from the qmk_firmware root and `VPATH` doesn't apply to `include`.
   - Fix 2: in `manna-harbour_miryoku.c`, change the `key_overrides` pointer-with-NULL form to `const key_override_t *key_overrides[] = { &capsword_key_override };`. Current QMK takes `ARRAY_SIZE(key_overrides)`, which is a hard error on a pointer.
   - Fix 3: keycode shims in `custom_config.h` (below) for the renamed mouse and RGB keycodes, instead of editing `miryoku_babel`. They also cover the `KC_BTN*` in the thumb combos in `manna-harbour_miryoku.c`.
   - `keyboards/splitkb/kyria/keymaps/manna-harbour_miryoku/` as `keyboards/splitkb/kyria/keymaps/rvannoord/`, with `USER_NAME := manna-harbour_miryoku` in its `rules.mk`. If the rename causes trouble, keep the name `manna-harbour_miryoku` and update `CLAUDE.md`.
   - `.gitattributes` with `* text=auto eol=lf`. A `rules.mk` with CRLF line endings breaks make.
   - `CLAUDE.md`, `.claude/rules/oled.md` and `docs/layout.md` from this folder.
   - A `VENDORED.md` recording where `users/manna-harbour_miryoku/` came from (my `rvannoord/miryoku_qmk`, branch `miryoku-with-latest-qmk`, commit `b12c2bb7c8`, itself Miryoku upstream) and each fix applied since. When a future QMK change breaks Miryoku, fix it here and note it in `VENDORED.md`. Checking whether drashna's port already fixed the same thing is an optional shortcut.
   - `qmk userspace-add -kb splitkb/kyria/rev3 -km rvannoord`. It writes `qmk.json`, which `qmk userspace-compile` and the fork's GitHub Actions workflow use.
4. **First build:** `qmk config user.overlay_dir="/mnt/c/src/personal/qmk_userspace"`, then `qmk compile -kb splitkb/kyria/rev3 -km rvannoord`. The output name has no `_liatris` suffix, because the suffix is worked out before the keymap's `rules.mk` is read. That's only cosmetic.
5. **Tap-hold:** replace bilateral combinations with Chordal Hold and Flow Tap (below). Remove every `BILATERAL_COMBINATIONS*` define and `DEFERRED_EXEC_ENABLE`. `LTO_ENABLE` in `custom_rules.mk` is redundant (the Kyria's `info.json` turns LTO on anyway), so removing it changes nothing.
6. **Host OS** handling and the SYS key (below).
7. **Scenario tests** (below), run against the old tree and the new setup.
8. **Flash both halves.** The first boot resets EEPROM, because the eeconfig format changed since 2023: RGB mode and brightness go back to defaults. That's expected. Use it for a week and log what happens in `docs/layout.md`. If anything goes badly wrong, flash the step 0 file.
9. **Archive** `rvannoord/miryoku_qmk` and `rvannoord/qmk_firmware` on GitHub, but only after a week or two as the daily driver on both the Windows and the Mac machine without needing the step 0 file. Check the archive tag is on GitHub first. Archiving is read-only and reversible; never delete the repos. Roel does this, not Claude.

Commit and push only when Roel asks.

## Keymap `rules.mk`

```make
USER_NAME := manna-harbour_miryoku
CONVERT_TO = liatris

MIRYOKU_MAPPING = EXTENDED_THUMBS
MIRYOKU_CLIPBOARD = WIN

OS_DETECTION_ENABLE = yes
SRC += oled.c tap_hold.c host_os.c
```

Miryoku's options are make variables (`post_rules.mk` turns them into `-DMIRYOKU_..._VALUE`). Setting them here, rather than with `-e` on the command line, means a plain `qmk compile` always gives the same layout. Options left out use Miryoku's defaults:

| Option | Values | Default | Mine |
|--------|--------|---------|------|
| `MIRYOKU_ALPHAS` | AZERTY, BEAKL15, COLEMAK, COLEMAKDH, COLEMAKDHK, DVORAK, HALMAK, QWERTY, QWERTZ, WORKMAN | COLEMAKDH | default |
| `MIRYOKU_EXTRA` | same list. The Extra layer, a second base layer selected through the tap-dance keys | QWERTY | default |
| `MIRYOKU_TAP` | same list. The Tap layer, a base layer without home-row mods or layer-taps | COLEMAKDH | default |
| `MIRYOKU_NAV` | INVERTEDT, VI | arrows on the right home row | default |
| `MIRYOKU_CLIPBOARD` | FUN, MAC, WIN | Shift/Ctrl+Insert, plus Undo/Redo keys that only Linux understands | WIN |
| `MIRYOKU_LAYERS` | FLIP | layer keys as designed | default |
| `MIRYOKU_MAPPING` | Kyria: EXTENDED_THUMBS | thumb-arc positions 2–4 | EXTENDED_THUMBS (3–5), as today. See Thumb mapping |
| `MIRYOKU_KLUDGE_THUMBCOMBOS` | yes | off | off (only for boards with two thumb keys per side) |

Never add `keymap.c` to `SRC`. QMK includes it into `keymap_introspection.c` itself.

## Thumb mapping

Each half's thumb arc has five positions. Numbered from the outside edge toward the middle of the board, they are 1–5 (left half: `L25 L24 L23 L22 L21` in `keyboard.json`, x = 2.5 → 6.5; right half mirrored, `R25` outermost). In `LAYOUT()` the left thumb row is listed 1→5 and the right one 5→1.

**Position 1 on both halves is a rotary encoder**, not a key. Its push-click is the key at that matrix position and is `XXX` today. Rotation: left = volume, right = screen brightness (`encoder_update_user` in `keymap.c`).

| Mapping | Positions | Left (1→5) | Right (5→1) |
|---------|-----------|------------|-------------|
| Miryoku default | 2–4 | `XXX, K32, K33, K34, XXX` | `XXX, K35, K36, K37, XXX` |
| `EXTENDED_THUMBS` (mine, staying) | 3–5 | `XXX, XXX, K32, K33, K34` | `K35, K36, K37, XXX, XXX` |

With `EXTENDED_THUMBS`, left 3–5 are Esc (Media), Space (Nav), Tab (Mouse), and right 5–3 are Enter (Sym), Backspace (Num), Delete (Fun). Position 2, between the encoder and Esc/Delete, is free on both halves. Both variants are already in the keymap's `config.h`; `U_SYS` goes in the top-left of both.

## Keycode shims

In `custom_config.h`. These keycodes were renamed in QMK after Miryoku's generated layers were written, and the old names no longer exist anywhere in `quantum/`. `config.h` is force-included before `keycodes.h`, so the shims don't clash. The RGB block follows drashna's port: it picks the RGB matrix codes, the rgblight codes, or nothing, depending on which feature is on.

```c
#define KC_MS_U MS_UP
#define KC_MS_D MS_DOWN
#define KC_MS_L MS_LEFT
#define KC_MS_R MS_RGHT
#define KC_WH_U MS_WHLU
#define KC_WH_D MS_WHLD
#define KC_WH_L MS_WHLL
#define KC_WH_R MS_WHLR
#define KC_BTN1 MS_BTN1
#define KC_BTN2 MS_BTN2
#define KC_BTN3 MS_BTN3

#if defined(RGB_MATRIX_ENABLE)
#    define RGB_TOG RM_TOGG
#    define RGB_MOD RM_NEXT
#    define RGB_HUI RM_HUEU
#    define RGB_SAI RM_SATU
#    define RGB_VAI RM_VALU
#elif defined(RGBLIGHT_ENABLE)
#    define RGB_TOG UG_TOGG
#    define RGB_MOD UG_NEXT
#    define RGB_HUI UG_HUEU
#    define RGB_SAI UG_SATU
#    define RGB_VAI UG_VALU
#else
#    define RGB_TOG KC_NO
#    define RGB_MOD KC_NO
#    define RGB_HUI KC_NO
#    define RGB_SAI KC_NO
#    define RGB_VAI KC_NO
#endif
```

If the compiler reports more renames, add them here the same way.

## Tap-hold

In `custom_config.h`, replacing every `BILATERAL_COMBINATIONS*` line:

```c
#define CHORDAL_HOLD
#define FLOW_TAP_TERM 160
```

`tap_hold.c` in the keymap folder. It includes `quantum.h` rather than `QMK_KEYBOARD_H` so the test build can compile the same file.

```c
#include "quantum.h"

char chordal_hold_handedness(keypos_t key) {
	return key.row < MATRIX_ROWS / 2 ? 'L' : 'R';
}

bool get_chordal_hold(uint16_t tap_hold_keycode, keyrecord_t *tap_hold_record, uint16_t other_keycode, keyrecord_t *other_record) {
	if (IS_QK_LAYER_TAP(tap_hold_keycode)) {
		return true;
	}
	uint16_t crossover = (QK_MOD_TAP_GET_MODS(tap_hold_keycode) & MOD_LGUI) ? 120 : 80;
	if (TIMER_DIFF_16(other_record->event.time, tap_hold_record->event.time) <= crossover) {
		return false;
	}
	return get_chordal_hold_default(tap_hold_record, other_record);
}

uint16_t get_flow_tap_term(uint16_t keycode, keyrecord_t *record, uint16_t prev_keycode) {
	if (!IS_QK_MOD_TAP(keycode) || (QK_MOD_TAP_GET_MODS(keycode) & 0x0F) == MOD_LSFT) {
		return 0;
	}
	return is_flow_tap_key(keycode) && is_flow_tap_key(prev_keycode) ? FLOW_TAP_TERM : 0;
}
```

On the real keyboard the handedness override is optional: QMK generates a handedness map from the Kyria's `keyboard.json`. Keep it anyway, because the test build has no `keyboard.json`.

`get_chordal_hold()` is also called when the other key is *released* within the term, and over the waiting buffer when the term expires. In those calls `other_record->event.time` is a release time, which is harmless for this check.

How the old settings map (verified against sunaku's `action.c` and current QMK):

| Old setting | New |
|-------------|-----|
| `BILATERAL_COMBINATIONS` | `CHORDAL_HOLD` |
| `TYPING_STREAK_TIMEOUT 160` | `FLOW_TAP_TERM 160` |
| `TYPING_STREAK_MODMASK (~MOD_MASK_SHIFT)` | `get_flow_tap_term()` returns 0 for Shift |
| `ALLOW_CROSSOVER_AFTER 80` | time check in `get_chordal_hold()` (`<=`, same boundary as the old code) |
| `DELAY_MODS_THAT_MATCH MOD_MASK_GUI` + `DELAY_MATCHED_MODS_BY 120` | 120 ms crossover for GUI. The delayed send has no equivalent |
| `ALLOW_SAMESIDED_AFTER 3000` | none. Chordal Hold stops at the tapping term (200 ms) |
| `LIMIT_CHORD_TO_N_KEYS 4` | not needed |
| (implicit) only mod-taps were affected | layer-taps exempt from both |

"No `PERMISSIVE_HOLD`" is the closer match. In the old tree, an opposite-hand nested tap inside the term was a tap, and `PERMISSIVE_HOLD` would make it a hold.

Known differences that remain:
- A same-hand chord held longer than 200 ms becomes a shortcut. The old code waited 3 s.
- Flow Tap only counts letters, Space and `. , ; /` as the previous key, and pauses while any tap-hold key is undecided. The old typing streak counted any key press, including Backspace, Enter, digits, arrows and thumb-key taps. If mods misfire right after those keys, override `is_flow_tap_key()` (keep its hotkey guard).
- GUI used to be sent 120 ms late, so a GUI home-row key held just past the term and released alone never flashed GUI. Now it does from 200 ms; on Windows, that means the Start menu.
- A left mod-tap plus a right mod-tap now combine. The old code turned both into letters.
- A mod-tap settled as a tap stays registered while it's held, so the OS can key-repeat it. The old code sent a single tap.

## Host OS

In `custom_config.h` (the clipboard option is set in the keymap's `rules.mk`, see above):

```c
#define OS_DETECTION_SINGLE_REPORT
```

In the keymap's `config.h`. Don't name it `OS_TOGG`: that is already QMK's one-shot toggle keycode and redefining it breaks the build.

```c
#define U_SYS QK_USER_0
```

Then put `U_SYS` in place of the top-left `XXX` of the first row, in both `LAYOUT_miryoku` variants (see Thumb mapping). That position is also the Bootmagic key (matrix `[0,6]`; `[4,6]` on the right half): holding it while plugging in enters the bootloader. Both uses coexist.

`host_os.h` / `host_os.c`:

```c
#pragma once
#include <stdbool.h>

bool host_is_mac(void);
void host_os_toggle(void);
```

```c
#include "quantum.h"
#include "host_os.h"

static bool is_mac = false;

static void apply_host_os(void) {
	keymap_config.swap_lctl_lgui = is_mac;
	keymap_config.swap_rctl_rgui = is_mac;
}

bool host_is_mac(void) {
	return is_mac;
}

void host_os_toggle(void) {
	is_mac = !is_mac;
	apply_host_os();
}

bool process_detected_host_os_user(os_variant_t os) {
	is_mac = os == OS_MACOS || os == OS_IOS;
	apply_host_os();
	return true;
}
```

`keymap.c` needs the Miryoku header. It's compiled inside `keymap_introspection.c` before Miryoku's own source, so `U_RDO` is undeclared without it.

```c
#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"
#include "host_os.h"

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
	switch (keycode) {
		case U_SYS:
			if (record->event.pressed) {
				host_os_toggle();
			}
			return false;
		case U_RDO:
			if (host_is_mac()) {
				if (record->event.pressed) {
					tap_code16(LSG(KC_Z));
				}
				return false;
			}
			break;
	}
	return true;
}
```

Verified in QMK source:
- The Ctrl/GUI swap reaches Miryoku's modded keycodes like `C(KC_C)`, the mod-taps, and plain `KC_LCTL`/`KC_LGUI`.
- `tap_code16(LSG(KC_Z))` is not swapped again.
- `QK_USER_0` exists. `LSG()` replaces the deprecated `SGUI()`.
- Miryoku defines no `process_record_user` or `process_detected_host_os_user` of its own.

The OLED master side gets a `Win` / `Mac` field from `host_is_mac()`.

## Scenario tests

QMK's unit test framework (GoogleTest, run with `make test:<dirname>` in WSL).

Setup:
- Tests live under `qmk_firmware/tests/<dir>/` with a `test.mk`, so copy them in from the userspace. Add `SRC += tap_hold.c` and copy or VPATH the file in.
- Old tree: initialise the `lib/googletest` submodule. `test.mk` needs `DEFERRED_EXEC_ENABLE = yes`, and the test `config.h` needs the old `BILATERAL_COMBINATIONS*` defines.
- Handedness: the test build isn't split and its matrix is 4 rows × 10 columns, so the old code splits by **column** (`col < 5` is left) while `chordal_hold_handedness()` splits by row. Place left-hand keys at rows 0–1 **and** cols 0–4, right-hand keys at rows 2–3 **and** cols 5–9, so both trees agree on which hand a key is.

A scenario that passes in one tree and fails in the other is a behaviour difference.

Colemak-DH home row: A=GUI, R=Alt, S=Ctrl, T=Shift | N=Shift, E=Ctrl, I=Alt, O=GUI.

1. Same-hand fast roll: S, T 30 ms apart → `st`
2. Same-hand slow: S held 250 ms, then T → Ctrl+T new, `st` old (known difference)
3. Opposite-hand roll: T, then U 40 ms later, T released first → `tu`
4. Opposite-hand early press: T, then U at 40 ms, T held past 200 ms → `t` then `u`. T settles as a tap but stays down until released, so expect T held in the reports until T goes up.
5. Opposite-hand shortcut: T, then U at 120 ms, T held past 200 ms → `U`
6. Typing streak: `a`, then S within 100 ms and held 300 ms → `s`, no Ctrl
7. Shift mid-word: `e`, then T within 100 ms and held past 200 ms, then U → `U`
8. GUI head start: O, then W at 100 ms → `ow`; O held past 200 ms with W at 150 ms → GUI+W
9. Thumb layer: Space (Nav) held past 200 ms, a right-hand Nav key pressed within 50 ms of Space → that Nav key, Space not typed
10. Host OS: call `host_os_toggle()` directly (OS detection runs on USB setup packets and can't be exercised here); then `U_CPY` → Cmd+C, `U_RDO` → Cmd+Shift+Z

## Fix while in there

- `oled.c` `render_mods()` and the lock indicators use blanks shorter than their labels (`CTRL` vs three spaces, `NUMLCK ` vs nine), which shifts the text and leaves stray characters.
- `oled.c` keeps its own layer enum. Use Miryoku's `U_*` enum.
- `oled_init_user` in `oled.c` is never called: `kyria.c`'s `oled_init_kb` returns rotation 180 itself. It's harmless; delete it or leave it.
- `RGBLIGHT_LIMIT_VAL 128` in the keymap's `config.h` does nothing, because `RGBLIGHT_ENABLE = no`. The board already caps RGB matrix brightness at 128 (`rgb_matrix.max_brightness` in `keyboard.json`). Delete it. The "rgb limiter" commit added only that define, so nothing is lost.
