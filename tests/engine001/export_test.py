"""Black-box offline boot, telemetry quantiles/counts, CLI and fixture failure paths."""
import csv
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile

exe = Path(sys.argv[1]).resolve()
fixture = Path(sys.argv[2]).resolve()


def run(*args, ok=True):
    env = dict(os.environ, HTTP_PROXY="http://127.0.0.1:9", HTTPS_PROXY="http://127.0.0.1:9")
    p = subprocess.run([str(exe), "--scene", str(fixture), *args],
                       capture_output=True, text=True, env=env, timeout=60)
    assert (p.returncode == 0) == ok, (p.stdout, p.stderr)
    return p


with tempfile.TemporaryDirectory(prefix="engine001-") as temp:
    root = Path(temp)
    for size in ("320x180", "400x225", "480x270"):
        prefix = root / size
        screenshot = root / f"{size}.ppm"
        run("--frames", "24", "--warmup", "0", "--stress", "--resolution", size,
            "--output", str(prefix), "--screenshot", str(screenshot))
        report = json.loads(prefix.with_suffix(".json").read_text())
        with open(f"{prefix}_frames.csv", newline="") as f:
            rows = list(csv.DictReader(f))
        assert report["schema"] == "wildsignal-render-bench-v0.1"
        assert report["sampleCount"] == len(rows) == 24
        assert report["pipeline"] == "HEADLESS_CPU_ONLY"
        assert report["result"] == "MEASURED_NOT_DEVICE_VALIDATED"
        assert report["presentMsP95"] is None and report["drawUiMsP95"] is None
        assert report["androidPssMiB"] is None and report["inputLatencyMsP95"] is None
        assert report["thermalObserved"] == "not-measured"
        times = sorted(float(row["frame_ms"]) for row in rows)
        for key, q in (("medianFrameMs", .5), ("p95FrameMs", .95), ("p99FrameMs", .99)):
            assert abs(report[key] - times[math.ceil(q * len(times)) - 1]) < .000002
        assert abs(report["fps"] - 1000 * len(times) / sum(times)) < .1
        assert all(int(row["creatures"]) == 8 for row in rows)
        assert report["worldResolution"] == list(map(int, size.split("x")))
        assert sum(c["sampleCount"] for c in report["configurations"]) == len(rows)
        w, h = map(int, size.split("x"))
        data = screenshot.read_bytes()
        assert data.startswith(f"P6\n{w} {h}\n255\n".encode())
        assert len(data.split(b"\n", 3)[3]) == w * h * 3
        # Existing evidence must never be silently replaced.
        run("--frames", "1", "--output", str(prefix), ok=False)
    prefix = root / "no-capture"
    run("--frames", "2", "--warmup", "60", "--output", str(prefix))
    report = json.loads(prefix.with_suffix(".json").read_text())
    assert report["sampleCount"] == 0 and report["medianFrameMs"] is None
    assert report["fps"] is None and report["result"] == "NOT_MEASURED"
    for args in (("--frames", "-1"), ("--frames", "1.5"), ("--fps", "nan"),
                 ("--fps", "inf"), ("--resolution", "99999x99999"),
                 ("--sprites", "7"), ("--sprites", "24.1"), ("--unknown",), ("--scene",), ("--camera", "0", "0", "0")):
        run(*args, ok=False)
    damaged = root / "broken.json"
    damaged.write_text('{"rows": [')
    run("--scene", str(damaged), "--frames", "1", ok=False)
    run("--scene", str(root / "missing.json"), "--frames", "1", ok=False)
    prefix = root / "duration"
    run("--duration", ".4", "--warmup", "0", "--output", str(prefix))
    report = json.loads(prefix.with_suffix(".json").read_text())
    assert report["durationSeconds"] >= .4
    # Test a complete short paced walkthrough, so the collision/input path also runs.
    prefix = root / "paced"
    run("--frames", "12", "--fps", "60", "--walkthrough", "--warmup", "0", "--output", str(prefix))
    report = json.loads(prefix.with_suffix(".json").read_text())
    assert report["sampleCount"] == 12 and report["walkthrough"] is True
    assert report["durationSeconds"] >= .15
print("PASS: T008 JSON/CSV, quantiles, unknown fields; T009 offline boot with dead proxies; CLI/fixture failures")
