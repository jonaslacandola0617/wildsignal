"""Real cgltf embedded GLB and flat untextured validation, no private assets."""
import copy,importlib.util,json,struct,subprocess,sys,tempfile
from pathlib import Path
def main(exe):
 spec=importlib.util.spec_from_file_location('fixture',Path(__file__).with_name('import.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 with tempfile.TemporaryDirectory() as tmp:
  root=Path(tmp);d,b=m.fixture(root)
  def run(doc,blob,ok=True):
   text=json.dumps(doc).encode();text+=b' '*((-len(text))%4);blob+=b'\0'*((-len(blob))%4)
   raw=struct.pack('<III',0x46546c67,2,12+8+len(text)+8+len(blob))+struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(blob),0x004e4942)+blob
   p=root/'test.glb';p.write_bytes(raw)
   r=subprocess.run([exe,str(p)],capture_output=True,text=True,timeout=20)
   assert (r.returncode==0)==ok,(r.stdout,r.stderr)
   return json.loads(r.stdout)if ok else None
  d['buffers'][0].pop('uri');d['meshes'][0]['primitives'][0]['attributes'].pop('TEXCOORD_0')
  got=run(d,b);assert got['triangles']==1 and got['bounds']==[[-1,2,3],[3,4,3]]
  # All source Wordline materials are untextured; UV absence is valid only then.
  bad=copy.deepcopy(d);bad['images']=[{'uri':'color.png'}];bad['textures']=[{'source':0}];bad['materials'][0]['pbrMetallicRoughness']['baseColorTexture']={'index':0};run(bad,b,False)
  bad=copy.deepcopy(d);bad['buffers'][0]['byteLength']+=16;run(bad,b,False)
  bad=copy.deepcopy(d);bad['materials'][0]['emissiveFactor']=[float('nan'),0,0];run(bad,b,False)
  valid=copy.deepcopy(d);valid['materials'][0]['emissiveFactor']=[.1,.2,.3];run(valid,b)
  bad=copy.deepcopy(d);bad['buffers'][0]['uri']='../escape.bin';run(bad,b,False)
  print('PASS embedded GLB, no-UV flat material, emission, invalid size/UV/NaN/traversal rejection')
if __name__=='__main__':main(sys.argv[1])
