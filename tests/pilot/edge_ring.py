"""REGION-001 build 019: the edge ring closes the map edge for the flat camera.

Usage: python -I tests/pilot/edge_ring.py TOOLS_DIR

Builds a tiny village with a 3 x 3 grid of flat stand-in ground tiles (no
private art), converts it with village_to_region.py (ring on and off) and
height_test_region.py (ground drawn from heights), and checks that:
  - the ring adds one chunk all round, flagged "ring", and the old chunks keep
    exactly their contents (ground tiles anchored in an old edge chunk aside);
  - the walk bounds, spawn and characters do not change;
  - ring rows use only approved models, have "ring_" ids, stand outside the
    walk bounds, sit on the ground, and need no colliders;
  - from every point along the walk edge, at all four camera corners, the
    flat camera's view of the ground (the dream look: half height 6.5 m,
    pitch 0.70, 16:9) is covered by ground in chunks inside the 3 x 3 window
    that is switched on around the player;
  - adding a ring twice is refused.
"""
import json
import math
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HALF, PITCH, ASPECT = 6.5, -0.70, 16 / 9  # pilot_dream.json "camera" and the engine's flat view
APPROVED = ("EA_GROUND", "WSP_TREE_", "WSP_BUSH_0", "EA_ROCK_")


def check(ok, message):
    if not ok:
        raise SystemExit("FAIL: " + message)


def flat_tile(assets):
    """A flat 20 m square standing in for EA_GROUND: local x -20..0, z 0..20."""
    pos = [(-20, 0, 0), (0, 0, 0), (0, 0, 20), (-20, 0, 20)]
    blob = b"".join(struct.pack("<3f", *p) for p in pos) + struct.pack("<6H", 0, 1, 2, 0, 2, 3)
    (assets / "EA_GROUND.bin").write_bytes(blob)
    (assets / "EA_GROUND.gltf").write_text(json.dumps({
        "asset": {"version": "2.0"}, "buffers": [{"uri": "EA_GROUND.bin", "byteLength": len(blob)}],
        "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": 48},
                        {"buffer": 0, "byteOffset": 48, "byteLength": 12}],
        "accessors": [{"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3",
                       "min": [-20, 0, 0], "max": [0, 0, 20]},
                      {"bufferView": 1, "componentType": 5123, "count": 6, "type": "SCALAR"}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
        "nodes": [{"mesh": 0}]}))


def village(assets):
    rows = [{"id": i, "model": m, "position": [x, 0, z], "yaw": 0, "scale": 1}
            for i, m, x, z in (("player_actor", "WS0P_PLAYER", 0, 0), ("companion", "PETALYN", 1, 0),
                               ("wild", "PETALYN", 2, 0), ("pilot_ws004", "WS004", -6, -2),
                               ("tree_a", "WSP_TREE_0", -20, 20), ("tree_b", "WSP_TREE_1", 12, -14),
                               ("tree_far", "WSP_TREE_FAR", -27, -27), ("bush", "WSP_BUSH_0", 5, 5),
                               ("rock", "EA_ROCK_0", 20, -20), ("barrel", "EA_BARREL", 3, -3))]
    for gz in (-19.9, 0, 19.9):
        for gx in (-19.9, 0, 19.9):
            rows.append({"id": "ground_%g_%g" % (gx, gz), "model": "EA_GROUND",
                         "position": [gx + 10, 0, gz - 10], "yaw": 0, "scale": 1})
    (assets / "scene.json").write_text(json.dumps({"name": "TINY", "instances": rows}))
    (assets / "game.json").write_text(json.dumps({
        "world_half_extent": 29.65, "solids": [[12, -14, .4, .4]], "house_solids": [], "ground_spacing": 19.9,
        "spawn": [-1, 0, 8], "wild": [-1.7, 0, 4], "rest_spots": [], "review_cameras": [],
        "exclude_materials": []}))
    grass = [{"position": [x, 0, z], "variant": 0, "scale": 1, "yaw": 0, "normal": [0, 1, 0], "color_variant": 0}
             for x, z in ((3, 3), (-8, 9))]
    (assets / "grass_00.json").write_text(json.dumps(grass))
    (assets / "vegetation.json").write_text(json.dumps({"instance_files": ["grass_00.json"]}))
    (assets / "terrain_height.bin").write_bytes(struct.pack("<%df" % (257 * 257), *([0.0] * 257 * 257)))
    flat_tile(assets)


