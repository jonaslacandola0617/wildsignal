"""Private, reproducible executable evidence; no drawings or authored screenshot pixels.
Requires Pillow; optional ffmpeg creates a convenience H.264 video from actual frames.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import zlib
import subprocess
from PIL import Image


def capture(exe, pack, data, output):
    exe,pack,data,output=[Path(p).resolve() for p in [exe,pack,data,output]]
    if output.exists():
        raise FileExistsError('Fresh private output directory required')
    if any((p/'.git').is_file() or (p/'.git/HEAD').is_file() for p in [output,*output.parents]):
        raise ValueError('Licensed captures must stay outside Git')
    output.mkdir(parents=True)
    runs=[]
    def run(name,res,*args):
        prefix=output/name
        cmd=[str(exe),'--headless','--assets',str(pack),'--data',str(data),'--resolution',res,'--output',str(prefix),*map(str,args)]
        p=subprocess.run(cmd,capture_output=True,text=True,check=True)
        (output/(name+'.log')).write_text(p.stdout+p.stderr)
        runs.append(dict(name=name,command=cmd,report=json.loads(prefix.with_suffix('.json').read_text())))
    poses=dict(front=(13,22,0),corner=(19.5,17,-40),street=(13,23.2,0),close=(13,15,0),side=(21.5,11.5,-90),vegetation=(25,17,-25))
    for res in ['320x180','480x270']:
        for view,pose in poses.items():
            name=res+'_'+view
            run(name,res,'--frames',1,'--pose',*pose,'--capture',output/(name+'.ppm'))
        name=res+'_room'
        run(name,res,'--frames',1,'--room','--capture',output/(name+'.ppm'))
        name=res+'_route'
        run(name,res,'--demo','--frames',1440,'--every',4,'--sequence',output/(name+'_sequence'),'--capture',output/(name+'.ppm'))
        rows=list(csv.DictReader((output/(name+'.csv')).open()))
        assert runs[-1]['report']['transitions']==2,'The captured route did not enter and leave the room'
        assert any(int(r['companion_pixels'])>0 and int(r['transition'])==0 for r in rows),'No visible moving follower evidence'
        motion=sorted((output/(name+'_sequence')).glob('*.ppm'))
        assert len(motion)==360
        for path in motion:
            with Image.open(path) as im:
                im.convert('RGB').save(path.with_suffix('.png'))
        # Write full-frame APNG directly so even unchanged frames retain their
        # exact sample identity and 1/15-second duration. No synthesized pixels.
        def chunk(kind, payload):
            return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind+payload))
        apng=output/(res+'_ACTUAL_24seconds.apng')
        width,height=map(int,res.split('x'))
        with apng.open('wb') as out:
            out.write(b'\x89PNG\r\n\x1a\n')
            out.write(chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0)))
            out.write(chunk(b'acTL',struct.pack('>II',len(motion),0)))
            serial=0
            for n,path in enumerate(motion):
                out.write(chunk(b'fcTL',struct.pack('>IIIIIHHBB',serial,width,height,0,0,1,15,0,0)));serial+=1
                raw=path.with_suffix('.png').read_bytes();position=8
                while position<len(raw):
                    size=struct.unpack('>I',raw[position:position+4])[0];kind=raw[position+4:position+8];payload=raw[position+8:position+8+size];position+=size+12
                    if kind==b'IDAT':
                        if n==0:out.write(chunk(kind,payload))
                        else:out.write(chunk(b'fdAT',struct.pack('>I',serial)+payload));serial+=1
            out.write(chunk(b'IEND',b''))
        with Image.open(apng) as anim:
            assert anim.n_frames==360
            duration=0
            for k,path in enumerate(motion):
                anim.seek(k);duration+=anim.info['duration']
                with Image.open(path) as raw:
                    assert anim.convert('RGB').tobytes()==raw.convert('RGB').tobytes()
            assert abs(duration-24000)<1
        # Named native stills taken from the same uninterrupted executable route.
        selections={
            'room_initial':next(int(r['frame']) for r in rows if r['mode']=='indoor' and int(r['frame'])%4==0 and int(r['stage'])==0 and float(r['seconds'])>10.3),
            'room_moving':next(int(r['frame']) for r in rows if r['mode']=='indoor' and int(r['frame'])%4==0 and int(r['stage'])==2),
            'room_exit':min((r for r in rows if r['mode']=='indoor' and int(r['frame'])%4==0),key=lambda r:(float(r['player_x'])-16.5)**2+(float(r['player_y'])-18.5)**2)['frame'],
            'return_outdoor':next(int(r['frame']) for r in rows if r['mode']=='outdoor' and int(r['transition'])==2 and int(r['frame'])%4==0 and int(r['frame'])>1000),
            'companion':min((r for r in rows if r['mode']=='outdoor' and int(r['transition'])==0 and int(r['frame'])%4==0 and int(r['companion_pixels'])>0),key=lambda r:abs(math.remainder(math.atan2(float(r['companion_x'])-float(r['player_x']),float(r['player_y'])-float(r['companion_y']))-float(r['yaw']),2*math.pi)))['frame']}
        for label,index in selections.items():
            path=output/(name+'_sequence')/f'frame_{int(index):05}.ppm'
            shutil.copyfile(path,output/(res+'_'+label+'.ppm'))
        ffmpeg=shutil.which('ffmpeg')
        if ffmpeg:
            command=[ffmpeg,'-v','error','-y','-f','rawvideo','-pix_fmt','rgb24','-s',res.replace('x','x'),'-r','15','-i','pipe:0','-vf','scale=960:540:flags=neighbor','-c:v','libx264','-crf','16','-pix_fmt','yuv420p',str(output/(res+'_ACTUAL_24seconds.mp4'))]
            process=subprocess.Popen(command,stdin=subprocess.PIPE)
            try:
                for path in motion:
                    with Image.open(path) as im:process.stdin.write(im.convert('RGB').tobytes())
            finally:
                process.stdin.close()
            if process.wait()!=0:raise RuntimeError('ffmpeg failed')
    for path in output.glob('*.ppm'):
        im=Image.open(path);im.save(path.with_suffix('.png'));im.resize((960,540),Image.Resampling.NEAREST).save(path.with_name(path.stem+'_nearest.png'))
    report=dict(executableSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),packSha256=hashlib.sha256(pack.read_bytes()).hexdigest(),sceneHashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in data.glob('*.json')},runs=runs,source='All PPM pixels are actual C++ executable output. PNG conversion and nearest enlargement only. APNG exactly 360 frames at 15fps, each checked against its executable PPM. MP4 is lossy convenience video. No simulated art.')
    (output/'capture_manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--pack',required=True);p.add_argument('--data',required=True);p.add_argument('--output',required=True);a=p.parse_args();capture(a.exe,a.pack,a.data,a.output)
