"""Deterministic Wordline OBJ/MTL -> GLB conversion, run by Blender 4.3.2.

blender -b --python convert_wordline.py -- upstream verified.json output
Authored OBJ/MTL bytes remain outside Git and unchanged. No substitute meshes,
texture painting, UV atlas, smoothing or geometry decimation is performed.
"""
import argparse, collections, hashlib, json, math, struct, sys
from pathlib import Path

ASSETS = [
 ('RURAL_05','WORLDLINE_ASSET_PACK_03A_80/rural_landmarks/RURAL_05.obj'),
 ('PLANT_01','WORLDLINE_ASSET_PACK_03A_80/plants/PLANT_01.obj'),
 ('PLANT_03','WORLDLINE_ASSET_PACK_03A_80/plants/PLANT_03.obj'),
 ('PLANT_05','WORLDLINE_ASSET_PACK_03A_80/plants/PLANT_05.obj'),
 ('TREE_01','WORLDLINE_LAB_ASSET_PACK_46/models/trees/TREE_01.obj'),
 ('TREE_06','WORLDLINE_LAB_ASSET_PACK_46/models/trees/TREE_06.obj'),
 ('TERRAIN_01','WORLDLINE_ASSET_PACK_03A_80/terrain/TERRAIN_01.obj')]

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def mtl(path):
    result={}; current=None
    for line in path.read_text().splitlines():
        s=line.split('#',1)[0].split()
        if not s: continue
        if s[0]=='newmtl':
            current=s[1]; result[current]={'Kd':[.8]*3,'Ke':[0.]*3,'d':1.}
        elif current and s[0] in ('Kd','Ke'): result[current][s[0]]=list(map(float,s[1:4]))
        elif current and s[0]=='d': result[current]['d']=float(s[1])
        elif current and s[0]=='Tr': result[current]['d']=1-float(s[1])
        elif s[0].lower().startswith('map_'): raise ValueError('External textures not authorized by this flat-color converter')
    for name,mat in result.items():
        values=mat['Kd']+mat['Ke']+[mat['d']]
        if any(not math.isfinite(v) or v<0 or v>1 for v in values): raise ValueError('MTL color/opacity range: '+name)
    return result

def obj(path):
    vertices=[]; normals=[]; groups=collections.OrderedDict(); group='default'; material=None
    def index(s,length):
        i=int(s); i=i-1 if i>0 else length+i
        if i<0 or i>=length: raise ValueError('OBJ index')
        return i
    for line in path.read_text().splitlines():
        s=line.split('#',1)[0].split()
        if not s: continue
        if s[0]=='v': vertices.append(tuple(map(float,s[1:4])))
        elif s[0]=='vn': normals.append(tuple(map(float,s[1:4])))
        elif s[0]=='g': group=' '.join(s[1:]) or 'default'
        elif s[0]=='usemtl': material=s[1]
        elif s[0]=='f':
            if len(s)!=4 or not material: raise ValueError('Only authored material-assigned triangles accepted')
            corners=[]
            for token in s[1:]:
                fields=token.split('/')
                if len(fields)!=3 or not fields[2]: raise ValueError('Authored normals required')
                corners.append((index(fields[0],len(vertices)),index(fields[2],len(normals))))
            groups.setdefault(group,[]).append((material,corners))
    if not vertices or not groups: raise ValueError('Empty source')
    if any(not math.isfinite(x) for v in vertices+normals for x in v): raise ValueError('Nonfinite source')
    return vertices,normals,groups

def unpack_glb(path):
    raw=path.read_bytes(); magic,version,length=struct.unpack_from('<III',raw)
    if magic!=0x46546c67 or version!=2 or length!=len(raw): raise ValueError('GLB header')
    at=12; document=None; binary=b''
    while at<len(raw):
        n,kind=struct.unpack_from('<II',raw,at); at+=8; data=raw[at:at+n]; at+=n
        if kind==0x4e4f534a: document=json.loads(data)
        elif kind==0x004e4942: binary=data
    return document,binary

def write_glb(path,document,binary):
    text=json.dumps(document,separators=(',',':'),ensure_ascii=True).encode(); text+=b' '*((-len(text))%4)
    binary+=b'\0'*((-len(binary))%4)
    path.write_bytes(struct.pack('<III',0x46546c67,2,12+8+len(text)+8+len(binary))+struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(binary),0x004e4942)+binary)

