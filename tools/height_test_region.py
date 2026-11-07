"""REGION-001 build 018: a test region with real height, made from the village.

Usage: python -I tools/height_test_region.py ASSETS_DIR [REGION_ID] [--no-ring]

Builds ASSETS_DIR/regions/REGION_ID/ (default height_test): 4 x 4 chunks of
32 m with the ground drawn from the height lattice ("terrain" in region.json),
no ground tiles. It has gentle rolling ground around a flat-ish middle (spawn
and the wild Signari keep their village spots), a round walkable hill, and a
raised meadow 3 m up with an uneven cliff outline, a 1.5 m ledge on its south
side and a walkable ramp on its west side.

Cells steeper than WALK_SLOPE become colliders, so nobody walks up or off a
cliff; trees and rocks get trunk colliders. Objects, grass and characters stand
on the ground. Sizes and grass variants are copied from the village's own rows.
Reads regions/hearthgrove_test (run village_to_region.py first). Private data:
the output is derived from private art data, so keep it out of git.
"""
import json
import math
import random
import shutil
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))  # edge_ring.py beside this tool
import edge_ring  # noqa: E402

CHUNK = 32.0
COLS = ROWS = 4
ORIGIN = -64.0
STEP = 0.5
WALK_SLOPE = 0.7  # rise over run; steeper cells are colliders (about 35 degrees)
PLATEAU = (28.0, -20.0, 11.0)  # centre x, z and mean radius of the raised meadow
PLATEAU_HEIGHT = 3.0
LEDGE = 1.5  # a lower step along the south side, so the cliff reads as stepped rock
RAMP = (2.0, 19.0, -23.0, -17.0)  # x0, x1, z0, z1; rises from 0 to the plateau going east
HILL = (-30.0, 26.0, 18.0, 4.0)  # x, z, radius, height
SPAWN = (-3.0, -20.0)  # a new game starts just west of the ramp foot, facing it
TERRAIN = {"step": STEP, "cliff_normal_y": 0.6, "lawn": [0.40, 0.62, 0.20], "rock": [0.48, 0.45, 0.50]}


def height(x, z):
    h = 0.35 * math.sin(x * 0.07) * math.cos(z * 0.05) + 0.15 * math.sin(z * 0.11 + 1.3)
    # Keep the village middle (spawn, wilds, rest spots) nearly flat.
    calm = min(1.0, max(0.0, (math.hypot(x, z) - 14.0) / 10.0))
    h *= calm
    hx, hz, hr, hh = HILL
    r = math.hypot(x - hx, z - hz)
    if r < hr:
        h += hh * math.cos(r / hr * math.pi / 2) ** 2
    px, pz, pr = PLATEAU
    a = math.atan2(z - pz, (x - px) / 1.3)
    edge = pr + 2.2 * math.sin(3 * a + 1.0) + 1.3 * math.sin(5 * a + 2.0) + 0.6 * math.sin(9 * a)
    d = math.hypot((x - px) / 1.3, z - pz)
    lift = 0.0
    if d < edge:
        lift = PLATEAU_HEIGHT
    elif d < edge + 3.5 and math.sin(a) > 0.25:  # south side (+z) ledge
        lift = LEDGE
    rx0, rx1, rz0, rz1 = RAMP
    if rx0 <= x <= rx1 and rz0 <= z <= rz1:
        lift = max(lift, PLATEAU_HEIGHT * min(1.0, (x - rx0) / (rx1 - 3 - rx0)))
    return h + lift


