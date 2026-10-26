"""Private real-model rendering parity and noncapture pacing diagnostics.
Replay images verify geometry/camera parity; they cannot demonstrate frame pacing.
Keep models, logs and captures outside the public repository.
"""
import argparse,csv,hashlib,json,os,subprocess
from pathlib import Path
from PIL import Image

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('executable',type=Path);p.add_argument('pack',type=Path);p.add_argument('simple',type=Path);p.add_argument('output',type=Path)
 p.add_argument('--baseline',type=Path);p.add_argument('--allow-software',action='store_true');p.add_argument('--benchmark',action='store_true')
 p.add_argument('--motion',action='store_true')
 p.add_argument('--frames',type=int,default=600);p.add_argument('--warmup',type=int,default=120)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 if a.benchmark and a.frames-a.warmup<480:raise ValueError('At least480 noncapture samples required')
 data=Path(__file__).resolve().parents[1]/'data';env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','windows'if os.name=='nt'else'offscreen');env.setdefault('MESA_SHADER_CACHE_DIR',str(a.output/'mesa'))
 fixtures=[('garden',a.pack,data/'garden.json'),('simple',a.simple/'assets/pack.json',a.simple/'data/simple_shapes.json')]
 runs=[];checks=[]
 def save():
  for name,items in [('RUNS',runs),('CHECKS',checks)]: (a.output/(name+'.json')).write_text(json.dumps(items,indent=2)+'\n')
 def run(fixture,width,pacer,cap,original=False,timing=False,pose=14,moving=False):
  name,pack,scene=fixture;out=a.output/f'{name}_{width}_{pacer}_{cap}_{"motion"if moving else"timing"if timing else"parity"}_p{pose}'
  cmd=[str((a.baseline if original else a.executable).resolve()),'--pack',str(pack.resolve()),'--scene',str(scene.resolve()),'--look',str(data/'TOON_DIAGNOSTIC.json'),'--style','C','--mode','PIXEL3D','--no-art-materials','--no-smooth-lighting','--study','B1','--pixel-camera','off','--edge-mode','combined','--shadow-proof','stable','--shadow','cached','--shadow-size','512','--shafts','off','--present-scale','2','--resolution',str(width),'--pose',str(pose),'--frames',str(a.frames if timing else 120 if moving else 6),'--warmup',str(a.warmup if timing else 0),'--cap',str(cap),'--hidden','--output',str(out)]
  if not original:cmd+=['--pacing',pacer,'--fixture-id',name]
  if timing or moving:cmd+=['--route','strafe']
  if not timing:cmd+=['--capture-presented','--capture-every','3'if moving else'1']
  if a.allow_software:cmd+=['--allow-software']
  result=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=600);out.mkdir(exist_ok=True)
  for filename,value in [('COMMAND.json',json.dumps(cmd,indent=2)),('stdout.txt',result.stdout),('stderr.txt',result.stderr)]: (out/filename).write_text(value)
  if result.returncode:raise RuntimeError(name+': '+result.stderr)
  for file in out.glob('*.ppm'):
   im=Image.open(file);png=file.with_suffix('.png');im.save(png);assert Image.open(png).tobytes()==im.tobytes();file.unlink()
  summary=json.loads((out/'summary.json').read_text());runs.append({'run':out.name,'source':summary['source'],'software':not summary['hardware_gpu_tested'],'timing':timing,'command':cmd});save();print(out.name,flush=True);return out
 def equal(name,old,new):
  images=sorted(old.glob('frame_*.png'))+[old/'final.png',old/'final_presented.png']
  assert images
  for image in images:
   x,y=Image.open(image),Image.open(new/image.name);assert x.size==y.size and x.tobytes()==y.tobytes(),name+image.name
  keys=['camera_x','camera_y','camera_z','yaw','pitch']
  def cameras(folder):return [[r[k]for k in keys]for r in csv.DictReader((folder/'frames.csv').open())]
  assert cameras(old)==cameras(new),name+' camera mismatch'
  checks.append({'test':name,'result':'PASS','exact_RGB_captures':len(images),'camera_rows_equal':True,'sha256_final_RGB':hashlib.sha256(Image.open(new/'final.png').tobytes()).hexdigest()});save()
 for fixture in fixtures:
  if not all(x.exists()for x in fixture[1:]):raise FileNotFoundError('Required actual fixture missing: '+fixture[0])
  for width in [480,640]:
   old=run(fixture,width,'PR32',0,original=True)if a.baseline else None
   ref=run(fixture,width,'legacy',0)
   if old:equal(f'{fixture[0]}_{width}_PR32_legacy',old,ref)
   for mode,cap in [('precise',0),('legacy',30),('precise',30),('legacy',60),('precise',60)]:equal(f'{fixture[0]}_{width}_{mode}_{cap}',ref,run(fixture,width,mode,cap))
   # An off-angle view is also actual engine output, not a generated illustration.
   corner=run(fixture,width,'precise',0,pose=13)
   if a.motion and width==640:
    before=run(fixture,width,'PR32',0,original=True,moving=True)if a.baseline else ref
    after=run(fixture,width,'precise',0,moving=True)
    if a.baseline:
     equal(f'{fixture[0]}_actual_moving_PR32_precise',before,after)
     movie=a.output/(fixture[0]+'_MOVING_RENDER_PARITY_NOT_PACING_PROOF.mp4')
     subprocess.run(['ffmpeg','-v','error','-y','-framerate','10','-pattern_type','glob','-i',str(before/'frame_*.png'),'-framerate','10','-pattern_type','glob','-i',str(after/'frame_*.png'),'-filter_complex','[0:v][1:v]hstack=inputs=2[v]','-map','[v]','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(movie)],check=True)
   if a.benchmark:
    cap=30 if width==480 else 60
    for mode in ['legacy','precise']:run(fixture,width,mode,cap,timing=True)
    run(fixture,width,'precise',0,timing=True)
 (a.output/'EVIDENCE_SCOPE.txt').write_text('Actual executable framebuffers; PPM converted losslessly to PNG. Identical native/presented pixels and camera rows verify preservation, NOT scanout cadence. Timing runs have no capture/readback/recording, 480+ measured samples; Mesa results are software diagnostics, never Intel UHD validation. Optional movies replay one capture per3 deterministic steps at10fps: left PR32/right precise, actual camera/image parity only, never pacing proof. Replay hides the scheduling fault. Use timestamp plots and run the paired visible Windows launchers on the owner device.\n')
 save()
if __name__=='__main__':main()
