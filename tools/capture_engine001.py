"""Capture actual headless smoke evidence; PNG encoding uses only the standard library.

Usage: python3 tools/capture_engine001.py build/direct/wildsignal_engine001_headless
These very short uncapped CPU samples are not the prescribed device benchmark.
"""
import json
from pathlib import Path
import struct
import subprocess
import sys
import zlib

root = Path(__file__).resolve().parents[1]
exe = Path(sys.argv[1]).resolve()
results = root / "docs/bench/results"
shots = root / "docs/bench/screenshots"
results.mkdir(parents=True, exist_ok=True)
shots.mkdir(parents=True, exist_ok=True)


def png_from_ppm(source, output):
    magic, dimensions, maximum, pixels = source.read_bytes().split(b"\n", 3)
    assert magic == b"P6" and maximum == b"255"
    w, h = map(int, dimensions.split())
    assert len(pixels) == w * h * 3

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    # Lossless encoding only: no resampling, painting or generated visual assets.
    scanlines = b"".join(b"\0" + pixels[y*w*3:(y+1)*w*3] for y in range(h))
    output.write_bytes(b"\x89PNG\r\n\x1a\n" +
                      chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                      chunk(b"IDAT", zlib.compress(scanlines, 9)) + chunk(b"IEND", b""))


for size in ("320x180", "400x225", "480x270"):
    for creatures in (1, 8):
        prefix = results / f"linux-cpu-{size}-{creatures}"
        ppm = root / f"bench/local/{prefix.name}.ppm"
        args = [str(exe), "--scene", str(root / "docs/architecture/ENGINE001_SCENE_FIXTURE_v0.1.json"),
                "--frames", "120", "--warmup", "0", "--fps", "0", "--resolution", size,
                "--output", str(prefix), "--screenshot", str(ppm)]
        if creatures == 8:
            args.append("--stress")
        subprocess.run(args, check=True, cwd=root)
        png_from_ppm(ppm, shots / f"{prefix.name}.png")
        r = json.loads(prefix.with_suffix(".json").read_text())
        print(size, creatures, "p50/p95/p99 ms:", r["medianFrameMs"], r["p95FrameMs"],
              r["p99FrameMs"], "peak RSS MiB:", r["processPeakRssMiB"], "source:", r["buildSha"])

prefix = results / "linux-cpu-near-400x225"
ppm = root / "bench/local/near.ppm"
subprocess.run([str(exe), "--scene", str(root / "docs/architecture/ENGINE001_SCENE_FIXTURE_v0.1.json"),
                "--frames", "1", "--warmup", "0", "--camera", "11.5", "14.5", "0",
                "--resolution", "400x225", "--output", str(prefix), "--screenshot", str(ppm)],
               check=True, cwd=root)
png_from_ppm(ppm, shots / "linux-cpu-near-400x225.png")