def accessor(doc,binary,index):
    a=doc['accessors'][index]; v=doc['bufferViews'][a['bufferView']]
    types={5121:('B',1),5123:('H',2),5125:('I',4),5126:('f',4)}
    f,size=types[a['componentType']]; n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
    start=v.get('byteOffset',0)+a.get('byteOffset',0); stride=v.get('byteStride',size*n)
    return [struct.unpack_from('<'+f*n,binary,start+i*stride) for i in range(a['count'])]

def external_oracle(directory,name,doc,binary):
    # Original PR26 importer requires external buffers + TEXCOORD_0. Add only
    # constant zero UVs to this separate parity oracle; these flat materials
    # never sample them. Real review models stay actual embedded-buffer GLB.
    d=json.loads(json.dumps(doc)); data=bytearray(binary[:d['buffers'][0]['byteLength']])
    for mesh in d['meshes']:
        for p in mesh['primitives']:
            if 'TEXCOORD_0' in p['attributes']: continue
            count=d['accessors'][p['attributes']['POSITION']]['count']
            data+=b'\0'*((-len(data))%4); offset=len(data); data+=b'\0'*(count*8)
            d.setdefault('bufferViews',[]).append({'buffer':0,'byteOffset':offset,'byteLength':count*8})
            d['accessors'].append({'bufferView':len(d['bufferViews'])-1,'componentType':5126,'count':count,'type':'VEC2'})
            p['attributes']['TEXCOORD_0']=len(d['accessors'])-1
    d['buffers']=[{'uri':name+'.bin','byteLength':len(data)}]
    (directory/(name+'.bin')).write_bytes(data)
    (directory/(name+'.gltf')).write_text(json.dumps(d,separators=(',',':'))+'\n')

def restore_authored_normals(d,b,vertices,normals,groups):
    """Blender quantizes custom split normals internally. Match each exported
    oriented triangle to its source, restore the authored normal and split its
    corners. No source triangle is added, removed, rewound or smoothed.
    """
    def key(coords):
        q=[tuple(round(v,4) for v in p) for p in coords]
        return min(tuple(q[i:]+q[:i]) for i in range(3))
    source={}
    for group,faces in groups.items():
        table=collections.defaultdict(collections.deque)
        for mat,corners in faces:
            coords=[vertices[v] for v,n in corners]
            ns=[normals[n] for v,n in corners]
            if max(abs(x-y) for n in ns[1:] for x,y in zip(ns[0],n))>1e-6: raise ValueError('Expected authored flat triangle normals')
            length=math.sqrt(sum(x*x for x in ns[0]))
            if length<1e-8: raise ValueError('Zero authored normal')
            table[(mat,key(coords))].append(tuple(x/length for x in ns[0]))
        source[group]=table
    blob=bytearray(); views=[]; accesses=[]
    def add(values,fmt,kind,component,minimum=None,maximum=None):
        blob.extend(b'\0'*((-len(blob))%4)); start=len(blob)
        for v in values: blob.extend(struct.pack('<'+fmt,*v))
        views.append({'buffer':0,'byteOffset':start,'byteLength':len(blob)-start})
        a={'bufferView':len(views)-1,'componentType':component,'count':len(values),'type':kind}
        if minimum is not None: a.update(min=minimum,max=maximum)
        accesses.append(a); return len(accesses)-1
    for mesh in d['meshes']:
        table=source[mesh['name']]
        for p in mesh['primitives']:
            old_positions=accessor(d,b,p['attributes']['POSITION']); ix=accessor(d,b,p['indices'])
            material=d['materials'][p['material']]['name']; ps=[]; ns=[]
            for at in range(0,len(ix),3):
                coords=[old_positions[ix[at+i][0]] for i in range(3)]
                normal=table[(material,key(coords))].popleft()
                ps.extend(coords);ns.extend([normal]*3)
            lo=[min(v[i] for v in ps) for i in range(3)]; hi=[max(v[i] for v in ps) for i in range(3)]
            p['attributes']={'POSITION':add(ps,'3f','VEC3',5126,lo,hi),'NORMAL':add(ns,'3f','VEC3',5126)}
            p['indices']=add([(i,) for i in range(len(ps))],'I','SCALAR',5125)
    if any(queue for table in source.values() for queue in table.values()): raise ValueError('Source triangle lost')
    d['accessors']=accesses;d['bufferViews']=views;d['buffers']=[{'byteLength':len(blob)}]
    return bytes(blob)

