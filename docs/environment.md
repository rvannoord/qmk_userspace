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
- picotool 2.3.1 is in `~/.local/bin` (2026-11-05). Reading the Kyria in BOOTSEL needs Zadig's WinUSB driver on "RP2 Boot (Interface 1)", which isn't installed yet, so a read-back on this PC is still open.
- Podman is installed, but its machine also runs the Monitor Postgres container for work. If a QMK container is needed, never reset or restart the podman machine.

## Environment (Mac, set up 2026-11-19)

A new Mac, set up from scratch on 2026-11-19: Apple Silicon, macOS 27.0. Builds and flashes natively; no WSL, no container. QMK now comes from QMK's own installer, the same way as in the work PC's WSL, so both machines run QMK CLI 1.2.0 with GCC 15.2.0.

- **QMK CLI and toolchain**: QMK's official installer, as docs.qmk.fm gives it, then `qmk setup`:
  `curl -fsSL https://install.qmk.fm | UV_NO_MODIFY_PATH=1 sh`. It brews `zstd clang-format make hidapi libusb dos2unix git`, puts `uv` and `qmk` 1.2.0 in `~/.local/bin` (the dotfiles' `.zshrc` has that on PATH, which is what `UV_NO_MODIFY_PATH=1` relies on: without it, uv's installer appends to `~/.zshrc`, which chezmoi owns), and unpacks QMK's prebuilt toolchains (`qmk_toolchains` gcc 15.2.0: ARM, AVR, RISC-V) and flashing tools (`qmk_flashutils`) into `~/Library/Application Support/qmk`. `qmk doctor` finds everything, the AVR and DFU tools included.
- **Not Homebrew's `qmk/qmk/qmk` any more.** It is stuck at 1.1.8 and broke on a fresh install: its bottle was built against python@3.13 while Homebrew now pulls python@3.15, so `qmk` had no interpreter, and building it from source failed on its pinned `rpds-py`. The tap's open issues (#95, #97, #98, #99 in `qmk/homebrew-qmk`) are about the same. The docs no longer mention it.
- **The installer wipes `~/Library/Application Support/qmk`** on every run, and `qmk.ini` lives there, so set the config again afterwards: `qmk config user.qmk_home=/Users/roelvannoord/qmk_firmware user.overlay_dir=/Users/roelvannoord/qmk_userspace`. Then `qmk compile` works from any directory.
- **`qmk setup`** says "ready to go, but minor problems": `upstream` is not a remote of `~/qmk_firmware`. That check is for forks; here `origin` is `qmk/qmk_firmware` itself.
- **Git**: the global identity is the personal one, `rvannoord <rvannoord@icloud.com>`, with `gpg.format=ssh`, `commit.gpgsign=true` and the signer at `/Applications/1Password.app/Contents/MacOS/op-ssh-sign`. `gpg.ssh.allowedSignersFile` is set now, so `git log --show-signature` verifies here. `origin` for the userspace is SSH (`git@github.com:rvannoord/qmk_userspace.git`), through 1Password's SSH agent.
- **`~/qmk_userspace`**: this repo. `qmk compile` drops a `.uf2` here as well as in the QMK tree; `.gitignore` covers it.
- **`~/qmk_firmware`**: stock `qmk/qmk_firmware`, master at `7a1bbf37c5` (0.34.6), the same commit the work PC builds against, submodules initialised.
- **picotool** 2.3.1 from Homebrew, for the read-backs and `picotool load` in `docs/flash-checklist.md`.
- **`uv`** is here now, so step 7's `old` comparison tests could run with the recipe in `tests/README.md`, but the `old` tree itself only exists in WSL on the work PC.
- **Flashing** can use `qmk flash` here, because macOS mounts the `RPI-RP2` volume. On the work PC the `.uf2` has to be copied in Explorer.
- **No `~/kyria-firmware-backup` here yet.** The first flash from this Mac starts it, with the read-backs of both halves the flash steps call for.
- **Verified 2026-11-19**: `qmk compile -kb splitkb/kyria/rev3 -km rvannoord` builds clean with no warnings, 212,548 bytes of text (the old Mac's GCC 8 build of the same firmware, `3457440`, was 214,652); `tests/run_scenarios.sh new` passes 32/32; `tests/run_unit.sh` is all good.
- **The previous Mac** (macOS 26.6.1, set up 2026-10-09, with Homebrew's QMK 1.1.8 and `arm-none-eabi-gcc@8`) built every firmware flashed up to `3457440`. It also held what isn't on this Mac: `~/miryoku_qmk` (the old clone, with the uncommitted mod-icon work), `~/qmk_firmware_sunaku_2024` (the 2024 tree with sunaku's bilateral branch) and `~/kyria-firmware-backup`.

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
