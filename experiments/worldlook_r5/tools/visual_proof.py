"""R5 small native A/B visual proof; actual external assets, no art generation."""
import argparse,os,subprocess,json,csv
from pathlib import Path
from PIL import Image,ImageDraw

def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--kit',required=True);p.add_argument('--surface',required=True);p.add_argument('--soft-kit',required=True);p.add_argument('--parent',required=True);p.add_argument('--output',required=True);p.add_argument('--motion',action='store_true');a=p.parse_args();out=Path(a.output);out.mkdir(parents=True,exist_ok=False);root=Path(__file__).resolve().parents[2];look=root/'worldlook001a_r4/data/TOON_DIAGNOSTIC.json';env=dict(os.environ);base=[a.exe,'--kit',a.kit,'--surface',a.surface,'--look',str(look),'--profile','PIXEL_R4'];extra=[]
 if os.name!='nt':extra=['--allow-software','--hidden'];env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('LIBGL_ALWAYS_SOFTWARE','1')
 base+=extra;commands=[];checks=[];labels=['A: unchanged R4','B: 70% fine filler / 30% soft leaves']
 def run(name,size,pose=0,soft=False,flags=(),parent=False):
  cmd=base.copy();cmd[0]=a.parent if parent else a.exe;cmd+=['--resolution',str(size),'--pose',str(pose),'--frames','2','--cap','0'];dest=out/name
  if soft:cmd+=['--soft-kit',a.soft_kit]
  cmd+=list(map(str,flags))+['--output',str(dest)];commands.append({'name':name,'argv':cmd});r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=90);(out/(name+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr;im=Image.open(dest/'final.ppm');im.save(dest/'native.png');assert Image.open(dest/'native.png').tobytes()==im.tobytes();return dest
 def sheet(paths,size,name):
  im=Image.new('RGB',(size*2,size*9//16+28),'#182734');d=ImageDraw.Draw(im)
  for i,path in enumerate(paths):im.paste(Image.open(path),(i*size,28));d.text((i*size+5,8),labels[i]+' | 1.65m eye / 60deg FOV',fill='white')
  im.save(out/name)
 for size in[480,640]:
  off=run(f'ground-OFF-{size}',size,flags=['--profile','R4_GRASS_OFF']);oldoff=run(f'parent-ground-OFF-{size}',size,flags=['--profile','R4_GRASS_OFF'],parent=True);assert Image.open(off/'native.png').tobytes()==Image.open(oldoff/'native.png').tobytes();checks.append({'check':'R4_ground_OFF_RGB_exact','width':size,'PASS':True})
  for pose in[0,1]:
   control=run(f'A_R4-{size}-pose{pose}',size,pose);soft=run(f'B_R5-{size}-pose{pose}',size,pose,True);parent=run(f'parent-{size}-pose{pose}',size,pose,parent=True);assert Image.open(control/'native.png').tobytes()==Image.open(parent/'native.png').tobytes(),'R4 control changed'
   arows=list(csv.DictReader((control/'transforms.csv').open()));brows=list(csv.DictReader((soft/'transforms.csv').open()));keys=['x','y','z','tint_r','tint_g','tint_b'];assert len(arows)==len(brows)==360;assert all(a[k]==b[k]for a,b in zip(arows,brows)for k in keys);assert sum(a['variant']!=b['variant']for a,b in zip(arows,brows))==108
   sheet([control/'native.png',soft/'native.png'],size,f'COMPARE_NATIVE_{size}_pose{pose}.png');checks.append({'check':'R4_control_RGB_exact_and_108_replacements','width':size,'pose':pose,'PASS':True})
  uncull=run(f'B-no-cull-{size}',size,soft=True,flags=['--no-cull']);assert Image.open(uncull/'native.png').tobytes()==Image.open(out/f'B_R5-{size}-pose0/native.png').tobytes();checks.append({'check':'culling_RGB_exact_default_full_patch','width':size,'PASS':True})
 if a.motion:
  for size in[480,640]:
   paths=[run(f'motion-{mode}-{size}',size,soft=i==1,flags=['--route','--frames','240','--capture-every','1','--cap','30'])for i,mode in enumerate(['A_R4','B_R5'])];rows=[list(csv.DictReader((p/'frames.csv').open()))for p in paths];keys=['camera_x','camera_y','camera_z','yaw','pitch'];assert all(len(r)==240 for r in rows)
   for frame in range(240):assert all(rows[0][frame][k]==rows[1][frame][k]for k in keys)
   checks.append({'check':'240frame_camera_identical_geometry_bounds_may_change_culling','width':size,'PASS':True})
   enc=subprocess.Popen(['ffmpeg','-v','error','-y','-f','rawvideo','-pix_fmt','rgb24','-s',f'{size*2}x{size*9//16+28}','-r','30','-i','-','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(out/f'COMPARE_MOTION_{size}_REPLAY_NOT_FPS.mp4')],stdin=subprocess.PIPE)
   for frame in range(240):
    im=Image.new('RGB',(size*2,size*9//16+28),'#182734');d=ImageDraw.Draw(im)
    for i,path in enumerate(paths):im.paste(Image.open(path/f'frame_{frame:06d}.ppm'),(i*size,28));d.text((i*size+5,8),labels[i]+' | actual engine replay, not FPS',fill='white')
    enc.stdin.write(im.tobytes())
   enc.stdin.close();assert enc.wait()==0;sheet([p/'frame_000239.ppm'for p in paths],size,f'COMPARE_NATIVE_{size}_near_turn.png')
   if size==480:
    path=paths[1];times=[int(row['after_swap_ns'])for row in rows[1]];concat=['ffconcat version 1.0']
    for frame in range(240):concat +=[f"file 'frame_{frame:06d}.ppm'",f'duration {((times[frame+1]-times[frame])/1e9 if frame<239 else 1/30):.9f}']
    concat.append("file 'frame_000239.ppm'");(path/'timed.ffconcat').write_text('\n'.join(concat)+'\n');subprocess.run(['ffmpeg','-v','error','-y','-safe','0','-i',str(path/'timed.ffconcat'),'-fps_mode','vfr','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(out/'R5_NATIVE480_ACTUAL_API_TIMING.mp4')],check=True)
 (out/'COMMANDS.json').write_text(json.dumps(commands,indent=2));(out/'VISUAL_CHECKS.json').write_text(json.dumps({'checks':checks,'renderer':'actual nativeC++/OpenGL; Mesa software only in Cloud','hardware_suite_run':False,'status':'R5_SOFT_LEAF_OWNER_VISUAL_REVIEW_PENDING'},indent=2));print('R5_SMALL_VISUAL_GATE_CAPTURED',flush=True)
if __name__=='__main__':main()
