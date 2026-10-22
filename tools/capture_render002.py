"""Capture private flat/house executable evidence; never manufactures scene imagery."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw


def capture(args):
    exe,packs,layout,treatment,house,output=[Path(getattr(args,k)).resolve() for k in ['exe','packs','layout','treatment','house','output']]
    if output.exists():raise FileExistsError('Fresh output required')
    if any((p/'.git').is_file() or (p/'.git/HEAD').is_file() for p in [output,*output.parents]):raise ValueError('Private captures must stay outside Git')
    inputs=[exe,layout,treatment,house,*(packs/f'sampling_{m}.wst' for m in [64,128,256])]
    hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    output.mkdir(parents=True);records=[]
    def run(name,variant,mode,size,camera,frames=1,screenshot=True):
        prefix=output/name
        cmd=[str(exe),'--sample-pack',str(packs/f'sampling_{mode}.wst'),'--facade-layout',str(layout),'--facade-scale','human','--facade-look','c','--facade-treatment',str(treatment),'--facade-projection','perspective','--camera',*map(str,camera),'--resolution',size,'--frames',str(frames),'--warmup','0','--fps','0','--output',str(prefix)]
        if variant=='house':cmd+=['--house-geometry',str(house)]
        if screenshot:cmd+=['--screenshot',str(prefix.with_suffix('.ppm'))]
        subprocess.run(cmd,check=True,stdout=subprocess.DEVNULL,timeout=180)
        report=json.loads(prefix.with_suffix('.json').read_text())
        assert report['pipeline']=='HEADLESS_CPU_ONLY'
        if screenshot:
            with Image.open(prefix.with_suffix('.ppm')) as im:im.save(prefix.with_suffix('.png'))
        record=dict(name=name,variant=variant,sampling=mode,resolution=size,camera=camera,command=cmd,report=report)
        if frames>=1000:
            with Path(str(prefix)+'_frames.csv').open(newline='') as f:rows=list(csv.DictReader(f))[500:]
            def q(key,fraction):
                values=sorted(float(r[key]) for r in rows);return values[math.ceil(fraction*len(values))-1]
            record['summary']=dict(medianMs=q('frame_ms',.5),p95Ms=q('frame_ms',.95),p99Ms=q('frame_ms',.99),geometryP95Ms=q('geometry_ms',.95),summaryFrames=len(rows),peakRssMiB=report['processPeakRssMiB'])
        records.append(record);return prefix.with_suffix('.png')
    poses=[('front',[11.5,19,0]),('corner',[19,17,-48]),('side',[19,9,-90]),('close',[11.5,14.5,0]),('normal',[12,16.5,0]),('rear',[11.5,3,180])]
    for mode in [64,128,256]:
        for size in ['320x180','480x270']:
            w,h=map(int,size.split('x'))
            for pose,camera in poses:
                sheet=Image.new('RGB',(2*w,h+28),(18,28,36));draw=ImageDraw.Draw(sheet)
                for i,variant in enumerate(['flat','house']):
                    path=run(f'{pose}_{mode}_{size}_{variant}',variant,mode,size,camera)
                    with Image.open(path) as im:sheet.paste(im,(i*w,28))
                    draw.text((i*w+4,8),f'{variant.upper()} / {pose} / {mode} / {size}',fill='white')
                sheet.save(output/f'comparison_{pose}_{mode}_{size}.png')
            print(f'Captured six A/B views: {mode} {size}',flush=True)
    # Identical explicit poses: no wall-clock simulation differences between A and B.
    for size in ['320x180','480x270']:
        w,h=map(int,size.split('x'));previews={v:[] for v in ['flat','house']};paired=[]
        for i in range(41):
            t=i/40;angle=t*math.pi/2
            x=11.5+7.5*math.sin(angle);y=9+10*math.cos(angle)
            yaw=math.degrees(math.atan2(11.5-x,y-9))
            camera=[x,y,yaw];sheet=Image.new('RGB',(w*2,h))
            for k,v in enumerate(['flat','house']):
                path=run(f'move_{size}_{v}_{i:02}',v,256,size,camera)
                with Image.open(path) as im:previews[v].append(im.copy());sheet.paste(im,(k*w,0))
            paired.append(sheet)
        for name,frames in [*previews.items(),('comparison',paired)]:
            frames[0].save(output/f'moving_{size}_{name}.apng',format='PNG',save_all=True,append_images=frames[1:],duration=100,loop=0,disposal=0,blend=0)
    for size in ['320x180','480x270']:
        for mode in [64,128,256]:
            for i,variant in enumerate(['flat','house','house','flat']):
                run(f'bench_{size}_{mode}_{i}_{variant}',variant,mode,size,[19,17,-48],frames=args.bench_frames,screenshot=False)
            print(f'Measured ABBA: {mode} {size}',flush=True)
    for path,digest in hashes.items():assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    (output/'capture_manifest.json').write_text(json.dumps(dict(schema='wildsignal-render002-evidence-v1',inputsSha256=hashes,records=records,timing='Uncapped Linux CPU-only shared host screening; ABBA at each configuration, first 500 frames excluded from summary. Not sustained/device acceptance.'),indent=2))

if __name__=='__main__':
    p=argparse.ArgumentParser()
    for k in ['exe','packs','layout','treatment','house','output']:p.add_argument('--'+k,required=True)
    p.add_argument('--bench-frames',type=int,default=2000)
    capture(p.parse_args())
