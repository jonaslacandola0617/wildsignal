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
