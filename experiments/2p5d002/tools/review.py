"""Private matched-camera evidence from real executables, never promotional pixels.
Pillow required; ffmpeg optional for MP4. Keep output outside Git.
"""
import argparse
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
from PIL import Image, ImageDraw


def review(a):
    output=Path(a.output).resolve()
    if output.exists():raise FileExistsError('Fresh private output required')
    if any((p/'.git').is_file() or (p/'.git/HEAD').is_file() for p in [output,*output.parents]):raise ValueError('Private captures must stay outside Git')
    output.mkdir(parents=True)
    helper=Path(__file__).resolve().parents[2]/'2p5d001/tools/capture.py'
    spec=importlib.util.spec_from_file_location('hybrid_capture',helper);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    configs=[('before',Path(a.baseline_exe).resolve(),Path(a.baseline_pack).resolve(),Path(a.baseline_data).resolve()),('after',Path(a.candidate_exe).resolve(),Path(a.candidate_pack).resolve(),Path(a.candidate_data).resolve())]
    results=[];poses=dict(front=(13,22,0),corner=(19.5,17,-40),side=(21.5,11.5,-90),street=(13,23.2,0),near_door=(13,15,0))
    for yaw in [-70,-45,-25,0,25,45,70]:poses['foliage_yaw_'+str(yaw)]=(25,17,yaw)
    for name,pose in dict(west=(21,14.5,90),south=(24.5,18,0),east=(28,14.5,-90),north=(24.5,11,180)).items():poses['foliage_orbit_'+name]=pose
    for label,exe,pack,data in configs:
        side=output/label;side.mkdir()
        # Full continuous native gameplay route, lossless APNG and actual stills.
        route=module.capture(exe,pack,data,side/'playthrough')
        results.append(dict(label=label,executableSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),packSha256=hashlib.sha256(pack.read_bytes()).hexdigest(),route=route))
        def snap(name,res,*args,scene=data):
            prefix=side/(res+'_'+name)
            command=[str(exe),'--headless','--assets',str(pack),'--data',str(scene),'--resolution',res,'--output',str(prefix),'--capture',str(prefix.with_suffix('.ppm')),'--frames','1',*map(str,args)]
            p=subprocess.run(command,capture_output=True,text=True,check=True)
            prefix.with_suffix('.command.json').write_text(json.dumps(command,indent=2)+'\n')
            image=Image.open(prefix.with_suffix('.ppm')).convert('RGB');image.save(prefix.with_suffix('.png'));image.resize((image.width*3,image.height*3),Image.Resampling.NEAREST).save(side/(prefix.name+'_3x.png'))
        for res in ['320x180','480x270']:
            for name,pose in poses.items():snap(name,res,'--pose',*pose)
            for name,point in [('room',(16.5,15)),('exit',(16.5,17.5))]:
                scene=side/('capture_scene_'+name)
                if not scene.exists():
                    shutil.copytree(data,scene);j=json.loads((scene/'hybrid.json').read_text());j['insideSpawn']={'x':point[0],'y':point[1]};(scene/'hybrid.json').write_text(json.dumps(j,indent=2)+'\n')
                snap(name,res,'--room',scene=scene)
            snap('companion_follow',res,'--pose',13,17,180,'--frames',120)
            rows=list(csv.DictReader((side/(res+'_companion_follow.csv')).open()))
            assert abs(float(rows[-1]['companion_y'])-float(rows[0]['companion_y']))>1,'Follower did not move'
            assert any(int(x['companion_pixels'])>0 for x in rows)
    # Matched individual poses remain pixel-authoritative. Paired sheets label
    # their origin clearly and only place the actual output images side by side.
    pairs=output/'pairs';pairs.mkdir()
    for p in sorted((output/'before').glob('*.ppm')):
        q=output/'after'/p.name
        if not q.exists():continue
        b=Image.open(p).convert('RGB');c=Image.open(q).convert('RGB');assert b.size==c.size
        pair=Image.new('RGB',(b.width*2,b.height+16),(24,31,38));pair.paste(b,(0,16));pair.paste(c,(b.width,16));d=ImageDraw.Draw(pair);d.text((4,3),'BEFORE /001',fill='white');d.text((b.width+4,3),'AFTER /002 v0.2',fill='white');pair.save(pairs/(p.stem+'_native.png'));pair.resize((pair.width*3,pair.height*3),Image.Resampling.NEAREST).save(pairs/(p.stem+'_3x.png'))
        left=list(csv.DictReader(p.with_suffix('.csv').open()))[-1];right=list(csv.DictReader(q.with_suffix('.csv').open()))[-1]
        for key in ['player_x','player_y','yaw']:assert left[key]==right[key],(p.name,key,left[key],right[key])
    (output/'manifest.json').write_text(json.dumps(dict(configurations=results,comparison='All matched pairs use exact same player coordinates/yaw,70degFOV,eye1.45 and native pixel size. Room camera center unchanged; exit snapshots alter only initial insideSpawn in private capture configs. Companion matched120-tick stationary-camera approach is actual following. Gameplay routes are continuous controller runs, not exact companion-dependent camera A/B trajectories.'),indent=2)+'\n')


if __name__=='__main__':
    p=argparse.ArgumentParser()
    for name in ['baseline-exe','baseline-pack','baseline-data','candidate-exe','candidate-pack','candidate-data','output']:p.add_argument('--'+name,required=True)
    review(p.parse_args())
