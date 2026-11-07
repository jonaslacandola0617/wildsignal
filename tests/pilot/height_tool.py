"""REGION-001 build 018: the height test region tool makes cliffs nobody can cross.

Usage: python -I tests/pilot/height_tool.py TOOLS_DIR

Builds a tiny village (actors, a tree, a rock and a few grass rows), converts it
with village_to_region.py, then runs height_test_region.py. Checks that
region.json asks for drawn terrain, that the chunk height blocks cover the
lattice exactly once, that every steep cell lies inside a collider, that no
object stands on a steep cell or the ramp, that objects and grass sit on the
ground, and that the actors stay persistent.
"""
import json
import math
import runpy
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


def check(ok, message):
    if not ok:
        raise SystemExit("FAIL: " + message)


def main():
    tools = Path(sys.argv[1]).resolve()
    tool = runpy.run_path(str(tools / "height_test_region.py"))
    height, step, walk_slope, ramp = tool["height"], tool["STEP"], tool["WALK_SLOPE"], tool["RAMP"]
    with tempfile.TemporaryDirectory() as tmp:
        assets = Path(tmp)
        rows = [{"id": i, "model": m, "position": [x, 0, z], "yaw": 0, "scale": 1}
                for i, m, x, z in (("player_actor", "WS0P_PLAYER", 0, 0), ("companion", "PETALYN", 1, 0),
                                   ("wild", "PETALYN", 2, 0), ("pilot_ws004", "WS004", -6, -2),
                                   ("tree_a", "WSP_TREE_0", -20, 20), ("rock_a", "EA_ROCK_0", 20, -20))]
        (assets / "scene.json").write_text(json.dumps({"name": "TINY", "instances": rows}))
        (assets / "game.json").write_text(json.dumps({
            "world_half_extent": 29.65, "solids": [], "house_solids": [], "ground_spacing": 19.9,
            "spawn": [-1, 0, 8], "wild": [-1.7, 0, 4], "rest_spots": [], "review_cameras": [],
            "exclude_materials": []}))
        grass = [{"position": [x, 0, 3], "variant": 0, "scale": 0.8, "yaw": 0, "normal": [0, 1, 0],
                  "color_variant": 1} for x in (1, 2, 3)]
        (assets / "grass_00.json").write_text(json.dumps(grass))
        (assets / "vegetation.json").write_text(json.dumps({"instance_files": ["grass_00.json"]}))
        (assets / "terrain_height.bin").write_bytes(struct.pack("<%df" % (257 * 257), *([0.0] * 257 * 257)))
        for cmd in ([sys.executable, "-I", str(tools / "village_to_region.py"), str(assets)],
                    [sys.executable, "-I", str(tools / "height_test_region.py"), str(assets)]):
            r = subprocess.run(cmd, capture_output=True, text=True)
            check(r.returncode == 0, "%s failed\n%s%s" % (Path(cmd[2]).name, r.stdout, r.stderr))

        region_dir = assets / "regions" / "height_test"
        region = json.loads((region_dir / "region.json").read_text())
        check(region.get("terrain", {}).get("step") == step, "region.json has no terrain block")
        ox, oz = region["height"]["origin"]
        n = region["height"]["count"][0]
        owned = [[0] * n for _ in range(n)]
        lattice = [[0.0] * n for _ in range(n)]
        solids, objects, grass_rows = [], [], []
        for entry in region["chunks"]:
            c = json.loads((region_dir / entry["file"]).read_text())
            b = (region_dir / "chunks" / c["height"]).read_bytes()
            check(b[:4] == b"WSH1", "height block tag")
            i0, j0, ni, nj = struct.unpack("<4I", b[4:20])
            vals = struct.unpack("<%df" % (ni * nj), b[20:])
            for j in range(nj):
                for i in range(ni):
                    owned[j0 + j][i0 + i] += 1
                    lattice[j0 + j][i0 + i] = vals[j * ni + i]
            solids += [s["box"] for s in c["solids"]]
            objects += c["instances"]
            grass_rows += c["grass"]
        check(all(v == 1 for row in owned for v in row), "height samples not owned exactly once")

        def inside(x, z):
            return any(abs(x - bx) <= hx and abs(z - bz) <= hz for bx, bz, hx, hz in solids)

        steep_cells = 0
        for j in range(n - 1):
            for i in range(n - 1):
                c = (lattice[j][i], lattice[j][i + 1], lattice[j + 1][i], lattice[j + 1][i + 1])
                if (max(c) - min(c)) / step > walk_slope:
                    steep_cells += 1
                    x, z = ox + (i + 0.5) * step, oz + (j + 0.5) * step
                    check(inside(x, z), "steep cell at (%.2f, %.2f) has no collider" % (x, z))
        check(steep_cells > 100, "no cliffs were made")

        def steep_at(x, z):
            i, j = int((x - ox) / step), int((z - oz) / step)
            c = (lattice[j][i], lattice[j][i + 1], lattice[j + 1][i], lattice[j + 1][i + 1])
            return (max(c) - min(c)) / step > walk_slope

        rx0, rx1, rz0, rz1 = ramp
        for o in objects:
            x, y, z = o["position"]
            check(not steep_at(x, z), "%s stands on a cliff" % o["id"])
            check(not (rx0 <= x <= rx1 and rz0 <= z <= rz1), "%s blocks the ramp" % o["id"])
            check(abs(y - height(x, z)) < 1e-6, "%s floats or sinks" % o["id"])
        for g in grass_rows:
            x, y, z = g["position"]
            check(abs(y - height(x, z)) < 1e-6 and not steep_at(x, z), "grass off the ground")
        check(len(objects) >= 2 and len(grass_rows) > 1000, "too few objects or grass rows")
        persistent = {r["id"] for r in region["persistent"]}
        check({"player_actor", "companion", "wild", "pilot_ws004"} <= persistent, "actors not persistent")
    print("height test region: %d steep cells, all inside colliders; objects and grass on the ground" % steep_cells)


if __name__ == "__main__":
    main()