def main():
    args = [a for a in sys.argv[1:] if a != "--no-ring"]
    assets = Path(args[0])
    region_id = args[1] if len(args) > 1 else "height_test"
    src = assets / "regions" / "hearthgrove_test"
    village = json.loads((src / "region.json").read_text())
    rows_by_model, grass_rows = {}, []
    for entry in village["chunks"]:
        if entry.get("ring"):  # sizes come from the village itself, not its edge ring
            continue
        c = json.loads((src / entry["file"]).read_text())
        for r in c["instances"]:
            if not r["id"].startswith("ring_"):
                rows_by_model.setdefault(r["model"], []).append(r)
        grass_rows += c["grass"]
    rng = random.Random(18)

    n = int(round(COLS * CHUNK / STEP)) + 1  # samples per side, both edges included
    hs = [[height(ORIGIN + i * STEP, ORIGIN + j * STEP) for i in range(n)] for j in range(n)]

    def chunk_of(x, z):
        cx = min(COLS - 1, max(0, int(math.floor((x - ORIGIN) / CHUNK))))
        cz = min(ROWS - 1, max(0, int(math.floor((z - ORIGIN) / CHUNK))))
        return cx, cz

    chunks = {(cx, cz): {"instances": [], "solids": [], "grass": []} for cz in range(ROWS) for cx in range(COLS)}

    # Steep cells: colliders, merged into runs along x.
    steep = [[False] * (n - 1) for _ in range(n - 1)]
    for j in range(n - 1):
        for i in range(n - 1):
            c = (hs[j][i], hs[j][i + 1], hs[j + 1][i], hs[j + 1][i + 1])
            steep[j][i] = (max(c) - min(c)) / STEP > WALK_SLOPE
    order = 0
    cliff_cells = 0
    for j in range(n - 1):
        i = 0
        while i < n - 1:
            if not steep[j][i]:
                i += 1
                continue
            start = i
            while i < n - 1 and steep[j][i] and (i - start) * STEP < 4:
                i += 1
            cliff_cells += i - start
            x = ORIGIN + (start + i) * STEP / 2
            z = ORIGIN + (j + 0.5) * STEP
            box = [x, z, (i - start) * STEP / 2 + 0.02, STEP / 2 + 0.02]
            chunks[chunk_of(x, z)]["solids"].append({"box": box, "house": False, "order": order})
            order += 1

    def near_steep(x, z, r):
        i0, i1 = int((x - r - ORIGIN) / STEP), int((x + r - ORIGIN) / STEP)
        j0, j1 = int((z - r - ORIGIN) / STEP), int((z + r - ORIGIN) / STEP)
        for j in range(max(0, j0), min(n - 1, j1 + 1)):
            for i in range(max(0, i0), min(n - 1, i1 + 1)):
                if steep[j][i]:
                    return True
        return False

    def normal(x, z):
        d = 0.25
        gx = (height(x + d, z) - height(x - d, z)) / (2 * d)
        gz = (height(x, z + d) - height(x, z - d)) / (2 * d)
        l = math.sqrt(gx * gx + 1 + gz * gz)
        return [-gx / l, 1 / l, -gz / l]

    def on_ramp(x, z, margin):
        rx0, rx1, rz0, rz1 = RAMP
        return rx0 - margin <= x <= rx1 + margin and rz0 - margin <= z <= rz1 + margin

    # Objects: copy a village row of the same model for its size, place it here.
    placed, trunks = [], []  # trunks: objects with a collider (grass keeps clear)
    lo, hi = ORIGIN + 2, ORIGIN + COLS * CHUNK - 2

    def place(model, count, keep_out, solid):
        nonlocal order
        templates = rows_by_model.get(model)
        if not templates:  # a village without this model: skip it
            return
        tries = 0
        while count and tries < count * 200:
            tries += 1
            x, z = rng.uniform(lo, hi), rng.uniform(lo, hi)
            if (math.hypot(x, z) < 12 or near_steep(x, z, keep_out) or on_ramp(x, z, keep_out + 1)
                    or math.hypot(x - SPAWN[0], z - SPAWN[1]) < 4):
                continue
            if any(math.hypot(x - px, z - pz) < keep_out + pr for px, pz, pr in placed):
                continue
            t = rng.choice(templates)
            row = dict(t)
            row["id"] = "%s_%d" % (model.lower(), len(placed))
            row["position"] = [x, height(x, z), z]
            row["yaw"] = rng.uniform(0, 2 * math.pi)
            row["order"] = 10 + len(placed)
            chunks[chunk_of(x, z)]["instances"].append(row)
            placed.append((x, z, keep_out))
            if solid:
                s = solid * float(row.get("scale", 1))
                trunks.append((x, z, s + 0.2))
                chunks[chunk_of(x, z)]["solids"].append({"box": [x, z, s, s], "house": False, "order": order})
                order += 1
            count -= 1

    for model, count, keep_out, solid in (
            ("WSP_TREE_0", 12, 1.6, 0.3), ("WSP_TREE_1", 10, 1.6, 0.3), ("WSP_TREE_2", 8, 1.6, 0.3),
            ("WSP_TREE_3", 8, 1.6, 0.3), ("WSP_BUSH_0", 160, 0.7, 0), ("EA_ROCK_0", 5, 1.0, 0.45),
            ("EA_ROCK_1", 5, 1.0, 0.45), ("EA_ROCK_2", 5, 1.0, 0.45), ("WSP_FLOWER_LILAC_0", 120, 0.3, 0),
            ("WSP_FLOWER_YELLOW_0", 80, 0.3, 0), ("WSP_FLOWER_PINK_0", 80, 0.3, 0),
            ("WSP_FLOWER_CREAM_0", 60, 0.3, 0), ("WSP_TUFT_RING_S", 600, 0.4, 0), ("WSP_TUFT_RING_L", 80, 0.6, 0)):
        place(model, count, keep_out, solid)

    # Grass on walkable ground, about the village's density.
    seed = 0
    gx = lo
    while gx < hi:
        gz = lo
        while gz < hi:
            x, z = gx + rng.uniform(-0.35, 0.35), gz + rng.uniform(-0.35, 0.35)
            if not near_steep(x, z, 0.4) and not any(math.hypot(x - px, z - pz) < pr for px, pz, pr in trunks):
                t = rng.choice(grass_rows)
                chunks[chunk_of(x, z)]["grass"].append({
                    "position": [x, height(x, z), z], "variant": t["variant"], "scale": t["scale"],
                    "yaw": rng.uniform(0, 2 * math.pi), "normal": normal(x, z),
                    "color_variant": t["color_variant"], "seed": seed})
                seed += 1
            gz += 0.8
        gx += 0.8

    out = assets / "regions" / region_id
    if out.exists():
        shutil.rmtree(out)
    (out / "chunks").mkdir(parents=True)
    listing = []
    for (cx, cz), c in sorted(chunks.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        name = "c_%d_%d" % (cx, cz)
        # This chunk owns the samples inside its square; the last row and column own the far edge.
        i0 = int(round(cx * CHUNK / STEP))
        j0 = int(round(cz * CHUNK / STEP))
        i1 = n - 1 if cx == COLS - 1 else int(round((cx + 1) * CHUNK / STEP)) - 1
        j1 = n - 1 if cz == ROWS - 1 else int(round((cz + 1) * CHUNK / STEP)) - 1
        ni, nj = i1 - i0 + 1, j1 - j0 + 1
        vals = [hs[j0 + j][i0 + i] for j in range(nj) for i in range(ni)]
        (out / "chunks" / (name + ".height.bin")).write_bytes(
            b"WSH1" + struct.pack("<4I", i0, j0, ni, nj) + struct.pack("<%df" % len(vals), *vals))
        chunk = {"revision": 1, "x": cx, "z": cz, **c, "height": name + ".height.bin"}
        (out / "chunks" / (name + ".json")).write_text(json.dumps(chunk, separators=(",", ":")))
        listing.append({"x": cx, "z": cz, "file": "chunks/" + name + ".json"})

    persistent = []
    for r in village["persistent"]:
        r2 = dict(r)
        x, z = r["position"][0], r["position"][2]
        r2["position"] = [x, height(x, z), z]
        persistent.append(r2)
    walk = COLS * CHUNK / 2 - 0.5
    region = {k: village[k] for k in ("ground_spacing", "terrain_instance_y_scale", "spawn", "wild",
                                      "rest_spots", "review_cameras", "exclude_materials")}
    region.update({
        "revision": 1, "id": region_id, "name": "Height test (hill, raised meadow, cliffs)",
        "spawn": [SPAWN[0], height(*SPAWN), SPAWN[1]],
        "chunk_size": CHUNK, "grid": {"origin": [ORIGIN, ORIGIN], "columns": COLS, "rows": ROWS},
        "height": {"origin": [ORIGIN, ORIGIN], "step": STEP, "count": [n, n]},
        "walk": {"min": [-walk, -walk], "max": [walk, walk]},
        "shared": village["shared"],
        "shadow_follow": True,
        "meadow": {"world_min": [-29.85, -29.85], "world_size": [59.7, 59.7], "repeat": True},
        "terrain": TERRAIN,
        "persistent": persistent,
        "chunks": listing,
    })
    (out / "region.json").write_text(json.dumps(region, indent=1))
    print("region", region_id, "grid", COLS, "x", ROWS, "objects",
          sum(len(c["instances"]) for c in chunks.values()), "colliders",
          sum(len(c["solids"]) for c in chunks.values()), "cliff cells", cliff_cells, "grass", seed)
    if "--no-ring" not in sys.argv:
        edge_ring.add_ring(assets, region_id)


if __name__ == "__main__":
    main()
