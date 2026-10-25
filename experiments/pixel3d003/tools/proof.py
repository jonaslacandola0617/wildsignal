"""Real executable proof. All model, image, video and timing outputs stay private.

No generated proof images. Contact sheets only place unaltered engine pixels.
Benchmark routes/configurations match; capture runs are separate from timing.
"""
import argparse,csv,hashlib,json,os,statistics,subprocess,sys
from pathlib import Path
from PIL import Image,ImageChops,ImageDraw

CASES={
 'off':[], 'live512':['--shadow','live'], 'cached512':['--shadow','cached'],
 'ground256':['--shadow','ground'], 'hard512':['--shadow','cached','--hard-shadow'],
 'cached1024':['--shadow','cached','--shadow-size','1024'],
 'screen':['--shadow','cached','--shafts','screen'],
 'volume12_half':['--shadow','cached','--shafts','volume'],
 'volume8_quarter':['--shadow','cached','--shafts','volume','--shaft-scale','4','--shaft-steps','8']}

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('executable',type=Path);p.add_argument('pack',type=Path);p.add_argument('output',type=Path)
 p.add_argument('--data',type=Path,default=Path(__file__).resolve().parents[1]/'data')
 for name in ['stills','motion','benchmark','allow-software']:p.add_argument('--'+name,action='store_true')
 p.add_argument('--benchmark-frames',type=int,default=120);p.add_argument('--motion-frames',type=int,default=600)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);env=os.environ.copy()
 if sys.platform=='linux':env.setdefault('SDL_VIDEODRIVER','offscreen')
 manifest=[];checks=[];measurements=[]
 def run(name,scene,w,flags,frames=4,capture=0,route=False):
  out=a.output/name
  cmd=[str(a.executable.resolve()),'--pack',str(a.pack.resolve()),'--scene',str((a.data/(scene+'.json')).resolve()),'--look',str((a.data/'sunlight_look.json').resolve()),'--output',str(out.resolve()),'--resolution',str(w),'--mode','PIXEL3D','--cap','0','--frames',str(frames),'--warmup',str(30 if route else 0),'--hidden',*flags]
  if a.allow_software:cmd+=['--allow-software']
  if capture:cmd+=['--capture-every',str(capture)]
  if route:cmd+=['--canopy-route'if scene=='canopy'else'--orbit']
  r=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=600)
  if r.returncode:raise RuntimeError(name+'\n'+r.stderr)
  meta=json.loads((out/'summary.json').read_text())
  manifest.append({'name':name,'command':cmd,'summary':meta,'final_ppm_sha256':hashlib.sha256((out/'final.ppm').read_bytes()).hexdigest()})
  Image.open(out/'final.ppm').save(out/'final.png');print(name,flush=True)
  return out
 if a.stills:
  for scene,poses in [('sunlit_cottage',[0,5,8]),('canopy',[6])]:
   for w in [480,640]:
    for pose in poses:
     images={}
     for case,flags in CASES.items():
      out=run(f'still_{scene}_{w}_pose{pose}_{case}',scene,w,['--pose',str(pose),*flags]);images[case]=Image.open(out/'final.png').convert('RGB')
     assert images['live512'].tobytes()==images['cached512'].tobytes(),'cache/live differ'
     for case in ['cached512','ground256','volume12_half']:
      ref=images['cached512'if case=='volume12_half'else'off'];diff=ImageChops.difference(ref,images[case]);checks.append({'scene':scene,'width':w,'pose':pose,'case':case,'changed_pixels':sum(any(v)for v in diff.getdata()),'max_channel_difference':max(t[1]for t in diff.getextrema())})
     # Render same pose with camera culling disabled; shadow casters must not
     # disappear just because they are behind the view camera.
     off=run(f'equality_{scene}_{w}_pose{pose}',scene,w,['--pose',str(pose),*CASES['volume12_half'],'--no-cull'])
     assert images['volume12_half'].tobytes()==Image.open(off/'final.png').tobytes(),'actual model culling changes radiance'
     selected=['off','cached512','screen','volume12_half']
     sheet=Image.new('RGB',(w*2,w*9//16*2+48),(25,28,34));draw=ImageDraw.Draw(sheet)
     for i,case in enumerate(selected):
      x=(i%2)*w;y=(i//2)*(w*9//16+24);draw.text((x+5,y+6),case,fill='white');sheet.paste(images[case],(x,y+24))
     sheet.save(a.output/f'comparison_{scene}_{w}_pose{pose}.png')
 if a.benchmark:
  # Repeated OFF controls expose drift. Every major pass has a matched case;
  # camera sequence, geometry, light, resolution, frame count and hardware match.
  for scene in ['sunlit_cottage','canopy']:
   for w in [480,640]:
    order=['off','live512','cached512','ground256','off','hard512','cached1024','screen','volume12_half','volume8_quarter','cached512','off']
    for index,case in enumerate(order):
     out=run(f'bench_{scene}_{w}_{index}_{case}',scene,w,CASES[case],frames=a.benchmark_frames,route=True)
     meta=json.loads((out/'summary.json').read_text());allrows=list(csv.DictReader((out/'frames.csv').open()));rows=allrows[30:]
     def mean(k):return statistics.mean(float(r[k])for r in rows)
     measurements.append({'run':out.name,'scene':scene,'width':w,'effect':case,'loop_median_ms':meta['loop_median_ms'],'loop_p95_ms':meta['loop_p95_ms'],'swap_interval_p95_ms':meta['swap_completion_interval_p95_ms'],'submit_median_ms':meta['submit_median_ms'],'shadow_submit_mean_ms':mean('shadow_submit_ms'),'shaft_submit_mean_ms':mean('shaft_submit_ms'),'api_elapsed_median_ms':meta['gpu_query_median_ms'],'query_missing':meta['gpu_query_missing_samples'],'draws_mean':mean('draw_calls'),'shadow_draws_mean':mean('shadow_draws'),'startup_shadow_draws':int(allrows[0]['shadow_draws']),'peak_MiB':meta['process_peak_rss_bytes']/1048576,'effect_bytes':meta['atmosphere_logical_allocated_bytes'],'base_texture_bytes':meta['texture_base_level_bytes'],'hardware_gpu':meta['hardware_gpu_tested']})
 if a.motion:
  for scene in ['sunlit_cottage','canopy']:
   for w in [480,640]:
    dirs=[]
    for case in ['cached512','volume12_half']:
     out=run(f'motion_{scene}_{w}_{case}',scene,w,CASES[case],frames=a.motion_frames,capture=2,route=True);dirs.append(out)
    # Side-by-side actual engine recordings, matched frame by frame. Native
    # dimensions avoid unnecessary upscaled-video size; original PNGs retained.
    cmd=['ffmpeg','-v','error','-y']
    for d in dirs:cmd+=['-framerate','15','-pattern_type','glob','-i',str(d/'frame_*.ppm')]
    cmd+=['-filter_complex','[0:v][1:v]hstack=inputs=2[v]','-map','[v]','-c:v','libx264','-crf','16','-pix_fmt','yuv420p',str(a.output/f'preview_{scene}_{w}_OFF_ON.mp4')]
    subprocess.run(cmd,check=True)
 for name,obj in [('RUNS.json',manifest),('CHECKS.json',checks),('PERFORMANCE.json',measurements)]:
  (a.output/name).write_text(json.dumps(obj,indent=2))
 if measurements:
  with(a.output/'PERFORMANCE.csv').open('w')as f:
   writer=csv.DictWriter(f,fieldnames=list(measurements[0]));writer.writeheader();writer.writerows(measurements)
 print('DONE',len(manifest),'real engine runs',flush=True)
if __name__=='__main__':main()