def run(*cmd):
    r = subprocess.run([sys.executable, "-I", *map(str, cmd)], capture_output=True, text=True)
    check(r.returncode == 0, "%s failed\n%s%s" % (Path(cmd[0]).name, r.stdout, r.stderr))
    return r


def load(region_dir):
    region = json.loads((region_dir / "region.json").read_text())
    chunks = {}
    for e in region["chunks"]:
        chunks[(e["x"], e["z"])] = (e, json.loads((region_dir / e["file"]).read_text()))
    return region, chunks


def walk_box(region):
    if "walk" in region:
        return (*region["walk"]["min"], *region["walk"]["max"])
    w = region["walk_half_extent"]
    return (-w, -w, w, w)


def footprint(px, pz, quarter, ground):
    """Ground points at the edges of the flat camera's view (engine maths)."""
    yaw = math.remainder(math.pi * .25 + math.pi * .5 * quarter, 2 * math.pi)
    f = (math.sin(yaw) * math.cos(PITCH), math.sin(PITCH), -math.cos(yaw) * math.cos(PITCH))
    r = (-f[2], 0.0, f[0])  # cross(f, up), normalised below
    n = math.hypot(r[0], r[2])
    r = (r[0] / n, 0.0, r[2] / n)
    u = (r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0])
    cam = (px - f[0] * 4, ground(px, pz) + 1 - f[1] * 4, pz - f[2] * 4)
    pts = []
    for k in range(9):
        for sx, sy in ((-1 + k / 4, -1), (-1 + k / 4, 1), (-1, -1 + k / 4), (1, -1 + k / 4)):
            p = [cam[i] + r[i] * sx * HALF * ASPECT + u[i] * sy * HALF for i in range(3)]
            g = 0.0
            for _ in range(4):  # follow the ray down to the ground under it
                t = (g - p[1]) / f[1]
                x, z = p[0] + f[0] * t, p[2] + f[2] * t
                g = ground(x, z)
            pts.append((x, z))
    return pts


def edge_points(walk, step=4.0):
    x0, z0, x1, z1 = walk
    pts = []
    nx, nz = int(math.ceil((x1 - x0) / step)), int(math.ceil((z1 - z0) / step))
    for k in range(nx + 1):
        x = x0 + (x1 - x0) * k / nx
        pts += [(x, z0), (x, z1)]
    for k in range(1, nz):
        z = z0 + (z1 - z0) * k / nz
        pts += [(x0, z), (x1, z)]
    return pts


def chunk_of(region, x, z):
    C = region["chunk_size"]
    ox, oz = region["grid"]["origin"]
    return int(math.floor((x - ox) / C)), int(math.floor((z - oz) / C))


def check_ring_rows(name, region, chunks, old_ids):
    walk = walk_box(region)
    ring = [k for k, (e, _) in chunks.items() if e.get("ring")]
    cols, rows = region["grid"]["columns"], region["grid"]["rows"]
    edge = {(x, z) for x in range(cols) for z in range(rows) if x in (0, cols - 1) or z in (0, rows - 1)}
    check(set(ring) == edge, "%s: ring chunks are exactly the outer row of the grid" % name)
    for k, (e, c) in chunks.items():
        for r in c["instances"]:
            if r["id"] in old_ids:
                continue
            check(r["id"].startswith("ring_"), "%s: new row %s has no ring_ id" % (name, r["id"]))
            check(r["model"].startswith(APPROVED), "%s: ring uses %s" % (name, r["model"]))
            x, z = r["position"][0], r["position"][2]
            if r["model"] != "EA_GROUND":
                check(e.get("ring"), "%s: %s stands in an old chunk" % (name, r["id"]))
                check(not (walk[0] < x < walk[2] and walk[1] < z < walk[3]),
                      "%s: %s stands inside the walk bounds" % (name, r["id"]))
        if e.get("ring"):
            check(not c["solids"] and not c["grass"], "%s: ring chunk %s has colliders or grass" % (name, k))
    return sum(len(c["instances"]) for k, (e, c) in chunks.items() if e.get("ring"))


