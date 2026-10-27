"""Actual executable small-plot evidence; private input/output only. Pillow/numpy/ffmpeg.
No art generated here. Captures never participate in cost measurements.
"""
import argparse,csv,hashlib,json,os,subprocess
from pathlib import Path
from PIL import Image,ImageDraw

def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--kit',required=True);p.add_argument('--legacy-kit');p.add_argument('--output',required=True);p.add_argument('--stills',action='store_true');p.add_argument('--motion',action='store_true');p.add_argument('--benchmark',action='store_true');p.add_argument('--pacing',action='store_true');a=p.parse_args()
 out=Path(a.output);out.mkdir(parents=True,exist_ok=False);root=Path(__file__).resolve().parents[1];env=dict(os.environ)
 base=[a.exe,'--r2-kit',a.kit,'--look',str(root/'data/TOON_DIAGNOSTIC.json'),'--no-art-materials','--no-smooth-lighting','--study','B1','--shadow-proof','stable','--shadow','cached','--shadow-size','512','--shafts','off','--pose','0','--present-scale','2','--r2-count','24']
 if os.name!='nt':base+=['--hidden','--allow-software'];env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('LIBGL_ALWAYS_SOFTWARE','1')
 commands=[]
 def run(name,flags):
  dest=out/name;cmd=base+list(map(str,flags))+['--output',str(dest)];commands.append({'name':name,'argv':cmd});print(name,flush=True)
  r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=240);(out/(name+'.log')).write_text(r.stdout+r.stderr)
  if r.returncode:raise RuntimeError(r.stderr)
  for f in dest.glob('*.ppm'):
   if f.name=='final.ppm' or f.name.startswith('frame_'):
    im=Image.open(f);im.save(f.with_suffix('.png'));assert Image.open(f.with_suffix('.png')).tobytes()==im.tobytes()
  return dest
 def still(name,model,res,ground='original',placement='uniform',pose=0,extra=()):
  flags=['--r2-model',model,'--resolution',res,'--r2-ground',ground,'--r2-placement',placement,'--pose',pose,'--frames',3,'--warmup',0,'--cap',0,*extra]
  if model=='B0':
   if not a.legacy_kit:return
   flags+=['--grass-kit',a.legacy_kit]
  return run(name,flags)
 if a.stills:
  for res in [480,640]:
   for model in ['off','B0','M1','M2']:
    for pose in [0,1,2,3]:still(f'shape-{model}-{res}-p{pose}',model,res,pose=pose)
   for model in ['M1','M2']:
    for ground in ['original','patches']:
     for place in ['uniform','clustered']:still(f'independent-{model}-{ground}-{place}-{res}',model,res,ground,place)
    for pose in range(4):still(f'studio-{model}-{res}-p{pose}',model,res,pose=pose,extra=['--r2-inspect','--r2-count',1,'--r2-diagnostic','albedo'])
    for cast in ['on','off']:still(f'cast-{model}-{cast}-{res}',model,res,'patches','clustered',extra=['--r2-cast',cast])
    c=still(f'cull-{model}-off-{res}',model,res,'patches','clustered',extra=['--r2-frustum','off'])
    reference=out/f'independent-{model}-patches-clustered-{res}'/'final.png'
    assert Image.open(c/'final.png').tobytes()==Image.open(reference).tobytes(),'actual art culling clipped pixels'
  # Contact sheets are labelled outside each unaltered native framebuffer.
  for res in [480,640]:
   for model in ['M1','M2']:
    paths=[out/f'independent-{model}-{g}-{l}-{res}'/'final.png'for g in ['original','patches']for l in ['uniform','clustered']]
    sheet=Image.new('RGB',(res*2,(res*9//16+28)*2),'#18232c');d=ImageDraw.Draw(sheet)
    for i,path in enumerate(paths):x=i%2*res;y=i//2*(res*9//16+28);sheet.paste(Image.open(path),(x,y+28));d.text((x+6,y+6),path.parent.name,fill='white')
    sheet.save(out/f'COMPARE-{model}-GROUND-PLACEMENT-{res}.png')
 if a.motion:
  for res in [480,640]:
   for model in ['M1','M2']:
    for route in (['walk','strafe','lookdown','orbit','turn']if res==480 else['walk']):
     name=f'motion-{model}-{route}-{res}';dest=run(name,['--r2-model',model,'--r2-ground','patches','--r2-placement','clustered','--resolution',res,'--r2-route',route,'--frames',450,'--warmup',30,'--capture-every',1,'--pacing','precise','--cap',30])
     rows=list(csv.DictReader((dest/'frames.csv').open()));times=[int(r['after_swap_ns'])for r in rows]
     ff=dest/'timed.ffconcat';lines=['ffconcat version 1.0']
     for i in range(len(rows)):
      lines.append(f"file 'frame_{i:06d}.png'");duration=(times[i+1]-times[i])/1e9 if i+1<len(rows)else 1/30;lines.append(f'duration {max(.001,duration):.9f}')
     lines.append(f"file 'frame_{len(rows)-1:06d}.png'");ff.write_text('\n'.join(lines)+'\n')
     subprocess.run(['ffmpeg','-v','error','-y','-safe','0','-i',str(ff),'-fps_mode','vfr','-c:v','libx264','-crf','15','-pix_fmt','yuv420p',str(out/(name+'.mp4'))],check=True)
     (dest/'motion_timing.json').write_text(json.dumps({'frames':len(rows),'actual_API_span_seconds':(times[-1]-times[0])/1e9,'timing':'VFR replay follows measured after_swap timestamps; captures included; NOT verified monitor scanout','model_transforms':'world-fixed; camera moved by executable','resolution':res},indent=2))
 if a.benchmark:
  for res in [480,640]:
   for model in ['off','B0','M1','M2']:
    if model=='B0'and not a.legacy_kit:continue
    for count in ([24]if model=='off'else[8,16,24]):
     flags=['--r2-model',model,'--r2-count',count,'--r2-ground','patches','--r2-placement','clustered','--resolution',res,'--frames',600,'--warmup',120,'--cap',0]
     if model=='B0':flags+=['--grass-kit',a.legacy_kit]
     run(f'cost-{model}-{count}-{res}',flags)
   for control,flags in [('ground',['--r2-ground','original']),('placement',['--r2-placement','uniform']),('cast',['--r2-cast','off']),('frustum',['--r2-frustum','off'])]:
    run(f'control-{control}-{res}',['--r2-model','M1','--r2-ground','patches','--r2-placement','clustered','--resolution',res,'--frames',600,'--warmup',120,'--cap',0,*flags])
 if a.pacing:
  for res in [480,640]:
   for model in ['M1','M2']:
    for cap in [30,60]:run(f'pacing-{model}-{res}-{cap}',['--r2-model',model,'--r2-ground','patches','--r2-placement','clustered','--resolution',res,'--frames',600,'--warmup',120,'--cap',cap,'--pacing','precise'])
 (out/'COMMANDS.json').write_text(json.dumps(commands,indent=2))
 print('PROOF_FINISHED',len(commands),flush=True)
if __name__=='__main__':main()
