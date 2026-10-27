"""Actual native engine projected coverage; no instance-count proxy.
Plain base green does NOT count. Geometry and explicit painted R4-palette cluster
footprints are measured separately, over designated visible meadow ground.
"""
import argparse,json,os,subprocess
from pathlib import Path
from PIL import Image
import numpy as np
p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('assets',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
env=dict(os.environ);env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('LIBGL_ALWAYS_SOFTWARE','1');rows=[]
for width in[480,640]:
 for pose in[0,1,2,3,4,6]:
  masks={}
  for kind in['normal','meadow','geometry','combined']:
   out=a.output/f'{width}_view{pose}_{kind}';cmd=[str(a.exe.resolve()),'--assets',str(a.assets.resolve()),'--benchmark','--pose',str(pose),'--resolution',str(width),'--frames','3','--cap','0','--grass-density','medium','--allow-software','--hidden','--output',str(out)]
   if kind!='normal':cmd+=['--coverage',kind]
   else:cmd+=['--capture-every','2']
   r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=90);assert r.returncode==0,r.stderr
   im=Image.open(out/'final_world.ppm').convert('RGB');assert im.size==(width,width*9//16);im.save(a.output/f'{width}_view{pose}_{kind}.png');masks[kind]=np.asarray(im)
   if kind=='normal':Image.open(out/'frame_000002.ppm').save(a.output/f'{width}_view{pose}_HUD.png')
  designated=np.all(masks['meadow']>245,axis=2);geom=np.all(masks['geometry']>245,axis=2);combined=np.all(masks['combined']>245,axis=2);den=int(np.count_nonzero(designated));assert den>200,(width,pose,'insufficient visible designated region')
  row={'resolution':[width,width*9//16],'camera_pose':pose,'kind':'overhead'if pose==6 else'gameplay_eye1.65m','designated_visible_pixels':den,'geometry_only_percent':100*np.count_nonzero(geom&designated)/den,'geometry_plus_explicit_painted_leaf_clusters_percent':100*np.count_nonzero(combined&designated)/den}
  row['target75_pass']=bool(row['geometry_plus_explicit_painted_leaf_clusters_percent']>=75);rows.append(row);print(row,flush=True)
(a.output/'COVERAGE.json').write_text(json.dumps({'method':'3 actual matched native engine diagnostic passes. Meadow mask excludes path, cottage, rock bases, interaction clearing. Geometry silhouette coverage is separate from geometry+explicit painted R4-palette leaf-cluster footprint. Plain base green is excluded. Percentages are projected screen area, not botanical density or aesthetic approval.','renderer':'See each run.log. Mesa is software,not IntelUHD.','rows':rows,'all_pass':all(r['target75_pass']for r in rows)},indent=2))
