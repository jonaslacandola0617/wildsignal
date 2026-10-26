"""Actual GL readbacks: selective outline and fractional nearest presentation.

These use generated code-test triangles only, never substitute review artwork.
Mesa evidence is explicitly a software diagnostic.
"""
import csv,importlib.util,json,math,os,struct,subprocess,sys,tempfile
from pathlib import Path
from PIL import Image

def main(exe):
 spec=importlib.util.spec_from_file_location('fixture',Path(__file__).with_name('import.py'));f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f)
 with tempfile.TemporaryDirectory() as tmp:
  root=Path(tmp);d,b=f.fixture(root)
  d['nodes']=[{'mesh':0}]
  # Two coplanar leaf triangles with a large authored normal discontinuity.
  # They must not acquire a painted diagonal from the normal-only detector.
  positions=[(-3,-2,0),(3,-2,0),(3,4,0),(-3,-2,0),(3,4,0),(-3,4,0)]
  normals=[(0,0,1)]*3+[(.8660254,0,.5)]*3
  b=b''.join(struct.pack('<3f',*p)for p in positions+normals)
  (root/'mesh.bin').write_bytes(b)
  d['buffers']=[{'uri':'mesh.bin','byteLength':len(b)}]
  d['bufferViews']=[{'buffer':0,'byteOffset':0,'byteLength':72},{'buffer':0,'byteOffset':72,'byteLength':72}]
  d['accessors']=[{'bufferView':i,'componentType':5126,'count':6,'type':'VEC3'}for i in range(2)]
  d['meshes'][0]['primitives']=[{'attributes':{'POSITION':0,'NORMAL':1},'material':0}]
  (root/'mesh.gltf').write_text(json.dumps(d))
  (root/'pack.json').write_text(json.dumps({'revision':1,'models':[{'id':'TREE_01','path':'mesh.gltf'}]}))
  (root/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'leaf','model':'TREE_01','position':[0,0,0],'yaw':0,'scale':1}]}))
  look=Path(__file__).parents[1]/'data/sunlight_look.json'
  env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('MESA_SHADER_CACHE_DIR',str(root/'mesa'))
  def run(name,flags):
   out=root/name
   cmd=[exe,'--pack',str(root/'pack.json'),'--scene',str(root/'scene.json'),'--look',str(look),'--style','C','--no-art-materials','--study','B2','--pixel-camera','off','--pose','1','--frames','3','--warmup','0','--cap','0','--hidden','--allow-software','--output',str(out),*flags]
   r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=60);assert r.returncode==0,r.stderr
   return out,Image.open(out/'final.ppm').copy()
  for width in (480,640):
   flags=['--resolution',str(width),'--snap-depth','13']
   _,on=run('outline_'+str(width),flags)
   _,off=run('outline_off_'+str(width),flags+['--no-outline'])
   # Only the one cardinal pixel silhouette may change. A known diagonal
   # between shaded planes is geometry-coplanar and must stay untouched.
   changed=0
   for y in range(2,on.height-2):
    for x in range(2,on.width-2):
     if on.getpixel((x,y))!=off.getpixel((x,y)):
      changed+=1
      # A flat-colored triangle edge has an adjacent sky sample. Background
      # varies vertically, so compare against the same-row clear sky value.
      assert any(off.getpixel((x+dx,y+dy))==off.getpixel((0,y+dy)) for dx,dy in [(-1,0),(1,0),(0,-1),(0,1)]),'spurious polygon outline or thick silhouette'
   assert changed>5,'outline shader inactive'
   raw,raw_im=run('raw_'+str(width),flags+['--pixel-camera','raw','--present-scale','3','--capture-presented'])
   comp,comp_im=run('comp_'+str(width),flags+['--pixel-camera','comp','--present-scale','3','--capture-presented'])
   assert raw_im.tobytes()==comp_im.tobytes(),'RAW/COMP changed geometry rendering'
   row=list(csv.DictReader((comp/'frames.csv').open()))[-1]
   sx=float(row['offset_x_pixels']);sy=float(row['offset_y_pixels'])
   shown=Image.open(comp/'final_presented.ppm').convert('RGB')
   source=comp_im.load();expected=bytearray();w,h=comp_im.size;ow,oh=shown.size
   for y in range(oh):
    iy=h-1-max(0,min(h-1,math.floor((oh-y-.5)/3-sy)))
    for x in range(ow):
     ix=max(0,min(w-1,math.floor((x+.5)/3-sx)))
     expected.extend(source[ix,iy])
   assert shown.tobytes()==bytes(expected),'actual fractional nearest presentation has wrong sign/scale/filter'
   raw_shown=Image.open(raw/'final_presented.ppm').convert('RGB')
   assert raw_shown.tobytes()==raw_im.resize(raw_shown.size,Image.Resampling.NEAREST).tobytes(),'RAW is not exact nearest'
   assert raw_shown.tobytes()!=shown.tobytes(),'compensation presentation inactive'
   _,nocull=run('cull_'+str(width),flags+['--pixel-camera','comp','--no-cull'])
   assert nocull.tobytes()==comp_im.tobytes(),'compensated camera culling changes pixels'
  # Color factors and emission reach the actual fragment shader, not just audit.
  d['materials'][0]['pbrMetallicRoughness']['baseColorFactor']=[.2,.4,.6,1]
  d['materials'][0]['emissiveFactor']=[0,0,0]
  (root/'mesh.gltf').write_text(json.dumps(d))
  _,color=run('material_color',['--diagnostic','albedo'])
  expected=tuple(round(v**(1/2.2)*255)for v in [.2,.4,.6])
  assert any(all(abs(p[k]-expected[k])<=1 for k in range(3))for p in color.getdata()),'material factor color mismatch'
  _,dark=run('emission_off',[])
  d['materials'][0]['emissiveFactor']=[.8,0,0];(root/'mesh.gltf').write_text(json.dumps(d))
  _,emission=run('emission_on',[])
  assert dark.tobytes()!=emission.tobytes(),'emissive material ignored'
  print('PASS real shader: selective foliage/no false diagonal, 1px ink, RAW/COMP geometry equality, exact displayed fractional nearest sampling, culling, material/emission')
if __name__=='__main__':main(sys.argv[1])