def main():
    tools = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory() as tmp:
        base, assets = Path(tmp) / "plain", Path(tmp) / "ringed"
        for a in (base, assets):
            a.mkdir()
            village(a)
        run(tools / "village_to_region.py", base, "--no-ring")
        run(tools / "village_to_region.py", assets)

        # Tile ground (the village).
        plain, plain_chunks = load(base / "regions" / "hearthgrove_test")
        region, chunks = load(assets / "regions" / "hearthgrove_test")
        check("edge_ring" not in plain and region["edge_ring"]["depth"] == 1, "ring recorded in region.json")
        check(region["grid"]["columns"] == plain["grid"]["columns"] + 2 and
              region["grid"]["rows"] == plain["grid"]["rows"] + 2, "grid grows one chunk all round")
        check(region["grid"]["origin"] == [o - region["chunk_size"] for o in plain["grid"]["origin"]],
              "grid origin moves out one chunk")
        for key in ("walk_half_extent", "height", "spawn", "wild", "persistent"):
            check(region[key] == plain[key], "%s unchanged by the ring" % key)
        old_ids = set()
        for (x, z), (e, c) in plain_chunks.items():
            e2, c2 = chunks[(x + 1, z + 1)]
            check(not e2.get("ring"), "old chunk flagged as ring")
            mine = [r for r in c2["instances"] if not r["id"].startswith("ring_")]
            check(mine == c["instances"] and c2["solids"] == c["solids"] and c2["grass"] == c["grass"],
                  "old chunk %d %d keeps its contents" % (x, z))
            extra = [r for r in c2["instances"] if r["id"].startswith("ring_")]
            check(all(r["model"] == "EA_GROUND" for r in extra), "only ground tiles join old chunks")
            if "height" in c:
                check((base / "regions/hearthgrove_test/chunks" / c["height"]).read_bytes() ==
                      (assets / "regions/hearthgrove_test/chunks" / c2["height"]).read_bytes(),
                      "old chunk heights unchanged")
            old_ids |= {r["id"] for r in c["instances"]}
        check(check_ring_rows("village", region, chunks, old_ids) > 20, "village ring has scenery")
        tiles = [(r, k) for k, (e, c) in chunks.items() for r in c["instances"] if r["model"] == "EA_GROUND"]
        check(len({r["yaw"] for r, _ in tiles}) == 1, "ring tiles keep the village's turn (seams meet)")

        def covered(x, z):
            for r, k in tiles:
                lx, lz = x - r["position"][0], z - r["position"][2]  # yaw 0
                if -20 <= lx <= 0 and 0 <= lz <= 20:
                    yield k

        for px, pz in edge_points(walk_box(region)):
            cx, cz = chunk_of(region, px, pz)
            for q in range(4):
                for x, z in footprint(px, pz, q, lambda x, z: 0.0):
                    keys = list(covered(x, z))
                    check(keys, "void: no ground at (%.1f, %.1f) seen from (%.1f, %.1f) corner %d" % (x, z, px, pz, q))
                    check(any(abs(k[0] - cx) <= 1 and abs(k[1] - cz) <= 1 for k in keys),
                          "ground at (%.1f, %.1f) is not switched on around (%.1f, %.1f)" % (x, z, px, pz))

        r = subprocess.run([sys.executable, "-I", str(tools / "edge_ring.py"), str(assets), "hearthgrove_test"],
                           capture_output=True, text=True)
        check(r.returncode != 0 and "already" in (r.stdout + r.stderr), "a second ring is refused")

        # Ground drawn from heights (the height test region).
        run(tools / "height_test_region.py", assets)
        region, chunks = load(assets / "regions" / "height_test")
        ox, oz = region["height"]["origin"]
        step = region["height"]["step"]
        nx, nz = region["height"]["count"]
        C = region["chunk_size"]
        gx, gz = region["grid"]["origin"]
        check(ox <= gx and oz <= gz and ox + (nx - 1) * step >= gx + region["grid"]["columns"] * C and
              oz + (nz - 1) * step >= gz + region["grid"]["rows"] * C, "height lattice covers the ring")
        lattice = [[None] * nx for _ in range(nz)]
        for k, (e, c) in chunks.items():
            b = (assets / "regions/height_test/chunks" / c["height"]).read_bytes()
            i0, j0, ni, nj = struct.unpack("<4I", b[4:20])
            vals = struct.unpack("<%df" % (ni * nj), b[20:])
            for j in range(nj):
                for i in range(ni):
                    check(lattice[j0 + j][i0 + i] is None, "height sample owned twice")
                    lattice[j0 + j][i0 + i] = vals[j * ni + i]
        check(all(v is not None for row in lattice for v in row), "every height sample owned")

        def ground(x, z):
            u = min(max((x - ox) / step, 0), nx - 1)
            v = min(max((z - oz) / step, 0), nz - 1)
            i, j = min(int(u), nx - 2), min(int(v), nz - 2)
            a, b = u - i, v - j
            h = lattice
            return (1 - b) * ((1 - a) * h[j][i] + a * h[j][i + 1]) + b * ((1 - a) * h[j + 1][i] + a * h[j + 1][i + 1])

        walk = walk_box(region)
        check(walk == (-63.5, -63.5, 63.5, 63.5), "height test walk bounds unchanged")
        ring_rows = [r for k, (e, c) in chunks.items() if e.get("ring") for r in c["instances"]]
        check(len(ring_rows) > 50, "height test ring has scenery")
        old_ids = {r["id"] for k, (e, c) in chunks.items() if not e.get("ring") for r in c["instances"]}
        check(not any(i.startswith("ring_") for i in old_ids), "no ring rows in the height test's old chunks")
        check_ring_rows("height test", region, chunks, old_ids)
        for r in ring_rows:
            x, y, z = r["position"]
            check(abs(y - ground(x, z)) < 1e-6, "%s floats or sinks" % r["id"])
        steep = 0
        for j in range(nz - 1):
            for i in range(nx - 1):
                x = ox + i * step
                z = oz + j * step
                if walk[0] < x < walk[2] and walk[1] < z < walk[3]:
                    continue
                c = (lattice[j][i], lattice[j][i + 1], lattice[j + 1][i], lattice[j + 1][i + 1])
                steep += (max(c) - min(c)) / step > 0.7
        check(steep == 0, "the ring's bank is gentle ground, not cliff (%d steep cells)" % steep)
        for px, pz in edge_points(walk, 8.0):
            cx, cz = chunk_of(region, px, pz)
            for q in range(4):
                for x, z in footprint(px, pz, q, ground):
                    k = chunk_of(region, x, z)
                    check(k in chunks, "void: (%.1f, %.1f) seen from (%.1f, %.1f) is past the grid" % (x, z, px, pz))
                    check(abs(k[0] - cx) <= 1 and abs(k[1] - cz) <= 1,
                          "ground at (%.1f, %.1f) is not switched on around (%.1f, %.1f)" % (x, z, px, pz))
    print("edge ring: grid grown, old chunks kept, rows outside the walk, every edge view covered")


if __name__ == "__main__":
    main()
