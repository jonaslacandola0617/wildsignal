"""Capture actual CPU-engine frames privately. No artwork generation or source writes.

Requires Pillow for lossless screenshot conversion/contact sheets only.
Frame timings are short headless screening, not target-device acceptance.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw


def capture(exe, packs, layout, output, frames=360):
    exe, packs, layout, output = (Path(p).resolve() for p in (exe, packs, layout, output))
    if frames < 61:
        raise ValueError('At least 61 frames required; the first 60 are excluded in the summary')
    if any((p/'.git').is_file() or (p/'.git/HEAD').is_file() for p in [output, *output.parents]):
        raise ValueError('Private screenshots must stay outside Git checkouts')
    if output.exists():
        raise FileExistsError('Use a fresh private output directory')
    inputs = [exe, layout, *(packs/f'sampling_{m}.wst' for m in [64, 128, 256])]
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    output.mkdir(parents=True)
    variants = {'current': (1, 1), 'a': (1.5, 1.4), 'b': (1.8, 1.7),
                'c': (2.1, 2), 'human': (3.2, 2.4)}
    records = []

    def shot(name, variant, mode, projection, camera, resolution='480x270', count=frames, original=False):
        prefix = output/name
        args = [str(exe), '--sample-pack', str(packs/f'sampling_{mode}.wst'), '--camera', *map(str, camera),
                '--resolution', resolution, '--warmup', '0', '--fps', '0', '--frames', str(count),
                '--output', str(prefix), '--screenshot', str(prefix.with_suffix('.ppm'))]
        if not original:
            args += ['--facade-layout', str(layout), '--facade-scale', variant, '--facade-projection', projection]
        subprocess.run(args, check=True, stdout=subprocess.DEVNULL, timeout=120)
        with Image.open(prefix.with_suffix('.ppm')) as im:
            im.save(prefix.with_suffix('.png'))
        report = json.loads(prefix.with_suffix('.json').read_text())
        if report['pipeline'] != 'HEADLESS_CPU_ONLY':
            raise ValueError('This controlled capture helper requires the headless executable; use F12 for SDL display shots')
        record = dict(name=name, variant=variant, projection=projection, sampling=mode,
                      camera=camera, worldResolution=resolution, engineReport=prefix.name+'.json',
                      screenshotSha256=hashlib.sha256(prefix.with_suffix('.ppm').read_bytes()).hexdigest())
        if count > 60:
            with Path(str(prefix)+'_frames.csv').open(newline='') as stream:
                times = sorted(float(row['frame_ms']) for row in list(csv.DictReader(stream))[60:])
            record.update(screeningFrames=len(times), medianMs=times[math.ceil(.5*len(times))-1],
                          p95Ms=times[math.ceil(.95*len(times))-1], p99Ms=times[math.ceil(.99*len(times))-1],
                          peakRssMiB=report['processPeakRssMiB'])
        records.append(record)
        return prefix.with_suffix('.png')

    for mode in [64, 128, 256]:
        for pose, projection, camera in [('ortho', 'orthographic', [12, 16.5, 0]),
                                         ('front', 'perspective', [12, 16.5, 0]),
                                         ('corner', 'perspective', [17, 14.5, -45])]:
            images = []
            for variant, factors in variants.items():
                name = f'{pose}_{mode}_{variant}'
                images.append(shot(name, variant, mode, projection, camera))
                print(f'Captured {name}', flush=True)
            # Labelled assembly of actual screenshots. Each world pixel is copied at 2x nearest.
            sheet = Image.new('RGB', (960, 5*568), (18, 28, 36));draw = ImageDraw.Draw(sheet)
            for row, ((variant, factors), path) in enumerate(zip(variants.items(), images)):
                draw.text((8, row*568+6), f'{pose} / {mode} / {variant}: door {factors[0]}x, window {factors[1]}x / CPU ENGINE', fill=(246, 235, 215))
                with Image.open(path) as image:
                    sheet.paste(image.resize((960, 540), Image.Resampling.NEAREST), (0, row*568+28))
            sheet.save(output/f'comparison_{pose}_{mode}.png')
        for size in ['320x180', '400x225']:
            shot(f'gameplay_{mode}_{size}_human', 'human', mode, 'perspective', [12, 16.5, 0], size)
        shot(f'original_sampling_{mode}', 'current', mode, 'perspective', [12, 16.5, 0], original=True)

    turns = []
    for i in range(25):
        turns.append(shot(f'turn_{i:02}', 'human', 256, 'perspective', [12, 16.5, -12+i], count=1))
    sequence = []
    for path in turns:
        with Image.open(path) as image:
            sequence.append(image.copy())
    sequence[0].save(output/'human_engine_turn.apng', format='PNG', save_all=True,
                     append_images=sequence[1:], duration=80, loop=0, disposal=0, blend=0)
    for path, expected in hashes.items():
        if hashlib.sha256(Path(path).read_bytes()).hexdigest() != expected:
            raise RuntimeError('An input changed during capture')
    summary = dict(schema='wildsignal-facade-captures-v1', inputsSha256=hashes,
                   timing='HEADLESS_CPU_ONLY: first 60 frames omitted from summary; all raw frames retained. Short shared-host screening, no SDL/device validation.',
                   factors=variants, records=records)
    (output/'capture_manifest.json').write_text(json.dumps(summary, indent=2)+'\n')
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', required=True)
    parser.add_argument('--packs', required=True)
    parser.add_argument('--layout', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--frames', type=int, default=360)
    args = parser.parse_args()
    capture(args.exe, args.packs, args.layout, args.output, args.frames)
