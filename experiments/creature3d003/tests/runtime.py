"""Actual skinned shader/texture/playback/determinism checks on private assets."""
import json,os,subprocess,sys,tempfile
from pathlib import Path
import numpy as np
from PIL import Image
exe=Path(sys.argv[1]).resolve();assets=Path(sys.argv[2]).resolve()
data=Path(__file__).parents[1]/'data'
env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','offscreen')
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);env.setdefault('MESA_SHADER_CACHE_DIR',str(root/'mesa'))
 def run(name,clip='Idle',time=0,lod=0,frames=1):
  out=root/name
  command=[str(exe),'--pack',str(assets/f'pack_lod{lod}.json'),'--scene',str(assets/'neutral_1.json'),'--look',str(data/'TOON_DIAGNOSTIC.json'),'--study','B1','--no-art-materials','--no-smooth-lighting','--sunlight-profile','off','--shadow','off','--resolution','480','--frames',str(frames),'--warmup','0','--cap','0','--hidden','--allow-software','--fixed-animation','--animation-time',str(time),'--clip',clip,'--capture-every','1','--output',str(out)]
  r=subprocess.run(command,env=env,capture_output=True,text=True,timeout=90)
  assert r.returncode==0,r.stderr
  return [np.array(Image.open(f)) for f in sorted(out.glob('frame_*.ppm'))]
 first=run('first');again=run('again');assert np.array_equal(first[0],again[0]),'nondeterministic fixed pose'
 for clip,period in [('Idle',2),('Walk',1),('Attack',2)]:
  frames=run(clip,clip=clip,frames=24)
  changed=max(np.count_nonzero(np.any(x!=frames[0],axis=2)) for x in frames[1:])
  assert changed>5,(clip,'no visible skin motion',changed)
  if clip!='Attack':
   endpoint=run(clip+'_endpoint',clip=clip,time=period)[0]
   assert np.count_nonzero(np.any(endpoint!=frames[0],axis=2))<10,'loop image discontinuity'
 for lod in [1,2]:
  picture=run('lod'+str(lod),lod=lod)[0]
  assert picture.shape==(270,480,3),'wrong native pixel resolution'
  # Independent eye color confirms the base-color atlas reached the shader.
  assert np.count_nonzero((picture[:,:,0]>picture[:,:,1]*1.3)&(picture[:,:,2]>picture[:,:,1]*1.1))>1,'eye atlas missing'
 print('PASS actual skin shader, atlas, clips, deterministic captures, loop images and LODs; software GL is not owner hardware certification')
