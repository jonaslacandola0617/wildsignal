"""Independent linear-light reference: averaging black/white gives sRGB188, not128."""
import importlib.util
from pathlib import Path
from PIL import Image
import tempfile
spec=importlib.util.spec_from_file_location('optimizer',Path(__file__).parents[1]/'tools/optimize.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
a=Image.new('RGBA',(2,2));a.putdata([(0,0,0,255),(255,255,255,255)]*2)
b=m.reduced(a,1)
assert b.size==(1,1) and b.getpixel((0,0))==(188,188,188,255)
with tempfile.TemporaryDirectory() as d:
    try:m.inside(Path(d),'../private.png')
    except ValueError:pass
    else:raise AssertionError('path escapes source')
print('PASS: independent sRGB/linear average and private path containment')

# Source-guided strokes are deterministic and follow a synthetic known grain.
import numpy as np
spec=importlib.util.spec_from_file_location('materials',Path(__file__).parents[1]/'tools/materials.py')
materials=importlib.util.module_from_spec(spec);spec.loader.exec_module(materials)
y,x=np.mgrid[:64,:64];v=np.rint(110+15*np.sin(y*1.2)).astype(np.uint8)
image=Image.fromarray(np.stack((v,np.maximum(v-30,0),np.maximum(v-60,0)),axis=2))
a,count=materials.strokes(image,image.convert('RGBA'),1);b,count2=materials.strokes(image,image.convert('RGBA'),1)
assert count>0 and count==count2 and a.tobytes()==b.tobytes()
assert a.getchannel('A').getextrema()==(255,255)
assert image.size==a.size
print('PASS: deterministic source-guided stroke tracing, dimensions and alpha preservation')
