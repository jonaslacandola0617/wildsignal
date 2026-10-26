"""Actual shader category/convexity controls using labeled synthetic geometry."""
import json,math,os,struct,subprocess,sys,tempfile
from pathlib import Path
import numpy as np
from PIL import Image

def main(exe):
 env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','offscreen')
 with tempfile.TemporaryDirectory()as tmp:
  r=Path(tmp);env.setdefault('MESA_SHADER_CACHE_DIR',str(r/'mesa'))
  def mesh(shape,model):
   # A folded strip: convex ridge or concave recess, identical coverage bounds.
   sign=-1 if shape=='convex'else 1
   pos=[];norm=[]
   for left,right in [(-3,0),(0,3)]:
    z0=sign*abs(left);z1=sign*abs(right)
    points=[(left,-2,z0),(right,-2,z1),(right,5,z1),(left,-2,z0),(right,5,z1),(left,5,z0)]
    slope=(z1-z0)/(right-left);n=(-slope/math.sqrt(1+slope*slope),0,1/math.sqrt(1+slope*slope))
    pos+=points;norm+=[n]*6
   b=b''.join(struct.pack('<3f',*v)for v in pos+norm);(r/'mesh.bin').write_bytes(b)
   d={'asset':{'version':'2.0'},'buffers':[{'uri':'mesh.bin','byteLength':len(b)}],'bufferViews':[{'buffer':0,'byteLength':144},{'buffer':0,'byteOffset':144,'byteLength':144}], 'accessors':[{'bufferView':i,'componentType':5126,'count':12,'type':'VEC3'}for i in [0,1]],'materials':[{'name':'leaf_mass','doubleSided':True,'pbrMetallicRoughness':{'baseColorFactor':[.2,.45,.15,1]}}],'meshes':[{'primitives':[{'attributes':{'POSITION':0,'NORMAL':1},'material':0}]}],'nodes':[{'mesh':0}],'scenes':[{'nodes':[0]}],'scene':0}
   (r/'mesh.gltf').write_text(json.dumps(d));(r/'pack.json').write_text(json.dumps({'revision':1,'models':[{'id':model,'path':'mesh.gltf'}]}));(r/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'folded_test_strip','model':model,'position':[0,0,0],'yaw':0,'scale':1}]}))
  def run(name,flags):
   out=r/name;cmd=[exe,'--pack',str(r/'pack.json'),'--scene',str(r/'scene.json'),'--look',str(Path(__file__).parents[1]/'data/sunlight_look.json'),'--style','C','--no-art-materials','--study','B1','--pose','13','--resolution','480','--frames','3','--warmup','0','--cap','0','--hidden','--allow-software','--no-outline','--edge-mode','only','--highlight-threshold','0','--output',str(out)]+flags
   v=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=90);assert v.returncode==0,v.stderr
   return np.array(Image.open(out/'final.ppm'))
  mesh('convex','RURAL_05');convex=run('convex',['--edge-mask']);assert np.count_nonzero(convex[:,:,0])>8,'architecture highlight inactive'
  mesh('concave','RURAL_05');concave=run('concave',['--edge-mask'])
  # Center crease only: avoid the legitimate outer silhouette highlights.
  assert np.count_nonzero(convex[80:190,238:242,0])>10,'convex ridge missing'
  assert np.count_nonzero(concave[80:190,238:242,0])==0,'concave crease incorrectly lit'
  mesh('convex','TREE_01');leaf=run('leaf',['--edge-mask']);assert np.count_nonzero(leaf[:,:,1])>8,'foliage category cannot reach effect'
  assert np.count_nonzero(leaf[80:190,238:242,1])==0,'leaf normal crease painted as triangle detail'
  excluded=run('leaf_masked',['--edge-mask','--highlight-mask','architecture']);assert not excluded.any(),'category mask ignored'
  off=run('off',['--edge-mode','off']);on=run('on',[]);again=run('again',[])
  assert not np.array_equal(off,on),'highlight-only depends on dark outline toggle'
  assert np.array_equal(on,again),'non-deterministic highlight output'
  disabled=run('disabled',['--no-edge-highlights']);assert np.array_equal(off,disabled),'highlight OFF not exact'
  dark=run('combined',['--edge-mode','combined']);assert np.array_equal(on,dark),'--no-outline should not disable highlights or add dark ink'
  print('PASS real GL architecture/foliage masks, signed concave rejection, no leaf tessellation ink, independent deterministic highlights')
if __name__=='__main__':main(sys.argv[1])
