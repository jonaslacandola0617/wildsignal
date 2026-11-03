"""REGION-001 parity: the village from region files must render exactly as before.

Usage: python -I tools/region_parity.py PLAY_FOLDER BASE_EXE NEW_EXE OUT_DIR [REGION_ID]

For each start spot, runs BASE_EXE (old files) and NEW_EXE twice (old files,
then --region) with the same fixed-step replay: stand, quarter turn, walk.
Captured frames are compared byte for byte, plus a BASE vs BASE control run
that shows the capture itself is repeatable. Writes OUT_DIR/parity.txt.
"""
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

# Spread over all four chunks, on and near the chunk borders (x = 0, z = 0).
SPOTS = [(-1, 8), (-4.4, -4.4), (0.3, 0.4), (10, -10), (-12, 12), (20, 20),
         (-20, -20), (15, 0.2), (-0.2, -15), (-25, 5), (5, 25), (25, -25)]
YAW = 2.3561945
REPLAY = """# stand, capture, quarter turn, walk
30 down X
32 up X
70 down W
100 up W
110 quit x
"""
FRAMES, EVERY = 110, 20


def run(exe, folder, out, spot, region):
    out.mkdir(parents=True, exist_ok=True)
    saves = out / "save"
    saves.mkdir(exist_ok=True)
    save = saves / "spot.json"
    save.write_text(json.dumps({"revision": 1, "x": spot[0], "z": spot[1], "yaw": YAW,
                                "pitch": 1.24749994, "hp": 30, "recruited": 0, "visited": 0}))
    for extra in ("cozy.json.bonds.json",):
        shutil.copy(folder / "saves" / extra, saves / ("spot.json" + extra[len("cozy.json"):]))
    replay = out / "replay.txt"
    replay.write_text(REPLAY)
    cmd = [str(exe), "--assets", str(folder / "assets"), "--save", str(save),
           "--camera", "raised", "--style-preset", "balanced", "--ink", "balanced",
           "--pacing", "precise", "--cap", "0", "--grass-density", "high", "--highlights", "on",
           "--outlines", "on", "--vegetation-edges", "on", "--haze", "off", "--shafts", "off",
           "--quality", "medium", "--air-proof", "bounded", "--grass-lod", "on",
           "--meadow-patches", "on", "--meadow-lighting", "on", "--meadow-response", "beam",
           "--grass-outlines", "off", "--shaft-profile", "defined", "--shadow", "on",
           "--shadow-policy", "clean", "--grass-cast", "off", "--hud", "off",
           "--pilot", str(folder / "assets/pilot/pilot_warm3d.json"),
           "--dream", str(folder / "assets/pilot/pilot_dream.json"),
           "--replay", str(replay), "--frames", str(FRAMES), "--capture-every", str(EVERY),
           "--hidden", "--output", str(out)]
    if region:
        cmd += ["--region", region]
    r = subprocess.run(cmd, cwd=exe.parent, capture_output=True, text=True, timeout=600)
    if r.returncode:
        raise SystemExit("run failed %s\n%s\n%s" % (out, r.stdout[-2000:], r.stderr[-2000:]))
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.glob("frame_*.ppm"))}


def main():
    folder, base, new, out = (Path(a).resolve() for a in sys.argv[1:5])
    region = sys.argv[5] if len(sys.argv) > 5 else "hearthgrove_test"
    lines, ok = [], True
    for n, spot in enumerate(SPOTS):
        runs = {
            "base": run(base, folder, out / ("%02d_base" % n), spot, None),
            "control": run(base, folder, out / ("%02d_control" % n), spot, None),
            "new_old_files": run(new, folder, out / ("%02d_new_old" % n), spot, None),
            "new_region": run(new, folder, out / ("%02d_new_region" % n), spot, region),
        }
        if not runs["base"]:
            raise SystemExit("no frames captured")
        same = {k: v == runs["base"] for k, v in runs.items() if k != "base"}
        ok &= all(same.values())
        lines.append("spot %2d %-14s frames=%d %s" % (n, spot, len(runs["base"]),
                     " ".join("%s=%s" % (k, "SAME" if v else "DIFF") for k, v in same.items())))
        print(lines[-1], flush=True)
    lines.append("RESULT " + ("ALL FRAMES BYTE-IDENTICAL" if ok else "DIFFERENCES FOUND"))
    (out / "parity.txt").write_text("\n".join(lines) + "\n")
    print(lines[-1])
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
