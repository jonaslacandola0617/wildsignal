"""Actual native mask comparisons; no art approval from a percentage.
Near denominator is visible designated ground within5m planar camera distance.
Geometry and painted leaf clusters are always reported separately.
"""
from pathlib import Path
import argparse,subprocess,os,json
from PIL import Image
import numpy as np
p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('assets',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
env=dict(os.environ,SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',LP_NUM_THREADS='2');rows=[]
for w in[480,640]:
 for pose in[0,1,3,4]:
  for name,layout,density in [('A_LEGACY','legacy','medium'),('B_MEDIUM','continuous','medium'),('B_HIGH','continuous','high')]:
   masks={}
   for kind in ['normal','meadow','nearmeadow','geometry','combined']:
    out=a.output/f'{w}_{pose}_{name}_{kind}'
    cmd=[str(a.exe.resolve()),'--assets',str(a.assets.resolve()),'--benchmark','--pose',str(pose),'--resolution',str(w),'--frames','3','--cap','0','--grass-layout',layout,'--grass-density',density,'--worldstyle','source','--allow-software','--hidden','--output',str(out)]
    if kind!='normal':cmd+=['--coverage',kind]
    r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=90);assert r.returncode==0,r.stderr
    im=Image.open(out/'final_world.ppm').convert('RGB');assert im.size==(w,w*9//16);im.save(a.output/f'{w}_{pose}_{name}_{kind}.png');masks[kind]=np.asarray(im)
    for f in out.glob('*.ppm'):f.unlink()
   white=lambda x:np.all(x>245,axis=2)
   meadow=white(masks['meadow']);near=white(masks['nearmeadow']);geom=white(masks['geometry']);combined=white(masks['combined'])
   row=dict(width=w,pose=pose,layout=name,density=density,visible_meadow_pixels=int(meadow.sum()),near_meadow_pixels=int(near.sum()))
   for label,mask in [('all',meadow),('near5m',near)]:
    assert mask.sum()>50
    row[label+'_geometry_percent']=float(100*(geom&mask).sum()/mask.sum());row[label+'_combined_percent']=float(100*(combined&mask).sum()/mask.sum())
   rows.append(row);print(row,flush=True)
(a.output/'COVERAGE_COMPARE.json').write_text(json.dumps(dict(method=__doc__,scope='Mesa software; actual native renderer masks. Painted ground is never blade geometry. Paths/solids/entrances/clearing excluded; no aesthetic approval inferred.',rows=rows),indent=2))
