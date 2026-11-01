"""Actual GL regression: unchanged pixels across camera, book and shadow changes.
Requires the existing private pilot assets; does not contain or generate art.
"""
import csv
import json
import statistics
import subprocess
import sys
from pathlib import Path

exe, assets, output = map(lambda p: Path(p).resolve(), sys.argv[1:4])
output.mkdir(parents=True, exist_ok=True)
replay = output / "render.txt"
replay.write_text("""5 down b
7 up b
20 down b
22 up b
25 down w
55 up w
35 mouse -18 0
60 down l
62 up l
90 down o
92 up o
120 down o
122 up o
135 down c
137 up c
150 mouse 12 0
""")
results = {}
for mode in ["reference", "optimized"]:
    dest = output / mode
    cmd = [str(exe), "--assets", str(assets), "--pilot", str(assets / "pilot/pilot_warm3d.json"),
           "--camera", "raised", "--save", str(output / (mode + ".json")), "--output", str(dest),
           "--new-game", "--allow-software", "--hidden", "--frames", "180", "--capture-every", "30",
           "--replay", str(replay), "--cap", "0", "--pacing", "off", "--hud", "off",
           "--shafts", "off", "--haze", "off", "--grass-density", "high"]
    if mode == "reference":
        cmd += ["--render-reference"]
    subprocess.run(cmd, check=True)
    rows = list(csv.DictReader(open(dest / "frames.csv")))[30:]
    results[mode] = {key: statistics.mean(float(row[key]) for row in rows)
                     for key in ["draws", "shadow_draws", "shadow_triangles", "cpu_submit_ms", "app_loop_ms"]}
images = sorted((output / "reference").glob("*.ppm"))
assert len(images) == 13, "Missing captures"
for image in images:
    assert image.read_bytes() == (output / "optimized" / image.name).read_bytes(), image.name
assert results["optimized"]["shadow_draws"] < results["reference"]["shadow_draws"], results
(output / "comparison.json").write_text(json.dumps(results, indent=2))
print("13 captures identical; moving casters, camera, book, shadow on/off and quality resize passed")
