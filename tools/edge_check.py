"""REGION-001 build 019: no empty space beyond the map edge from any edge spot.

Usage: python -I tools/edge_check.py PLAY_FOLDER EXE OUT_DIR REGION_ID [STEP] [--keep]

Stands the player just inside the walk bounds at every corner and every STEP
metres (default 16) along each edge, then turns the flat camera through all
four corners (X), capturing every 10 frames, so the turns in between are seen
too. The game runs with --mark-void, which draws any empty space beyond the
world pure magenta, and the captured world frames are searched for it.
Uses the play folder's look settings (dream look included), hidden, uncapped.
Writes OUT_DIR/edge_check.txt; exits 1 if any frame shows empty space.
--keep keeps every captured frame (otherwise only frames with void are kept).
"""
import json
import shutil
import subprocess
import sys
from pathlib import Path

INSET = 0.3  # metres inside the walk bounds; the player can stand there
FRAMES, EVERY = 170, 10
REPLAY = "30 down X\n32 up X\n70 down X\n72 up X\n110 down X\n112 up X\n150 down X\n152 up X\n170 quit x\n"


def read_ppm(path):
    data = path.read_bytes()
    parts, pos = [], 0
    while len(parts) < 4:  # P6, width, height, maxval
        while data[pos:pos + 1].isspace():
            pos += 1
        end = pos
        while not data[end:end + 1].isspace():
            end += 1
        parts.append(data[pos:end])
        pos = end
    if parts[0] != b"P6" or parts[3] != b"255":
        raise SystemExit("unexpected image " + str(path))
    return int(parts[1]), int(parts[2]), data[pos + 1:]


def void_pixels(path):
    w, h, px = read_ppm(path)
    r, g, b = px[0::3], px[1::3], px[2::3]
    return sum(1 for a, c, d in zip(r, g, b) if a >= 250 and c <= 5 and d >= 250), w * h


def spots(walk, step):
    x0, z0, x1, z1 = walk[0] + INSET, walk[1] + INSET, walk[2] - INSET, walk[3] - INSET
    out = []

    def along(a, b):
        n = max(1, int(round((b - a) / step)))
        return [a + (b - a) * k / n for k in range(n + 1)]

    for x in along(x0, x1):
        out += [(x, z0), (x, z1)]
    for z in along(z0, z1)[1:-1]:
        out += [(x0, z), (x1, z)]
    return out


def run(folder, exe, out, region, spot):
    out.mkdir(parents=True, exist_ok=True)
    (out / "save").mkdir(exist_ok=True)
    save = out / "save" / "edge.json"
    save.write_text(json.dumps({"revision": 1, "x": spot[0], "z": spot[1], "yaw": 2.3561945,
                                "pitch": 1.24749994, "hp": 30, "recruited": 0, "visited": 0}))
    shutil.copy(folder / "saves" / "cozy.json.bonds.json", out / "save" / "edge.json.bonds.json")
    (out / "replay.txt").write_text(REPLAY)
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
           "--region", region, "--mark-void", "--replay", str(out / "replay.txt"),
           "--frames", str(FRAMES), "--capture-every", str(EVERY), "--hidden", "--output", str(out)]
    r = subprocess.run(cmd, cwd=exe.parent, capture_output=True, text=True, timeout=600)
    if r.returncode:
        raise SystemExit("run failed %s\n%s\n%s" % (out, r.stdout[-2000:], r.stderr[-2000:]))
    rows = (out / "frames.csv").read_text().splitlines()
    head = rows[0].split(",")
    last = dict(zip(head, rows[-1].split(",")))
    return float(last["player_x"]), float(last["player_z"])


def main():
    args = [a for a in sys.argv[1:] if a != "--keep"]
    keep = "--keep" in sys.argv
    folder, exe, out = (Path(a).resolve() for a in args[:3])
    region_id = args[3]
    step = float(args[4]) if len(args) > 4 else 16.0
    region = json.loads((folder / "assets/regions" / region_id / "region.json").read_text())
    if "walk" in region:
        walk = (*region["walk"]["min"], *region["walk"]["max"])
    else:
        w = float(region["walk_half_extent"])
        walk = (-w, -w, w, w)
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    lines, total_void, moved = [], 0, 0
    lines.append("region %s, walk bounds x %.2f..%.2f z %.2f..%.2f, edge ring: %s" % (
        region_id, walk[0], walk[2], walk[1], walk[3], json.dumps(region.get("edge_ring", "none"))))
    for n, spot in enumerate(spots(walk, step)):
        d = out / ("spot_%02d" % n)
        px, pz = run(folder, exe, d, region_id, spot)
        frames = sorted(p for p in d.glob("frame_*_world.ppm"))
        worst, worst_frame, size = 0, "", 1
        for f in frames:
            v, size = void_pixels(f)
            if v > worst:
                worst, worst_frame = v, f.name
        total_void += worst
        off = abs(px - spot[0]) > 1 or abs(pz - spot[1]) > 1
        moved += off
        lines.append("spot %2d at (%7.2f, %7.2f) player ended (%7.2f, %7.2f)%s frames %d  void pixels max %d%s" % (
            n, spot[0], spot[1], px, pz, " MOVED" if off else "", len(frames), worst,
            " (%.2f%% in %s)" % (100.0 * worst / size, worst_frame) if worst else ""))
        print(lines[-1], flush=True)
        if not keep:
            for f in d.glob("frame_*.ppm"):
                if worst == 0 or f.name.replace("_world", "") != worst_frame.replace("_world", ""):
                    f.unlink()
            for f in d.glob("final_*.ppm"):
                f.unlink()
    lines.append("RESULT " + ("NO EMPTY SPACE SEEN" if total_void == 0 else "EMPTY SPACE SEEN") +
                 ("; %d spot(s) did not start where asked" % moved if moved else ""))
    (out / "edge_check.txt").write_text("\n".join(lines) + "\n")
    print(lines[-1])
    sys.exit(0 if total_void == 0 else 1)


if __name__ == "__main__":
    main()
