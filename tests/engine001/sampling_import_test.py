"""Synthetic engineering inputs only; no private art is fetched or bundled in CI."""
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from PIL import Image

exe = Path(sys.argv[1]).resolve()
root = Path(sys.argv[2]).resolve()
spec = importlib.util.spec_from_file_location('sampling_importer', root / 'tools/import_sampling_pack.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as temp:
    temp = Path(temp)
    image = Image.new('RGBA', (256, 256))
    image.putdata([(x, y, (x ^ y), 255) for y in range(256) for x in range(256)])
    image.save(temp / 'source.png')
    lamp = Image.new('RGBA', (16, 64), (17, 203, 29, 255));lamp.putpixel((0, 0), (0, 0, 0, 0))
    lamp.save(temp / 'fixed.png')
    def entry(name, size):
        return dict(id=name, path=name + '.png', size=size, sha256=hashlib.sha256((temp / (name + '.png')).read_bytes()).hexdigest())
    manifest = dict(schema='wildsignal-private-sampling-input-v1', materials=[entry('source', [256, 256])], fixedProp=entry('fixed', [16, 64]))
    config = temp / 'sources.json';config.write_text(json.dumps(manifest))
    module.prepare(config, temp / 'caches')
    for mode in [64, 128, 256]:
        cache = temp / f'caches/sampling_{mode}.wst';data = cache.read_bytes()
        assert data[:8] == b'WSTEX001'
        width, height, digest = struct.unpack_from('<IIQ', data, 16)
        expected = image.resize((mode, mode), Image.Resampling.NEAREST).tobytes()
        assert (width, height) == (mode, mode) and data[32:32+len(expected)] == expected
        assert digest == module.fnv(expected)
        # The same decoded prop bytes at all three resolutions.
        assert data[-16*64*4:] == lamp.tobytes()
        prefix = temp / f'run_{mode}'
        subprocess.run([str(exe), '--sample-pack', str(cache), '--frames', '6', '--warmup', '0', '--output', str(prefix)], check=True)
        report = json.loads(prefix.with_suffix('.json').read_text())
        assert report['textureSamplingResolution'] == mode
        assert report['externalTextureCacheBytes'] == mode*mode*4 + 16*64*4
        assert report['zone'] == 'VIS002_CALIBRATION_001' and report['sampleCount'] == 6
        assert report['configurations'][0]['creatures'] == 0
        corrupt = bytearray(data);corrupt[-1] ^= 1;(temp / 'corrupt.wst').write_bytes(corrupt)
        failed = subprocess.run([str(exe), '--sample-pack', str(temp / 'corrupt.wst')], capture_output=True)
        assert failed.returncode != 0
        for payload in [data[:20], data + b'x', b'WSTEX001' + struct.pack('<II', mode, 1000)]:
            (temp / 'bad.wst').write_bytes(payload)
            assert subprocess.run([str(exe), '--sample-pack', str(temp / 'bad.wst')], capture_output=True).returncode != 0
    if '--native' in sys.argv:
        native = Path(sys.argv[sys.argv.index('--native')+1]).resolve()
        import os
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        for mode in [64, 128, 256]:
            subprocess.run([str(native), '--sample-pack', str(temp/f'caches/sampling_{mode}.wst'), '--frames', '12', '--fps', '0', '--warmup', '0', '--output', str(temp/f'native_{mode}')], env=env, check=True)
    assert hashlib.sha256((temp/'source.png').read_bytes()).hexdigest() == manifest['materials'][0]['sha256']
    manifest['materials'][0]['sha256'] = '0'*64;config.write_text(json.dumps(manifest))
    try:
        module.prepare(config, temp / 'invalid')
    except ValueError:
        pass
    else:
        raise AssertionError('Changed source accepted')
print('PASS: exact nearest derivatives, native fixed prop, metadata, malformed/cache checksum/source hash rejection')
