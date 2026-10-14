"""STAGE1.txt -> bjorn_art.c / bjorn_art.h

    python3 -I gen_bjorn.py STAGE1.txt <output-dir>

Frames are emitted as block-aligned 64-byte patches, never whole columns, so the
renderer can stay inside the per-step block budget. Patch block index is the
driver's own: block = buffer_index / 64, and the left column's page p starts at
buffer index p*128, so its block is p*2.
"""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from oled_art import pixels_to_bytes

ON = '#'

def parse(path):
    blocks, name, meta, rows = {}, None, None, []
    for line in open(path):
        line = line.rstrip('\n')
        if line.startswith('@'):
            if name:
                blocks[name] = (meta, rows)
            f = line.split()
            name = f[1]
            meta = {'w': int(f[2]), 'h': int(f[3]), 'ox': 0, 'oy': 0}
            if 'origin' in f:
                i = f.index('origin'); meta['ox'], meta['oy'] = int(f[i+1]), int(f[i+2])
            rows = []
        elif line:
            rows.append([1 if c == ON else 0 for c in line])
    blocks[name] = (meta, rows)
    for n, (m, r) in blocks.items():
        assert len(r) == m['h'], f"{n}: {len(r)} rows, header says {m['h']}"
        assert all(len(x) == m['w'] for x in r), f"{n}: ragged"
        assert m['h'] % 8 == 0, f"{n}: height not a multiple of 8"
    return blocks

src = parse(sys.argv[1])

def stamp(base, patch_name):
    """Overlay a patch grid onto a copy of a 64x64 pose."""
    m, rows = src[patch_name]
    out = [r[:] for r in base]
    for y, row in enumerate(rows):
        for x, v in enumerate(row):
            out[m['oy'] + y][m['ox'] + x] = v
    return out

tiers = [src[f'bjorn_tier{i}'][1] for i in range(4)]
base  = tiers[1]                                   # tier 1 is the resting column
base_bytes = pixels_to_bytes(64, 64, base)         # 8 pages x 64 B, page-major

def page_bytes(grid):
    return pixels_to_bytes(64, 64, grid)

def patches_vs(grid, ref_bytes):
    """Differing pages only, as (block, 64 bytes). Left column: block = page*2."""
    b = page_bytes(grid)
    out = []
    for page in range(8):
        chunk = b[page*64:(page+1)*64]
        if chunk != ref_bytes[page*64:(page+1)*64]:
            out.append((page * 2, chunk))
    return out

frames = []   # (enum_name, comment, [(block, bytes)])
for i in (0, 2, 3):
    frames.append((f'BJORN_F_POSE_T{i}', f'WPM tier {i}, against the tier-1 base',
                   patches_vs(tiers[i], base_bytes)))
frames.insert(1, ('BJORN_F_POSE_T1', 'WPM tier 1 is the base itself', []))

for i in range(4):
    g = stamp(tiers[i], f'bjorn_blink_tier{i}')
    frames.append((f'BJORN_F_BLINK_T{i}', f'eyes shut on tier {i}, against that tier',
                   patches_vs(g, page_bytes(tiers[i]))))

# A patch is a whole 64-byte block, so a blink block carries whatever else lives on
# those pages -- the left beard edge and the right horn tip. Stamped onto the LIFTED
# frame, it therefore carries their lifted positions; stamped onto the base it would
# drag the horn tip back down a pixel every time he blinks mid-breath.
#
# bjorn_breath_swell is unlifted in rows 24-39, so the plain tier-1 blink still fits
# it. bjorn_breath and bjorn_breath_settle are identical in rows 24-39 (the swell
# starts at row 41), so ONE lifted variant serves both: diffed against its own frame
# it yields the same bytes either way.
g = stamp(src['bjorn_breath'][1], 'bjorn_blink_lift')
frames.append(('BJORN_F_BLINK_LIFT', 'eyes shut while lifted, against either lifted frame',
               patches_vs(g, page_bytes(src['bjorn_breath'][1]))))

# The slow half of a blink, and the lid on the way down into a doze. Same window,
# size and origin as the shut patches, so they compose identically.
for i in range(4):
    g = stamp(tiers[i], f'bjorn_half_tier{i}')
    frames.append((f'BJORN_F_HALF_T{i}', f'lid halfway on tier {i}, against that tier',
                   patches_vs(g, page_bytes(tiers[i]))))
g = stamp(src['bjorn_breath'][1], 'bjorn_half_lift')
frames.append(('BJORN_F_HALF_LIFT', 'lid halfway while lifted, against either lifted frame',
               patches_vs(g, page_bytes(src['bjorn_breath'][1]))))

# The axe sits in his beard at rows 40-55, disjoint from the blink window at 32-39,
# so the two overlays compose without needing combined variants.
for i in range(4):
    frames.append((f'BJORN_F_AXE_T{i}', f'axe in hand on tier {i}, against that tier',
                   patches_vs(src[f'bjorn_axe_t{i}'][1], page_bytes(tiers[i]))))

