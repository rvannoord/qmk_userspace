"""Draws the OLED board inside the Kyria rev3 OLED cut-out (right half, top view, mm).

    python3 kyria_fit.py OLED_COVER.dxf ../kyria-fit.svg

OLED_COVER.dxf is "Kyria rev3 OLED Cover - No Kerf.dxf" from https://github.com/splitkb/kyria/tree/master/rev3
(CC BY-NC-SA 4.0, so it isn't committed here; download it next to this script, *.dxf is git-ignored).

The cover outline equals the top plate's cut-out. It is moved into top-plate coordinates by putting its
157.106 mm top-edge arc on the plate's arc centre (10.926, -96.891).

The socket, Liatris and the board position below are MEASURED VALUES, read from splitkb's top-down rev3 PCB
photo (about +-1 mm). Replace them with real measurements, then re-run.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dxf_dump import entities, geom  # noqa: E402

# --- measured on the photo (plate coordinates, mm, y up) --------------------------------------------
PIN_Y = 9.8                        # OLED socket pin row
PIN_CX = -49.6                     # centre of the 4 socket pins; GND (square pad) is the rightmost
LIATRIS = (-50.9, -32.8, 13.2, 42.1)  # x0, x1, y0, y1, approximate

# --- the board (see DESIGN.md) -------------------------------------------------------------------------
PIN_FROM_EDGE = 1.3                # pin row to the board's header edge
BODY = (25.9, 30.1)                # width, length
TAB = (12.0, 2.3)                  # ribbon tab width, length incl. the wrapped ribbon
STOCK = (26.1, 26.2)               # splitkb 128x64 module

COVER_TO_PLATE = (10.926 + 45.386, -96.891 + 89.444)  # translation, mm


def sample(g, n=64):
    if g[0] == "L":
        return [(g[1], g[2]), (g[3], g[4])]
    if g[0] == "C":
        return [(g[1] + g[3] * math.cos(t * 2 * math.pi / n), g[2] + g[3] * math.sin(t * 2 * math.pi / n)) for t in range(n + 1)]
    if g[0] == "A":
        cx, cy, r, a0, a1 = g[1:]
        if a1 < a0:
            a1 += 360
        return [(cx + r * math.cos(math.radians(a0 + (a1 - a0) * t / n)),
                 cy + r * math.sin(math.radians(a0 + (a1 - a0) * t / n))) for t in range(n + 1)]
    return list(g[1])


def top_edge_y(x):
    """Keyboard top edge (plate arc r = 157.106) at x."""
    return -96.891 + math.sqrt(157.106 ** 2 - (x - 10.926) ** 2)


def main(cover_dxf, out_svg):
    dx, dy = COVER_TO_PLATE
    cover = [g for g in (geom(e) for e in entities(cover_dxf)) if g]
    outline = [[(x + dx, y + dy) for x, y in sample(g)] for g in cover if g[0] != "C"]
    holes = [(g[1] + dx, g[2] + dy) for g in cover if g[0] == "C"]

    pins = [(PIN_CX + d, PIN_Y) for d in (3.81, 1.27, -1.27, -3.81)]  # GND, VCC, SCL, SDA
    lo, hi = PIN_CX - BODY[0] / 2, PIN_CX + BODY[0] / 2
    bottom = PIN_Y - PIN_FROM_EDGE
    body_top = bottom + BODY[1]
    tab = (PIN_CX - TAB[0] / 2, PIN_CX + TAB[0] / 2, body_top, body_top + TAB[1])
    stock = (PIN_CX - STOCK[0] / 2, PIN_CX + STOCK[0] / 2, bottom, bottom + STOCK[1])
    corner_spare = top_edge_y(lo) - body_top   # ignores the 1 mm corner fillet, which costs ~0.03 here
    tab_spare = top_edge_y(tab[0]) - tab[3]

    x0, x1, y0, y1 = -70.0, -20.0, -16.0, 60.0
    s, left, top = 9.0, 30.0, 40.0
    X = lambda x: left + (x - x0) * s
    Y = lambda y: top + (y1 - y) * s
    w, h = left + (x1 - x0) * s + 260, top + (y1 - y0) * s + 40

    def rect(r, **kw):
        a = " ".join(f'{k.replace("_", "-")}="{v}"' for k, v in kw.items())
        return f'<rect x="{X(r[0]):.1f}" y="{Y(r[3]):.1f}" width="{(r[1]-r[0])*s:.1f}" height="{(r[3]-r[2])*s:.1f}" {a}/>'

    o = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w:.0f} {h:.0f}" width="{w:.0f}" height="{h:.0f}" font-family="Segoe UI, Arial, sans-serif" font-size="11">',
         f'<rect width="{w:.0f}" height="{h:.0f}" fill="#ffffff"/>',
         '<text x="20" y="24" font-size="15" font-weight="600" fill="#222">Fit in the Kyria rev3 OLED cut-out (right half, top view, mm)</text>']
    for gx in range(-70, -19, 5):
        o.append(f'<line x1="{X(gx):.1f}" y1="{Y(y1):.1f}" x2="{X(gx):.1f}" y2="{Y(y0):.1f}" stroke="#f0f0f0"/>')
    for gy in range(-15, 61, 5):
        o.append(f'<line x1="{X(x0):.1f}" y1="{Y(gy):.1f}" x2="{X(x1):.1f}" y2="{Y(gy):.1f}" stroke="#f0f0f0"/>')
    o.append(rect(LIATRIS, fill="none", stroke="#999", stroke_dasharray="3 3"))
    o.append(f'<text x="{X(LIATRIS[0])+4:.1f}" y="{Y(LIATRIS[3])-4:.1f}" fill="#888">Liatris (approx.)</text>')
    for poly in outline:
        o.append('<polyline fill="none" stroke="#000" stroke-width="2" points="%s"/>' % " ".join(f"{X(x):.1f},{Y(y):.1f}" for x, y in poly))
    o.append(rect((lo, hi, bottom, body_top), fill="#2e5e8c", fill_opacity="0.55", stroke="#1d3d5c", stroke_width="1.5"))
    o.append(rect(tab, fill="#d08a2c", fill_opacity="0.8", stroke="#9a6418"))
    o.append(rect(stock, fill="none", stroke="#e07020", stroke_width="2", stroke_dasharray="7 4"))
    for hx, hy in holes:
        o.append(f'<circle cx="{X(hx):.1f}" cy="{Y(hy):.1f}" r="{2.0*s:.1f}" fill="#bbb" stroke="#555"/>')
        o.append(f'<circle cx="{X(hx):.1f}" cy="{Y(hy):.1f}" r="{1.2*s:.1f}" fill="#fff" stroke="#555"/>')
    for i, (px, py) in enumerate(pins):
        if i == 0:
            o.append(f'<rect x="{X(px)-6:.1f}" y="{Y(py)-6:.1f}" width="12" height="12" fill="#c9a227"/>')
        else:
            o.append(f'<circle cx="{X(px):.1f}" cy="{Y(py):.1f}" r="6" fill="#c9a227"/>')
    o.append(f'<line x1="{X(lo):.1f}" y1="{Y(body_top):.1f}" x2="{X(lo):.1f}" y2="{Y(top_edge_y(lo)):.1f}" stroke="#c00" stroke-width="2"/>')
    o.append(f'<text x="{X(lo)+6:.1f}" y="{Y(body_top + corner_spare/2 - 0.5):.1f}" fill="#c00" font-weight="600">~{corner_spare:.1f}</text>')
    o.append(f'<line x1="{X(tab[0]):.1f}" y1="{Y(tab[3]):.1f}" x2="{X(tab[0]):.1f}" y2="{Y(top_edge_y(tab[0])):.1f}" stroke="#c00" stroke-width="2"/>')
    o.append(f'<text x="{X(tab[0])+6:.1f}" y="{Y(tab[3] + tab_spare/2 - 0.5):.1f}" fill="#c00" font-weight="600">~{tab_spare:.1f}</text>')
    o.append(f'<text x="{X(-45):.1f}" y="{Y(53):.1f}" fill="#222" transform="rotate(-14 {X(-45):.1f} {Y(53):.1f})">keyboard top edge</text>')
    o.append(f'<text x="{X(-45):.1f}" y="{Y(-8):.1f}" fill="#222">thumb keys</text>')
    o.append(f'<text x="{X(-66.5):.1f}" y="{Y(20):.1f}" fill="#222" transform="rotate(-90 {X(-66.5):.1f} {Y(20):.1f})">inner edge</text>')
    o.append(f'<text x="{X(-27):.1f}" y="{Y(30):.1f}" fill="#222" transform="rotate(-90 {X(-27):.1f} {Y(30):.1f})">switch columns</text>')
    for hx, hy in holes:
        o.append(f'<text x="{X(hx):.1f}" y="{Y(hy - 4.5):.1f}" text-anchor="middle" fill="#555">cover spacer</text>')
    o.append(f'<text x="{X(PIN_CX):.1f}" y="{Y(PIN_Y)+28:.1f}" text-anchor="middle" fill="#222">OLED socket (GND at the square pad)</text>')
    lx, ly = left + (x1 - x0) * s + 20, top + 20
    legend = [("#000", "solid", "top-plate cut-out = OLED cover outline"),
              ("#2e5e8c", "fill", f"new board body, {BODY[0]} x {BODY[1]}"),
              ("#d08a2c", "fill", f"ribbon tab, {TAB[0]} x {TAB[1]}"),
              ("#e07020", "dash", "stock splitkb 128x64 module"),
              ("#999", "dash", "Liatris, approximate")]
    for i, (c, st, txt) in enumerate(legend):
        yy = ly + i * 22
        if st == "fill":
            o.append(f'<rect x="{lx}" y="{yy-9}" width="18" height="12" fill="{c}" fill-opacity="0.7"/>')
        else:
            dash = ' stroke-dasharray="6 3"' if st == "dash" else ""
            o.append(f'<line x1="{lx}" y1="{yy-3}" x2="{lx+18}" y2="{yy-3}" stroke="{c}" stroke-width="2"{dash}/>')
        o.append(f'<text x="{lx+26}" y="{yy}" fill="#222">{txt}</text>')
    notes = ["Red: spare length to the cut-out (mm).",
             "Positions of the socket, spacers and",
             "Liatris are read from a product photo:",
             "about +-1 mm. Measure your own board",
             "before ordering (see DESIGN.md).",
             "",
             "Left half: mirror image."]
    for i, t in enumerate(notes):
        o.append(f'<text x="{lx}" y="{ly + 140 + i*16}" fill="#555">{t}</text>')
    o.append("</svg>")
    with open(out_svg, "w", newline="\n") as f:
        f.write("\n".join(o) + "\n")
    print(f"cover holes: {[(round(a, 2), round(b, 2)) for a, b in holes]}")
    print(f"board x {lo:.2f}..{hi:.2f}, y {bottom:.2f}..{body_top:.2f}, tab to {tab[3]:.2f}")
    print(f"spare to the top edge: {corner_spare:.2f} at the inner corner, {tab_spare:.2f} at the tab")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