def convert(source,path,destination):
    import bpy
    vertices,normals,groups=obj(source); materials=mtl(source.with_suffix('.mtl'))
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
    for block in list(bpy.data.meshes): bpy.data.meshes.remove(block)
    for block in list(bpy.data.materials): bpy.data.materials.remove(block)
    mats={}
    for name,value in materials.items():
        mat=bpy.data.materials.new(name); mat.use_nodes=True
        bs=mat.node_tree.nodes.get('Principled BSDF'); bs.inputs['Base Color'].default_value=(*value['Kd'],value['d'])
        bs.inputs['Metallic'].default_value=0; bs.inputs['Roughness'].default_value=1
        bs.inputs['Alpha'].default_value=value['d']
        bs.inputs['Emission Color'].default_value=(*value['Ke'],1)
        bs.inputs['Emission Strength'].default_value=1
        mats[name]=mat
    # Rotation X+90 from authored Y-up to Blender Z-up. glTF Y-up export
    # applies its inverse. No heading correction or pivot rebasing is baked.
    def blender_axis(v): return (v[0],-v[2],v[1])
    for group,faces in groups.items():
        coords=[]; triangles=[]; loop_normals=[]; face_material=[]
        # Split triangle corners to preserve exact authored per-face normals.
        for material,corners in faces:
            if material not in mats: raise ValueError('Unknown MTL '+material)
            start=len(coords)
            for vi,ni in corners:
                coords.append(blender_axis(vertices[vi])); loop_normals.append(blender_axis(normals[ni]))
            triangles.append((start,start+1,start+2)); face_material.append(material)
        mesh=bpy.data.meshes.new(group); mesh.from_pydata(coords,[],triangles); mesh.update()
        used=list(dict.fromkeys(face_material))
        for name in used: mesh.materials.append(mats[name])
        for poly,name in zip(mesh.polygons,face_material): poly.material_index=used.index(name); poly.use_smooth=True
        mesh.normals_split_custom_set(loop_normals)
        ob=bpy.data.objects.new(group,mesh); bpy.context.collection.objects.link(ob)
    bpy.ops.export_scene.gltf(filepath=str(destination),export_format='GLB',export_yup=True,export_normals=True,export_materials='EXPORT',export_animations=False,export_cameras=False,export_lights=False,export_extras=False)
    d,b=unpack_glb(destination)
    # Deterministically express authored Kd/Ke/d directly in flat PBR fields.
    # Treat Kd as linear, matching the existing renderer; no color baking.
    for m in d['materials']:
        value=materials[m['name']]; m.clear(); m['name']=next(k for k,v in materials.items() if v is value)
        m['pbrMetallicRoughness']={'baseColorFactor':value['Kd']+[value['d']],'metallicFactor':0,'roughnessFactor':1}
        m['emissiveFactor']=value['Ke']; m['alphaMode']='BLEND' if value['d']<1 else 'OPAQUE'; m['doubleSided']=False
    d.pop('extensionsUsed',None); d.pop('extensionsRequired',None)
    if d.get('textures') or d.get('images'): raise ValueError('Unexpected texture generation')
    b=restore_authored_normals(d,b,vertices,normals,groups)
    positions=[]; triangles=0; converted_groups={}
    for mesh in d['meshes']:
        total=0
        for p in mesh['primitives']:
            pos=accessor(d,b,p['attributes']['POSITION']); ns=accessor(d,b,p['attributes']['NORMAL'])
            ix=accessor(d,b,p['indices']); triangles+=len(ix)//3; total+=len(ix)//3; positions+=pos
            for k in range(0,len(ix),3):
                nn=[ns[ix[k+j][0]] for j in range(3)]
                if any(max(abs(nn[0][c]-n[c]) for c in range(3))>2e-4 for n in nn[1:]): raise ValueError('Normals were smoothed')
        converted_groups[mesh['name']]=total
    if triangles!=sum(len(x) for x in groups.values()): raise ValueError('Triangle count changed')
    bounds=lambda a:[[min(v[i] for v in a) for i in range(3)],[max(v[i] for v in a) for i in range(3)]]
    if any(abs(x-y)>3e-4 for aa,bb in zip(bounds(vertices),bounds(positions)) for x,y in zip(aa,bb)): raise ValueError('Axis/bounds changed')
    write_glb(destination,d,b); external_oracle(destination.parent/'oracle',path.stem,d,b)
    return {'id':path.stem,'source_obj':str(path),'source_obj_sha256':sha(source),'source_mtl_sha256':sha(source.with_suffix('.mtl')),'glb_sha256':sha(destination),'triangles':triangles,'bounds':bounds(vertices),'dimensions':[bounds(vertices)[1][i]-bounds(vertices)[0][i] for i in range(3)],'source_groups':{k:len(v) for k,v in groups.items()},'converted_groups':converted_groups,'materials':materials,'normal_policy':'authored flat normals restored by oriented triangle matching after Blender custom-normal quantization; no smoothing','axis':'Y-up source preserved; no pivot/heading changes'}

