"""Minimal DXF reader for the Kyria case files: LINE, ARC, CIRCLE and LWPOLYLINE from the ENTITIES section.

Usage: python3 dxf_dump.py FILE.dxf    prints every entity, in mm, in the file's own coordinates.
"""
import sys


def _pairs(path):
    with open(path, encoding="latin-1") as f:
        lines = [line.rstrip("\r\n") for line in f]
    return [(lines[i].strip(), lines[i + 1]) for i in range(0, len(lines) - 1, 2)]


def entities(path):
    out, in_entities, cur = [], False, None
    for code, val in _pairs(path):
        if code == "2" and val.strip() == "ENTITIES":
            in_entities = True
            continue
        if not in_entities:
            continue
        if code == "0":
            if cur:
                out.append(cur)
            if val.strip() == "ENDSEC":
                break
            cur = {"type": val.strip(), "pts": []}
        elif cur is not None:
            if cur["type"] == "LWPOLYLINE" and code in ("10", "20"):
                if code == "10":
                    cur["pts"].append([float(val), None])
                else:
                    cur["pts"][-1][1] = float(val)
            else:
                cur.setdefault(code, val.strip())
    return out


def geom(e):
    """('L', x1, y1, x2, y2) | ('C', cx, cy, r) | ('A', cx, cy, r, start_deg, end_deg) | ('P', [(x, y), ...]) | None"""
    t = e["type"]
    if t == "LINE":
        return ("L", float(e["10"]), float(e["20"]), float(e["11"]), float(e["21"]))
    if t == "CIRCLE":
        return ("C", float(e["10"]), float(e["20"]), float(e["40"]))
    if t == "ARC":
        return ("A", float(e["10"]), float(e["20"]), float(e["40"]), float(e["50"]), float(e["51"]))
    if t == "LWPOLYLINE":
        return ("P", [tuple(p) for p in e["pts"]])
    return None


if __name__ == "__main__":
    for g in (geom(e) for e in entities(sys.argv[1])):
        if g:
            print(tuple(round(v, 3) if isinstance(v, float) else v for v in g))
