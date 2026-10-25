"""Controlled real-engine A/B/C material study. Every output stays private.

No generated scene images: layouts/crops only arrange actual framebuffer pixels.
Uses fixed geometry, scene, light and camera routes across all three modes.
"""
import argparse,csv,hashlib,json,os,statistics,subprocess,sys
from pathlib import Path
from PIL import Image,ImageChops,ImageDraw

DIAG={
 'albedoA':['--style','A','--diagnostic','albedo'],
 'albedoB':['--style','B','--diagnostic','albedo'],
 'authored_normal':['--diagnostic','normal'],
 'geometric_normal':['--diagnostic','normal','--mode','BASE'],
 'vertex_color':['--diagnostic','vertex'],
 'no_vertex_tint':['--no-vertex-tint'],
 'no_bands':['--no-bands'],
 'geometric_light':['--mode','BASE','--no-bands'],
 'shadow_off':['--shadow','off','--shafts','off'],
 'hard512':['--hard-shadow','--shafts','off'],
 'soft1024':['--shadow-size','1024','--shafts','off'],
 'zero_bias':['--bias-scale','0','--shafts','off'],
 'small_bias_only':['--bias-scale','.10909','--shafts','off'],
 'double_bias':['--bias-scale','2','--shafts','off'],
 'source_base_lod':['--diagnostic','albedo','--sampling','base'],
 'source_linear':['--diagnostic','albedo','--sampling','linear'],
 'visibility_all_A':['--diagnostic','shadow'],
 'visibility_building_A':['--diagnostic','shadow','--casters','building'],
 'visibility_all_C':['--style','C','--no-art-materials','--diagnostic','shadow'],
 'visibility_uncorrected_C':['--style','C','--no-art-materials','--no-receiver-correction','--diagnostic','shadow'],
 'roof512_C':['--style','C','--no-art-materials','--diagnostic','shadow','--casters','roof','--pose','5'],
 'roof1024_C':['--style','C','--no-art-materials','--diagnostic','shadow','--casters','roof','--shadow-size','1024','--pose','5'],
 'wire':['--diagnostic','wire','--shafts','off'],
 'C_banded_light':['--style','C','--no-smooth-lighting'],
 'C_old_air':['--style','C','--no-smooth-air'],
 'B_no_ink':['--style','B','--no-refined-outlines'],
 'B_no_highlight':['--style','B','--no-edge-highlights']}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('executable',type=Path);p.add_argument('pack',type=Path);p.add_argument('output',type=Path)
 p.add_argument('--data',type=Path,default=Path(__file__).resolve().parents[1]/'data')
 p.add_argument('--baseline003',type=Path);p.add_argument('--resample-pack',type=Path)
 for k in ['stills','diagnostics','benchmark','motion','allow-software']:p.add_argument('--'+k,action='store_true')
 p.add_argument('--benchmark-frames',type=int,default=150);p.add_argument('--motion-frames',type=int,default=450)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);env=os.environ.copy()
 if sys.platform=='linux':env.setdefault('SDL_VIDEODRIVER','offscreen')
 ledger=[];metrics=[];checks=[]
 def run(name,scene,w,style,flags=(),frames=4,route=False,capture=0,baseline=False,pack=None):
  out=a.output/name;cmd=[str((a.baseline003 if baseline else a.executable).resolve()),'--pack',str((pack or a.pack).resolve()),'--scene',str((a.data/(scene+'.json')).resolve()),'--look',str((a.data/'sunlight_look.json').resolve()),'--output',str(out.resolve()),'--resolution',str(w),'--mode','PIXEL3D','--shadow','cached','--shafts','volume','--cap','0','--frames',str(frames),'--warmup',str(30 if route else 0),'--hidden']
  if not baseline:cmd+=['--style',style,'--atmosphere-width','240'if w==480 else'320']
  if a.allow_software:cmd+=['--allow-software']
  if route:cmd+=['--canopy-route'if scene=='canopy'else'--orbit']
  if capture:cmd+=['--capture-every',str(capture)]
  cmd+=list(flags)
  subprocess.run(cmd,env=env,capture_output=True,text=True,check=True,timeout=600)
  meta=json.loads((out/'summary.json').read_text());Image.open(out/'final.ppm').save(out/'final.png')
  ledger.append({'name':name,'command':cmd,'summary':meta,'framebuffer_sha256':hashlib.sha256((out/'final.ppm').read_bytes()).hexdigest()});print(name,flush=True)
  return out
 def sheet(name,outputs,w):
  im=Image.new('RGB',(w*3,w*9//16+24),(25,28,34));d=ImageDraw.Draw(im)
  for i,(label,path)in enumerate(outputs):d.text((i*w+6,7),label,fill='white');im.paste(Image.open(path/'final.png'),(i*w,24))
  im.save(a.output/name)
 if a.stills:
  for w,views in [(640,[('sunlit_cottage',x)for x in [1,5,8,9,10,11]]+[('canopy',6)]),(480,[('sunlit_cottage',5),('canopy',6)]),(960,[('sunlit_cottage',x)for x in [5,9,10]])]:
   for scene,pose in views:
    paths=[]
    for style in 'ABC':paths.append((style,run(f'still_{scene}_{w}_p{pose}_{style}',scene,w,style,['--pose',str(pose)])))
    sheet(f'ABC_{scene}_{w}_p{pose}.png',paths,w)
    # Culling must not alter strokes/lighting in modeC.
    if w==640 and pose in (5,6):
     off=run(f'cull_equality_{scene}',scene,w,'C',['--pose',str(pose),'--no-cull']);assert Image.open(paths[2][1]/'final.png').tobytes()==Image.open(off/'final.png').tobytes()
    if a.baseline003 and w in (480,640)and pose in (5,8):
     old=run(f'actual003_{w}_p{pose}',scene,w,'A',['--pose',str(pose)],baseline=True)
     equal=Image.open(old/'final.png').tobytes()==Image.open(paths[0][1]/'final.png').tobytes();checks.append({'test':'exact003_A','width':w,'pose':pose,'equal':equal});assert equal,'A no longer matches actual003 baseline'
 if a.diagnostics:
  for name,flags in DIAG.items():
   out=run('diagnostic_'+name,'sunlit_cottage',640,'A',['--pose','9','--debug-effects',*flags])
   raw=out/'raw_shadow_depth.pfm'
   if raw.exists():
    import numpy as np
    with raw.open('rb')as f:
     assert f.readline()==b'Pf\n';w,h=map(int,f.readline().split());f.readline();depth=np.frombuffer(f.read(),'<f4').reshape(h,w)[::-1].copy()
    # Fixed shared display range; PFM retains original float depth unchanged.
    gray=np.where(depth>=.9999,255,np.clip((depth-.25)/.5*220,0,220)).astype(np.uint8)
    Image.fromarray(gray).save(out/'shadow_depth_display.png')
  if a.resample_pack:
   out=run('diagnostic_resize_only_material','sunlit_cottage',640,'B',['--pose','9','--diagnostic','albedo'],pack=a.resample_pack)
   # Same256px atlases/FBO; isolate actual stroke derivation from resizing.
   sheet('MATERIAL_TREATMENT_CONTROL.png',[('Original source',a.output/'diagnostic_albedoA'),('Resize only',out),('Source-guided strokes',a.output/'diagnostic_albedoB')],640)
  for w in (480,960):run(f'diagnostic_resolution_A_{w}','sunlit_cottage',w,'A',['--pose','9','--shadow','off','--shafts','off'])
 if a.benchmark:
  for scene,widths in [('sunlit_cottage',[480,640,960]),('canopy',[640])]:
   for w in widths:
    for i,style in enumerate('ABCCBA'):
     out=run(f'bench_{scene}_{w}_{i}_{style}',scene,w,style,frames=a.benchmark_frames,route=True);s=json.loads((out/'summary.json').read_text());rows=list(csv.DictReader((out/'frames.csv').open()))[30:]
     metrics.append({'scene':scene,'width':w,'style':style,'run':out.name,'loop_median_ms':s['loop_median_ms'],'loop_p95_ms':s['loop_p95_ms'],'swap_interval_p95_ms':s['swap_completion_interval_p95_ms'],'submit_median_ms':s['submit_median_ms'],'api_query_median_ms':s['gpu_query_median_ms'],'api_query_missing':s['gpu_query_missing_samples'],'draws_mean':statistics.mean(float(r['draw_calls'])for r in rows),'peak_RSS_MiB':s['process_peak_rss_bytes']/1048576,'texture_base_bytes':s['texture_base_level_bytes'],'texture_mip_bytes':s['texture_full_mip_rgba8_bytes'],'effect_bytes':s['atmosphere_logical_allocated_bytes'],'air_width':s['atmosphere_width_actual'],'hardware_gpu':s['hardware_gpu_tested']})
 if a.motion:
  for scene in ['sunlit_cottage','canopy']:
   dirs=[]
   for style in 'ABC':dirs.append(run(f'motion_{scene}_{style}',scene,640,style,frames=a.motion_frames,route=True,capture=3))
   cmd=['ffmpeg','-v','error','-y']
   for d in dirs:cmd+=['-framerate','10','-pattern_type','glob','-i',str(d/'frame_*.ppm')]
   cmd+=['-filter_complex','[0:v][1:v][2:v]hstack=inputs=3[v]','-map','[v]','-c:v','libx264','-crf','16','-pix_fmt','yuv420p',str(a.output/f'ABC_MOVING_{scene}.mp4')];subprocess.run(cmd,check=True)
 for name,content in [('RUNS.json',ledger),('PERFORMANCE.json',metrics),('CHECKS.json',checks)]: (a.output/name).write_text(json.dumps(content,indent=2))
 if metrics:
  with(a.output/'PERFORMANCE.csv').open('w')as f:
   writer=csv.DictWriter(f,fieldnames=list(metrics[0]));writer.writeheader();writer.writerows(metrics)
 print('DONE',len(ledger),'actual executable runs',flush=True)
if __name__=='__main__':main()
