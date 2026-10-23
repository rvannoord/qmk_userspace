---
paths:
  - "keyboards/**/oled*"
  - "keyboards/**/*logo*"
  - "keyboards/**/bjorn_*"
  - "keyboards/**/panel_*"
  - "keyboards/**/split_*"
  - "keyboards/**/art/**"
  - "tests/unit/**"
  - "tools/**"
---

# OLED and split work

The facts — page format, the dirty-block rule, which half shows what, the split transport,
the flash-together rule — live in the keymap's `readme.md`, which is the human-facing copy
and the one to keep current. Read it before touching any file this rule covers; the design
rules are in `docs/layout.md`, OLED section. This file is only about how to work here.

## The rule that matters most

**Never clear-then-redraw into the driver buffer.** Any byte that changes dirties its block,
a dirty block re-arms the display timeout, and that is what kept both panels awake for ever
(Bug A, `docs/oled-v2.md` §1.2). Compose in `u_canvas` and commit once; an unchanged frame
must dirty nothing, and the unit tests assert it.

## Where code goes

- Edges are latched in the per-loop tasks (`oled_master_task`, `oled_slave_task`, both called
  from `housekeeping_task_user` in `keymap.c`). `oled_render_*` only draw. A transient flag has
  to stay set for at least 100 ms or the 20 Hz render can miss it.
- New source files go into `SRC` in the keymap's `rules.mk`, never into `keymap.c`.
- Everything under `art/` is generated or vendored and is never edited by hand. The generators
  live under `tools/`; hand corrections to the layer pictures go in
  `tools/panel/icon_overrides.txt`, never into the `.c`.
- On the slave, every layer keeps x=127 and y=0 dark (the Caps Lock frame) and y=63 is the
  floor. Anything that starts in the idle show asks `u_director_claim()` first: one starter a
  step, Bjorn before the goat. The goat's art is the grids in `tools/sprites/goat.txt`.
- `oled.h` is the module's interface. A new slave animation adds an enum member there, its
  entry points, and a case in `oled_slave_select.c`.
- Keep the keymap `readme.md` file table and `docs/layout.md`'s Files table in step with any
  file that moves, appears or goes.

## After a change

Run `tests/run_unit.sh` after touching any OLED or split file; `--dump` prints the frames as
text, `--raw` emits the buffers for a renderer. Build with `qmk compile`, flash both halves
with the same `.uf2`, and record it in `docs/flash-checklist.md`.
