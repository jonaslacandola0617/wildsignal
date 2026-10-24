"""Synthetic-only executable, loader and SDL event tests. Never loads licensed pixels."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

exe,repo=Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve()
def fnv(data):
    n=14695981039346656037
    for b in data:n=((n^b)*1099511628211)&((1<<64)-1)
    return n

def fixture(root):
    scene=json.loads((repo/'data/lantern_quay.json').read_text());ids=set(scene['legend'].values())|{x['sprite'] for x in scene['objects']};images=[]
    for i,id in enumerate(sorted(ids)):
        w=h=16
        if id=='wall':w,h=96,48
        if id=='roof':w,h=128,96
        if id=='window':w=h=32
        if id=='door':w,h=32,48
        if id=='tree':w,h=32,64
        if id=='bush':w=h=32
        if id=='fence':w,h=32,16
        if id=='fence_vertical':w,h=16,48
        rgba=bytes([20+i*7,70+i*3,100,255])*(w*h);file=f'{id}.rgba';(root/file).write_bytes(b'TDIMG001'+struct.pack('<IIQ',w,h,fnv(rgba))+rgba);images.append(dict(id=id,file=file))
    # A synthetic four-frame actor, with source transparency, proves animation and decode isolation.
    pixels=bytearray(64*16*4)
    for frame in range(4):
        for y in range(3,16):
            for x in range(frame*16+4,frame*16+12):
                p=(y*64+x)*4;pixels[p:p+4]=bytes([230,50+frame*40,40,255])
    (root/'actor.rgba').write_bytes(b'TDIMG001'+struct.pack('<IIQ',64,16,fnv(pixels))+pixels);images.append(dict(id='actor',file='actor.rgba'))
    clip=dict(image='actor',w=16,h=16,count=4,ax=8,ay=16,fps=9,flip=False);actor={state:{d:clip|dict(flip=d=='left') for d in ['down','up','left','right']} for state in ['idle','walk']}
    pack=dict(schema='wildsignal-topdown001-pack-v1',images=images,player=actor,companion=actor);(root/'pack.json').write_text(json.dumps(pack));return scene,pack

with tempfile.TemporaryDirectory(prefix='topdown001-synthetic-') as temp:
    root=Path(temp);scene,pack=fixture(root);sceneFile=root/'map.json';sceneFile.write_text(json.dumps(scene));serial=0
    def run(args=(),native=False,ok=True):
        global serial
        serial+=1;prefix=root/f'run{serial}';binary=Path(sys.argv[sys.argv.index('--native')+1]).resolve() if native else exe
        cmd=[str(binary),'--assets',str(root/'pack.json'),'--map',str(sceneFile),'--frames','180','--fps','0','--output',str(prefix),'--capture',str(prefix.with_suffix('.ppm'))]
        if not native:cmd+=['--headless','--demo']
        result=subprocess.run([*cmd,*args],capture_output=True,text=True,timeout=60,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'))
        assert (result.returncode==0)==ok,(cmd,result.stdout,result.stderr)
        if ok:return json.loads(prefix.with_suffix('.json').read_text()),prefix.with_suffix('.ppm').read_bytes()
    report,frame=run();report2,frame2=run();assert frame==frame2 and report['player']==report2['player']
    assert report['frameCount']==180 and report['pixelCacheBytes']<1024*1024
    run(['--zoom','2']);run(['--assets',str(root/'missing.json')],ok=False);run(['--zoom','3'],ok=False)
    for changed in [scene|{'width':41},scene|{'spawn':dict(x=0,y=0)},scene|{'legend':dict(scene['legend'],g='missing_sprite')}]:
        sceneFile.write_text(json.dumps(changed));run(ok=False)
    sceneFile.write_text(json.dumps(scene))
    for changed in [pack|{'images':pack['images']+[pack['images'][0]]},pack|{'images':[pack['images'][0]|dict(file='../outside.rgba')]},pack|{'schema':'invalid'}]:
        (root/'pack.json').write_text(json.dumps(changed));run(ok=False)
    (root/'pack.json').write_text(json.dumps(pack));file=root/'actor.rgba';original=file.read_bytes();file.write_bytes(original[:-1]);run(ok=False);file.write_bytes(original[:-1]+bytes([original[-1]^1]));run(ok=False);file.write_bytes(original)
    if '--native' in sys.argv:
        nativeReport,_=run(['--smoke-test'],native=True);assert nativeReport['zoom']==2 and nativeReport['pipeline']=='SDL_WINDOW_SOFTWARE'
print('PASS: deterministic real executable, malformed/missing assets/map/indices/checksum, native zoom; SDL keyboard/resize/focus when requested')
