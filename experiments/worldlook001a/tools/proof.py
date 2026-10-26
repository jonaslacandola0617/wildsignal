"""Private actual-art executable proof. No scene images are synthesized.
PNGs are lossless captures; labelled sheets only arrange engine images.
Movies replay sampled deterministic camera frames, NOT displayed FPS/pacing proof.
"""
import argparse,csv,hashlib,json,os,statistics,subprocess
from pathlib import Path
from PIL import Image,ImageDraw

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('executable',type=Path);p.add_argument('garden_pack',type=Path);p.add_argument('grass_kit',type=Path);p.add_argument('output',type=Path);p.add_argument('--baseline',type=Path);p.add_argument('--allow-software',action='store_true');p.add_argument('--stills',action='store_true');p.add_argument('--motion',action='store_true');p.add_argument('--benchmark',action='store_true');p.add_argument('--pacing-benchmark',action='store_true');p.add_argument('--frames',type=int,default=600);p.add_argument('--warmup',type=int,default=120);p.add_argument('--motion-frames',type=int,default=120);a=p.parse_args()
 if (a.benchmark or a.pacing_benchmark)and a.frames-a.warmup<480:raise ValueError('Need480 measured samples')
 a.output.mkdir(parents=True,exist_ok=False);data=Path(__file__).resolve().parents[1]/'data';env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','windows'if os.name=='nt'else'offscreen');env.setdefault('MESA_SHADER_CACHE_DIR',str(a.output/'mesa'))
 kit=json.loads(a.grass_kit.read_text());assert hashlib.sha256((a.grass_kit.parent/kit['atlas']).read_bytes()).hexdigest()==kit['atlas_sha256']
 runs=[];checks=[];metrics=[]
 def save():
  for name,value in [('RUNS',runs),('CHECKS',checks),('PERFORMANCE',metrics)]: (a.output/(name+'.json')).write_text(json.dumps(value,indent=2)+'\n')
  if metrics:
   keys=list(dict.fromkeys(k for m in metrics for k in m))
   with (a.output/'PERFORMANCE.csv').open('w',newline='')as f:w=csv.DictWriter(f,fieldnames=keys);w.writeheader();w.writerows(metrics)
 def run(name,width=640,grass='medium',pose=14,flags=(),frames=3,capture=0,route=None,timing=False,original=False):
  out=a.output/name;exe=a.baseline if original else a.executable
  cmd=[str(exe.resolve()),'--pack',str(a.garden_pack.resolve()),'--scene',str(data/'garden.json'),'--fixture-id','garden','--look',str(data/'TOON_DIAGNOSTIC.json'),'--style','C','--no-art-materials','--no-smooth-lighting','--mode','PIXEL3D','--study','B1','--pixel-camera','off','--edge-mode','combined','--shadow-proof','stable','--shadow','cached','--shadow-size','512','--shafts','off','--present-scale','2','--resolution',str(width),'--pose',str(pose),'--cap','0','--frames',str(frames),'--warmup',str(a.warmup if timing else 0),'--hidden','--output',str(out)]
  if not original:cmd+=['--grass-kit',str(a.grass_kit.resolve()),'--grass',grass,'--grass-facing','view','--grass-seed','17001']
  if a.allow_software:cmd+=['--allow-software']
  if route:cmd+=['--route',route]
  if not timing:cmd+=['--capture-presented']
  if capture:cmd+=['--capture-every',str(capture)]
  cmd+=list(flags);r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=600);out.mkdir(exist_ok=True)
  (out/'COMMAND.json').write_text(json.dumps(cmd,indent=2));(out/'stdout.txt').write_text(r.stdout);(out/'stderr.txt').write_text(r.stderr)
  if r.returncode:raise RuntimeError(name+'\n'+r.stderr)
  for file in out.glob('*.ppm'):
   im=Image.open(file);png=file.with_suffix('.png');im.save(png);assert Image.open(png).tobytes()==im.tobytes();file.unlink()
  s=json.loads((out/'summary.json').read_text());runs.append({'run':name,'source':s['source'],'command':cmd,'scope':'MESA_SOFTWARE_NOT_INTEL_UHD'if not s['hardware_gpu_tested']else'ACTUAL_DEVICE_API_NOT_SCANOUT','timing':timing,'grass':grass if not original else'original_H4'})
  if timing:
   rows=[r for r in csv.DictReader((out/'frames.csv').open())if int(r['frame'])>=a.warmup];g=[r for r in csv.DictReader((out/'grass_frames.csv').open())if int(r['frame'])>=a.warmup];assert len(rows)>=480
   t=json.loads((out/'pacing.json').read_text());m=json.loads((out/'grass.json').read_text());q=lambda v,p:sorted(v)[int((len(v)-1)*p)]
   record={'run':name,'width':width,'density':grass,'scope':runs[-1]['scope'],'source':s['source'],'cap':s['cap_fps'],'pacing':s['pacing'],'timed_samples':len(rows),'loop_p50_ms':t['loop_p50_ms'],'loop_p95_ms':t['loop_p95_ms'],'loop_p99_ms':t['loop_p99_ms'],'API_return_p50_ms':t['API_present_interval_p50_ms'],'API_return_p95_ms':t['API_present_interval_p95_ms'],'API_return_p99_ms':t['API_present_interval_p99_ms'],'CPU_work_p95_ms':t['CPU_work_wall_p95_ms'],'CPU_grass_p50_ms':q([float(x['CPU_grass_ms'])for x in g],.5),'CPU_grass_p95_ms':q([float(x['CPU_grass_ms'])for x in g],.95),'device_or_software_query_p50_ms':s['gpu_query_median_ms'],'device_or_software_query_p95_ms':s['gpu_query_p95_ms'],'query_valid':s['gpu_query_valid_samples'],'query_missing':s['gpu_query_missing_samples'],'process_peak_MiB':s['process_peak_rss_bytes']/1048576,'process_CPU_core_percent':t['process_CPU_core_percent'],'atlas_bytes':m['atlas_bytes'],'grass_buffer_bytes':m['buffer_bytes'],'visible_clumps_mean':statistics.mean(int(x['visible'])for x in g),'grass_draw_calls_mean':statistics.mean(int(x['grass_draw_calls'])for x in g),'grass_draw_calls_max':max(int(x['grass_draw_calls'])for x in g),'grass_triangles_mean':statistics.mean(int(x['grass_triangles'])for x in g),'scene_draw_calls_mean':statistics.mean(int(x['draw_calls'])for x in rows),'scene_triangles_mean':statistics.mean(int(x['total_scene_triangles_submitted'])for x in rows),'near_mean':statistics.mean(int(x['near'])for x in g),'mid_mean':statistics.mean(int(x['mid'])for x in g),'frustum_culled_mean':statistics.mean(int(x['frustum_culled'])for x in g),'distance_culled_mean':statistics.mean(int(x['distance_culled'])for x in g),'scanout_verified':False,'energy_thermals':'NOT_MEASURED','owner_Intel_UHD':'NOT_RUN'if not s['hardware_gpu_tested']else'SEE_ACTUAL_DEVICE'}
   metrics.append(record);assert record['grass_draw_calls_max']<=2
  save();print(name,flush=True);return out
 def equal(name,x,y,moving=False):
  images=[x/'final.png',x/'final_presented.png']
  if moving:images+=sorted(x.glob('frame_*.png'))
  for file in images:assert Image.open(file).tobytes()==Image.open(y/file.name).tobytes(),name+' '+file.name
  keys=['camera_x','camera_y','camera_z','yaw','pitch'];read=lambda d:[[r[k]for k in keys]for r in csv.DictReader((d/'frames.csv').open())]
  assert read(x)==read(y),name+' camera mismatch';checks.append({'name':name,'result':'PASS','exact_RGB_captures':len(images),'camera_rows_equal':True});save()
 def sheet(name,items):
  images=[Image.open(d/'final.png').convert('RGB')for _,d in items];w,h=images[0].size;sheet=Image.new('RGB',(w*2,(h+24)*((len(images)+1)//2)),(21,30,26));draw=ImageDraw.Draw(sheet)
  for i,((label,_),im)in enumerate(zip(items,images)):x=i%2*w;y=i//2*(h+24);draw.text((x+8,y+5),label,fill='white');sheet.paste(im,(x,y+24))
  sheet.save(a.output/name)
 if a.stills:
  for width in [480,640]:
   for pose in [14,13,22,23,24]:
    items=[(density.upper(),run(f'still_{width}_p{pose}_{density}',width,density,pose))for density in ['off','sparse','medium','dense']];sheet(f'ACTUAL_NATIVE_{width}_p{pose}.png',items)
    for density,d in items:
     im=Image.open(d/'final.png');im.crop((width//4,im.height//2,width*3//4,im.height)).save(d/'detail_1x.png')
    if a.baseline and pose in [14,13]:equal(f'grass_OFF_PR35_{width}_p{pose}',run(f'PR35_{width}_p{pose}',width,'off',pose,original=True),items[0][1])
   for pose in [22,23]:
    ref=a.output/f'still_{width}_p{pose}_medium';items=[('VIEW_FACING',ref)]
    for facing in ['cylindrical','crossed']:items.append((facing.upper(),run(f'facing_{width}_p{pose}_{facing}',width,pose=pose,flags=['--grass-facing',facing])))
    sheet(f'FACING_ACTUAL_{width}_p{pose}.png',items)
    noCull=run(f'frustum_OFF_{width}_p{pose}',width,pose=pose,flags=['--grass-frustum','off']);equal(f'conservative_grass_culling_{width}_{pose}',ref,noCull)
 if a.motion:
  for width in [480,640]:
   for route in ['strafe','orbit']:
    dirs=[run(f'motion_{width}_{route}_{density}',width,density,frames=a.motion_frames,capture=2,route=route,flags=['--grass-eye-route'])for density in ['off','sparse','medium','dense']]
    cmd=['ffmpeg','-v','error','-y']
    for d in dirs:cmd+=['-framerate','15','-pattern_type','glob','-i',str(d/'frame_*.png')]
    height=width*9//16;filt=f'[0:v][1:v][2:v][3:v]xstack=inputs=4:layout=0_0|{width}_0|0_{height}|{width}_{height}[v]'
    subprocess.run(cmd+['-filter_complex',filt,'-map','[v]','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(a.output/f'GRASS_OFF_SPARSE_MEDIUM_DENSE_{width}_{route}_REPLAY_NOT_FPS.mp4')],check=True)
    if a.baseline:
     parent=run(f'PR35_motion_{width}_{route}',width,'off',frames=a.motion_frames,capture=2,route=route,original=True)
     control=run(f'H4_route_OFF_{width}_{route}',width,'off',frames=a.motion_frames,capture=2,route=route)
     equal(f'exact_original_H4_route_{width}_{route}',parent,control,True)
   run(f'LOD_approach_{width}',width,'dense',frames=a.motion_frames,capture=2,flags=['--grass-lod-route'])
 if a.benchmark:
  for width in [480,640]:
   for density in ['off','sparse','medium','dense']:run(f'cost_{width}_{density}',width,density,frames=a.frames,route='strafe',timing=True)
 if a.pacing_benchmark:
  for width,cap in [(480,30),(640,60)]:
   for density in ['off','sparse','medium','dense']:
    for mode in ['legacy','precise']:run(f'paced_{width}_{density}_{mode}',width,density,frames=a.frames,route='strafe',timing=True,flags=['--cap',str(cap),'--pacing',mode])
 (a.output/'CAPTURE_SCOPE.txt').write_text('Actual original atlas and running executable. Native/presented PNGs are lossless PPM conversions. Sheets only arrange images with external captions. Movies replay one capture per2 deterministic frames at15fps: top OFF/SPARSE, bottom MEDIUM/DENSE; native component frames are retained. These are motion/appearance evidence, NOT displayed FPS, real-time footage or pacing proof. Timing runs have no capture/readback; software measurements are NOT Intel UHD. Original H4 route OFF parity is checked separately without the new eye-height override. Top approximation and eye/side views use the same scene/lighting. No reference pixels or AI imagery are used as engine evidence.\n')
 save();print('DONE',len(runs),'runs',len(checks),'checks',len(metrics),'benchmarks',flush=True)
if __name__=='__main__':main()
