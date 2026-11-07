"""REGION-001 walk test: walk a long recorded path through a region and report
frame times around chunk changes, memory and streaming counts.

Usage: python -I tools/stream_walk.py PLAY_FOLDER EXE OUT_DIR REGION_ID [X Z]

Uses the play folder's assets and look settings with the real 30 FPS cap.
The walk holds W for 40 s, turning a quarter every 10 s (X), so it crosses
many chunk borders. "Work" is a frame's time minus the wait for the cap.
Frames over 50 ms of work count as hitches. Writes OUT_DIR/walk.txt.
"""
import csv
import json
import re
import shutil
import statistics as st
import subprocess
import sys
from pathlib import Path

FRAMES = 1260
REPLAY = "10 down W\n300 down X\n302 up X\n600 down X\n602 up X\n900 down Z\n902 up Z\n1210 up W\n"


def main():
    folder, exe, out = (Path(a).resolve() for a in sys.argv[1:4])
    region = sys.argv[4]
    x, z = (float(sys.argv[5]), float(sys.argv[6])) if len(sys.argv) > 6 else (-1.0, 8.0)
    if out.exists():
        shutil.rmtree(out)
    (out / "save").mkdir(parents=True)
    save = out / "save" / "walk.json"
    save.write_text(json.dumps({"revision": 1, "x": x, "z": z, "yaw": 2.3561945, "pitch": 1.24749994,
                                "hp": 30, "recruited": 0, "visited": 0}))
    shutil.copy(folder / "saves" / "cozy.json.bonds.json", out / "save" / "walk.json.bonds.json")
    (out / "replay.txt").write_text(REPLAY)
    cmd = [str(exe), "--assets", str(folder / "assets"), "--save", str(save),
           "--camera", "raised", "--style-preset", "balanced", "--ink", "balanced",
           "--pacing", "precise", "--cap", "30", "--grass-density", "high", "--highlights", "on",
           "--outlines", "on", "--vegetation-edges", "on", "--haze", "off", "--shafts", "off",
           "--quality", "medium", "--air-proof", "bounded", "--grass-lod", "on",
           "--meadow-patches", "on", "--meadow-lighting", "on", "--meadow-response", "beam",
           "--grass-outlines", "off", "--shaft-profile", "defined", "--shadow", "on",
           "--shadow-policy", "clean", "--grass-cast", "off", "--hud", "off",
           "--pilot", str(folder / "assets/pilot/pilot_warm3d.json"),
           "--dream", str(folder / "assets/pilot/pilot_dream.json"),
           "--region", region, "--replay", str(out / "replay.txt"), "--frames", str(FRAMES),
           "--hidden", "--output", str(out / "run")]
    r = subprocess.run(cmd, cwd=exe.parent, capture_output=True, text=True, timeout=900)
    if r.returncode:
        raise SystemExit("run failed\n" + r.stdout[-2000:] + r.stderr[-2000:])
    rows = list(csv.DictReader(open(out / "run" / "frames.csv")))[30:]
    gpu = {int(g["frame"]): float(g["gpu_elapsed_ms"]) for g in csv.DictReader(open(out / "run" / "gpu.csv"))}
    changes = [int(l.split(",")[0]) for l in open(out / "run" / "events.csv") if ",chunks_changed," in l]
    work = [(int(f["frame"]), float(f["app_loop_ms"]) - float(f["pacing_ms"])) for f in rows]
    w = sorted(v for _, v in work)
    near = lambda f: any(0 <= f - c <= 40 for c in changes)
    hitches = [(f, v) for f, v in work if v > 50]
    g = sorted(v for k, v in gpu.items() if k >= 30)
    log = (out / "run" / "run.log").read_text()
    lines = [
        "region %s, start (%.1f, %.1f), %d frames at the 30 FPS cap" % (region, x, z, len(rows)),
        "walk end at (%s, %s)" % (rows[-1]["player_x"], rows[-1]["player_z"]),
        "frame work ms: median %.1f  p95 %.1f  p99 %.1f  max %.1f" % (
            st.median(w), w[int(len(w) * .95)], w[int(len(w) * .99)], w[-1]),
        "GPU ms: median %.1f  p95 %.1f  max %.1f" % (st.median(g), g[int(len(g) * .95)], g[-1]) if g else "GPU ms: n/a",
        "chunk changes: %d; hitches (>50 ms work): %d, of which within 40 frames of a change: %d" % (
            len(changes), len(hitches), sum(near(f) for f, _ in hitches)),
        "worst frames: " + ", ".join("%d:%.0fms%s" % (f, v, "*" if near(f) else "") for f, v in
                                     sorted(work, key=lambda t: -t[1])[:6]) + "   (* = near a chunk change)",
        "peak memory: %.0f MB" % (max(int(f["peak_rss_bytes"]) for f in rows) / 2 ** 20),
    ]
    for key in ("region_load_ms", "region_start_models", "stream_chunk_loads", "stream_chunk_unloads",
                "stream_model_loads", "stream_model_frees", "stream_models_resident", "stream_max_read_ms",
                "stream_max_upload_ms", "stream_max_activate_ms", "stream_max_deactivate_ms",
                "stream_max_release_ms", "stream_max_step_ms", "stream_max_drop_ms", "stream_max_grass_ms",
                "stream_max_collect_ms", "texture_bytes_end", "gpu_buffer_bytes_end"):
        m = re.search(key + r"=(\S+)", log)
        lines.append("%s = %s" % (key, m.group(1) if m else "?"))
    (out / "walk.txt").write_text("\n".join(lines) + "\n")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
