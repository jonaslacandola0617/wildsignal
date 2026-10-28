"""Public synthetic geometry only. Tests real C++ import/profile/error behavior."""
import json,struct,subprocess,sys,tempfile,copy
from pathlib import Path
exe=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='wildsignal-vegetation-')as tmp:
 r=Path(tmp);(r/'mesh.bin').write_bytes(struct.pack('<18f',0,0,0,.2,0,0,0,.3,0,*([0,0,1]*3)))
 model=dict(asset=dict(version='2.0'),scene=0,scenes=[dict(nodes=[0])],nodes=[dict(mesh=0)],meshes=[dict(primitives=[dict(attributes=dict(POSITION=0,NORMAL=1),material=0,mode=4)])],materials=[dict(doubleSided=True,pbrMetallicRoughness=dict(baseColorFactor=[1,1,1,1],metallicFactor=0,roughnessFactor=1))],buffers=[dict(uri='mesh.bin',byteLength=72)],bufferViews=[dict(buffer=0,byteOffset=0,byteLength=36),dict(buffer=0,byteOffset=36,byteLength=36)],accessors=[dict(bufferView=0,componentType=5126,count=3,type='VEC3',min=[0,0,0],max=[.2,.3,0]),dict(bufferView=1,componentType=5126,count=3,type='VEC3')])
 (r/'mesh.gltf').write_text(json.dumps(model));(r/'ground.png').write_bytes(b'CPU path fixture; never decoded as an image');(r/'ids.png').write_bytes(b'CPU path fixture')
 roots=[dict(variant=0,family=i,position=[x,y,z],normal=[0,1,0],yaw=0,width=.88,height=.82)for i,(x,y,z)in enumerate([(-.4,.12,0),(.4,.05,0),(.9,0,.7)])]
 cfg=dict(revision=1,ground='ground.png',patch_ids='ids.png',sun_tint=.4,shadow_floor=.18,models=[dict(near='mesh.gltf',far='mesh.gltf')],families=[['#335544','#557755','#99aa66']for _ in range(4)],instances=roots)
 def run(c,success):
  (r/'vegetation.json').write_text(json.dumps(c));p=subprocess.run([str(exe),str(r)],capture_output=True,text=True);assert(p.returncode==0)==success,(p.stdout,p.stderr,c);return p
 print(run(cfg,True).stdout.strip());chunk=copy.deepcopy(cfg);chunk.pop('instances');chunk['instance_files']=['roots.json'];(r/'roots.json').write_text(json.dumps(roots));run(chunk,True)
 bad=[]
 c=copy.deepcopy(cfg);c['instances'][0]['variant']=.5;bad.append(c)
 c=copy.deepcopy(cfg);c['instances'][0]['family']=5;bad.append(c)
 c=copy.deepcopy(cfg);c['instances'][0]['position'][0]=7;bad.append(c)
 c=copy.deepcopy(cfg);c['instances'][0]['normal']=[0,0,0];bad.append(c)
 c=copy.deepcopy(cfg);c['families'][0][0]='#23GG44';bad.append(c)
 c=copy.deepcopy(cfg);c['models']*=33;bad.append(c)
 c=copy.deepcopy(chunk);c['instance_files']*=65;bad.append(c)
 c=copy.deepcopy(cfg);c['models'][0]['near']='../escape.gltf';bad.append(c)
 c=copy.deepcopy(cfg);c['shadow_floor']=.9;bad.append(c)
 for c in bad:run(c,False)
 print('PASS chunked metadata and9invalid descriptors rejected; no legacy grass source files provided')
