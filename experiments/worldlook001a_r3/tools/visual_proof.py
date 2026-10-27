"""Bounded real-engine R3 visual gate. No artwork generation or benchmark suite."""
import argparse,csv,json,os,subprocess
from pathlib import Path
from PIL import Image,ImageDraw
PROFILES=['OLD_M1','OLD_M2','FINE_R3','R3_GRASS_OFF']
LABELS=['OLD M1: 24 original-scale tufts','OLD M2: 24 original-scale tufts','FINE R3: 360 fine tufts','GRASS OFF: same terrain and camera']

def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--kit',required=True);p.add_argument('--old-kit',required=True);p.add_argument('--output',required=True);p.add_argument('--stills',action='store_true');p.add_argument('--motion',action='store_true');p.add_argument('--checks',action='store_true');p.add_argument('--parent');a=p.parse_args();out=Path(a.output);out.mkdir(parents=True,exist_ok=False)
 root=Path(__file__).resolve().parents[1];env=dict(os.environ);base=[a.exe,'--kit',a.kit,'--old-kit',a.old_kit,'--look',str(root/'data/TOON_DIAGNOSTIC.json')]
 if os.name!='nt':base+=['--allow-software','--hidden'];env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('LIBGL_ALWAYS_SOFTWARE','1')
 commands=[];checks=[];labels=LABELS.copy();labels[2]=f"FINE R3: {len(json.loads(Path(a.kit).read_text())['roots'])} fine tufts"
 def run(name,flags):
  dest=out/name;cmd=base+list(map(str,flags))+['--output',str(dest)];commands.append({'name':name,'argv':cmd});print(name,flush=True)
  r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=45);(out/(name+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr
  im=Image.open(dest/'final.ppm');im.save(dest/'native.png');assert Image.open(dest/'native.png').tobytes()==im.tobytes();return dest
 if a.stills:
  for size in[480,640]:
   sheet=Image.new('RGB',(size*2,(size*9//16+28)*2),'#162430');d=ImageDraw.Draw(sheet)
   for i,profile in enumerate(PROFILES):
    dest=run(f'{profile}-{size}',['--profile',profile,'--resolution',size,'--frames',2,'--cap',0]+(['--debug']if profile=='FINE_R3'and size==480 else[]));x=i%2*size;y=i//2*(size*9//16+28);sheet.paste(Image.open(dest/'native.png'),(x,y+28));d.text((x+6,y+7),labels[i]+' | 1.65m eye',fill='white')
   sheet.save(out/f'COMPARISON_NATIVE_{size}.png');run(f'FINE_R3_DOWN-{size}',['--profile','FINE_R3','--pose',1,'--resolution',size,'--frames',2,'--cap',0])
 if a.motion:
  for size in[480,640]:
   paths=[];rows=[]
   for profile in PROFILES:
    dest=run(f'motion-{profile}-{size}',['--profile',profile,'--resolution',size,'--frames',240,'--route','--capture-every',1,'--cap',30]);paths.append(dest);rows.append(list(csv.DictReader((dest/'frames.csv').open())))
   keys=['camera_x','camera_y','camera_z','yaw','pitch','width','height'];assert all(len(r)==240 for r in rows)
   for i in range(240):assert all(all(r[i][k]==rows[0][i][k]for k in keys)for r in rows)
   height=(size*9//16+28)*2;movie=out/f'COMPARISON_MOTION_{size}_CAMERA_ALIGNED_NOT_FPS.mp4'
   enc=subprocess.Popen(['ffmpeg','-v','error','-y','-f','rawvideo','-pix_fmt','rgb24','-s',f'{size*2}x{height}','-r','30','-i','-','-c:v','libx264','-crf','20','-pix_fmt','yuv420p',str(movie)],stdin=subprocess.PIPE)
   for frame in range(240):
    im=Image.new('RGB',(size*2,height),'#162430');d=ImageDraw.Draw(im)
    for i,path in enumerate(paths):
     x=i%2*size;y=i//2*(size*9//16+28);im.paste(Image.open(path/f'frame_{frame:06d}.ppm'),(x,y+28));d.text((x+6,y+7),labels[i]+' | ENGINE REPLAY, not FPS',fill='white')
    enc.stdin.write(im.tobytes())
   enc.stdin.close();assert enc.wait()==0
   checks.append({'type':'motion','resolution':size,'frames':240,'four_profiles_camera_exact':True,'eye_height_1_65m':all(abs(float(r['camera_y'])-1.65)<1e-5 for r in rows[0]),'rendered_route':'walk/strafe/turn/look down','review_movie':'camera-aligned30fps replay, not hardware FPS'})
   if size==480:
    dest=paths[2];times=[int(r['after_swap_ns'])for r in rows[2]];lines=['ffconcat version 1.0']
    for i in range(240):lines +=[f"file 'frame_{i:06d}.ppm'",f'duration {((times[i+1]-times[i])/1e9 if i<239 else 1/30):.9f}']
    lines.append("file 'frame_000239.ppm'");(dest/'timed.ffconcat').write_text('\n'.join(lines)+'\n');subprocess.run(['ffmpeg','-v','error','-y','-safe','0','-i',str(dest/'timed.ffconcat'),'-fps_mode','vfr','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(out/'FINE_R3_NATIVE480_ACTUAL_API_TIMING.mp4')],check=True)
 if a.checks:
  for size in[480,640]:
   normal=out/f'FINE_R3-{size}';assert normal.exists(),'--checks requires --stills'
   unculled=run(f'culling-off-{size}',['--profile','FINE_R3','--resolution',size,'--no-cull','--frames',2,'--cap',0]);assert Image.open(normal/'native.png').tobytes()==Image.open(unculled/'native.png').tobytes();checks.append({'type':'actual_mesh_frustum_RGB_exact','resolution':size,'PASS':True})
   if a.parent:
    new=run(f'legacy-off-new-{size}',['--profile','R3_GRASS_OFF','--resolution',size,'--pose',14,'--frames',2,'--cap',0]);parent=out/f'legacy-off-parent-{size}';old=subprocess.run([a.parent,'--pack',str(Path(a.kit).parent/'pack.json'),'--scene',str(Path(a.kit).parent/'scene.json'),'--look',str(root/'data/TOON_DIAGNOSTIC.json'),'--style','C','--no-art-materials','--no-smooth-lighting','--study','B1','--shadow-proof','stable','--shadow','cached','--shafts','off','--grass','off','--resolution',str(size),'--pose','14','--frames','2','--warmup','0','--cap','0','--allow-software','--hidden','--output',str(parent)],env=env,capture_output=True,text=True,timeout=45);assert old.returncode==0,old.stderr;assert Image.open(new/'native.png').tobytes()==Image.open(parent/'final.ppm').tobytes(),'opaque renderer output changed';checks.append({'type':'actual_parent_grass_OFF_RGB_exact','resolution':size,'PASS':True})
 (out/'COMMANDS.json').write_text(json.dumps(commands,indent=2));(out/'VISUAL_CHECKS.json').write_text(json.dumps({'status':'R3_FINE_MEADOW_ART_REVIEW_PENDING','checks':checks,'benchmark_suite_run':False,'Windows_Intel_UHD_Android':'DEFERRED_NOT_RUN'},indent=2));print('SMALL_VISUAL_GATE_FINISHED',flush=True)
if __name__=='__main__':main()
