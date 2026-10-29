"""Independent private-runtime QA: raycast final rendered glTF triangles versus cache.
Python stdlib only. No art or original private coordinates are embedded here.
Usage: terrain_mesh.py ASSETS ASSEMBLY_JSON OUTPUT_JSON [--baseline]
"""
import json,math,struct,sys,statistics,hashlib
from pathlib import Path
assets,layout,out=map(Path,sys.argv[1:4]);baseline='--baseline'in sys.argv
cfg=json.loads((assets/'game.json').read_text());scene=json.loads((assets/'scene.json').read_text());pack=json.loads((assets/'pack.json').read_text());spec=json.loads(layout.read_text());paths={m['id']:assets/m['path']for m in pack['models']};half=cfg['world_half_extent'];dim=257;cache=struct.unpack('<'+'f'*(dim*dim),(assets/'terrain_height.bin').read_bytes())
def accessor(g,b,k):
 a=g['accessors'][k];v=g['bufferViews'][a['bufferView']];fmt={5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']];width={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];stride=v.get('byteStride',struct.calcsize('<'+fmt*width));start=v.get('byteOffset',0)+a.get('byteOffset',0)
 return [struct.unpack_from('<'+fmt*width,b,start+i*stride)for i in range(a['count'])]
def transform(p,i):
 x,y,z=p;a=math.radians(i['yaw']);s=i['scale'];t=i['position'];return(x*s*math.cos(a)+z*s*math.sin(a)+t[0],y*s+t[1],-x*s*math.sin(a)+z*s*math.cos(a)+t[2])
triangles=[];modelCache={}
for i in scene['instances']:
 label=i['model'];selected=label.startswith('HG_')if not baseline else label=='EA_GROUND'or label.startswith(('EA_GREEN','EA_PATH'))or label in['EA_COBBLE','EA_MOSS']
 if not selected:continue
 if label not in modelCache:
  g=json.loads(paths[label].read_text());b=(paths[label].parent/g['buffers'][0]['uri']).read_bytes();parts=[]
  for m in g['meshes']:
   for p in m['primitives']:
    pos=accessor(g,b,p['attributes']['POSITION']);idx=[v[0]for v in accessor(g,b,p['indices'])];parts.extend(tuple(pos[idx[k+j]]for j in range(3))for k in range(0,len(idx),3))
  modelCache[label]=parts
 triangles.extend(tuple(transform(p,i)for p in t)for t in modelCache[label])
assert triangles,'No final visible walkable surfaces'
bins={};step=2.
for idx,t in enumerate(triangles):
 for bx in range(math.floor(min(p[0]for p in t)/step),math.floor(max(p[0]for p in t)/step)+1):
  for bz in range(math.floor(min(p[2]for p in t)/step),math.floor(max(p[2]for p in t)/step)+1):bins.setdefault((bx,bz),[]).append(idx)
def rendered(x,z):
 top=-math.inf
 for k in bins.get((math.floor(x/step),math.floor(z/step)),[]):
  (x0,y0,z0),(x1,y1,z1),(x2,y2,z2)=triangles[k];det=(z1-z2)*(x0-x2)+(x2-x1)*(z0-z2)
  if abs(det)<1e-10:continue
  u=((z1-z2)*(x-x2)+(x2-x1)*(z-z2))/det;v=((z2-z0)*(x-x2)+(x0-x2)*(z-z2))/det;w=1-u-v
  if min(u,v,w)>=-1e-5:top=max(top,u*y0+v*y1+w*y2)
 if not math.isfinite(top):raise RuntimeError(f'No rendered support at {x},{z}')
 return top
def cached(x,z):
 u=max(0,min(256,(x+half)/(2*half)*256));v=max(0,min(256,(z+half)/(2*half)*256));i=min(int(u),255);j=min(int(v),255);a=u-i;b=v-j
 return (1-b)*((1-a)*cache[j*257+i]+a*cache[j*257+i+1])+b*((1-a)*cache[(j+1)*257+i]+a*cache[(j+1)*257+i+1])
def free(x,z):return abs(x)<half-.5 and abs(z)<half-.5 and all(max(abs(x-a)-hx,0)**2+max(abs(z-b)-hz,0)**2>=.5**2 for a,b,hx,hz in cfg['solids'])
def slope(x,z,e=.12):
 dx=(rendered(x+e,z)-rendered(x-e,z))/(2*e);dz=(rendered(x,z+e)-rendered(x,z-e))/(2*e);return math.degrees(math.atan(math.hypot(dx,dz)))
rows=[];blocked=[]
for a,b in spec['route_segments']:
 dx,dz=b[0]-a[0],b[1]-a[1];dist=math.hypot(dx,dz);normal=(-dz/dist,dx/dist);n=max(1,math.ceil(dist/.15))
 for i in range(n+1):
  for side in [-.5,0,.5]:
   x=a[0]+dx*i/n+normal[0]*side;z=a[1]+dz*i/n+normal[1]*side
   if not free(x,z):blocked.append([x,z]);continue
   rows.append({'position':[x,z],'slope_degrees':slope(x,z),'mesh_cache_error_m':abs(rendered(x,z)-cached(x,z))})
slopes=sorted(r['slope_degrees']for r in rows);errors=sorted(r['mesh_cache_error_m']for r in rows);padStats=[]
for cx,cz,hx,hz in spec['pads']:
 vals=[slope(cx+hx*.7*i/5,cz+hz*.7*j/5)for i in range(-5,6)for j in range(-5,6)];padStats.append({'center':[cx,cz],'max_slope_deg':max(vals),'samples':len(vals)})
# Adjacent source tile interfaces, tested on the final rendered geometry.
seams=[]
for coord in [-9.95,9.95]:
 for q in range(-28,29):
  seams.extend([abs(rendered(coord-.015,q)-rendered(coord+.015,q)),abs(rendered(q,coord-.015)-rendered(q,coord+.015))])
exceptions=[r for r in rows if r['slope_degrees']>12 or r['mesh_cache_error_m']>.04]
result={'baseline':baseline,'triangles_checked':len(triangles),'route_samples':len(rows),'corridor_half_width_m':.5,'finite_difference_span_m':.24,'route_slope_p95_deg':slopes[math.ceil(len(slopes)*.95)-1],'route_slope_max_deg':max(slopes),'route_share_at_or_below_8deg':sum(s<=8 for s in slopes)/len(slopes),'mesh_cache_error_max_m':max(errors),'mesh_cache_error_p95_m':errors[math.ceil(len(errors)*.95)-1],'tile_interface_step_max_m':max(seams),'building_pads':padStats,'route_blocked_samples':blocked,'exceptions':exceptions[:60],'exception_count':len(exceptions),'cache_sha256':hashlib.sha256((assets/'terrain_height.bin').read_bytes()).hexdigest()}
out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2))
if not baseline:
 assert not blocked,('route crosses collision',blocked[:8])
 assert result['route_share_at_or_below_8deg']>=.95,result
 assert max(slopes)<=12,(max(slopes),exceptions[:3])
 assert max(errors)<=.04,(max(errors),exceptions[:3])
 assert max(seams)<=.10,max(seams)
 assert all(p['max_slope_deg']<=3 for p in padStats),padStats
 print('PASS final rendered-mesh/cache route, slope, pad and seam validation')
print({k:v for k,v in result.items()if k not in['exceptions','route_blocked_samples','building_pads']})