def main():
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('verified',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);a.output.mkdir(parents=True,exist_ok=False);(a.output/'oracle').mkdir()
    verified=json.loads(a.verified.read_text())
    for f in verified['files']:
        if sha(a.source/f['relative'])!=f['sha256']: raise ValueError('Source hash mismatch '+f['relative'])
    records=[]
    for name,path in ASSETS:
        records.append(convert(a.source/path,Path(path),a.output/(name+'.glb')))
    import bpy
    manifest={'upstream_repository':verified['repository'],'upstream_commit':verified['commit'],'blender':bpy.app.version_string,'rights':'Owner-declared own Wordline assets; issue27 authorizes this private experiment. No redistributable license inferred.','color_mapping':'MTL Kd treated as linear RGB -> baseColorFactor; Ke -> emissiveFactor; d/Tr -> alpha','records':records}
    (a.output/'CONVERSION.json').write_text(json.dumps(manifest,indent=2)+'\n')
    # Explicitly labelled engineering-only two-triangle ground, no art atlas.
    ground={'asset':{'version':'2.0','generator':'HOLLAND001 engineering test quad'},'scene':0,'scenes':[{'nodes':[0]}],'nodes':[{'mesh':0}],
      'buffers':[{'byteLength':108}],
      'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':48},{'buffer':0,'byteOffset':48,'byteLength':48},{'buffer':0,'byteOffset':96,'byteLength':12}],
      'accessors':[{'bufferView':0,'componentType':5126,'count':4,'type':'VEC3','min':[-18,-.04,-18],'max':[18,-.04,18]},
       {'bufferView':1,'componentType':5126,'count':4,'type':'VEC3'},{'bufferView':2,'componentType':5123,'count':6,'type':'SCALAR'}],
      'materials':[{'name':'ENGINEERING_GROUND_NOT_PRODUCTION_ART','pbrMetallicRoughness':{'baseColorFactor':[.19,.36,.105,1],'metallicFactor':0,'roughnessFactor':1}}],
      'meshes':[{'name':'ENGINEERING_GROUND','primitives':[{'attributes':{'POSITION':0,'NORMAL':1},'indices':2,'material':0}]}]}
    gb=struct.pack('<12f',-18,-.04,-18,-18,-.04,18,18,-.04,18,18,-.04,-18)+struct.pack('<12f',*([0,1,0]*4))+struct.pack('<6H',0,1,2,0,2,3)
    write_glb(a.output/'TEST_GROUND.glb',ground,gb); external_oracle(a.output/'oracle','TEST_GROUND',ground,gb)
    manifest['engineering_ground']={'file':'TEST_GROUND.glb','triangles':2,'not_source_art':True,'sha256':sha(a.output/'TEST_GROUND.glb')}
    (a.output/'CONVERSION.json').write_text(json.dumps(manifest,indent=2)+'\n')
    pack={'revision':1,'models':[{'id':r['id'],'path':r['id']+'.glb'} for r in records]+[{'id':'TEST_GROUND','path':'TEST_GROUND.glb'}]}
    (a.output/'pack.json').write_text(json.dumps(pack,indent=2)+'\n')
    pack['models']=[{'id':r['id'],'path':r['id']+'.gltf'} for r in records]+[{'id':'TEST_GROUND','path':'TEST_GROUND.gltf'}]
    (a.output/'oracle'/'pack.json').write_text(json.dumps(pack,indent=2)+'\n')
    print('CONVERTED',len(records),'verified source models',sum(r['triangles'] for r in records),'triangles')
if __name__=='__main__':main()
