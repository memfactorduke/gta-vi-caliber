"""Independent no-overlap validator. NO `import unreal`.

The solver guarantees no overlap by construction; this proves it with an exact,
zero-dependency uniform grid-hash over the 800-snapped rectangles, and — crucially —
also catches solver bugs. Inputs are axis-aligned integer rects, so "overlap" is
exact: touching edges (party walls, plots meeting a setback line) share zero area and
are LEGAL; any positive shared area is a violation.

Forbidden interior overlaps:
    plot-plot, plot-ROW, plot-water, building-building, building-ROW, building-water.
Allowed: ROW-ROW (roads cross at intersections), building-plot (containment).
"""

from layout import interiors_overlap

BUCKET = 8000   # grid-hash cell (cm)

# Pairs whose interiors must be disjoint, keyed by frozenset of kinds.
_FORBIDDEN = {
    frozenset({"plot"}),
    frozenset({"building"}),
    frozenset({"plot", "row"}),
    frozenset({"plot", "water"}),
    frozenset({"building", "row"}),
    frozenset({"building", "water"}),
}


def _buckets(rect):
    x0, y0, x1, y1 = rect
    for bx in range(x0 // BUCKET, (x1 - 1) // BUCKET + 1):
        for by in range(y0 // BUCKET, (y1 - 1) // BUCKET + 1):
            yield (bx, by)


def validate_rects(named):
    """`named` = list of (kind, label, rect). Returns a report dict."""
    grid = {}
    for i, (kind, label, rect) in enumerate(named):
        for b in _buckets(rect):
            grid.setdefault(b, []).append(i)

    violations = []
    seen_pairs = set()
    for items in grid.values():
        for a in range(len(items)):
            for b in range(a + 1, len(items)):
                i, j = items[a], items[b]
                pair = (i, j) if i < j else (j, i)
                if pair in seen_pairs:
                    continue
                seen_pairs.add(pair)
                ka, la, ra = named[i]
                kb, lb, rb = named[j]
                if frozenset({ka, kb}) not in _FORBIDDEN:
                    continue
                if interiors_overlap(ra, rb):
                    violations.append({"a": [ka, la, list(ra)], "b": [kb, lb, list(rb)]})

    counts = {}
    for kind, _, _ in named:
        counts[kind] = counts.get(kind, 0) + 1
    return {
        "status": "PASS" if not violations else "FAIL",
        "counts": counts,
        "violation_count": len(violations),
        "violations": violations[:50],   # cap the report
    }


def validate(layout):
    """Build the named-rect list from a solved Layout and validate it."""
    named = []
    for rd in layout.roads:
        named.append(("row", rd["name"], rd["row"]))
    for w in layout.water:
        named.append(("water", "water", w))
    for i, p in enumerate(layout.plots):
        named.append(("plot", "plot_%d" % i, p["rect"]))
    for i, b in enumerate(layout.buildings):
        named.append(("building", "bldg_%d" % i, b["rect"]))
    return validate_rects(named)
