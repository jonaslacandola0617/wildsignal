"""Stage private, scene-used Quaternius models and auditable texture derivatives.

Original glTF/bin/PNG/ZIP files are never edited. Only opaque textures with
verified constant alpha may shrink. Alpha-sensitive foliage retains native pixels.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import numpy as np
from PIL import Image

SMALL = {'T_RockTrim_BaseColor.png','T_Brick_BaseColor.png',
         'T_UnevenBrick_BaseColor.png','Rocks_Diffuse.png',
         'T_Trim_Metal_BaseColor.png','T_Trim_Furniture_BaseColor.png'}
def digest(data): return hashlib.sha256(data).hexdigest()
def inside(root,name):
    p=(root/name).resolve()
    if root not in p.parents: raise ValueError('unsafe path')
    return p
def reduced(im,limit):
    # Linear-light area filtering, only on opaque, integer power-of-two inputs.
    w,h=im.size
    factor=max(w,h)//limit
    if factor<2:return im
    if w%factor or h%factor:raise ValueError('noninteger reduction')
    rgba=np.asarray(im,dtype=np.float32)/255
    rgb=rgba[:,:,:3]
    linear=np.where(rgb<=.04045,rgb/12.92,((rgb+.055)/1.055)**2.4)
    linear=linear.reshape(h//factor,factor,w//factor,factor,3).mean(axis=(1,3))
    srgb=np.where(linear<=.0031308,linear*12.92,1.055*linear**(1/2.4)-.055)
    a=np.full((h//factor,w//factor,1),255,dtype=np.uint8)
    return Image.fromarray(np.concatenate((np.clip(np.rint(srgb*255),0,255).astype(np.uint8),a),axis=2))
def build(source,scene,output,profile):
    source=source.resolve();output=output.resolve()
    repo=Path(__file__).resolve().parents[3]
    if output.exists()or output==repo or repo in output.parents:raise ValueError('new private output required')
    pack=json.loads((source/'pack.json').read_text());fixture=json.loads(scene.read_text())
    inventory=json.loads((source/'ASSET_INVENTORY.json').read_text())
    for f in inventory['files']:
        if digest(inside(source,f['path']).read_bytes())!=f['sha256']:raise ValueError('source hash mismatch')
    used={i['model']for i in fixture['instances']}
    if 'ground'in used:used.add('Grass_Common_Short')
    selected=[m for m in pack['models']if m['id']in used]
    missing=used-{'ground'}-{m['id']for m in selected}
    if missing:raise ValueError(f'missing models {missing}')
    output.mkdir(parents=True)
    overrides=[];textures={};models=[];source_files=[]
    for m in selected:
        p=inside(source,m['path']);d=json.loads(p.read_text());base=p.parent
        for f in [p]+[inside(base,b['uri'])for b in d['buffers']]:
            rel=f.relative_to(source);dest=output/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,dest)
            source_files.append({'path':str(rel),'sha256':digest(f.read_bytes())})
        models.append(m)
        for material in d['materials']:
            t=material.get('pbrMetallicRoughness',{}).get('baseColorTexture')
            if not t:continue
            image=d['images'][d['textures'][t['index']]['source']]['uri'];png=inside(base,image);key=str(png.relative_to(source))
            if key in textures:continue
            im=Image.open(png).convert('RGBA');alpha=im.getchannel('A').getextrema()
            limit=512 if png.name in SMALL else 1024
            resized=profile=='balanced' and max(im.size)>limit and alpha==(255,255)
            derived=reduced(im,limit)if resized else im
            pixel_hash=digest(struct.pack('<II',*derived.size)+derived.tobytes())
            rel=f'textures/{pixel_hash}.png';dest=output/rel
            if not dest.exists():
                dest.parent.mkdir(exist_ok=True)
                if resized:derived.save(dest)
                else:shutil.copy2(png,dest)
            overrides.append({'source':key,'file':rel})
            textures[key]={'source':key,'source_sha256':digest(png.read_bytes()),
                'source_size':list(im.size),'file':rel,'file_sha256':digest(dest.read_bytes()),
                'size':list(derived.size),'rgba_bytes':derived.width*derived.height*4,
                'alpha_extrema':list(alpha),'resized':resized,'profile':profile,
                'method':'linear-light box / opaque only'if resized else'exact source bytes',
                'decoded_sha256':pixel_hash}
    shutil.copytree(source/'licenses',output/'licenses')
    out={'revision':1,'models':models,'texture_overrides':overrides}
    (output/'pack.json').write_text(json.dumps(out,indent=2))
    unique={x['file']:x for x in textures.values()}
    mip_bytes=0
    for x in unique.values():
        w,h=x['size']
        while True:
            mip_bytes+=w*h*4
            if w==h==1:break
            w=max(1,w//2);h=max(1,h//2)
    report={'profile':profile,'source_pack':str(source),'scene':str(scene),
        'models':len(models),'original_model_bytes_unchanged':source_files,
        'textures':list(textures.values()),'unique_textures':len(unique),
        'base_bytes':sum(x['rgba_bytes']for x in unique.values()),'mip_bytes':mip_bytes,
        'source_files_verified':len(inventory['files']),
        'omitted':'Unreferenced model and unsupported normal/roughness images; originals remain in source pack.',
        'license':'Quaternius CC0-1.0, actual notices copied unchanged'}
    (output/'OPTIMIZATION.json').write_text(json.dumps(report,indent=2))
    print(json.dumps({k:report[k]for k in ('profile','models','unique_textures','base_bytes','mip_bytes')}))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('scene',type=Path);p.add_argument('output',type=Path);p.add_argument('--profile',choices=['native','balanced'],default='balanced');a=p.parse_args();build(a.source,a.scene,a.output,a.profile)
