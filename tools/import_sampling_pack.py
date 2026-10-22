"""Import explicitly supplied private RGBA sources to native-resolution experiment caches.
No source writes. Pillow is an offline tool dependency; runtime needs no PNG library.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from PIL import Image


def fnv(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return value


def prepare(config, output):
    config = Path(config).resolve()
    output = Path(output).resolve()
    if any((parent / ".git").is_file() or (parent / ".git/HEAD").is_file()
           for parent in [output, *output.parents]):
        raise ValueError("Private caches must be outside any Git checkout")
    spec = json.loads(config.read_text())
    if spec.get('schema') not in ['wildsignal-private-sampling-input-v1', 'wildsignal-private-sampling-input-v2']:
        raise ValueError('Unsupported source manifest')
    experimental = spec['schema'].endswith('-v2')
    if not 1 <= len(spec['materials']) <= 16:
        raise ValueError('Expected 1..16 material sources')
    entries = [*spec['materials'], spec['fixedProp']]
    images = []
    for index, entry in enumerate(entries):
        source = (config.parent / entry['path']).resolve()
        if output == source or output in source.parents:
            raise ValueError('Output must not contain source files')
        data = source.read_bytes()
        if hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise ValueError('Source hash mismatch')
        with Image.open(source) as image:
            image.load()
            if image.mode != 'RGBA' or list(image.size) != entry['size']:
                raise ValueError('Source must be manifest-sized RGBA')
            fixed = index == len(entries)-1
            if fixed and any(k in entry for k in ['padTo', 'contrast', 'opaqueRgb']):
                raise ValueError('Fixed approved prop cannot be transformed')
            if not fixed and image.size != (256, 256) and not (experimental and entry.get('padTo') == [256, 256]):
                raise ValueError('Non-256 material requires explicit v2 transparent padding')
            if not experimental and any(k in entry for k in ['padTo', 'contrast', 'opaqueRgb']):
                raise ValueError('Transforms require the explicit experimental v2 schema')
            if not (0 < image.width <= 1024 and 0 < image.height <= 1024):
                raise ValueError('Source dimensions outside cache bounds')
            opaque_rgb = entry.get('opaqueRgb', False)
            if type(opaque_rgb) is not bool or (opaque_rgb and image.getchannel('A').getextrema()[0] < 240):
                raise ValueError('Opaque RGB experiment requires a nearly opaque source (alpha >=240)')
            if not opaque_rgb and set(image.getchannel('A').tobytes()) - {0, 255}:
                raise ValueError('Binary alpha required by this experiment')
            if index == 0 and image.getchannel('A').getextrema() != (255, 255):
                raise ValueError('Background/floor material must be opaque')
            derived = image.copy()
            if opaque_rgb:
                derived.putalpha(255)  # Explicit material interpretation; RGB and master remain unchanged.
            if 'contrast' in entry:
                transform = entry['contrast']
                n, d, center = transform['numerator'], transform['denominator'], transform['center']
                if type(n) is not int or type(d) is not int or not 1 <= n <= d <= 256 or len(center) != 3 or any(type(c) is not int or not 0 <= c <= 255 for c in center):
                    raise ValueError('Invalid deterministic contrast transform')
                channels = derived.split()
                derived = Image.merge('RGBA', tuple(channels[c].point([(v*n+center[c]*(d-n)+d//2)//d for v in range(256)]) for c in range(3)) + (channels[3],))
            if 'padTo' in entry:
                if entry['padTo'] != [256, 256] or derived.width > 256 or derived.height > 256:
                    raise ValueError('Only transparent top-left padding into a 256 square is supported')
                canvas = Image.new('RGBA', (256, 256))
                canvas.paste(derived, (0, 0));derived = canvas
            if index == 0 and derived.getchannel('A').getextrema() != (255, 255):
                raise ValueError('Prepared background must be opaque')
            images.append(derived)
    if output.exists():
        raise FileExistsError('Use a fresh private output directory')
    output.mkdir(parents=True)
    report = dict(schema='wildsignal-sampling-import-v1', sourceManifestSha256=hashlib.sha256(config.read_bytes()).hexdigest(),
                  mapping='EXPERIMENT_ONLY: unit-square UVs, not approved facade/module/world dimensions',
                  resampling='Pillow NEAREST pixel-center sampling; originals not modified', sources=entries, caches=[],
                  experimentalTransforms='Explicit v2 only: nearly-opaque RGB interpretation, integer contrast LUT, transparent top-left padding. No geometry or final-art approval.')
    for mode in [64, 128, 256]:
        content = bytearray(b'WSTEX001' + struct.pack('<II', mode, len(images)-1))
        decoded = 0
        for index, source in enumerate(images):
            image = source if index == len(images)-1 else source.resize((mode, mode), Image.Resampling.NEAREST)
            pixels = image.tobytes()
            content.extend(struct.pack('<IIQ', image.width, image.height, fnv(pixels)))
            content.extend(pixels)
            decoded += len(pixels)
        target = output / f'sampling_{mode}.wst'
        target.write_bytes(content)
        report['caches'].append(dict(mode=mode, file=target.name, sha256=hashlib.sha256(content).hexdigest(),
                                     decodedRgbaBytes=decoded, fixedPropUnchanged=True))
    # Reverify originals after derivative generation.
    for entry in entries:
        if hashlib.sha256((config.parent / entry['path']).resolve().read_bytes()).hexdigest() != entry['sha256']:
            raise RuntimeError('Original source changed during import')
    (output / 'import_report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('source_manifest', type=Path)
    parser.add_argument('private_output', type=Path)
    args = parser.parse_args()
    report = prepare(args.source_manifest, args.private_output)
    print('PASS: source hashes/dimensions/alpha; 3 nearest-neighbor caches; native fixed prop preserved')
