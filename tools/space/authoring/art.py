"""Parse art/oled_spaceship.h into byte arrays, and page-format helpers."""
import re, pathlib
HDR = pathlib.Path(__file__).resolve().parents[3] / 'keyboards/splitkb/kyria/keymaps/rvannoord/art/oled_spaceship.h'

def arrays():
    src = HDR.read_text()
    out = {}
    for m in re.finditer(r'static const char PROGMEM (\w+)\[\]\s*=\s*\{([^}]*)\}', src):
        out[m.group(1)] = bytes(int(t, 16) for t in re.findall(r'0x[0-9a-fA-F]+', m.group(2)))
    return out

def pages_to_bits(pages, width):
    """pages: list of byte rows (each `width` long) -> rows of bool, 8 per page."""
    rows = []
    for p in pages:
        for b in range(8):
            rows.append([bool((p[x] >> b) & 1) for x in range(width)])
    return rows

def ascii(bits, lit='#', dark='.'):
    return '\n'.join(''.join(lit if v else dark for v in r) for r in bits)

if __name__ == '__main__':
    a = arrays()
    for k, v in a.items(): print(k, len(v))
