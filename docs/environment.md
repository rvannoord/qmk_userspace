# Environment

The two machines this is built on, as they were set up when the repo moved from the old
`miryoku_qmk` fork to this external userspace on 2026-10-09 (the fork is kept at tag
`archive/miryoku-with-latest-qmk` in `rvannoord/miryoku_qmk`; what was taken from it is in
`VENDORED.md`). A known-good `.uf2` lives outside the repo in `~/kyria-firmware-backup/` on
each machine. The last step of that migration, archiving `rvannoord/miryoku_qmk` and
`rvannoord/qmk_firmware` on GitHub, was still open when its notes were retired on 2026-10-16.

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
- **Before the first commit**, personal repos need their own identity, without touching the work repos. Proposed: an `includeIf` in the global config that applies to everything under `C:/src/personal/`. Changing the global config is a deliberate step.

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
- 1Password has to be unlocked when committing, and may ask Roel to approve each signature.
- Verify before the first commit: `git -C C:/src/personal/qmk_userspace config user.email` must print `rvannoord@icloud.com`. After it, `git log --show-signature -1` should show a good signature (that needs `gpg.ssh.allowedSignersFile` to verify locally; GitHub verifies it anyway if the key is registered there as a signing key).
