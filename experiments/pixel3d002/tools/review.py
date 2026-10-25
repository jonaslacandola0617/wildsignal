"""Run the actual SDL/OpenGL executable; keep models and every capture PRIVATE.

Requires Pillow; optional ffmpeg for a convenience MP4. No scene art generation.
Software GL must be explicitly opted into, and never validates target hardware.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
from PIL import Image, ImageChops, ImageDraw

def execute(exe, pack, scene, root, name, flags, software):
    output=root/name
    cmd=[str(exe),'--pack',str(pack),'--scene',str(scene),'--output',str(output),'--hidden','--cap','0',*flags]
    env=os.environ.copy()
    if software:
        cmd+=['--allow-software']
        env.setdefault('SDL_VIDEODRIVER','offscreen')
    result=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=900)
    (root/(name+'.command.json')).write_text(json.dumps({'argv':cmd,'returncode':result.returncode,'stdout':result.stdout,'stderr':result.stderr},indent=2))
    if result.returncode:
        raise RuntimeError(result.stderr)
    return output

def png(path):
    im=Image.open(path).convert('RGB')
    im.save(path.with_suffix('.png'))
    im.resize((im.width*3,im.height*3),Image.Resampling.NEAREST).save(path.with_name(path.stem+'_3x.png'))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('executable',type=Path);p.add_argument('pack',type=Path);p.add_argument('scene',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--software-diagnostic',action='store_true')
    p.add_argument('--stills',action='store_true');p.add_argument('--motion',action='store_true');p.add_argument('--benchmark',action='store_true');p.add_argument('--correctness',action='store_true')
    p.add_argument('--benchmark-frames',type=int,default=600)
    a=p.parse_args();root=a.output.resolve()
    if root.exists():raise ValueError('choose a new PRIVATE output directory')
    root.mkdir(parents=True)
    def run(name,flags):return execute(a.executable.resolve(),a.pack.resolve(),a.scene.resolve(),root,name,flags,a.software_diagnostic)
    if a.stills:
        for w in (480,640):
            for mode in ('BASE','TOON','PIXEL3D'):
                for pose,name in enumerate(('front','corner','side','near','far')):
                    out=run(f'hero_{w}_{mode}_{name}',['--resolution',str(w),'--mode',mode,'--pose',str(pose),'--frames','3','--warmup','0'])
                    png(out/'final.ppm')
    if a.motion:
        for w in (480,640):
            for snap in (False,True):
                out=run(f'motion_{w}_snap_{int(snap)}',['--resolution',str(w),'--orbit','--frames','1200','--warmup','120','--capture-every','4']+(['--snap']if snap else[]))
                frames=[]
                for f in sorted(out.glob('frame_*.ppm')):
                    im=Image.open(f).convert('RGB');im.save(f.with_suffix('.png'));frames.append(im)
                assert len(frames)==300
                frames[0].save(out/'native_20s.apng',save_all=True,append_images=frames[1:],duration=1000/15,loop=0,disposal=0,blend=0)
                try:
                    subprocess.run(['ffmpeg','-y','-v','error','-framerate','15','-pattern_type','glob','-i',str(out/'frame_*.png'),'-vf','scale=1280:720:flags=neighbor','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(out/'preview_20s.mp4')],check=True,timeout=180)
                except FileNotFoundError:pass
    if a.correctness:
        results=[]
        for stress in (False,True):
            for w in (480,640):
                on=run(f'cull_check_{stress}_{w}_on',['--resolution',str(w),'--orbit','--frames','120','--capture-every','4']+(['--stress']if stress else[]))
                off=run(f'cull_check_{stress}_{w}_off',['--resolution',str(w),'--orbit','--frames','120','--capture-every','4','--no-cull']+(['--stress']if stress else[]))
                count=0
                for f in sorted(on.glob('*.ppm')):
                    same=f.read_bytes()==(off/f.name).read_bytes()
                    if not same:raise AssertionError(f'Culling pixel mismatch {f}')
                    count+=1
                left=list(csv.DictReader((on/'frames.csv').open()));right=list(csv.DictReader((off/'frames.csv').open()))
                assert len(left)==len(right)
                for l,r in zip(left,right):
                    for k in ('camera_x','camera_y','camera_z','yaw','pitch','width','height','mode'):
                        assert l[k]==r[k]
                png(on/'final.ppm');png(off/'final.ppm')
                results.append({'stress':stress,'width':w,'identical_framebuffers':count,'all_camera_rows_equal':True})
        (root/'CULLING_CORRECTNESS.json').write_text(json.dumps(results,indent=2))
    if a.benchmark:
        table=[]
        for stress in (False,True):
            for w in (480,640):
                for mode in ('BASE','TOON','PIXEL3D'):
                    # ABBA avoids a one-way run-order bias. No capture/encoding in
                    # timed routes. Same source/poses/driver; no glFinish per draw.
                    for repetition,on in enumerate((False,True,True,False)):
                        out=run(f'bench_{stress}_{w}_{mode}_{repetition}_{int(on)}',['--resolution',str(w),'--mode',mode,'--orbit','--frames',str(a.benchmark_frames),'--warmup','120']+(['--stress']if stress else[])+([]if on else['--no-cull']))
                        s=json.loads((out/'summary.json').read_text());s.update(scene='stress'if stress else'hero',width=w,mode=mode,cull=on,order=repetition)
                        table.append(s)
                        print(out.name,s['loop_median_ms'],s['loop_p95_ms'],flush=True)
        (root/'BENCHMARK_TABLE.json').write_text(json.dumps(table,indent=2))
    files=[]
    for f in sorted(root.rglob('*')):
        if f.is_file():files.append({'path':str(f.relative_to(root)),'sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'bytes':f.stat().st_size})
    (root/'FILE_MANIFEST.json').write_text(json.dumps(files,indent=2))

if __name__=='__main__':main()
