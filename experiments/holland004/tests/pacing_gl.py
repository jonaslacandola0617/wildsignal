"""Actual GL parity and timestamp ordering; no real-time speed assertion in CI."""
import csv,importlib.util,json,os,subprocess,sys,tempfile
from pathlib import Path
from PIL import Image

def main(exe):
 fpath=Path(__file__).parents[2]/'holland001/tests/import.py';spec=importlib.util.spec_from_file_location('fixture',fpath);f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f)
 env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','offscreen')
 with tempfile.TemporaryDirectory()as t:
  root=Path(t);env.setdefault('MESA_SHADER_CACHE_DIR',str(root/'mesa'));d,b=f.fixture(root);d['nodes']=[{'mesh':0}];(root/'mesh.gltf').write_text(json.dumps(d))
  (root/'pack.json').write_text(json.dumps({'revision':1,'models':[{'id':'RURAL_05','path':'mesh.gltf'}]}));(root/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'test','model':'RURAL_05','position':[0,0,0],'yaw':0,'scale':1}]}))
  images=[];camera_rows=[]
  for mode,cap in [('legacy',0),('precise',0),('legacy',30),('precise',30),('legacy',60),('precise',60)]:
   out=root/f'{mode}{cap}';cmd=[exe,'--pack',str(root/'pack.json'),'--scene',str(root/'scene.json'),'--look',str(Path(__file__).parents[1]/'data/TOON_DIAGNOSTIC.json'),'--style','C','--no-art-materials','--no-smooth-lighting','--study','B1','--shadow','cached','--shadow-proof','stable','--edge-mode','combined','--route','strafe','--frames','4','--warmup','0','--cap',str(cap),'--pacing',mode,'--fixture-id','synthetic','--hidden','--allow-software','--output',str(out)]
   r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=60);assert r.returncode==0,r.stderr
   images.append(Image.open(out/'final.ppm').tobytes());rows=list(csv.DictReader((out/'frames.csv').open()));meta=json.loads((out/'pacing.json').read_text())
   camera_rows.append([[r[k]for k in ['camera_x','camera_y','camera_z','yaw','pitch']]for r in rows])
   assert len(rows)==4
   for r in rows:
    begin,ready,called,swapped,ended=map(int,[r[k]for k in ['frame_begin_ns','work_ready_ns','swap_called_ns','after_swap_ns','frame_completed_ns']])
    assert begin<=ready<=called<=swapped<=ended
    assert float(r['cpu_work_wall_ms'])<=float(r['loop_ms'])+.001
    assert r['display_interval_ms']=='-1','API return is not scanout'
    if not cap:
     assert r['wait_target_ns']=='-1'and float(r['sleep_call_wall_ms'])==float(r['active_wait_wall_ms'])==0
     assert meta['wait_backend']=='not_used'
    elif mode=='precise':assert called>=int(r['scheduled_presentation_deadline_ns'])
    else:assert r['scheduled_presentation_deadline_ns']=='-1'
   assert meta['scanout_verified']is False
  assert all(x==images[0]for x in images),'pacing changed rendering'
  assert all(x==camera_rows[0]for x in camera_rows),'pacing changed deterministic camera/input path'
  print('PASS actual GL cap0/30/60 legacy/precise bit parity, camera routes, timestamp ordering, no cap0 waits, scanout unavailable')
if __name__=='__main__':main(sys.argv[1])
