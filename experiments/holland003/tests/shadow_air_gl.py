"""Real GL shadow/geometry separation and conservative near-occluder controls."""
import json,os,struct,subprocess,sys,tempfile,importlib.util
from pathlib import Path
import numpy as np
from PIL import Image

def main(exe):
 old=Path(__file__).parents[2]/'holland001/tests/import.py';spec=importlib.util.spec_from_file_location('fixture',old);f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f)
 env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','offscreen')
 with tempfile.TemporaryDirectory()as tmp:
  r=Path(tmp);env.setdefault('MESA_SHADER_CACHE_DIR',str(r/'mesa'));d,b=f.fixture(r);d['nodes']=[{'mesh':0}];d['materials'][0]['doubleSided']=True;d['meshes'][0]['name']='roof';(r/'mesh.gltf').write_text(json.dumps(d))
  (r/'pack.json').write_text(json.dumps({'revision':1,'models':[{'id':'RURAL_05','path':'mesh.gltf'}]}));(r/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'proof','model':'RURAL_05','position':[0,0,0],'yaw':0,'scale':1}]}))
  def run(name,flags):
   out=r/name;cmd=[exe,'--pack',str(r/'pack.json'),'--scene',str(r/'scene.json'),'--look',str(Path(__file__).parents[1]/'data/TOON_DIAGNOSTIC.json'),'--study','B1','--style','C','--no-art-materials','--no-smooth-lighting','--resolution','480','--pose','1','--frames','3','--warmup','0','--cap','0','--hidden','--allow-software','--shadow-proof','stable','--edge-mode','combined','--debug-effects','--output',str(out)]+flags
   v=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=90);assert v.returncode==0,v.stderr
   return out,np.array(Image.open(out/'final.ppm'))
  a,lit=run('cached',['--shadow','cached']);b0,live=run('live',['--shadow','live']);assert np.array_equal(lit,live),'static cache differs from live'
  for mode,flags in [('zero',['--bias-scale','0']),('high',['--bias-scale','4']),('hard',['--hard-shadow'])]:
   out,_=run(mode,['--shadow','cached']+flags);assert(a/'raw_shadow_depth.pfm').read_bytes()==(out/'raw_shadow_depth.pfm').read_bytes(),'receiver bias/filter changed caster geometry'
  roof,_=run('roof',['--shadow','cached','--casters','roof']);vegetation,_=run('vegetation',['--shadow','cached','--casters','vegetation'])
  data=np.frombuffer((roof/'raw_shadow_depth.pfm').read_bytes().split(b'\n',3)[3],dtype='<f4');assert(data<.9999).any(),'Wordline roof group filter empty'
  empty=np.frombuffer((vegetation/'raw_shadow_depth.pfm').read_bytes().split(b'\n',3)[3],dtype='<f4');assert(empty==1).all(),'caster category excludes building incorrectly'
  # A stationary world light map must not follow the moving view camera.
  moved,_=run('moved',['--shadow','cached','--route','strafe']);assert(a/'raw_shadow_depth.pfm').read_bytes()==(moved/'raw_shadow_depth.pfm').read_bytes()
  _,normal_off=run('normal_off',['--diagnostic','normal','--shadow','off']);_,normal_on=run('normal_on',['--diagnostic','normal','--shadow','cached']);assert np.array_equal(normal_off,normal_on),'shadow altered actual normals/geometry'
  # Near solid wall: no marchable air, including conservative low-resolution
  # reconstruction. No sky/occluder radiance may leak through it.
  pos=[(-100.,-100.,12.8),(100.,-100.,12.8),(0.,100.,12.8)];raw=b''.join(struct.pack('<3f',*v)for v in pos)+b[36:];(r/'mesh.bin').write_bytes(raw)
  _,off=run('wall_off',['--pose','0','--shadow','cached','--sunlight-profile','off'])
  for profile in ['low','standard']:
   out,on=run('wall_'+profile,['--pose','0','--shadow','cached','--sunlight-profile',profile]);assert np.array_equal(off,on),'volume leaks through near solid geometry'
   s=json.loads((out/'summary.json').read_text());assert s['atmosphere_width_actual']==(120 if profile=='low'else 240)
  _,zero=run('zero_radiance',['--sunlight-profile','standard','--shaft-strength','0']);_,clear=run('clear',[]);assert np.array_equal(zero,clear),'zero radiance changes crisp material pixels'
  print('PASS real GL static shadow stability/caster groups, bias vs geometry separation, bounded 8-quarter/12-half, near-occluder and zero-radiance invariance')
if __name__=='__main__':main(sys.argv[1])
