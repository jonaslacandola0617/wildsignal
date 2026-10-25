"""Private exact-byte staging of owner-authorized Quaternius Standard archives.

Never extracts into the public worktree. Python/Pillow are offline import tools,
not runtime dependencies. No source pixel/mesh changes or automatic decimation.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import struct
import zipfile
from PIL import Image

PACKS = {
    'village': ('e60dea67c10f30dccccfbff92a7933f5ea5cfe99be0e2a0fa5118cceabeec5c4', [
        'Wall_Plaster_Straight', 'Wall_Plaster_Door_Round',
        'Wall_Plaster_Window_Wide_Flat', 'Door_1_Round',
        'DoorFrame_Round_WoodDark', 'Window_Wide_Flat1',
        'Roof_RoundTiles_4x4', 'Roof_Front_Brick4', 'Corner_Exterior_Wood',
        'Prop_Chimney', 'Prop_WoodenFence_Single', 'Floor_UnevenBrick']),
    'nature': ('298f6732b872e4cf7b30e6e7abf9641c7f6dc6b326df37ac089533ed7e3d58c9', [
        'CommonTree_1', 'CommonTree_2', 'CommonTree_3', 'Bush_Common_Flowers',
        'Flower_3_Group', 'Fern_1', 'Grass_Common_Short', 'Rock_Medium_1',
        'RockPath_Round_Thin']),
    'props': ('8b6f7e806d222e585478f0e1bdc6b271bbc7bc6f84dd6af8ca703a7c64f0cb1e', [
        'Lantern_Wall', 'Barrel', 'Bench']),
}
def digest(b):
    return hashlib.sha256(b).hexdigest()

def safe_member(base, uri):
    p = PurePosixPath(uri)
    if p.is_absolute() or '..' in p.parts or any(x in uri for x in (':', '\\', '%')):
        raise ValueError('unsafe URI')
    return str(base / p)

def accessor(d, buffers, index):
    a = d['accessors'][index]
    if a.get('sparse'):
        raise ValueError('sparse accessor unsupported')
    v = d['bufferViews'][a['bufferView']]
    n = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
    code = {5121: 'B', 5123: 'H', 5125: 'I', 5126: 'f'}[a['componentType']]
    fmt = '<' + code*n
    step = v.get('byteStride', struct.calcsize(fmt))
    offset = v.get('byteOffset', 0) + a.get('byteOffset', 0)
    b = buffers[v['buffer']]
    if a['count'] > 2_000_000 or offset + max(0,a['count']-1)*step + struct.calcsize(fmt) > v.get('byteOffset',0)+v['byteLength']:
        raise ValueError('accessor bounds')
    return [struct.unpack_from(fmt, b, offset+i*step) for i in range(a['count'])]

def prepare(archives, output):
    output = Path(output).resolve()
    repo = Path(__file__).resolve().parents[3]
    if output == repo or repo in output.parents or output.exists():
        raise ValueError('choose a NEW private output directory outside the repository')
    inventory = {'revision': 1, 'archives': [], 'models': [], 'files': []}
    payloads, models = {}, []
    for kit, source in zip(PACKS, archives):
        raw = Path(source).read_bytes()
        expected, names = PACKS[kit]
        if digest(raw) != expected:
            raise ValueError(kit+' archive SHA mismatch')
        with zipfile.ZipFile(io.BytesIO(raw)) as z:
            if z.testzip() is not None:
                raise ValueError('ZIP CRC failure')
            lic = next(n for n in z.namelist() if n.endswith('License_Standard.txt'))
            license_bytes = z.read(lic)
            if b'CC0 1.0' not in license_bytes:
                raise ValueError('expected packaged CC0 license missing')
            payloads[f'licenses/{kit}.txt'] = license_bytes
            inventory['archives'].append({'kit':kit, 'sha256':expected, 'bytes':len(raw),
                'license_member':lic, 'license_sha256':digest(license_bytes), 'license':'CC0-1.0'})
            for name in names:
                member = next(n for n in z.namelist() if n.endswith('/'+name+'.gltf') and '/glTF/' in '/'+n)
                doc_bytes = z.read(member)
                d = json.loads(doc_bytes)
                base = PurePosixPath(member).parent
                buffers = [z.read(safe_member(base,b['uri'])) for b in d['buffers']]
                primitives=[]
                for mesh in d['meshes']:
                    for p in mesh['primitives']:
                        if p.get('mode',4)!=4:
                            raise ValueError('only triangles supported')
                        v = accessor(d,buffers,p['attributes']['POSITION'])
                        ix = [x[0] for x in accessor(d,buffers,p['indices'])] if 'indices' in p else list(range(len(v)))
                        if len(ix)%3 or not ix or max(ix)>=len(v):
                            raise ValueError('triangle indices')
                        primitives.append({'vertices':len(v),'triangles':len(ix)//3,'material':p['material'],
                            'attributes':{k:{f:d['accessors'][ai].get(f) for f in ('type','componentType','count','normalized')} for k,ai in p['attributes'].items()},
                            'bounds':[list(map(min,zip(*v))),list(map(max,zip(*v)))]})
                image_records=[]
                for im in d.get('images',[]):
                    b=z.read(safe_member(base,im['uri']))
                    with Image.open(io.BytesIO(b)) as pic:
                        pic.load()
                        image_records.append({'uri':im['uri'],'sha256':digest(b),'width':pic.width,'height':pic.height,'rgba_bytes':pic.width*pic.height*4})
                files=[name+'.gltf']+[b['uri'] for b in d['buffers']]+[i['uri'] for i in d.get('images',[])]
                for f in files:
                    original=z.read(safe_member(base,f))
                    key=f'{kit}/{f}'
                    if key in payloads and payloads[key]!=original:
                        raise ValueError('conflicting shared texture')
                    payloads[key]=original
                models.append({'id':name,'path':f'{kit}/{name}.gltf'})
                inventory['models'].append({'id':name,'archive':kit,'member':member,'sha256':digest(doc_bytes),
                    'license':'CC0-1.0','primitives':primitives,'triangles':sum(x['triangles']for x in primitives),
                    'materials':d.get('materials',[]),'images':image_records,'nodes':d.get('nodes',[])})
    output.mkdir(parents=True)
    for name,b in payloads.items():
        p=output/name
        p.parent.mkdir(parents=True,exist_ok=True)
        p.write_bytes(b)
        inventory['files'].append({'path':name,'sha256':digest(b),'bytes':len(b)})
    (output/'pack.json').write_text(json.dumps({'revision':1,'models':models},indent=2))
    (output/'ASSET_INVENTORY.json').write_text(json.dumps(inventory,indent=2))
    print(f'Validated and staged {len(models)} exact-byte models; originals unchanged: {output}')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('village_zip');p.add_argument('nature_zip');p.add_argument('props_zip');p.add_argument('private_output')
    a=p.parse_args()
    prepare([a.village_zip,a.nature_zip,a.props_zip],a.private_output)
