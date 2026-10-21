"""drashna's Quantum Painter art read back to 1-bit pixels: QGF images and QFF fonts.

QMK ships converters into these formats and nothing that reads them back.
`tools/panel/qgf.py` reads QGF; this adds QFF on top of the same block helpers.
Everything is cut to 1 bit at half brightness, which is all an SSD1306 can show.
"""
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent / 'panel'))
import qgf  # noqa: E402

CUT = 128


def image(blob):
    """-> (w, h, rows of 0/1)."""
    w, h, rows = qgf.read_bytes(blob)
    return w, h, [[1 if v >= CUT else 0 for v in row] for row in rows]


def _u24(d, o):
    return d[o] | d[o + 1] << 8 | d[o + 2] << 16


def font(blob):
    """A QFF font -> (line height, {code point: (w, rows of 0/1)}).

    Each glyph is its own packed run of pixels (RLE'd on its own when the font is),
    starting at a byte offset into the data block: qmk's painter_qff.py writes them
    one by one, so a glyph ends where its pixel count says, not at the next offset.
    """
    d = blob
    t, length, o = qgf._header(d, 0)
    if t != 0x00 or length != 20 or _u24(d, o) != 0x464651:
        raise ValueError('qff: no font descriptor')
    line_h, has_ascii = d[o + 12], d[o + 13]
    n_uni = d[o + 14] | d[o + 15] << 8
    fmt, compression = d[o + 16], d[o + 18]
    o += 20
    table = {}
    if has_ascii:
        t, length, o = qgf._header(d, o)
        if t != 0x01:
            raise ValueError('qff: no ASCII glyph table')
        for i in range(95):
            v = _u24(d, o + 3 * i)
            table[0x20 + i] = (v & 0x3f, v >> 6)
        o += length
    if n_uni:
        t, length, o = qgf._header(d, o)
        if t != 0x02:
            raise ValueError('qff: no Unicode glyph table')
        for i in range(n_uni):
            v = _u24(d, o + 6 * i + 3)
            table[_u24(d, o + 6 * i)] = (v & 0x3f, v >> 6)
        o += length
    palette = None
    t, length, o2 = qgf._header(d, o)
    if t == 0x03:
        palette = [tuple(d[o2 + i:o2 + i + 3]) for i in range(0, length, 3)]
        o = o2 + length
        t, length, o2 = qgf._header(d, o)
    if t != 0x04:
        raise ValueError('qff: no data block')
    data = d[o2:o2 + length]
    bpp = qgf.BPP[fmt]
    per, mask = 8 // bpp, (1 << bpp) - 1
    glyphs = {}
    for cp, (w, off) in sorted(table.items()):
        n = w * line_h
        need = (n + per - 1) // per
        if compression == 0x01:
            raw, i = bytearray(), off
            while len(raw) < need:
                marker = data[i]
                i += 1
                if marker >= 128:
                    k = marker - 127         # qgf.py's docstring: the decoder, not the doc
                    raw += data[i:i + k]
                    i += k
                else:
                    raw += bytes([data[i]]) * marker
                    i += 1
        else:
            raw = data[off:off + need]
        vals = [(b >> (k * bpp)) & mask for b in raw for k in range(per)][:n]
        grey = [palette[v][2] for v in vals] if palette else [v * 255 // mask for v in vals]
        glyphs[cp] = (w, [[1 if grey[y * w + x] >= CUT else 0 for x in range(w)] for y in range(line_h)])
    return line_h, glyphs
