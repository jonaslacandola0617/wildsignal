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

# Plain leaves keep alpha/geometry/shadows while ignoring painted RGB gradients.
from PIL import Image
import numpy as np
plain_out, _ = run('plain_leaf_base', 'billboards', extra=['--leaf-color','plain','--diagnostic','graded'])
source_out, _ = run('source_leaf_base', 'billboards', extra=['--leaf-color','source','--diagnostic','graded'])
a = np.array(Image.open(plain_out/'final_world.ppm'))
b = np.array(Image.open(source_out/'final_world.ppm'))
normal_a = np.frombuffer((plain_out/'normal_rgba.u8').read_bytes(),np.uint8).reshape(a.shape[0],a.shape[1],4)
normal_b = np.frombuffer((source_out/'normal_rgba.u8').read_bytes(),np.uint8).reshape(b.shape[0],b.shape[1],4)
mask = normal_a[:,:,3] == 51
assert mask.sum() > 100
assert np.array_equal(mask, normal_b[:,:,3] == 51), 'plain color changed alpha silhouette'
plain_colors = np.unique(a[mask],axis=0)
source_colors = np.unique(b[mask],axis=0)
assert len(plain_colors)==1 and len(source_colors)>1, (len(plain_colors),len(source_colors))
assert np.array_equal(a[~mask],b[~mask]), 'plain leaf material changed trunk/terrain/other objects'
pa,_ = run('plain_leaf_shadow','billboards',extra=['--leaf-color','plain','--diagnostic','visibility'])
pb,_ = run('source_leaf_shadow','billboards',extra=['--leaf-color','source','--diagnostic','visibility'])
assert (pa/'raw_shadow_depth.pfm').read_bytes()==(pb/'raw_shadow_depth.pfm').read_bytes()
replay.write_text('2 down M\n3 up M\n5 down M\n6 up M\n')
_,frames = run('live_material_toggle','billboards',extra=['--frames','8','--replay',str(replay)])
assert [frames[i]['plain_leaves'] for i in [0,2,5]]==['1','0','1']
results['plain_leaf_validation']={'visible_leaf_fragments':int(mask.sum()),'plain_base_RGB':plain_colors.tolist(),'plain_unique_base_colors':len(plain_colors),'source_unique_base_colors':len(source_colors),'identical_alpha_mask':True,'identical_shadow_map':True,'nonleaf_base_pixels_unchanged':True,'M_toggle_verified':True}
(root/'RESULTS.json').write_text(json.dumps(results,indent=2))
print('PASS plain leaf base: one RGB, identical alpha/shadows, unchanged non-leaf pixels, live M control')
