"""Actual GL synthetic card tests. No private artwork or GPU speed assertion."""
import csv,json,os,struct,subprocess,sys,tempfile
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw

def main(exe):
 with tempfile.TemporaryDirectory()as temp:
  root=Path(temp);env=dict(os.environ);env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('MESA_SHADER_CACHE_DIR',str(root/'mesa'))
  # Engineering-only cards: a solid test color and an obvious cutout hole.
  atlas=Image.new('RGBA',(128,128));d=ImageDraw.Draw(atlas);items=[]
  for i in range(12):
   x=i%4*32;y=i//4*32;d.rectangle((x+3,y+8,x+28,y+30),fill=(130,185,90,255));d.rectangle((x+11,y+9,x+21,y+25),fill=(0,0,0,0))
   family=['SHORT','MEDIUM','TALL','FLOWER'][i//3]
   items.append({'id':f'WL_GRASS_SYNTHETIC_{i}','family':family,'atlas_rect_px':[x,y,32,32],'world_size':[1,.8],'pivot_px':[16,30],'alpha_cutoff':.5,'cards_near':2,'cards_mid':1})
  atlas.save(root/'atlas.png');kit={'revision':1,'kit_id':'SYNTHETIC_GL_TEST_NOT_ART','atlas':'atlas.png','atlas_sha256':'a'*64,'atlas_size':[128,128],'assets':items};(root/'kit.json').write_text(json.dumps(kit))
  # A bounded vertical opaque shadow/occlusion fixture; no borrowed geometry.
  verts=[(-15,-.4,0),(15,-.4,0),(15,.4,0),(-15,.4,0)];normals=[(0,0,-1)]*4
  blob=struct.pack('<12f',*(v for p in verts for v in p))+struct.pack('<12f',*(v for p in normals for v in p))+struct.pack('<6H',0,1,2,0,2,3);(root/'mesh.bin').write_bytes(blob)
  gltf={'asset':{'version':'2.0'},'buffers':[{'uri':'mesh.bin','byteLength':len(blob)}],'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':48},{'buffer':0,'byteOffset':48,'byteLength':48},{'buffer':0,'byteOffset':96,'byteLength':12}],'accessors':[{'bufferView':0,'componentType':5126,'count':4,'type':'VEC3','min':[-15,-.4,0],'max':[15,.4,0]},{'bufferView':1,'componentType':5126,'count':4,'type':'VEC3'},{'bufferView':2,'componentType':5123,'count':6,'type':'SCALAR'}],'materials':[{'doubleSided':True,'pbrMetallicRoughness':{'baseColorFactor':[.3,.2,.1,1]}}],'meshes':[{'primitives':[{'attributes':{'POSITION':0,'NORMAL':1},'indices':2,'material':0}]}],'nodes':[{'mesh':0}],'scenes':[{'nodes':[0]}],'scene':0}
  (root/'mesh.gltf').write_text(json.dumps(gltf));(root/'pack.json').write_text(json.dumps({'revision':1,'models':[{'id':'RURAL_05','path':'mesh.gltf'}]}))
  def scene(position): (root/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'ENGINEERING_WALL','model':'RURAL_05','position':position,'yaw':0,'scale':1}]}))
  scene([0,-30,0]);look=Path(__file__).parents[1]/'data/TOON_DIAGNOSTIC.json';serial=0
  def run(grass=1,flags=(),pose=25):
   nonlocal serial
   serial+=1;out=root/str(serial);cmd=[exe,'--pack',str(root/'pack.json'),'--scene',str(root/'scene.json'),'--look',str(look),'--style','C','--no-art-materials','--no-smooth-lighting','--mode','PIXEL3D','--study','B1','--edge-mode','combined','--shadow-proof','stable','--shadow','cached','--shafts','off','--resolution','480','--pose',str(pose),'--grass-count',str(grass),'--grass-kit',str(root/'kit.json'),'--fixture-id','synthetic','--frames','2','--warmup','0','--cap','0','--hidden','--allow-software','--debug-effects','--output',str(out)]+list(flags)
   r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=60);assert r.returncode==0,r.stderr
   return out
  def image(out):return np.array(Image.open(out/'final.ppm'))
  def mask(out):return np.array(Image.open(out/'grass_mask.pgm'))>0
  def depth(out):
   b=(out/'scene_depth.pfm').read_bytes();head=b.split(b'\n',3);w,h=map(int,head[1].split());return np.flipud(np.frombuffer(head[3],dtype='<f4').reshape(h,w))
  def coherent(off,on):
   m=mask(on);a,b=depth(off),depth(on)
   assert np.array_equal(a[~m],b[~m]),'alpha holes changed depth without a matching grass normal'
   assert np.all(b[m]<=a[m]),'grass defeated opaque depth testing'
   return m
  off=run(0);on=run(1,['--grass-diagnostic','root-light']);m=coherent(off,on);assert m.sum()>150
  coords=np.argwhere(m);lo=coords.min(0);hi=coords.max(0);assert (~m[lo[0]:hi[0]+1,lo[1]:hi[1]+1]).sum()>150,'cutout hole disappeared'
  values=np.unique(image(on)[m],axis=0);assert len(values)==1,'root lighting varied over card height'
  point=json.loads((on/'grass_placement.json').read_text())[0]['root'];x,y,z=point
  for facing in ['cylindrical','crossed']:
   candidate=run(1,['--grass-facing',facing,'--grass-diagnostic','root-light']);v=np.unique(image(candidate)[mask(candidate)],axis=0);assert v.shape==values.shape and np.array_equal(v,values),'card rotation changed root lighting'
  # Light ray reaches this wall at height2 above the root. The card's upper
  # pixels would cross a shadow boundary if fragment height were used instead.
  scene([x-.55/.48*2,y+2,z+.68/.48*2])
  shadow=run(1,['--grass-diagnostic','root-shadow']);pixels=image(shadow)[mask(shadow)];assert len(pixels)>150 and pixels.max()<80 and len(np.unique(pixels,axis=0))==1,'root shadow absent or split by card height'
  lit=run(1,['--shadow','off','--grass-diagnostic','root-shadow']);assert np.all(image(lit)[mask(lit)]==255),'shadow OFF was not independent'
  # Nearer actual geometry fully occludes the grass, without sorting/blending.
  scene([x,y+.35,z+1]);hidden_off=run(0);hidden_on=run(1);assert not mask(hidden_on).any();assert np.array_equal(image(hidden_off),image(hidden_on)),'hidden cards corrupted opaque rendering'
  scene([0,-30,0]);dense=run(800,pose=14);g=list(csv.DictReader((dense/'grass_frames.csv').open()))[-1];assert int(g['visible'])>100 and 1<=int(g['grass_draw_calls'])<=2;assert int(g['grass_triangles'])==4*int(g['near'])+2*int(g['mid']);assert int(g['total'])==int(g['visible'])+int(g['frustum_culled'])+int(g['distance_culled'])
  # Same image with frustum culling disabled: conservative bounds preserve it.
  unculled=run(800,['--grass-frustum','off'],pose=14);assert np.array_equal(image(dense),image(unculled)),'frustum culling clipped visible cards'
  # Reject filtering-incompatible alpha instead of silently accepting fringes.
  bad=atlas.copy();bad.putpixel((3,8),(130,185,90,128));bad.save(root/'atlas.png')
  out=root/'bad';cmd=[exe,'--pack',str(root/'pack.json'),'--scene',str(root/'scene.json'),'--grass-count','1','--grass-kit',str(root/'kit.json'),'--frames','1','--hidden','--allow-software','--output',str(out)]
  r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=60);assert r.returncode!=0 and 'binary straight alpha'in r.stderr
  print('PASS actual GL instancing <=2 draws, cutout/depth/normal coherence, opaque occlusion, root light rotation/height invariance, real cast shadow at base, shadow OFF, culling parity and invalid-alpha rejection')
if __name__=='__main__':main(sys.argv[1])
