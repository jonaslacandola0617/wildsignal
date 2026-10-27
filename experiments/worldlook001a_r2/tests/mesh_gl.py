"""Public engineering-only tetrahedra. Never private artwork or a speed gate."""
import csv,json,os,struct,subprocess,sys,tempfile
from pathlib import Path
import numpy as np
from PIL import Image

def main(exe):
 with tempfile.TemporaryDirectory() as tmp:
  root=Path(tmp);env=dict(os.environ);env.setdefault('SDL_VIDEODRIVER','offscreen')
  def mesh(name,positions,indices,colours,double=False):
   # Geometry is deliberately a simple test tetrahedron/ground, not an art substitute.
   normals=[(0,1,0)]*len(positions);data=bytearray();views=[];access=[]
   for values,typ,width in [(positions,'VEC3',3),(normals,'VEC3',3),(colours,'VEC4',4)]:
    offset=len(data);data.extend(struct.pack('<'+'f'*len(values)*width,*(n for row in values for n in row)));views.append({'buffer':0,'byteOffset':offset,'byteLength':len(data)-offset});a={'bufferView':len(views)-1,'componentType':5126,'count':len(values),'type':typ}
    if len(access)==0:a.update(min=np.min(positions,axis=0).tolist(),max=np.max(positions,axis=0).tolist())
    access.append(a)
   offset=len(data);data.extend(struct.pack('<'+'H'*len(indices),*indices));views.append({'buffer':0,'byteOffset':offset,'byteLength':len(data)-offset});access.append({'bufferView':3,'componentType':5123,'count':len(indices),'type':'SCALAR'})
   (root/(name+'.bin')).write_bytes(data)
   gltf={'asset':{'version':'2.0'},'buffers':[{'uri':name+'.bin','byteLength':len(data)}],'bufferViews':views,'accessors':access,'materials':[{'doubleSided':double,'pbrMetallicRoughness':{'baseColorFactor':[1,1,1,1]}}],'meshes':[{'primitives':[{'attributes':{'POSITION':0,'NORMAL':1,'COLOR_0':2},'indices':3,'material':0}]}],'nodes':[{'mesh':0}],'scenes':[{'nodes':[0]}],'scene':0};(root/(name+'.gltf')).write_text(json.dumps(gltf))
  mesh('tetra',[(-.25,0,-.15),(.25,0,-.15),(0,0,.3),(0,.7,0)],[0,1,2,0,3,1,1,3,2,2,3,0],[(.18,.5,.08,1)]*4)
  for name,c in [('original',(.19,.36,.105,1)),('patches',(.3,.45,.16,1))]:
   mesh(name,[(-4,0,-4),(4,0,-4),(4,0,4),(-4,0,4)],[0,2,1,0,3,2],[c]*4)
   (root/('plot_'+name+'_pack.json')).write_text(json.dumps({'revision':1,'models':[{'id':'TEST_GROUND','path':name+'.gltf'}]}))
  (root/'plot_scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'testground','model':'TEST_GROUND','position':[0,0,0],'yaw':0,'scale':1}]}))
  points=[[x,z]for z in [2,1,0,-1,-2,-3]for x in [-2.8,-1.4,1.4,2.8]]
  kit={'revision':1,'plot_metres':8,'max_clumps':24,'kit_id':'SYNTHETIC_ONLY','geometry':[{'id':i,'file':'tetra.gltf','materials':1,'textures':0,'alpha':'OPAQUE'}for i in ['WL_R2_M1','WL_R2_M2','WL_R2_M1_LOD1','WL_R2_M2_LOD1']],'layout':{'roots_uniform':points,'roots_clustered':list(reversed(points))}}
  (root/'mesh_kit.json').write_text(json.dumps(kit));n=0
  def run(*args):
   nonlocal n;n+=1;out=root/str(n)
   cmd=[exe,'--r2-kit',str(root/'mesh_kit.json'),'--output',str(out),'--pose','0','--frames','3','--warmup','0','--cap','0','--hidden','--allow-software','--shadow','cached','--shadow-proof','stable','--shafts','off','--style','C','--no-art-materials','--no-smooth-lighting',*args]
   result=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=60);assert result.returncode==0,result.stderr
   return out
  def img(p):return np.asarray(Image.open(p/'final.ppm'))
  def rows(p):return list(csv.DictReader((p/'mesh_frames.csv').open()))
  off=run('--r2-model','off');on=run('--r2-model','M1','--r2-count','24');assert np.count_nonzero(img(on)!=img(off))>100
  g=rows(on);assert int(g[0]['shadow_draws'])==1 and int(g[1]['shadow_draws'])==0,'cached shadow must not rerender each frame'
  assert int(g[0]['shadow_triangles'])==96
  for row in g:
   assert int(row['total'])==24 and int(row['visible'])+int(row['culled'])==24
   assert 1<=int(row['draws'])<=2 and int(row['triangles'])==4*int(row['visible'])
  for pose in [0,1,2,3]:
   a=run('--pose',str(pose),'--r2-count','24');b=run('--pose',str(pose),'--r2-count','24','--r2-frustum','off');assert np.array_equal(img(a),img(b)),'conservative bounds clipped grass'
  nocast=run('--r2-cast','off');assert all(int(r['shadow_draws'])==0 for r in rows(nocast))
  patch=run('--r2-ground','patches');cluster=run('--r2-placement','clustered');base=run();assert not np.array_equal(img(base),img(patch));assert not np.array_equal(img(base),img(cluster));assert (base/'mesh_roots.csv').read_bytes()==(patch/'mesh_roots.csv').read_bytes(),'ground toggle moved roots'
  look=run('--r2-route','orbit','--r2-lod','on','--r2-model','M2');assert len(rows(look))==3
  run('--r2-inspect','--r2-count','1','--pose','2')
  bad=subprocess.run([exe,'--r2-kit',str(root/'mesh_kit.json'),'--r2-count','25','--output',str(root/'bad')],env=env,capture_output=True,text=True);assert bad.returncode!=0
  print('PASS actual opaque mesh instancing, cached cast-shadow accounting, conservative culling image parity, independent ground/layout, bounded plot, real camera route')
if __name__=='__main__':main(sys.argv[1])
