"""C arrays out of drashna's sources, read through the preprocessor.

    arrays = load(path, defines=['OLED_FONT_EIN'])
    arrays['font'].dims    -> [256, 6]
    arrays['font'].values  -> flat list of ints, 0-255

The sources keep their variants behind `#if` blocks (nine font faces in one file, a
128x64 and a 128x128 matrix rain), so they are run through `cc -E -P` first and only
then parsed. `PROGMEM` is defined empty on the command line, not in a header:
`yoshi_animation` puts it after the declarator, where a header's definition is not
in effect yet at the point a stub could provide it.

Nothing from the sources is ever executed. Array dimensions are evaluated by a small
arithmetic walker over Python's `ast`, which accepts numbers and + - * / and nothing
else, and an initialiser is refused outright if it holds anything but integers,
braces and commas. Run the generators with `python3 -I`, and never from inside a
clone.
"""

import ast
import os
import pathlib
import re
import subprocess

CC = os.environ.get('CC', 'cc')
STUB = pathlib.Path(__file__).resolve().parent / 'stub'


class Array:
    def __init__(self, dims, values):
        self.dims, self.values = dims, values

    def __repr__(self):
        return f'Array({self.dims}, {len(self.values)} values)'


def preprocess(path, defines=()):
    cmd = [CC, '-E', '-P', '-x', 'c', '-DPROGMEM=', '-I', str(STUB)]
    cmd += ['-D' + d for d in defines]
    cmd.append(str(path))
    return subprocess.run(cmd, capture_output=True, text=True, check=True).stdout


def _arith(expr):
    """An array dimension: integers and + - * / only."""
    def walk(n):
        if isinstance(n, ast.Expression):
            return walk(n.body)
        if isinstance(n, ast.Constant) and type(n.value) is int:
            return n.value
        if isinstance(n, ast.BinOp) and type(n.op) in (ast.Add, ast.Sub, ast.Mult, ast.Div, ast.FloorDiv):
            a, b = walk(n.left), walk(n.right)
            return {ast.Add: a + b, ast.Sub: a - b, ast.Mult: a * b}.get(type(n.op), a // b if b else 0)
        if isinstance(n, ast.UnaryOp) and isinstance(n.op, ast.USub):
            return -walk(n.operand)
        raise ValueError(f'not a plain dimension: {expr!r}')
    return walk(ast.parse(expr.strip(), mode='eval'))


DECL = re.compile(r'\b([A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)+)=\s*\{')
TOKEN = re.compile(r'0[xX][0-9a-fA-F]+|\d+|[{},]')
ALLOWED = re.compile(r'[\s{},]|0[xX][0-9a-fA-F]+|\d+')


def _close(text, start):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return i
    raise ValueError('unbalanced braces')


def _size(dims):
    n = 1
    for d in dims:
        n *= d
    return n


def _parse(tok, pos, dims):
    """tok[pos] is '{'. Fills one aggregate of shape dims (dims[0] may be None),
    padding short initialisers with zeros the way C does. Returns (values, pos)."""
    assert tok[pos] == '{'
    pos += 1
    sub = dims[1:]
    step = _size(sub)
    out, count = [], 0
    while tok[pos] != '}':
        if tok[pos] == ',':
            pos += 1
            continue
        if not sub:
            out.append(int(tok[pos], 0))
            pos += 1
        elif tok[pos] == '{':
            vals, pos = _parse(tok, pos, sub)
            out += vals
        else:
            # brace elision: the next `step` scalars fill one sub-aggregate
            vals = []
            while len(vals) < step and tok[pos] != '}':
                if tok[pos] != ',':
                    vals.append(int(tok[pos], 0))
                pos += 1
            out += vals + [0] * (step - len(vals))
        count += 1
    if dims[0] is not None:
        if count > dims[0]:
            raise ValueError(f'{count} initialisers for {dims[0]} elements')
        out += [0] * ((dims[0] - count) * step)
    return out, pos + 1


def arrays_in(text):
    found = {}
    for m in DECL.finditer(text):
        name = m.group(1)
        dims = [(_arith(d) if d.strip() else None) for d in re.findall(r'\[([^\]]*)\]', m.group(2))]
        if None in dims[1:]:
            raise ValueError(f'{name}: only the first dimension may be left out')
        start = m.end() - 1
        body = text[start:_close(text, start) + 1]
        junk = ALLOWED.sub('', body)
        if junk:
            raise ValueError(f'{name}: initialiser holds more than integers: {junk[:40]!r}')
        vals, _ = _parse(TOKEN.findall(body), 0, dims)
        if dims[0] is None:
            dims[0] = len(vals) // _size(dims[1:])
        if len(vals) != _size(dims):
            raise ValueError(f'{name}: {len(vals)} values for {dims}')
        if any(v < 0 or v > 0xff for v in vals):
            raise ValueError(f'{name}: a value outside a byte')
        found[name] = Array(dims, vals)
    return found


def load(path, defines=()):
    return arrays_in(preprocess(path, defines))


def header(path):
    """The source's own copyright and licence lines, to carry into what is made from it."""
    lines = []
    for line in pathlib.Path(path).read_text(errors='replace').splitlines()[:8]:
        line = line.strip()
        if line.startswith('// Copyright') or 'License-Identifier' in line:
            lines.append(re.sub(r'\s+', ' ', line))
    return lines


def pinned(clone, sha):
    """Refuse a clone that is not at the pinned commit. Reads .git/HEAD rather than
    running git inside a third-party checkout."""
    head = (pathlib.Path(clone) / '.git' / 'HEAD').read_text().strip()
    if head != sha:
        raise SystemExit(f'{clone} is at {head[:12]}, not the pinned {sha[:12]}: see VENDORED.md')


def frames(values, count, size, keep):
    """Split a flat array into `count` frames of `size`, keeping the first `keep`
    bytes of each: drashna's arrays carry a trailing pad byte ([32+1], [1024+1])."""
    assert len(values) == count * size, (len(values), count, size)
    return [values[i * size:i * size + keep] for i in range(count)]


def c_bytes(vals, indent='    ', per_line=16):
    return ''.join(indent + ', '.join(f'0x{v:02x}' for v in vals[i:i + per_line]) + ',\n'
                   for i in range(0, len(vals), per_line))
