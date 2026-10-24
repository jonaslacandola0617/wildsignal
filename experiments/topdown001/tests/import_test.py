"""Synthetic ZIP/catalog test; never contains Pixel Crawler bytes."""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import zipfile
from PIL import Image
repo=Path(sys.argv[1]).resolve();spec=importlib.util.spec_from_file_location('importer',repo/'tools/import_assets.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
with tempfile.TemporaryDirectory(prefix='topdown-import-') as temp:
    root=Path(temp);image=Image.new('RGBA',(32,32),(70,150,30,128));buf=io.BytesIO();image.save(buf,format='PNG');png=buf.getvalue();terms=b'SYNTHETIC TEST ONLY';archive=root/'test.zip'
    with zipfile.ZipFile(archive,'w') as z:z.writestr('sprite.png',png);z.writestr('Terms.txt',terms)
    cfg=dict(schema='wildsignal-topdown001-catalog-v1',zipSha256=hashlib.sha256(archive.read_bytes()).hexdigest(),terms='Terms.txt',termsSha256=hashlib.sha256(terms).hexdigest(),images=[dict(id='synthetic',source='sprite.png',sha256=hashlib.sha256(png).hexdigest(),rect=[0,0,16,16])],player={},companion={});catalog=root/'catalog.json';catalog.write_text(json.dumps(cfg))
    report=module.prepare(archive,catalog,root/'pack');assert report['rgbaBytes']==16*16*4
    payload=(root/'pack/000.rgba').read_bytes();assert payload[24:]==image.crop((0,0,16,16)).tobytes()
    for bad in [cfg|{'zipSha256':'wrong'},cfg|{'termsSha256':'wrong'},cfg|{'images':[cfg['images'][0]|{'rect':[0,0,33,32]}]},cfg|{'images':[cfg['images'][0]|{'sha256':'wrong'}]}]:
        catalog.write_text(json.dumps(bad))
        try:module.prepare(archive,catalog,root/'bad')
        except ValueError:pass
        else:raise AssertionError('Invalid source import accepted')
    catalog.write_text(json.dumps(cfg));git=root/'fake-repo/.git';git.mkdir(parents=True);(git/'HEAD').write_text('synthetic marker')
    try:module.prepare(archive,catalog,root/'fake-repo/private')
    except ValueError:pass
    else:raise AssertionError('Private import into Git allowed')
print('PASS: exact partial-alpha native crop, ZIP/source/terms integrity, bounds, private-only output')
