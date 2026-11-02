"""Synthetic code-only importer tests; no licensed model/texture bytes."""
import copy
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

def fixture(root):
    v=[(-1.,0.,0.),(1.,0.,0.),(0.,2.,0.)]
    b=b''.join(struct.pack('<3f',*p)for p in v)+struct.pack('<9f',*(0.,0.,1.)*3)+struct.pack('<6f',0,0,1,0,.5,1)+struct.pack('<3H',0,1,2)
    (root/'mesh.bin').write_bytes(b)
    d={'asset':{'version':'2.0'},'scene':0,'scenes':[{'nodes':[0]}],
       'nodes':[{'mesh':0,'translation':[1,2,3],'scale':[2,1,1]}],
       'buffers':[{'uri':'mesh.bin','byteLength':len(b)}],
       'bufferViews':[{'buffer':0,'byteOffset':o,'byteLength':n}for o,n in [(0,36),(36,36),(72,24),(96,6)]],
       'accessors':[{'bufferView':i,'componentType':5123 if i==3 else 5126,'count':3,'type':['VEC3','VEC3','VEC2','SCALAR'][i]}for i in range(4)],
       'materials':[{'pbrMetallicRoughness':{'baseColorFactor':[.8,.2,.1,1]}}],
       'meshes':[{'primitives':[{'attributes':{'POSITION':0,'NORMAL':1,'TEXCOORD_0':2},'indices':3,'material':0}]}]}
    (root/'mesh.gltf').write_text(json.dumps(d))
    return d,b

def run(exe,root,d,ok=True):
    (root/'mesh.gltf').write_text(json.dumps(d))
    r=subprocess.run([exe,str(root/'mesh.gltf')],capture_output=True,text=True,timeout=20)
    assert (r.returncode==0)==ok,(r.stdout,r.stderr)
    return json.loads(r.stdout)if ok else None

def main(exe):
    with tempfile.TemporaryDirectory()as tmp:
        root=Path(tmp);d,b=fixture(root)
        got=run(exe,root,d)
        assert got['triangles']==1 and got['bounds']==[[-1,2,3],[3,4,3]]
        # Normalized unsigned COLOR_0 is a base-color multiplier, not a
        # disposable editor channel. Preserve three/four components correctly.
        x=copy.deepcopy(d);col=struct.pack('<12H',*([32768,16384,65535,65535]*3))
        (root/'mesh.bin').write_bytes(b+col);x['buffers'][0]['byteLength']=len(b+col)
        x['bufferViews'].append({'buffer':0,'byteOffset':len(b),'byteLength':len(col)})
        x['accessors'].append({'bufferView':4,'componentType':5123,'normalized':True,'count':3,'type':'VEC4'})
        x['meshes'][0]['primitives'][0]['attributes']['COLOR_0']=4
        got=run(exe,root,x);assert abs(got['vertex_color_min_r']-.500008)<.00001 and abs(got['vertex_color_max_g']-.250004)<.00001
        bad_color=copy.deepcopy(x);bad_color['accessors'][4]['count']=2;run(exe,root,bad_color,False)
        (root/'mesh.bin').write_bytes(b)
        # A child inherits its parent's transform; reflected transforms remain valid.
        x=copy.deepcopy(d);x['nodes'].append({'translation':[2,0,0],'children':[0]});x['scenes'][0]['nodes']=[1]
        assert run(exe,root,x)['bounds']==[[1,2,3],[5,4,3]]
        x=copy.deepcopy(d);x['nodes'][0]['scale']=[-2,1,1];assert run(exe,root,x)['bounds']==[[-1,2,3],[3,4,3]]
        cases=[]
        def bad(change):
            x=copy.deepcopy(d);change(x);cases.append(x)
        bad(lambda x:x['meshes'][0]['primitives'][0].update(mode=5))
        bad(lambda x:x['meshes'][0]['primitives'][0].update(material=17))
        bad(lambda x:x['meshes'][0]['primitives'][0]['attributes'].pop('NORMAL'))
        bad(lambda x:x['buffers'][0].update(uri='../mesh.bin'))
        bad(lambda x:x['buffers'][0].update(uri='https://example.com/model.bin'))
        bad(lambda x:x['buffers'][0].update(byteLength=100000))
        bad(lambda x:x['accessors'][0].update(count=999999999))
        bad(lambda x:x['nodes'][0].update(scale=[0,1,1]))
        bad(lambda x:x['materials'][0].update(extensions={'KHR_materials_unlit':{}}))
        bad(lambda x:x.update(extensionsRequired=['unrecognized_extension']))
        bad(lambda x:x['accessors'][2].update(type='VEC3'))
        for x in cases:run(exe,root,x,False)
        (root/'mesh.bin').write_bytes(b[:-6]+struct.pack('<3H',0,1,17));run(exe,root,d,False)
        (root/'mesh.bin').write_bytes(struct.pack('<f',float('nan'))+b[4:]);run(exe,root,d,False)
        (root/'mesh.bin').write_bytes(b)
        # Unused entries must not import unavailable models; used entries and
        # unsafe/missing texture overrides still fail closed.
        pack={'revision':1,'models':[{'id':'used','path':'mesh.gltf'}, {'id':'unused','path':'absent.gltf'}]}
        scene={'revision':1,'instances':[{'id':'one','model':'used','position':[0,0,0],'yaw':0,'scale':1}]}
        (root/'mesh.gltf').write_text(json.dumps(d))
        (root/'scene.json').write_text(json.dumps(scene))
        def world(p,ok):
            (root/'pack.json').write_text(json.dumps(p))
            result=subprocess.run([exe,str(root/'pack.json'),str(root/'scene.json')],capture_output=True,text=True,timeout=20)
            assert (result.returncode==0)==ok,(result.stdout,result.stderr)
            return json.loads(result.stdout) if ok else None
        assert world(pack,True)['models']==1
        for path in ('../outside.png','missing.png'):
            x=copy.deepcopy(pack);x['texture_overrides']=[{'source':'color.png','file':path}];world(x,False)
        x=copy.deepcopy(pack);x['models'][0]['path']='absent.gltf';world(x,False)
        for file,kind in [('../secret.png',1),('missing.png',1),('mesh.bin',7)]:
            x=copy.deepcopy(pack);x['texture_styles']=[{'source':'color.png','file':file,'kind':kind}];world(x,False)
        print('PASS: real cgltf importer transforms/materials/topology/indices/URI/schema/NaN rejection (synthetic only)')

if __name__=='__main__': main(sys.argv[1])
