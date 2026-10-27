"""Bounded R4 art gate. Captures actual external art, never generates art."""
import argparse,csv,json,os,subprocess,hashlib
from pathlib import Path
from PIL import Image,ImageDraw
PROFILES=['A_R3','B_R4','B_GROUND_ONLY'];LABELS=['A: original R3','B: R4 material, same R3 meshes','R4 ground, grass OFF']
def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--kit',required=True);p.add_argument('--surface',required=True);p.add_argument('--parent',required=True);p.add_argument('--output',required=True);p.add_argument('--motion',action='store_true');a=p.parse_args();out=Path(a.output);out.mkdir(parents=True,exist_ok=False);root=Path(__file__).resolve().parents[1];env=dict(os.environ);look=root/'data/TOON_DIAGNOSTIC.json';base=[a.exe,'--kit',a.kit,'--look',str(look)];extra=[]
 if os.name!='nt':extra=['--allow-software','--hidden'];env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('LIBGL_ALWAYS_SOFTWARE','1')
 base+=extra;commands=[];checks=[]
 def run(name,profile,size,pose=0,flags=(),executable=None):
  cmd=base.copy();cmd[0]=executable or a.exe;cmd+=['--profile','R3_GRASS_OFF'if profile.endswith('OFF')or profile=='B_GROUND_ONLY'else'FINE_R3','--resolution',str(size),'--pose',str(pose),'--frames','2','--cap','0']
  if profile.startswith('B'):cmd+=['--surface',a.surface]
  dest=out/name;cmd+=list(map(str,flags))+['--output',str(dest)];commands.append({'name':name,'argv':cmd});r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=90);(out/(name+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr;im=Image.open(dest/'final.ppm');im.save(dest/'native.png');assert im.tobytes()==Image.open(dest/'native.png').tobytes();return dest
 for size in[480,640]:
  for pose in[0,1]:
   images=[]
   for profile in PROFILES:images.append(run(f'{profile}-{size}-pose{pose}',profile,size,pose))
   sheet=Image.new('RGB',(size*3,size*9//16+28),'#162430');d=ImageDraw.Draw(sheet)
   for i,dest in enumerate(images):sheet.paste(Image.open(dest/'native.png'),(size*i,28));d.text((size*i+5,8),LABELS[i]+' | 1.65m eye / 60deg FOV',fill='white')
   sheet.save(out/f'COMPARE_NATIVE_{size}_pose{pose}.png')
   for profile in['A_R3','A_OFF']:
    normal=images[0] if profile=='A_R3'else run(f'{profile}-{size}-pose{pose}',profile,size,pose)
    parent=run(f'parent-{profile}-{size}-pose{pose}',profile,size,pose,executable=a.parent);assert Image.open(normal/'native.png').tobytes()==Image.open(parent/'native.png').tobytes(),'R3 RGB changed';checks.append({'check':'R3_RGB_exact','size':size,'pose':pose,'grass':profile=='A_R3','PASS':True})
  unculled=run(f'B-no-cull-{size}','B_R4',size,flags=['--no-cull']);assert Image.open(unculled/'native.png').tobytes()==Image.open(out/f'B_R4-{size}-pose0/native.png').tobytes();checks.append({'check':'B_culling_default_view_RGB_exact','size':size,'coverage':'all patch visible at default view','PASS':True})
 albedo=run('B-ground-albedo-480','B_GROUND_ONLY',480,flags=['--albedo'])
 shade=json.loads(look.read_text());shade['sun_color']=[0,0,0];shadepath=out/'AMBIENT_ONLY_SHADE_CONTROL.json';shadepath.write_text(json.dumps(shade,indent=2))
 for profile in PROFILES:run(f'{profile}-480-ambient',profile,480,flags=['--look',shadepath])
 if a.motion:
  for size in[480,640]:
   paths=[];rows=[]
   for profile in PROFILES:
    dest=run(f'motion-{profile}-{size}',profile,size,flags=['--route','--frames','240','--capture-every','1','--cap','30']);paths.append(dest);rows.append(list(csv.DictReader((dest/'frames.csv').open())))
   keys=['camera_x','camera_y','camera_z','yaw','pitch','width','height','near_instances','far_instances','grass_triangles']
   for frame in range(240):
    assert all(rows[0][frame][k]==rows[1][frame][k]for k in keys)
    assert all(rows[0][frame][k]==rows[2][frame][k]for k in keys[:7]);assert float(rows[0][frame]['camera_y'])==1.64999998
   enc=subprocess.Popen(['ffmpeg','-v','error','-y','-f','rawvideo','-pix_fmt','rgb24','-s',f'{size*3}x{size*9//16+28}','-r','30','-i','-','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(out/f'COMPARE_MOTION_{size}_REPLAY_NOT_FPS.mp4')],stdin=subprocess.PIPE)
   for frame in range(240):
    im=Image.new('RGB',(size*3,size*9//16+28),'#162430');d=ImageDraw.Draw(im)
    for i,path in enumerate(paths):im.paste(Image.open(path/f'frame_{frame:06d}.ppm'),(size*i,28));d.text((size*i+5,8),LABELS[i]+' | real engine replay, not FPS',fill='white')
    enc.stdin.write(im.tobytes())
   enc.stdin.close();assert enc.wait()==0
   checks.append({'check':'camera_route_A_B_OFF_exact','size':size,'frames':240,'A_B_LOD_draw_counts_exact':True,'PASS':True})
   # A separate 480 native movie follows actual API completion intervals.
   if size==480:
    path=paths[1];times=[int(r['after_swap_ns'])for r in rows[1]];concat=['ffconcat version 1.0']
    for frame in range(240):concat +=[f"file 'frame_{frame:06d}.ppm'",f"duration {((times[frame+1]-times[frame])/1e9 if frame<239 else 1/30):.9f}"]
    concat.append("file 'frame_000239.ppm'");(path/'timed.ffconcat').write_text('\n'.join(concat)+'\n');subprocess.run(['ffmpeg','-v','error','-y','-safe','0','-i',str(path/'timed.ffconcat'),'-fps_mode','vfr','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(out/'R4_NATIVE480_ACTUAL_API_TIMING.mp4')],check=True)
 (out/'COMMANDS.json').write_text(json.dumps(commands,indent=2));(out/'VISUAL_CHECKS.json').write_text(json.dumps({'status':'R4_PIXEL_MEADOW_OWNER_VISUAL_REVIEW_PENDING','checks':checks,'benchmark_suite_run':False,'Windows_Intel_UHD_Android':'NOT_RUN_DEFERRED'},indent=2));print('R4_SMALL_VISUAL_GATE_CAPTURED',flush=True)
if __name__=='__main__':main()
