"""Offline private import: verified original ZIP + visually reviewed private crop catalog.
No redraw/rescale. Runtime caches preserve decoded RGBA and partial alpha exactly.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import zipfile
from PIL import Image


def fnv(data):
    value=14695981039346656037
    for b in data:value=((value^b)*1099511628211)&((1<<64)-1)
    return value


def prepare(archive,catalog,output):
    archive,catalog,output=[Path(p).resolve() for p in [archive,catalog,output]]
    if output.exists():raise FileExistsError('Fresh private directory required')
    if any((p/'.git').is_file() or (p/'.git/HEAD').is_file() for p in [output,*output.parents]):raise ValueError('Private pixels must stay outside Git')
    spec=json.loads(catalog.read_text());raw=archive.read_bytes()
    if spec['schema']!='wildsignal-topdown001-catalog-v1':raise ValueError('Unsupported catalog')
    if hashlib.sha256(raw).hexdigest()!=spec['zipSha256']:raise ValueError('Original ZIP checksum mismatch')
    images=[];sources=[];names=set()
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        terms=z.read(spec['terms'])
        if hashlib.sha256(terms).hexdigest()!=spec['termsSha256']:raise ValueError('Terms checksum mismatch')
        for entry in spec['images']:
            if entry['id'] in names:raise ValueError('Duplicate crop ID')
            names.add(entry['id']);data=z.read(entry['source'])
            if hashlib.sha256(data).hexdigest()!=entry['sha256']:raise ValueError('Source PNG checksum mismatch')
            with Image.open(io.BytesIO(data)) as source:
                if source.width>2048 or source.height>2048:raise ValueError('Source dimensions exceed budget')
                source=source.convert('RGBA');x,y,w,h=entry['rect']
                if any(type(v) is not int for v in [x,y,w,h]) or min(x,y)<0 or min(w,h)<1 or max(w,h)>1024 or x+w>source.width or y+h>source.height:raise ValueError('Crop outside original sheet')
                cropped=source.crop((x,y,x+w,y+h));pixels=cropped.tobytes()
                images.append((entry['id'],w,h,pixels));sources.append(entry|dict(originalSize=list(source.size),decodedRgbaSha256=hashlib.sha256(pixels).hexdigest()))
    if not 1<=len(images)<=128 or sum(len(i[3]) for i in images)>12*1024*1024:raise ValueError('Runtime pixel budget exceeded')
    output.mkdir(parents=True);manifest=dict(schema='wildsignal-topdown001-pack-v1',images=[],player=spec['player'],companion=spec['companion'])
    for i,(name,w,h,pixels) in enumerate(images):
        file=f'{i:03}.rgba';(output/file).write_bytes(b'TDIMG001'+struct.pack('<IIQ',w,h,fnv(pixels))+pixels);manifest['images'].append(dict(id=name,file=file))
    (output/'pack.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (output/'Terms.txt').write_bytes(terms)
    report=dict(sourceZipSha256=spec['zipSha256'],catalogSha256=hashlib.sha256(catalog.read_bytes()).hexdigest(),sources=sources,rgbaBytes=sum(len(i[3]) for i in images),pixelEdits='NONE; integer native-size crops only; originals unchanged',companion='Temporary licensed pack actor, not canonical creature')
    (output/'import_report.json').write_text(json.dumps(report,indent=2)+'\n')
    assert hashlib.sha256(archive.read_bytes()).hexdigest()==spec['zipSha256']
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('zip');p.add_argument('catalog');p.add_argument('private_output');a=p.parse_args()
    result=prepare(a.zip,a.catalog,a.private_output);print('PASS: verified source ZIP/terms/PNG hashes, native RGBA crops;',result['rgbaBytes'],'pixel bytes')
