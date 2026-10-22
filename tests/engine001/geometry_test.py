"""Public synthetic data only: geometry CLI/import, A/B isolation and optional SDL boots."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from PIL import Image

exe, repo = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
spec = importlib.util.spec_from_file_location('importer', repo/'tools/import_sampling_pack.py')
importer = importlib.util.module_from_spec(spec);spec.loader.exec_module(importer)
with tempfile.TemporaryDirectory(prefix='render002-test-') as temp:
    root=Path(temp);entries=[]
    for i in range(6):
        image=Image.new('RGBA',(256,256),(30+i*30,50+i*20,80,255))
        if i in [3,4]:
            image=Image.new('RGBA',(256,256));image.paste((220,20,20,255) if i==3 else (20,220,20,255),(76,92,180,252))
        path=root/f'{i}.png';image.save(path)
        entries.append(dict(id=f'SYNTHETIC_{i}',path=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),size=[256,256]))
    lamp=root/'lamp.png';Image.new('RGBA',(16,64),(250,200,40,255)).save(lamp)
    source=root/'sources.json';source.write_text(json.dumps(dict(schema='wildsignal-private-sampling-input-v1',materials=entries,fixedProp=dict(id='SYNTHETIC_FIXED',path=str(lamp),size=[16,64],sha256=hashlib.sha256(lamp.read_bytes()).hexdigest()))))
    importer.prepare(source,root/'caches')
    def region(slot,u1=1,v1=1):return dict(material=slot,u0=0,v0=0,u1=u1,v1=v1)
    layout=root/'layout.json';layout.write_text(json.dumps(dict(schema='wildsignal-facade-calibration-v1',frameMaterial=1,doorMaterial=3,windowMaterial=4,doorAnchorU=.5,doorAnchorV=252/256,windowAnchorU=.5,windowAnchorV=.5)))
    treatment=root/'treatment.json';treatment.write_text(json.dumps(dict(schema='wildsignal-facade-treatment-v1',plasterMaterial=0,ground=region(0),vertical=region(1,.125),horizontal=region(1,1,.125),frameBorder=.125,groundRepeat=.8,baseHeight=.2,doorX=10.325,windowX=12.675,windowZ=1.4)))
    config=dict(schema='wildsignal-house-geometry-experiment-v1',roof=region(5),wood=region(1),rise=1.4,eave=.35,thickness=.12,recess=.14,roofStatus='TEMPORARY_ENGINEERING')
    house=root/'house.json';house.write_text(json.dumps(config));serial=0
    def run(mode,size,geometry,native=False,ok=True,extra=()):
        global serial
        serial+=1;prefix=root/f'run_{serial}'
        binary=Path(sys.argv[sys.argv.index('--native')+1]).resolve() if native else exe
        cmd=[str(binary),'--sample-pack',str(root/f'caches/sampling_{mode}.wst'),'--facade-layout',str(layout),'--facade-scale','human','--facade-look','c','--facade-treatment',str(treatment),'--facade-projection','perspective','--camera','19','17','-48','--resolution',size,'--frames','2','--warmup','0','--fps','0','--output',str(prefix),'--screenshot',str(prefix.with_suffix('.ppm'))]
        if geometry:cmd+=['--house-geometry',str(house)]
        result=subprocess.run([*cmd,*extra],capture_output=True,text=True,timeout=30,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'))
        assert (result.returncode==0)==ok,(result.stdout,result.stderr)
        if not ok:return
        report=json.loads(prefix.with_suffix('.json').read_text())
        assert report['staticGeometryTriangles']==(38 if geometry else 0)
        assert report['renderer']==('grid_raycast_static_triangles' if geometry else 'grid_raycast')
        assert report['textureSamplingResolution']==mode and report['facadeLook']=='c'
        return prefix.with_suffix('.ppm').read_bytes()
    for mode in [64,128,256]:
        for size in ['320x180','480x270']:
            flat=run(mode,size,False);spatial=run(mode,size,True)
            assert flat!=spatial and flat==run(mode,size,False)
            if '--native' in sys.argv:
                for geometry in [False,True]:assert run(mode,size,geometry,True)==(spatial if geometry else flat)
    for key,value in [('rise',0),('eave',5),('thickness',0),('recess',-1),('roofStatus','APPROVED'),('schema','invalid')]:
        house.write_text(json.dumps(config|{key:value}));run(64,'320x180',True,ok=False)
    for key,value in [('material',99),('material',1.5),('u0',1),('u1',.000001),('v0',-.1)]:
        house.write_text(json.dumps(config|{'roof':config['roof']|{key:value}}));run(64,'320x180',True,ok=False)
    house.write_text(json.dumps(config));run(64,'320x180',True,ok=False,extra=['--facade-look','b'])
print('PASS: RENDER-002 synthetic import, modes/resolutions, baseline isolation, input rejection; 12 SDL A/B boots/pixel equality when requested')