# The misfire raise. Whole frames against the base: the renderer paints the base and
# then the frame, and the driver charges only for the bytes that actually changed, so
# the on-wire cost is the step delta rather than the patch count.
for i in (1, 2, 3):
    frames.append((f'BJORN_F_MISFIRE_R{i}', f'axe raised, step {i}, against the base',
                   patches_vs(src[f'bjorn_misfire_r{i}'][1], base_bytes)))

# The crown sits on the dome at pages 0-1; the patch carries the horns behind it, which
# differ per posture, so it needs a variant each.
for i in range(4):
    g = stamp(tiers[i], f'bjorn_crown_t{i}')
    frames.append((f'BJORN_F_CROWN_T{i}', f'Caps Word crown on tier {i}',
                   patches_vs(g, page_bytes(tiers[i]))))

frames.append(('BJORN_F_BREATH', 'peak inhale: risen 1 px and swelled, against the resting pose',
               patches_vs(src['bjorn_breath'][1], base_bytes)))
frames.append(('BJORN_F_BREATH_SWELL', 'the body fills before he rises, against the resting pose',
               patches_vs(src['bjorn_breath_swell'][1], base_bytes)))
frames.append(('BJORN_F_BREATH_SETTLE', 'the body empties, head still up, against the resting pose',
               patches_vs(src['bjorn_breath_settle'][1], base_bytes)))
frames.append(('BJORN_F_DOZE', 'asleep, against the base',
               patches_vs(src['bjorn_doze'][1], base_bytes)))
# the sleeping breath alternates with the doze pose, so it is diffed against that
frames.append(('BJORN_F_DOZE_BREATH', 'the sleeping breath, against the doze pose',
               patches_vs(src['bjorn_doze_breath'][1], page_bytes(src['bjorn_doze'][1]))))

# floor for the empty right column: page 7, right column -> block 15
fm, fr = src['floor_right']
floor = pixels_to_bytes(64, 8, fr)
frames.append(('BJORN_F_FLOOR_RIGHT', 'the horizon carried across the empty right column',
               [(15, floor)]))

hdr = '''// Bjorn, generated from his own sprites. Do not edit by hand.
//
// Artwork: Copyright (c) infinition, from the Bjorn project
// <https://github.com/infinition/Bjorn>, MIT licensed. Box-filtered from 78x78
// to 64x64 and dithered; see docs/layout.md and VENDORED.md.

#pragma once

#include <stdint.h>
#include "progmem.h"

#define BJORN_PATCH_BYTES 64      // one dirty block: 64 columns of one page

// `block` is the driver's own dirty-block index, buffer_index / 64. Bjorn owns the
// left column, so his blocks are the even ones; block 15 is the right column's page 7.
typedef struct {
    uint8_t     block;
    const char *data;
} bjorn_patch_t;

typedef struct {
    const bjorn_patch_t *patches;
    uint8_t              count;
} bjorn_frame_t;

enum bjorn_frame_id {
%s
    BJORN_FRAME_COUNT
};

extern const char          PROGMEM bjorn_base[512];
extern const bjorn_frame_t         bjorn_frames[BJORN_FRAME_COUNT];
''' % '\n'.join(f'    {n},' for n, _, _ in frames)

def carr(name, data):
    out = [f'static const char PROGMEM {name}[{len(data)}] = {{']
    for i in range(0, len(data), 16):
        out.append('    ' + ' '.join(f'0x{b:02x},' for b in data[i:i+16]))
    out.append('};')
    return '\n'.join(out)

body = ['''// Bjorn, generated from his own sprites. Do not edit by hand.
//
// Artwork: Copyright (c) infinition, from the Bjorn project
// <https://github.com/infinition/Bjorn>, MIT licensed. Box-filtered from 78x78
// to 64x64 and dithered; see docs/layout.md and VENDORED.md.

#include "bjorn_art.h"
''', carr('bjorn_base_data', base_bytes).replace('static const', 'const'), '']
body[1] = body[1].replace('bjorn_base_data', 'bjorn_base')

for name, comment, pats in frames:
    body.append(f'// {comment}')
    for k, (blk, data) in enumerate(pats):
        body.append(carr(f'{name.lower()}_{k}', data))
    if pats:
        body.append(f'static const bjorn_patch_t {name.lower()}_p[] = {{')
        for k, (blk, _) in enumerate(pats):
            body.append(f'    {{ {blk:2}, {name.lower()}_{k} }},')
        body.append('};')
    body.append('')

body.append('const bjorn_frame_t bjorn_frames[BJORN_FRAME_COUNT] = {')
for name, _, pats in frames:
    if pats:
        body.append(f'    [{name}] = {{ {name.lower()}_p, {len(pats)} }},')
    else:
        body.append(f'    [{name}] = {{ NULL, 0 }},')
body.append('};')

out = pathlib.Path(sys.argv[2])
(out / 'bjorn_art.h').write_text(hdr)
(out / 'bjorn_art.c').write_text('\n'.join(body) + '\n')

print(f"{len(frames)} frames")
for name, _, pats in frames:
    print(f"  {name:22} {len(pats)} patch(es)  blocks {[b for b,_ in pats]}")
tot = 512 + sum(len(d) for _,_,ps in frames for _,d in ps)
print(f"flash: {tot} B")
