"""Black-box facade calibration checks using only synthetic rectangles, no private art."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

exe = Path(sys.argv[1]).resolve()
variants = {'current': (1, 1), 'a': (1.5, 1.4), 'b': (1.8, 1.7),
            'c': (2.1, 2), 'human': (3.2, 2.4)}


def fnv(data):
    h = 14695981039346656037
    for b in data:
        h = ((h ^ b) * 1099511628211) & ((1 << 64) - 1)
    return h


with tempfile.TemporaryDirectory(prefix='facade-test-') as temp:
    root = Path(temp)
    layout = dict(schema='wildsignal-facade-calibration-v1', frameMaterial=1,
                  doorMaterial=3, windowMaterial=4, doorAnchorU=.5, doorAnchorV=1,
                  windowAnchorU=.5, windowAnchorV=.5)
    config = root / 'layout.json';config.write_text(json.dumps(layout))
    for mode in [64, 128, 256]:
        data = bytearray(b'WSTEX001' + struct.pack('<II', mode, 5))
        for slot in range(6):
            w, h = (16, 64) if slot == 5 else (mode, mode)
            pixels = bytearray()
            for y in range(h):
                for x in range(w):
                    color = (100, 100, 100, 255)
                    if slot == 1: color = (60, 40, 20, 255)
                    if slot in [3, 4]:
                        opaque = w//4 <= x < 3*w//4 and h//4 <= y < (h if slot == 3 else 3*h//4)
                        color = ((220, 20, 20, 255) if slot == 3 else (20, 220, 20, 255)) if opaque else (0, 0, 0, 0)
                    if slot == 5: color = (250, 200, 10, 255)
                    pixels.extend(color)
            data.extend(struct.pack('<IIQ', w, h, fnv(pixels)));data.extend(pixels)
        (root/f'sampling_{mode}.wst').write_bytes(data)

    serial = 0
    def run(executable, mode, variant, projection='orthographic', size='480x270', extra=(), ok=True):
        global serial
        serial += 1
        prefix = root/f'run_{serial}'
        args = [str(executable), '--sample-pack', str(root/f'sampling_{mode}.wst'),
                '--facade-layout', str(config), '--facade-scale', variant,
                '--facade-projection', projection, '--resolution', size,
                '--frames', '2', '--warmup', '0', '--fps', '0', '--output', str(prefix),
                '--screenshot', str(prefix.with_suffix('.ppm')), *extra]
        result = subprocess.run(args, capture_output=True, text=True, timeout=30,
                                env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'))
        assert (result.returncode == 0) == ok, result.stdout + result.stderr
        if not ok: return
        report = json.loads(prefix.with_suffix('.json').read_text())
        assert report['facadeScaleVariant'] == variant
        assert (report['facadeDoorScale'], report['facadeWindowScale']) == variants[variant]
        assert report['projection'] == projection and report['sampleCount'] == 2
        assert report['zone'] == 'VIS002_SCALE_CALIBRATION_001'
        return prefix.with_suffix('.ppm').read_bytes()

    for projection in ['orthographic', 'perspective']:
        previous = 0
        for variant in variants:
            reference = run(exe, 64, variant, projection)
            assert run(exe, 128, variant, projection) == reference
            assert run(exe, 256, variant, projection) == reference
            raw = reference.split(b'\n', 3)[3]
            door = sum(raw[i:i+3] == bytes([220, 20, 20]) for i in range(0, len(raw), 3))
            assert door > previous;previous = door
    for size in ['320x180', '400x225']:
        run(exe, 256, 'human', size=size)
    for overrides in [dict(doorMaterial=1), dict(windowMaterial=999), dict(frameMaterial=1.5),
                      dict(doorAnchorV=1.1), dict(windowAnchorU=-1), dict(schema='unknown')]:
        config.write_text(json.dumps(layout | overrides))
        run(exe, 64, 'current', ok=False)
    config.write_text('{')
    run(exe, 64, 'current', ok=False)
    config.write_text(json.dumps(layout))
    run(exe, 64, 'invalid', ok=False)
    run(exe, 64, 'current', projection='invalid', ok=False)
    for incomplete in [['--facade-scale', 'a'], ['--facade-projection', 'orthographic'],
                       ['--facade-layout', str(config)]]:
        assert subprocess.run([str(exe), *incomplete], capture_output=True).returncode != 0
    if '--native' in sys.argv:
        native = Path(sys.argv[sys.argv.index('--native')+1]).resolve()
        for projection in ['orthographic', 'perspective']:
            for variant in variants:
                run(native, 256, variant, projection)
print('PASS: facade presets, sampling/projection invariance, increasing silhouettes, telemetry, malformed layout/CLI; native boots when requested')
