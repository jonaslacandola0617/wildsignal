"""Reproducible CPU-only scene matrix; --standard adds three 60s+180s runs.
No SDL present, target-device, thermal or input-latency claims are made.
"""
import argparse
import csv
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('executable', type=Path)
p.add_argument('output', type=Path)
p.add_argument('--standard', action='store_true')
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
rows = []
for scene in ['echo', 'quay', 'forest']:
    for resolution in ['320x180', '400x225', '480x270']:
        for actors in [1, 8, 24]:
            prefix = a.output / f'{scene}_{resolution}_{actors}'
            cmd = [str(a.executable.resolve()), '--scene', scene, '--resolution', resolution,
                   '--sprites', str(actors), '--frames', '60', '--warmup', '0',
                   '--output', str(prefix)]
            if resolution == '320x180' and actors == 8:
                cmd += ['--screenshot', str(a.output / f'{scene}.ppm')]
            subprocess.run(cmd, check=True, timeout=60)
            r = json.loads(prefix.with_suffix('.json').read_text())
            with open(str(prefix) + '_frames.csv', newline='') as f:
                frames = list(csv.DictReader(f))
            assert len(frames) == r['sampleCount'] == 60
            assert r['zone'] == {'echo': 'BENCH_ECHO_Z04_MICRO_001', 'quay': 'BENCH_Z01_QUAY_001', 'forest': 'BENCH_Z03_LILTWOOD_001'}[scene]
            assert all(int(s['creatures']) == actors for s in frames)
            assert all(0 <= int(s['actors_visible']) <= actors for s in frames)
            rows.append(dict(scene=scene, resolution=resolution, submittedActors=actors,
                             visibleActorsMedian=r['actorsVisibleP50'], medianMs=r['medianFrameMs'],
                             p95Ms=r['p95FrameMs'], p99Ms=r['p99FrameMs'], peakRssMiB=r['processPeakRssMiB'],
                             worldP95Ms=r['drawWorldMsP95'], spritesP95Ms=r['drawSpritesMsP95']))
with open(a.output / 'matrix.csv', 'w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=rows[0]);writer.writeheader();writer.writerows(rows)
if a.standard:
    for scene in ['echo', 'quay', 'forest']:
        subprocess.run([str(a.executable.resolve()), '--scene', scene, '--resolution', '480x270',
                        '--sprites', '24', '--warmup', '60', '--duration', '180', '--fps', '30',
                        '--walkthrough', '--output', str(a.output / f'{scene}_standard')],
                       check=True, timeout=270)
print('PASS: 27 scene/resolution/population exports; CPU-only evidence')
