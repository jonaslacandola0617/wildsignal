"""Actual HOLLAND-003 executable evidence. No generated scene imagery.
Requires Pillow/NumPy/ffmpeg for lossless format conversion and comparison layout.
All assets and output must remain private; software measurements are not GPU proof.
"""
import argparse,csv,hashlib,json,os,statistics,subprocess
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('executable',type=Path);p.add_argument('pack',type=Path);p.add_argument('output',type=Path)
 p.add_argument('--baseline',type=Path);p.add_argument('--allow-software',action='store_true')
 for k in ['stills','motion','benchmark']:p.add_argument('--'+k,action='store_true')
 p.add_argument('--benchmark-frames',type=int,default=150);p.add_argument('--motion-frames',type=int,default=180)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);data=Path(__file__).resolve().parents[1]/'data'
 env=os.environ.copy();env.setdefault('MESA_SHADER_CACHE_DIR',str(a.output/'mesa'));env.setdefault('SDL_VIDEODRIVER','offscreen'if os.name!='nt'else'windows')
 runs=[];checks=[];perf=[]
 def save():
  for n,v in [('RUNS',runs),('CHECKS',checks),('PERFORMANCE',perf)]: (a.output/(n+'.json')).write_text(json.dumps(v,indent=2)+'\n')
  if perf:
   with (a.output/'PERFORMANCE.csv').open('w',newline='')as f:w=csv.DictWriter(f,fieldnames=list(perf[0]));w.writeheader();w.writerows(perf)
 def run(name,width=640,pose=14,flags=(),frames=3,route=None,capture=0,original=False,timing=False):
  out=a.output/name;cmd=[str((a.baseline if original else a.executable).resolve()),'--pack',str(a.pack.resolve()),'--scene',str(data/'garden.json'),'--look',str(data/'TOON_DIAGNOSTIC.json'),'--style','C','--no-art-materials','--no-smooth-lighting','--study','B1','--pixel-camera','off','--resolution',str(width),'--pose',str(pose),'--shadow','off','--shafts','off','--frames',str(frames),'--warmup',str(30 if timing else 0),'--cap','0','--hidden','--output',str(out)]
  if not original:cmd+=['--edge-mode','legacy','--shadow-proof','legacy','--air-proof','legacy']
  if route:cmd+=['--route',route]
  if not timing:cmd+=['--capture-presented','--present-scale','2','--debug-effects']
  if capture:cmd+=['--capture-every',str(capture)]
  if a.allow_software:cmd+=['--allow-software']
  cmd+=list(flags);r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=600)
  out.mkdir(exist_ok=True)
  (out/'COMMAND.json').write_text(json.dumps(cmd,indent=2));(out/'stdout.txt').write_text(r.stdout);(out/'stderr.txt').write_text(r.stderr)
  if r.returncode:raise RuntimeError(name+'\n'+r.stdout+'\n'+r.stderr)
  hashes={}
  for f in out.glob('*.ppm'):
   im=Image.open(f);png=f.with_suffix('.png');im.save(png)
   assert Image.open(png).tobytes()==im.tobytes(),'lossless capture conversion failed'
   if f.name.startswith(('frame_','presented_')):f.unlink() # PNG retains exact captured pixels.
  for f in sorted(out.glob('effects_*/raw_shadow_depth.pfm')):hashes[f.parent.name]=hashlib.sha256(f.read_bytes()).hexdigest()
  for f in out.glob('raw_shadow_depth.pfm'):
   b=f.read_bytes();head=b.split(b'\n',3);w,h=map(int,head[1].split());v=np.frombuffer(head[3],dtype='<f4').reshape(h,w)
   Image.fromarray(np.flipud(np.clip((.75-v)/.5*255,0,255).astype('uint8'))).save(out/'SHADOW_DEPTH_FIXED_0.25_0.75.png')
  s=json.loads((out/'summary.json').read_text());runs.append({'name':name,'command':cmd,'summary':s,'captured_shadow_map_sha256_by_frame':hashes})
  if timing:
   rows=list(csv.DictReader((out/'frames.csv').open()))[30:];metric={'name':name,'width':width,'software':not s['hardware_gpu_tested'],'loop_median_ms':s['loop_median_ms'],'loop_p95_ms':s['loop_p95_ms'],'present_p95_ms':s['swap_completion_interval_p95_ms'],'CPU_submit_median_ms':s['submit_median_ms'],'shadow_CPU_submit_median_ms':statistics.median(float(x['shadow_submit_ms'])for x in rows),'shafts_CPU_submit_median_ms':statistics.median(float(x['shaft_submit_ms'])for x in rows),'API_query_median_ms':s['gpu_query_median_ms'],'API_query_missing':s['gpu_query_missing_samples'],'draws_mean':statistics.mean(float(x['draw_calls'])for x in rows),'view_triangles_mean':statistics.mean(float(x['triangles_submitted'])for x in rows),'shadow_triangles_mean':statistics.mean(float(x['shadow_triangles'])for x in rows),'peak_process_MiB':s['process_peak_rss_bytes']/1048576,'logical_effect_bytes':s['atmosphere_logical_allocated_bytes'],'texture_bytes':s['texture_base_level_bytes']};perf.append(metric)
  save();print(name,flush=True);return out
 def same(name,x,y,filename='final.png'):
  eq=Image.open(x/filename).tobytes()==Image.open(y/filename).tobytes();checks.append({'name':name,'equal':eq});save();assert eq,name
 def sheet(name,items,presented=False):
  images=[Image.open(f/('final_presented.png'if presented else'final.png')).convert('RGB')for _,f in items];w,h=images[0].size
  canvas=Image.new('RGB',(w*2,(h+24)*((len(items)+1)//2)),(24,30,36));d=ImageDraw.Draw(canvas)
  for i,((label,_),im)in enumerate(zip(items,images)):x=i%2*w;y=i//2*(h+24);d.text((x+8,y+5),label,fill='white');canvas.paste(im,(x,y+24))
  canvas.save(a.output/name)
 edges=[('OFF',['--edge-mode','off']),('PR28_B1',['--edge-mode','legacy']),('HIGHLIGHT_ONLY',['--edge-mode','only']),('COMBINED',['--edge-mode','combined'])]
 stable=['--shadow-proof','stable','--shadow','cached'];combined=['--edge-mode','combined']
 if a.stills:
  for width in [480,640]:
   for pose in [13,14,17,1,16]:
    items=[]
    for name,flags in edges:items.append((name,run(f'edge_{width}_p{pose}_{name}',width,pose,flags)))
    sheet(f'EDGES_NATIVE_{width}_p{pose}.png',items);sheet(f'EDGES_PRESENTED_{width}_p{pose}.png',items,True)
    for _,d in items:
     im=Image.open(d/'final.png');im.crop((width//4,im.height//4,width*3//4,im.height*3//4)).save(d/'detail_native.png')
   if a.baseline:
    for pose in [13,14,12,1]:
     old=run(f'actual_PR28_{width}_p{pose}',width,pose,original=True)
     new=run(f'legacy_parity_{width}_p{pose}',width,pose)
     same(f'actual_owner_toon_PR28_parity_{width}_{pose}',old,new)
   for pose in [12,16]:
    cases=[('OFF',[]),('PR28_512',['--shadow','cached']),('STABLE_512',stable),('STABLE_1024',stable+['--shadow-size','1024'])]
    items=[(n,run(f'shadow_{width}_p{pose}_{n}',width,pose,f))for n,f in cases];sheet(f'SHADOWS_NATIVE_{width}_p{pose}.png',items);sheet(f'SHADOWS_PRESENTED_{width}_p{pose}.png',items,True)
   diagnostic=[]
   for n,f in [('zero_bias',['--bias-scale','0']),('high_bias',['--bias-scale','4']),('hard',['--hard-shadow']),('building',['--casters','building']),('vegetation',['--casters','vegetation']),('roof',['--casters','roof']),('normals',['--diagnostic','normal']),('wire',['--diagnostic','wire']),('contacts',['--wordline-contacts']),('live',['--shadow','live'])]:
    diagnostic.append((n,run(f'diag_{width}_{n}',width,12,stable+f)))
   ref=a.output/f'shadow_{width}_p12_STABLE_512';same('actual_static_cache_live_'+str(width),ref,diagnostic[-1][1])
   for name,d in diagnostic[:3]:
    eq=(ref/'raw_shadow_depth.pfm').read_bytes()==(d/'raw_shadow_depth.pfm').read_bytes();checks.append({'name':f'bias_filter_preserves_caster_geometry_{width}_{name}','equal':eq});assert eq
   for pose in [14,19,20]:
    cases=[('OFF',['--sunlight-profile','off']),('8_QUARTER',['--sunlight-profile','low']),('INHERITED_12_HALF',['--air-proof','legacy','--shafts','volume','--shaft-steps','12','--shaft-scale','2','--atmosphere-width',str(width//2)]),('BOUNDED_12_HALF',['--sunlight-profile','standard'])]
    items=[(n,run(f'air_{width}_p{pose}_{n}',width,pose,combined+stable+f))for n,f in cases]
    sheet(f'SUNLIGHT_NATIVE_{width}_p{pose}.png',items);sheet(f'SUNLIGHT_PRESENTED_{width}_p{pose}.png',items,True)
    haze=run(f'haze_{width}_p{pose}',width,pose,combined+stable+['--sunlight-profile','haze']);sheet(f'HAZE_ONLY_{width}_p{pose}.png',[items[0],('HAZE_ONLY',haze)])
   for pose in [13,14,17,1]:
    before=a.output/f'edge_{width}_p{pose}_PR28_B1';after=run(f'final_{width}_p{pose}',width,pose,combined+stable+['--wordline-contacts','--sunlight-profile','low']);sheet(f'FINAL_BEFORE_AFTER_{width}_p{pose}.png',[('OWNER_TOON_CONTROL',before),('OPTIONAL_FULL_COMBINATION',after)])
  save()
 def video(name,dirs,grid=False):
  cmd=['ffmpeg','-v','error','-y']
  for d in dirs:cmd+=['-framerate','10','-pattern_type','glob','-i',str(d/'frame_*.png')]
  filt=''.join(f'[{i}:v]'for i in range(len(dirs)))
  filt+=('xstack=inputs=4:layout=0_0|640_0|0_360|640_360'if grid else f'hstack=inputs={len(dirs)}')+'[v]'
  subprocess.run(cmd+['-filter_complex',filt,'-map','[v]','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(a.output/name)],check=True)
 if a.motion:
  edge_dirs={}
  for route in ['strafe','orbit']:
   dirs=[run(f'motion_edge_{route}_{n}',flags=f,frames=a.motion_frames,route=route,capture=2)for n,f in edges];edge_dirs[route]=dirs
   video(f'EDGES_OFF_PR28_ONLY_COMBINED_{route}.mp4',dirs,True)
  shadows=[run('motion_shadow_'+n,flags=f,frames=a.motion_frames,route='strafe',capture=2)for n,f in [('PR28',['--shadow','cached']),('512',stable),('1024',stable+['--shadow-size','1024'])]]
  video('SHADOW_PR28_STABLE512_STABLE1024_strafe.mp4',shadows)
  for d in shadows:
   hs={hashlib.sha256(f.read_bytes()).hexdigest()for f in d.glob('effects_*/raw_shadow_depth.pfm')};checks.append({'name':d.name+'_world_fixed_depth_map','unique_captured_maps':len(hs)});assert len(hs)==1
  air=[run('motion_sun_'+n,flags=combined+stable+['--sunlight-profile',n],frames=a.motion_frames,route='sun',capture=2)for n in ['off','low','standard']]
  video('SUNLIGHT_OFF_8QUARTER_12HALF.mp4',air)
  final=run('motion_final_orbit',flags=combined+stable+['--wordline-contacts','--sunlight-profile','low'],frames=a.motion_frames,route='orbit',capture=2);video('FINAL_OWNER_TOON_VS_COMBINATION_orbit.mp4',[edge_dirs['orbit'][1],final]);save()
 if a.benchmark:
  cases=[('owner_toon',[]),('edges_off',['--edge-mode','off']),('highlights_only',['--edge-mode','only']),('edges_combined',combined),('cached512',combined+stable),('cached1024',combined+stable+['--shadow-size','1024']),('live512',combined+stable+['--shadow','live']),('contacts',combined+stable+['--wordline-contacts']),('air8quarter',combined+stable+['--sunlight-profile','low']),('air12half',combined+stable+['--sunlight-profile','standard']),('haze',combined+stable+['--sunlight-profile','haze'])]
  for width in [480,640]:
   for repeat,order in enumerate([cases,list(reversed(cases))]):
    for name,flags in order:run(f'bench_{width}_{repeat}_{name}',width,flags=flags,frames=a.benchmark_frames,route='strafe',timing=True)
 (a.output/'REPLAY_NOTE.txt').write_text('Actual engine framebuffers, lossless PNG conversion. Movies replay one capture per two deterministic route steps at 10 fps, NOT measured engine/display FPS. Edge clips: top OFF/PR28 B1; bottom highlight-only/combined. Shadow clip: PR28/stable512/stable1024. Sunlight: OFF/8-quarter/12-half. Final: owner toon/full optional combination. No interpolation or AI imagery. Native and integer-presented stills are separate. Raw shadow PFM values and light matrices are actual readbacks; grayscale previews use the labeled fixed0.25..0.75 range.\n')
 save();print('DONE',len(runs),'actual engine runs',flush=True)
if __name__=='__main__':main()
