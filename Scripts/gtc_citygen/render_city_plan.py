#!/usr/bin/env python3
"""Top-down SVG preview of the generated Caliber City (offline; no deps, no editor).

Reads city_plan.json + road_network.json and draws water, roads, and per-district
buildings so the layout can be eyeballed without applying into the editor.

    python3 render_city_plan.py   ->  city_preview.svg
"""

import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

X0, Y0, X1, Y1 = -300000, -320000, 340000, 320000
PXW = 1400
SCALE = (PXW - 80) / (X1 - X0)
PXH = int((Y1 - Y0) * SCALE) + 80

DCOLOR = {
    "Downtown": "#e6194b", "Wynwood": "#f58231", "LittleHavana": "#3cb44b",
    "Port": "#911eb4", "Outersuburbanring": "#808080", "OceanDrive": "#00b8b8",
}
RCOLOR = {"highway": "#222", "arterial": "#555", "causeway": "#7a5", "local": "#999"}


def sx(wx):
    return 40 + (wx - X0) * SCALE


def sy(wy):
    return 40 + (Y1 - wy) * SCALE   # flip Y so north is up


def main():
    out_dir = os.path.join(REPO, json.load(open(os.path.join(HERE, "city_config.json")))["output_dir_rel"])
    plan = json.load(open(os.path.join(out_dir, "city_plan.json")))
    net = json.load(open(os.path.join(out_dir, "road_network.json")))

    s = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">' % (PXW, PXH, PXW, PXH)]
    s.append('<rect width="%d" height="%d" fill="#f4f1ea"/>' % (PXW, PXH))

    # water
    def water_rect(a, b, c, d, fill="#bcd6e8"):
        s.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s"/>'
                 % (sx(a), sy(d), (sx(c) - sx(a)), (sy(b) - sy(d)), fill))
    water_rect(320000, Y0, X1, Y1)               # Atlantic
    water_rect(150000, -180000, 250000, 180000)  # Biscayne Bay
    water_rect(150000, -150000, 215000, -95000)  # port basin
    # river
    s.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#bcd6e8" stroke-width="6"/>'
             % (sx(150000), sy(-10000), sx(-40000), sy(-25000)))

    # roads
    for r in net["roads"]:
        col = RCOLOR.get(r["class"], "#777")
        wpx = max(1.0, r["width"] * SCALE)
        pts = " ".join("%.1f,%.1f" % (sx(p[0]), sy(p[1])) for p in r["centerline"])
        s.append('<polyline points="%s" fill="none" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>'
                 % (pts, col, wpx))

    # buildings, colored by district
    per = {}
    for a in plan["actors"]:
        f = a["folder"]
        if "/Districts/" in f:
            key = f.split("/Districts/")[-1]
        elif f.endswith("/Buildings"):
            key = "OceanDrive"
        else:
            continue
        per[key] = per.get(key, 0) + 1
        col = DCOLOR.get(key, "#333")
        L = a["location"]
        s.append('<rect x="%.1f" y="%.1f" width="3.2" height="3.2" fill="%s"/>'
                 % (sx(L["x"]) - 1.6, sy(L["y"]) - 1.6, col))

    # legend
    ly = 55
    s.append('<text x="50" y="35" font-family="sans-serif" font-size="22" fill="#111">Caliber City — generated layout (top-down, north up)</text>')
    for k in sorted(per):
        s.append('<rect x="50" y="%d" width="14" height="14" fill="%s"/>' % (ly, DCOLOR.get(k, "#333")))
        s.append('<text x="70" y="%d" font-family="sans-serif" font-size="14" fill="#111">%s (%d)</text>' % (ly + 12, k, per[k]))
        ly += 22
    s.append('</svg>')

    p = os.path.join(out_dir, "city_preview.svg")
    open(p, "w").write("\n".join(s))
    print("wrote %s  (%d buildings across %d groups)" % (os.path.relpath(p, REPO), sum(per.values()), len(per)))
    print("  ", ", ".join("%s=%d" % (k, v) for k, v in sorted(per.items())))
    return p


if __name__ == "__main__":
    main()
