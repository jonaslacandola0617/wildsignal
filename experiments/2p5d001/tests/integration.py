"""Synthetic executable/SDL regression tests. Never accesses private artwork."""
import csv
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def fnv(data):
    n = 14695981039346656037
    for b in data:
        n = ((n ^ b) * 1099511628211) & ((1 << 64) - 1)
    return n


def fixture(root, source):
    paths = [source / 'data' / n for n in ['approach.json', 'interior.json', 'hybrid.json']]
    outside, inside, scene = [json.loads(p.read_text()) for p in paths]
    data = root / 'data'
    data.mkdir()
    for p in paths:
        (data / p.name).write_bytes(p.read_bytes())
    ids = set(scene['materials'].values()) | {p['sprite'] for p in scene['props']}
    if 'visualRefinement' in scene:
        ids |= {scene['visualRefinement']['facade'],scene['visualRefinement']['groundUnderlay']}
    for m in [outside, inside]:
        ids |= set(m['legend'].values()) | {o['sprite'] for o in m['objects']}
    dimensions = dict(tree=(32,64), bush=(32,32), fence=(32,16), door=(32,48), window=(32,32), room_wall=(48,64), room_bed=(32,64), room_hearth=(48,96), room_table=(32,48), room_chair=(16,16), room_pot=(32,32), room_rug=(48,48), room_exit=(48,48), facade_brick=(96,48), windowbox=(32,16), lupine=(16,32), threshold=(32,16), room_chest=(32,32), room_window=(32,32))
    images = []
    def save(id, w, h, pixels):
        file = id + '.rgba'
        (root / file).write_bytes(b'TDIMG001' + struct.pack('<IIQ', w, h, fnv(pixels)) + pixels)
        images.append(dict(id=id, file=file))
    for i, id in enumerate(sorted(ids)):
        w, h = dimensions.get(id, (16,16))
        pixels = bytearray(bytes([20+(i*13)%200, 50+(i*7)%180, 110, 255]) * (w*h))
        if id in ['tree','bush','window','door']:
            for y in range(h):
                for x in range(w):
                    if x < 2 or x >= w-2:
                        pixels[(y*w+x)*4+3] = 0
        save(id, w, h, pixels)
    pixels = bytearray(64*16*4)
    for k in range(4):
        for y in range(3,16):
            for x in range(4,12):
                p=(y*64+k*16+x)*4
                pixels[p:p+4]=bytes([220,40+k*40,40,255])
    save('actor',64,16,pixels)
    clip=dict(image='actor',w=16,h=16,count=4,ax=8,ay=16,fps=9,flip=False)
    anim={s:{d:clip|dict(flip=d=='left') for d in ['down','up','left','right']} for s in ['idle','walk']}
    pack=dict(schema='wildsignal-topdown001-pack-v1',images=images,player=anim,companion=anim)
    (root/'pack.json').write_text(json.dumps(pack))
    return outside,inside,scene,pack


def main():
    exe,source=Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve()
    native=Path(sys.argv[sys.argv.index('--native')+1]).resolve() if '--native' in sys.argv else None
    with tempfile.TemporaryDirectory(prefix='hybrid001-synthetic-') as temp:
        root=Path(temp);outside,inside,scene,pack=fixture(root,source);serial=0
        def run(args=(),sdl=False,ok=True):
            nonlocal serial
            serial+=1;prefix=root/f'run{serial}'
            cmd=[str(native if sdl else exe),'--assets',str(root/'pack.json'),'--data',str(root/'data'),'--output',str(prefix),'--capture',str(prefix.with_suffix('.ppm')),'--frames','1','--fps','0']
            if not sdl:cmd+=['--headless']
            r=subprocess.run([*cmd,*args],capture_output=True,text=True,timeout=180,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'))
            assert (r.returncode==0)==ok,(cmd,args,r.stdout,r.stderr)
            if ok:return json.loads(prefix.with_suffix('.json').read_text()),prefix.with_suffix('.ppm').read_bytes(),list(csv.DictReader(prefix.with_suffix('.csv').open()))
            assert '2P5D-001 ERROR' in r.stderr
        a,p,_=run();_,q,_=run();assert p==q
        a,p,rows=run(['--demo','--frames','1440']);assert a['transitions']==2,a
        assert {r['mode'] for r in rows}=={'outdoor','indoor'}
        assert any(int(r['companion_pixels'])>0 and int(r['transition'])==0 for r in rows),'Follower never visible while moving outdoors'
        insideRows=[r for r in rows if r['mode']=='indoor'];assert max(float(r['player_x'])for r in insideRows)-min(float(r['player_x'])for r in insideRows)>5
        _,q,_=run(['--demo','--frames','1440']);assert p==q
        for res in ['320x180','480x270']:
            for mode in [[],['--room']]:run(['--resolution',res,*mode])
        run(['--assets',str(root/'missing.json')],ok=False)
        for args in [['--resolution','640x360'],['--pose','13','10','0'],['--frames','3bad'],['--fps','nan']]:run(args,ok=False)
        config=root/'data/hybrid.json'
        for bad in [scene|{'schema':'invalid'},scene|{'materials':scene['materials']|{'roof':'missing'}},scene|{'houses':[scene['houses'][0]|{'w':100}]}]:
            config.write_text(json.dumps(bad));run(ok=False)
        bad=scene|{'houses':[scene['houses'][0]|{'ridgeAxis':'z'},*scene['houses'][1:]]}
        config.write_text(json.dumps(bad));run(ok=False)
        if 'visualRefinement' in scene:
            bad=scene|{'visualRefinement':scene['visualRefinement']|{'revision':'unknown'}}
            config.write_text(json.dumps(bad));run(ok=False)
        config.write_text(json.dumps(scene))
        if 'visualRefinement' in scene:
            facadeId=scene['visualRefinement']['facade']
            bad=pack|{'images':[e|dict(file='actor.rgba') if e['id']==facadeId else e for e in pack['images']]}
            (root/'pack.json').write_text(json.dumps(bad));run(ok=False)
            (root/'pack.json').write_text(json.dumps(pack))
            path=root/next(e['file'] for e in pack['images']if e['id']==facadeId)
            original=path.read_bytes();changed=bytearray(original);changed[27]=128
            changed[16:24]=struct.pack('<Q',fnv(changed[24:]));path.write_bytes(changed);run(ok=False);path.write_bytes(original)
        actor=root/'actor.rgba';raw=actor.read_bytes();actor.write_bytes(raw[:-1]);run(ok=False);actor.write_bytes(raw)
        if native:
            a,q,_=run(sdl=True);_,p,_=run();assert p==q,'SDL loop changed world pixels'
            a,_,rows=run(['--smoke-test','--frames','90'],sdl=True)
            assert a['pipeline']=='SDL_SOFTWARE_WINDOW' and a['resolution']==[480,270]
            assert a['sdlEvents']['keys']>=4 and a['sdlEvents']['mouse']>=1 and a['sdlEvents']['focus']>=2 and a['sdlEvents']['resize']>=1,a
            assert abs(a['yaw']-.0625)<1e-6,a
            assert a['player'][0]>13 and abs(a['player'][1]-outside['spawn']['y']/16)<1e-6
            a,_,_=run(['--demo','--frames','1440'],sdl=True);assert a['transitions']==2,a
    print('PASS: deterministic real hybrid frames, both modes/resolutions, complete enter/walk/exit/return route, validation failures; real SDL events/equality when requested')

if __name__=='__main__':main()
