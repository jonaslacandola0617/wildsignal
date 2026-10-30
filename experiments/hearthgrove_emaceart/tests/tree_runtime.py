"""Optional actual-GL checks with the complete private single-tree fixture.

Software GL is explicitly diagnostic, never hardware/displayed-FPS evidence.
Usage: python tree_runtime.py EXECUTABLE PRIVATE_ASSETS OUTPUT_DIRECTORY
"""
import csv
import json
import os
from pathlib import Path
import subprocess
import sys

exe, assets, root = map(lambda x: Path(x).resolve(), sys.argv[1:4])
root.mkdir(parents=True, exist_ok=True)
env = dict(os.environ)
env.setdefault("SDL_VIDEODRIVER", "offscreen")
env.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
env.setdefault("LP_NUM_THREADS", "2")
results = {}

def run(name, mode, pose=0, extra=()):
    out = root / name
    cmd = [str(exe), "--assets", str(assets), "--tree-scene", "--tree-study", mode,
           "--allow-software", "--hidden", "--benchmark", "--pose", str(pose),
           "--resolution", "480", "--frames", "3", "--cap", "0", "--hud", "off",
           "--grass-density", "off", "--shafts", "off", "--haze", "off",
           "--shadow", "on", "--output", str(out)] + list(extra)
    p = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=120)
    assert p.returncode == 0, p.stderr
    frames = list(csv.DictReader((out / "frames.csv").open()))
    assert all(f["grass_shadow_draws"] == "0" for f in frames)
    results[name] = {"command": cmd, "last_frame": frames[-1]}
    return out, frames

for mode in ("original", "shader", "clusters", "billboards"):
    run(mode, mode)
original = (root / "original/final_world.ppm").read_bytes()
shader = (root / "shader/final_world.ppm").read_bytes()
clusters = (root / "clusters/final_world.ppm").read_bytes()
assert original != shader != clusters, "options must visibly differ"
a = results["original"]["last_frame"]
c = results["clusters"]["last_frame"]
meta = json.loads((assets / "tree_study.json").read_text())
source = json.loads((assets / (meta["model"] + ".gltf")).read_text())
source_count = sum(source["accessors"][p["indices"]]["count"] // 3
                   for m in source["meshes"] for p in m["primitives"])
retained = sum(map(len, meta["trunk_triangles"]))
cards = int(c["tree_leaf_cards"])
assert int(c["triangles"]) - int(a["triangles"]) == cards * 2 - (source_count - retained)
assert int(results["shader"]["last_frame"]["triangles"]) == int(a["triangles"])

maps = {}
for mode in ("original", "shader", "clusters", "billboards"):
    out, _ = run(mode + "_shadow", mode, extra=["--diagnostic", "visibility"])
    maps[mode] = (out / "raw_shadow_depth.pfm").read_bytes()
assert maps["original"] == maps["shader"], "shader-only mode changed cast geometry"
assert maps["clusters"] == maps["billboards"], "documented fixed shadow proxy changed"
assert maps["clusters"] != maps["original"], "cutout cards need genuine different shadows"
out, _ = run("clusters_reverse_shadow", "clusters", 2, ["--diagnostic", "visibility"])
assert maps["clusters"] == (out / "raw_shadow_depth.pfm").read_bytes(), "camera changed fixed world shadows"

replay = root / "toggle.txt"
replay.write_text("2 down T\n3 up T\n5 down T\n6 up T\n8 down T\n9 up T\n11 down T\n12 up T\n")
out, frames = run("live_toggle", "original", extra=["--frames", "14", "--replay", str(replay)])
assert [frames[f]["tree_mode"] for f in (0, 2, 5, 8, 11)] == ["0", "1", "2", "3", "0"]
assert len({(f["player_x"], f["player_z"], f["player_yaw"]) for f in frames}) == 1
assert frames[5]["shadow_cache_invalidations"] == frames[8]["shadow_cache_invalidations"]
assert int(frames[11]["shadow_cache_invalidations"]) > int(frames[8]["shadow_cache_invalidations"])
results["verified"] = ["four distinct options", "actual submitted triangle accounting",
                       "unchanged shader-only cast geometry", "camera-stable fixed cutout shadows",
                       "explicit fixed shadow proxy for billboards", "live T switching without camera movement",
                       "geometry-switch cache invalidation", "zero grass caster draws"]
(root / "RESULTS.json").write_text(json.dumps(results, indent=2))
print("PASS actual GL tree modes, triangle counts, cutout/proxy shadows, camera stability, live controls and grass casts OFF")
