"""Thins a region's trees so the ground and paths can be seen from the raised camera.

Usage: python -I tools/thin_trees.py REGION_FOLDER ROUTES_JSON RADIUS DROP_EVERY

ROUTES_JSON is {"routes": [[[x, z], [x, z], ...], ...]}, the walking lines to keep open.
Every tree within RADIUS metres of a route goes, along with its trunk's collider. Of the
trees left inside the walk bounds, every DROP_EVERY-th goes too (0 keeps them all); which ones
is fixed by their position. Trees in the outer 3 m and beyond (the forest wall that closes
the map edge) are always kept. Running it twice with the same numbers changes nothing further
for routes; DROP_EVERY is meant to be applied once, to a fresh copy.
"""
import json
import math
import sys
from pathlib import Path


def dist_to_segment(p, a, b):
    ax, az = a
    dx, dz = b[0] - ax, b[1] - az
    n = dx * dx + dz * dz
    t = 0 if n == 0 else max(0, min(1, ((p[0] - ax) * dx + (p[1] - az) * dz) / n))
    return math.hypot(p[0] - ax - t * dx, p[1] - az - t * dz)


def main():
    folder, routes = Path(sys.argv[1]), json.loads(Path(sys.argv[2]).read_text(encoding="utf-8"))["routes"]
    radius, every = float(sys.argv[3]), int(sys.argv[4])
    region = json.loads((folder / "region.json").read_text(encoding="utf-8"))
    half = region.get("walk_half_extent", 1e9) - 3
    removed = kept = wall = 0
    for entry in region["chunks"]:
        path = folder / entry["file"]
        chunk = json.loads(path.read_text(encoding="utf-8"))
        gone, rows = [], []
        for row in chunk.get("instances", []):
            if not row["model"].startswith("WSP_TREE"):
                rows.append(row)
                continue
            x, z = row["position"][0], row["position"][2]
            if max(abs(x), abs(z)) > half:
                wall += 1
                rows.append(row)
                continue
            near = any(dist_to_segment((x, z), r[i], r[i + 1]) < radius for r in routes for i in range(len(r) - 1))
            pick = every > 0 and int(abs(x * 13.7 + z * 7.3)) % every == 0
            if near or pick:
                gone.append((x, z))
                removed += 1
            else:
                rows.append(row)
                kept += 1
        if gone:
            chunk["instances"] = rows
            chunk["solids"] = [s for s in chunk.get("solids", []) if not (not s.get("house") and any(abs(s["box"][0] - x) < .8 and abs(s["box"][1] - z) < .8 for x, z in gone))]
            path.write_text(json.dumps(chunk, indent=1), encoding="utf-8")
    print("%s: %d trees removed, %d kept inside, %d in the forest wall" % (folder, removed, kept, wall))


if __name__ == "__main__":
    main()
