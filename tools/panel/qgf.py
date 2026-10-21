"""Read a QMK Graphics Format (QGF) image back out of its generated C file.

QMK ships a converter *into* QGF and nothing that reads one back, so this exists to
lift splitkb's Halcyon icons out of `antontanderup/qmk_userspace` as pixels. See
`VENDORED.md` for the source and the pinned commit.

Only what the files read so far use is implemented: QGF v1, frame 0, no delta frames.
A palette frame is resolved through the V of its HSV888 entries, which is all a
1-bit panel can use anyway. RGB565 frames (drashna's TFT logos) come back as their
luminance. `blobs()` splits a file that holds several images, as drashna's
`graphics.qgf.c` does.

One trap, and the reason this is not a twenty-line function. `docs/quantum_painter_qgf.md`
says a non-repeating RLE run has `length = marker - 128`. The decoder that actually
runs on the keyboard, `qp_drawimage_byte_rle_decoder` in `quantum/painter/qp_draw_codec.c`,
uses `c - 127`. The doc is off by one. Following the doc drops a byte per literal run
and every image after the first run decodes as noise, which looks like a corrupt file
rather than an off-by-one. Follow the decoder, not the doc.
"""

import re
import struct

MAGIC = 0x464751          # "QGF"
BPP = {0: 1, 1: 2, 2: 4, 3: 8, 4: 1, 5: 2, 6: 4, 7: 8}
RGB565 = 0x08


def _blob(text):
    """The one hex array in a .qgf.c file, as bytes."""
    body = text.split('= {', 1)[1].rsplit('};', 1)[0]
    return bytes(int(v, 16) for v in re.findall(r'0x[0-9a-fA-F]{2}', body))


def blobs(text):
    """Every hex array in a generated file, by name, in file order."""
    return {m.group(1): bytes(int(v, 16) for v in re.findall(r'0x[0-9a-fA-F]{2}', m.group(2)))
            for m in re.finditer(r'\b(\w+)\s*\[\s*\d*\s*\]\s*=\s*\{(.*?)\};', text, re.S)}


def _unrle(d):
    out, i = bytearray(), 0
    while i < len(d):
        marker = d[i]
        i += 1
        if marker >= 128:
            n = marker - 127          # not -128; see the module docstring
            out += d[i:i + n]
            i += n
        else:
            out += bytes([d[i]]) * marker
            i += 1
    return bytes(out)


def _header(d, o):
    type_id, neg = d[o], d[o + 1]
    if neg != (~type_id) & 0xFF:
        raise ValueError(f'qgf: bad negated type id at offset {o}')
    length = d[o + 2] | d[o + 3] << 8 | d[o + 4] << 16
    return type_id, length, o + 5


def read(path):
    """-> (width, height, rows) where a row is a list of 0-255 greys, 255 = the glyph."""
    return read_bytes(_blob(open(path).read()))


def read_bytes(d):
    """read(), for an image already out of its C array."""

    type_id, length, o = _header(d, 0)
    if type_id != 0x00 or length != 18:
        raise ValueError('qgf: no graphics descriptor')
    if (d[o] | d[o + 1] << 8 | d[o + 2] << 16) != MAGIC:
        raise ValueError('qgf: bad magic')
    if d[o + 3] != 0x01:
        raise ValueError(f'qgf: version {d[o + 3]}, only v1 is implemented')
    w, h, _frames = struct.unpack_from('<HHH', d, o + 12)
    o += 18

    type_id, length, o = _header(d, o)
    if type_id != 0x01:
        raise ValueError('qgf: no frame offset block')
    offsets = struct.unpack_from('<%dI' % (length // 4), d, o)

    o = offsets[0]
    type_id, length, o = _header(d, o)
    if type_id != 0x02:
        raise ValueError('qgf: no frame descriptor')
    fmt, flags, compression = d[o], d[o + 1], d[o + 2]
    o += 6
    if flags & 0x02:
        raise ValueError('qgf: delta frames are not implemented')

    if fmt == RGB565:
        type_id, length, o = _header(d, o)
        if type_id != 0x05:
            raise ValueError('qgf: no frame data block')
        data = d[o:o + length]
        if compression == 0x01:
            data = _unrle(data)
        if len(data) < w * h * 2:
            raise ValueError(f'qgf: {len(data)} bytes decoded, expected {w * h * 2}')
        grey = []
        for i in range(w * h):
            v = data[2 * i] << 8 | data[2 * i + 1]        # big-endian, as qmk's rgb_to565 packs it
            r, g, b = (v >> 11) << 3, ((v >> 5) & 0x3f) << 2, (v & 0x1f) << 3
            grey.append((r * 299 + g * 587 + b * 114) // 1000)
        return w, h, [grey[y * w:(y + 1) * w] for y in range(h)]

    palette = None
    if fmt >= 0x04:
        type_id, length, o = _header(d, o)
        if type_id != 0x03:
            raise ValueError('qgf: palette frame without a palette block')
        palette = [tuple(d[o + i:o + i + 3]) for i in range(0, length, 3)]
        o += length

    type_id, length, o = _header(d, o)
    if type_id != 0x05:
        raise ValueError('qgf: no frame data block')
    data = d[o:o + length]
    if compression == 0x01:
        data = _unrle(data)

    bpp = BPP[fmt]
    if bpp == 8:
        values = list(data[:w * h])
    else:
        per, mask = 8 // bpp, (1 << bpp) - 1
        values = [(byte >> (k * bpp)) & mask for byte in data for k in range(per)][:w * h]
    if len(values) != w * h:
        raise ValueError(f'qgf: {len(values)} pixels decoded, expected {w * h}')

    if palette is not None:
        grey = [palette[v][2] for v in values]       # V of HSV888
    else:
        top = (1 << bpp) - 1
        grey = [v * 255 // top for v in values]
    return w, h, [grey[y * w:(y + 1) * w] for y in range(h)]
