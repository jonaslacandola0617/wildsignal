"""VIS-003 synthetic import, control isolation, parser and SDL checks. No private art."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from PIL import Image

exe, repo = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
spec = importlib.util.spec_from_file_location('importer', repo/'tools/import_sampling_pack.py')
importer = importlib.util.module_from_spec(spec);spec.loader.exec_module(importer)

with tempfile.TemporaryDirectory(prefix='vis003-test-') as temp:
    root = Path(temp)
    entries = []
    for i in range(10):
        size = (32, 48) if i == 7 else (32, 224) if i == 8 else (224, 32) if i == 9 else (256, 256)
        im = Image.new('RGBA', size, (30+i*20, 40+i*10, 50, 247 if i == 7 else 255))
        if i in [3, 4]:
            im = Image.new('RGBA', size)
            for y in range(64, 224 if i == 3 else 192):
                for x in range(64, 192):im.putpixel((x, y), (220, 20, 20, 255) if i == 3 else (20, 220, 20, 255))
        if i in [5, 6]:
            im.putdata([(x, y, 80, 255) for y in range(256) for x in range(256)])
        path = root/f'{i}.png';im.save(path)
        entry = dict(id=f'SYNTHETIC_{i}', path=path.name, sha256=hashlib.sha256(path.read_bytes()).hexdigest(), size=list(size))
        if i >= 7:entry['padTo'] = [256, 256]
        if i == 7:entry['opaqueRgb'] = True
        if i == 6:entry['contrast'] = dict(numerator=1, denominator=2, center=[128, 128, 128])
        entries.append(entry)
    lamp = root/'lamp.png';Image.new('RGBA', (16, 64), (250, 200, 40, 255)).save(lamp)
    fixed = dict(id='SYNTHETIC_FIXED', path=lamp.name, sha256=hashlib.sha256(lamp.read_bytes()).hexdigest(), size=[16, 64])
    source = dict(schema='wildsignal-private-sampling-input-v2', materials=entries, fixedProp=fixed)
    source_file = root/'sources.json';source_file.write_text(json.dumps(source))
    importer.prepare(source_file, root/'caches')
    data = (root/'caches/sampling_256.wst').read_bytes();offset = 16
    for i in range(11):
        w,h,_ = struct.unpack_from('<IIQ', data, offset);offset += 16
        pixels = data[offset:offset+w*h*4];offset += w*h*4
        if i == 6:
            original = Image.open(root/'6.png').tobytes()
            expected = bytes((value+128+1)//2 if n%4 < 3 else value for n,value in enumerate(original))
            assert pixels == expected
        if i >= 7 and i < 10:
            native = Image.open(root/f'{i}.png');prepared = Image.new('RGBA', (256,256))
            if i == 7:native.putalpha(255)
            prepared.paste(native,(0,0));assert prepared.tobytes() == pixels
        if i == 10:assert pixels == Image.open(lamp).tobytes()
    layout = root/'facade.json';layout.write_text(json.dumps(dict(schema='wildsignal-facade-calibration-v1', frameMaterial=1,doorMaterial=3,windowMaterial=4,doorAnchorU=.5,doorAnchorV=224/256,windowAnchorU=.5,windowAnchorV=.5)))
    def region(slot,u1=1,v1=1):return dict(material=slot,u0=0,v0=0,u1=u1,v1=v1)
    treatment = dict(schema='wildsignal-facade-treatment-v1',plasterMaterial=6,ground=region(7,32/256,48/256),vertical=region(8,32/256,224/256),horizontal=region(9,224/256,32/256),frameBorder=.125,groundRepeat=.8,baseHeight=.2,doorX=10.325,windowX=12.675,windowZ=1.4)
    treatment_file = root/'treatment.json';treatment_file.write_text(json.dumps(treatment))
    serial = 0
    def run(look, mode=256, size='480x270', native=False, ok=True, overrides=()):
        global serial
        serial += 1;prefix=root/f'run_{serial}'
        binary=Path(sys.argv[sys.argv.index('--native')+1]).resolve() if native else exe
        args=[str(binary),'--sample-pack',str(root/f'caches/sampling_{mode}.wst'),'--facade-layout',str(layout),'--facade-scale','human','--facade-projection','perspective','--resolution',size,'--warmup','0','--fps','0','--frames','2','--output',str(prefix),'--screenshot',str(prefix.with_suffix('.ppm'))]
        if look:args += ['--facade-look',look,'--facade-treatment',str(treatment_file)]
        result=subprocess.run([*args,*overrides],capture_output=True,text=True,timeout=30,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'))
        assert (result.returncode == 0) == ok,(result.stdout,result.stderr)
        if not ok:return
        report=json.loads(prefix.with_suffix('.json').read_text())
        if look:
            assert report['facadeLook']==look and report['zone']=='VIS003_READABILITY_001'
            assert report['facadeDoorScale']==3.2 and report['facadeWindowScale']==2.4
        return prefix.with_suffix('.ppm').read_bytes()
    for mode in [64,128,256]:
        for size in ['320x180','480x270']:
            old=run('',mode,size);a=run('a',mode,size);b=run('b',mode,size);c=run('c',mode,size)
            assert old==a and a!=b and b!=c
            if '--native' in sys.argv:
                for look in 'abc':run(look,mode,size,native=True)
    # Ground-only control must leave wall pixels above the horizon unchanged.
    treatment_file.write_text(json.dumps(treatment | dict(plasterMaterial=-1)))
    a=run('a').split(b'\n',3)[3];ground_only=run('b').split(b'\n',3)[3]
    assert a[:480*135*3]==ground_only[:480*135*3] and a!=ground_only
    for key,value in [('plasterMaterial',99),('plasterMaterial',1.5),('baseHeight',2),('doorX',0),('frameBorder',.8),('schema','invalid')]:
        treatment_file.write_text(json.dumps(treatment | {key:value}));run('b',ok=False)
    for key,value in [('u1',0),('u0',-1),('material',99),('u1',.00001)]:
        treatment_file.write_text(json.dumps(treatment | {'ground':treatment['ground'] | {key:value}}));run('b',ok=False)
    treatment_file.write_text(json.dumps(treatment));run('b',ok=False,overrides=['--facade-scale','c'])
    for changed in [source | {'schema':'wildsignal-private-sampling-input-v1'},source | {'fixedProp':fixed | {'opaqueRgb':True}},source | {'materials':[entries[0] | {'contrast':dict(numerator=0,denominator=2,center=[128]*3)}]}]:
        source_file.write_text(json.dumps(changed))
        try:importer.prepare(source_file,root/'invalid')
        except ValueError:pass
        else:raise AssertionError('Invalid import transform accepted')
    for entry in entries+[fixed]:assert hashlib.sha256((root/entry['path']).read_bytes()).hexdigest()==entry['sha256']
print('PASS: VIS-003 native padding/contrast/near-opaque RGB, fixed prop, A exact baseline, A/B/C isolation, modes, invalid inputs; native SDL when requested')
