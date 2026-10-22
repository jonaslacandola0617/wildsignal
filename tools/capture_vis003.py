"""Actual private VIS-003 executable frames and short balanced CPU screening.
No generated artwork. Pillow only converts/assembles captured frames for review.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw


def capture(args):
    exe, packs, layout, treatment, study, output = [Path(p).resolve() for p in
        (args.exe, args.packs, args.layout, args.treatment, args.study, args.output)]
    if output.exists(): raise FileExistsError('Use a fresh private directory')
    if any((p/'.git').is_file() or (p/'.git/HEAD').is_file() for p in [output,*output.parents]):
        raise ValueError('Private evidence must stay outside Git checkouts')
    inputs=[exe,layout,treatment,study,*(packs/f'sampling_{m}.wst' for m in [64,128,256])]
    hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    base=json.loads(treatment.read_text());studies=json.loads(study.read_text())
    output.mkdir(parents=True);records=[]

    def run(name, look, mode, size, camera, projection='perspective', config=treatment, frames=2, screenshot=True):
        prefix=output/name
        cmd=[str(exe),'--sample-pack',str(packs/f'sampling_{mode}.wst'),'--facade-layout',str(layout),
             '--facade-scale','human','--facade-look',look,'--facade-treatment',str(config),
             '--facade-projection',projection,'--camera',*map(str,camera),'--resolution',size,
             '--frames',str(frames),'--warmup','0','--fps','0','--output',str(prefix)]
        if screenshot:cmd+=['--screenshot',str(prefix.with_suffix('.ppm'))]
        subprocess.run(cmd,check=True,stdout=subprocess.DEVNULL,timeout=180)
        report=json.loads(prefix.with_suffix('.json').read_text())
        if report['pipeline']!='HEADLESS_CPU_ONLY':raise ValueError('Use the headless executable; F12 captures the SDL display')
        if screenshot:
            with Image.open(prefix.with_suffix('.ppm')) as im:im.save(prefix.with_suffix('.png'))
        record=dict(name=name,look=look,sampling=mode,resolution=size,camera=camera,projection=projection,
                    report=prefix.name+'.json',screenshot=screenshot)
        if frames>=1000:
            with Path(str(prefix)+'_frames.csv').open(newline='') as f:
                values=sorted(float(row['frame_ms']) for row in list(csv.DictReader(f))[500:])
            record.update(medianMs=values[math.ceil(.5*len(values))-1],p95Ms=values[math.ceil(.95*len(values))-1],
                          p99Ms=values[math.ceil(.99*len(values))-1],peakRssMiB=report['processPeakRssMiB'],summaryFrames=len(values))
        records.append(record)
        return prefix.with_suffix('.png')

    poses=[('close',[11.5,15,0],'perspective'),('front',[12,16.5,0],'perspective'),
           ('street',[12,21.5,0],'perspective'),('corner',[17,14.5,-45],'perspective'),
           ('side',[18,9,-90],'perspective'),('ortho',[12,16.5,0],'orthographic')]
    for mode in [64,128,256]:
        for size in ['320x180','480x270']:
            width,height=map(int,size.split('x'))
            for pose,camera,projection in poses:
                sheet=Image.new('RGB',(3*width,height+28),(18,28,36));draw=ImageDraw.Draw(sheet)
                for i,look in enumerate('abc'):
                    name=f'{pose}_{mode}_{size}_{look}'
                    path=run(name,look,mode,size,camera,projection)
                    with Image.open(path) as im:sheet.paste(im,(i*width,28))
                    draw.text((i*width+4,8),f'{look.upper()} / {mode} / {size} / {pose}',fill=(246,235,215))
                sheet.save(output/f'comparison_{pose}_{mode}_{size}.png')
                print(f'Captured {pose} {mode} {size}',flush=True)

    for size in ['320x180','480x270']:
        width,height=map(int,size.split('x'))
        items=studies['plasters'];sheet=Image.new('RGB',(2*width,math.ceil(len(items)/2)*(height+28)),(18,28,36));draw=ImageDraw.Draw(sheet)
        for i,item in enumerate(items):
            config=output/(item['name']+'.layout.json');config.write_text(json.dumps(base|{'plasterMaterial':item['material']}))
            path=run(f'plaster_{item["name"]}_{size}','b',256,size,[12,16.5,0],config=config)
            x=(i%2)*width;y=(i//2)*(height+28)
            draw.text((x+4,y+8),item['name']+' / fixed framing, ground, pose',fill='white')
            with Image.open(path) as im:sheet.paste(im,(x,y+28))
        sheet.save(output/f'comparison_plasters_{size}.png')
        controls={'ground_only':base|{'plasterMaterial':-1},
                  'plaster_only':base|{'ground':dict(material=0,u0=0,v0=0,u1=1,v1=1),'groundRepeat':1},
                  'alternative_ground':base|{'ground':studies['alternativeGround']}}
        for name,value in controls.items():
            config=output/(name+'.layout.json');config.write_text(json.dumps(value))
            run(f'control_{name}_{size}','b',256,size,[12,16.5,0],config=config)

    for size in ['320x180','480x270']:
        for look in 'abc':
            sequence=[]
            for i in range(25):
                t=i/24;camera=[11.8+.4*t,17-t,-12+24*t]
                path=run(f'turn_{size}_{look}_{i:02}',look,256,size,camera,frames=1)
                with Image.open(path) as im:sequence.append(im.copy())
            sequence[0].save(output/f'engine_turn_{size}_{look}.apng',format='PNG',save_all=True,append_images=sequence[1:],duration=80,loop=0,disposal=0,blend=0)

    # Balanced order, all reports retain the warm frames; summary discards first 500.
    for i,look in enumerate('abccba'):
        run(f'bench_{i}_{look}',look,256,'480x270',[12,16.5,0],frames=3000,screenshot=False)
        print(f'Measured screening {i} {look}',flush=True)
    for path,h in hashes.items():
        if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=h:raise RuntimeError('An input changed during capture')
    summary=dict(schema='wildsignal-vis003-captures-v1',inputsSha256=hashes,records=records,
                 timing='HEADLESS_CPU_ONLY, shared host, uncapped; six 3000-frame runs in A/B/C/C/B/A order. Summary discards first 500. Not device acceptance or standard sustained protocol.')
    (output/'capture_manifest.json').write_text(json.dumps(summary,indent=2)+'\n')


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    for name in ['exe','packs','layout','treatment','study','output']:parser.add_argument('--'+name,required=True)
    capture(parser.parse_args())
