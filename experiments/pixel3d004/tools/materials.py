"""Private, source-guided material clusters; no random patterns or source edits.

Every derived stroke is selected from existing atlas colors/features. Original
UVs stay unchanged. Spatial processing is material-specific, not a screen filter.
"""
import argparse,hashlib,json,shutil,struct
from pathlib import Path
import numpy as np
from PIL import Image,ImageFilter

RULES={
 'T_WoodTrim_BaseColor.png':(1,256,32,'horizontal grain bundles; endgrain/metal protected'),
 'T_Plaster_BaseColor.png':(2,256,24,'small low-contrast diagonal paint clusters'),
 'T_RoundTiles_BaseColor.png':(3,256,32,'vertical wear/color groupings; source tile boundaries retained'),
 'T_RockTrim_BaseColor.png':(4,256,24,'isotropic stone color clusters'),
 'T_Brick_BaseColor.png':(4,256,24,'isotropic brick color clusters'),
 'T_UnevenBrick_BaseColor.png':(4,256,28,'isotropic stone color clusters'),
 'Rocks_Diffuse.png':(4,256,28,'isotropic stone color clusters')}
def sha(b):return hashlib.sha256(b).hexdigest()
def source_path(root,name):
 p=(root/name).resolve()
 if root not in p.parents:raise ValueError('source path escapes pack')
 return p
def linear_reduce(im,size):
 # Original opaque source only. Working float linear light; no hidden palette
 # shift from averaging gamma-encoded samples.
 a=np.asarray(im,dtype=np.float32)/255
 rgb=a[:,:,:3];linear=np.where(rgb<=.04045,rgb/12.92,((rgb+.055)/1.055)**2.4)
 channels=[]
 for i in range(3):channels.append(np.asarray(Image.fromarray(linear[:,:,i]).resize((size,size),Image.Resampling.BOX)))
 v=np.stack(channels,axis=2);srgb=np.where(v<=.0031308,v*12.92,1.055*v**(1/2.4)-.055)
 return Image.fromarray(np.rint(np.clip(srgb,0,1)*255).astype(np.uint8))
def clusters(im,kind,colors):
 rgb=np.asarray(im,dtype=np.int16);h,w=rgb.shape[:2]
 # Palette is learned independently from each material atlas. Color families
 # (e.g. moss/terracotta, metal/timber) remain distinct; no global posterization.
 indexed=im.quantize(colors=colors,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
 quant=np.asarray(indexed.convert('RGB'),dtype=np.int16)
 if kind==1: offsets=[(-3,0),(-2,0),(-1,0),(0,0),(1,0),(2,0),(3,0)]
 elif kind==2:offsets=[(-1,-1),(0,0),(1,1)]
 elif kind==3:offsets=[(0,-2),(0,-1),(0,0),(0,1),(0,2)]
 else:offsets=[(-1,0),(1,0),(0,-1),(0,1),(0,0)]
 # Select a local medoid among already-authored palette colors; retain source
 # discontinuities. No regular block grid, UV distortion or generated noise.
 samples=np.stack([np.roll(quant,(dy,dx),axis=(0,1))for dx,dy in offsets])
 distances=np.sum((samples-quant[None]).astype(np.float32)**2,axis=3)
 valid=distances<28**2
 weights=valid[...,None].astype(np.float32)
 mean=np.sum(samples*weights,axis=0)/np.maximum(1,weights.sum(axis=0))
 nearest=np.argmin(np.sum((samples-mean[None])**2,axis=3),axis=0)
 result=np.take_along_axis(samples,nearest[None,:,:,None],axis=0)[0].astype(np.uint8)
 # Protect narrow high-contrast source joins/cracks against stroke coalescing.
 grad=np.maximum(np.max(np.abs(rgb-np.roll(rgb,1,axis=0)),axis=2),np.max(np.abs(rgb-np.roll(rgb,1,axis=1)),axis=2))
 result=np.where((grad>35)[...,None],quant,result).astype(np.uint8)
 if kind==1:
  # Horizontal grain treatment only in the two actual long-grain atlas bands.
  # End-grain and neutral-metal bottom strip keep source-aligned isotropic detail.
  result[int(h*.625):]=quant[int(h*.625):]
 if kind==2:
  # Plaster stays restrained: preserve source local chroma/luminance envelope.
  result=np.clip(rgb+(result.astype(np.int16)-rgb).clip(-8,8),0,255).astype(np.uint8)
 # Wrapped neighbours are allowed only in truly tileable atlas regions; retain
 # a 3px source-derived guard at edges to avoid creating new atlas wrap seams.
 result[:3]=quant[:3];result[-3:]=quant[-3:];result[:,:3]=quant[:,:3];result[:,-3:]=quant[:,-3:]
 return Image.fromarray(result).convert('RGBA')
def strokes(original,clustered,kind):
 rgb=np.asarray(original,dtype=np.float32);out=np.asarray(clustered).copy();h,w=out.shape[:2]
 luminance=rgb[:,:,:3]@np.array([.2126,.7152,.0722])
 # Ridge orientation is perpendicular to each material's long stroke axis.
 axis=0 if kind in (1,2) else 1
 ridge=luminance-(np.roll(luminance,2,axis=axis)+np.roll(luminance,-2,axis=axis))*.5
 limit={1:2.0,2:.8,3:1.2,4:2.0}[kind];budget={1:900,2:450,3:900,4:500}[kind]
 valid=(np.abs(ridge)>limit)
 valid[:4]=False;valid[-4:]=False;valid[:,:4]=False;valid[:,-4:]=False
 if kind==1:valid[int(h*.625):]=False
 if kind==3:valid &= rgb[:,:,0]>rgb[:,:,1]*1.12
 # Strong joins/cracks already exist; keep stroke tracing inside their material.
 order=np.argsort(-np.abs(ridge).ravel(),kind='stable');occupied=np.zeros((h,w),bool);count=0
 for flat in order:
  y,x=divmod(int(flat),w)
  if not valid[y,x]or occupied[y,x]:continue
  sign=1 if ridge[y,x]>0 else -1;seed=rgb[y,x,:3].copy();points=[(y,x)]
  for direction in [-1,1]:
   cy,cx=y,x
   for step in range(3 if kind in (1,3)else 1):
    if kind==1:candidates=[(cy+dy,cx+direction)for dy in (-1,0,1)]
    elif kind==3:candidates=[(cy+direction,cx+dx)for dx in (-1,0,1)]
    elif kind==2:candidates=[(cy+direction,cx+direction)]
    else:
     gy=luminance[min(h-1,cy+1),cx]-luminance[max(0,cy-1),cx];gx=luminance[cy,min(w-1,cx+1)]-luminance[cy,max(0,cx-1)];candidates=[(cy,cx+direction)]if abs(gy)>abs(gx)else[(cy+direction,cx)]
    candidates=[(a,b)for a,b in candidates if 3<=a<h-3 and 3<=b<w-3 and valid[a,b]and sign*ridge[a,b]>.35 and np.linalg.norm(rgb[a,b,:3]-seed)<30]
    if not candidates:break
    cy,cx=max(candidates,key=lambda q:sign*ridge[q]);points.append((cy,cx))
  if len(points)<2:continue
  amount={1:16,2:5,3:13,4:9}[kind]*sign
  for cy,cx in points:
   # Value strokes preserve each original material color family/alpha. Warm
   # high strokes and cool low strokes stay local rather than a global grade.
   delta=np.array([1.,.85,.65])*amount
   out[cy,cx,:3]=np.clip(out[cy,cx,:3].astype(float)+delta,0,255).astype(np.uint8)
   occupied[max(0,cy-1):cy+2,max(0,cx-3):cx+4]=True
  count+=1
  if count>=budget:break
 return Image.fromarray(out),count
def build(source,balanced,output,treatment="strokes"):
 source=source.resolve();balanced=balanced.resolve();output=output.resolve();repo=Path(__file__).resolve().parents[3]
 if output.exists()or output==repo or repo in output.parents:raise ValueError('new private output outside repository required')
 inventory=json.loads((source/'ASSET_INVENTORY.json').read_text())
 for f in inventory['files']:
  if sha(source_path(source,f['path']).read_bytes())!=f['sha256']:raise ValueError('source hash mismatch')
 validation=json.loads((balanced/'OPTIMIZATION.json').read_text())
 for t in validation['textures']:
  if sha(source_path(balanced,t['file']).read_bytes())!=t['file_sha256']:raise ValueError('balanced texture hash mismatch')
 base_bytes=validation['base_bytes']
 shutil.copytree(balanced,output)
 pack=json.loads((output/'pack.json').read_text());styles=[];ledger=[]
 for entry in pack['texture_overrides']:
  name=Path(entry['source']).name
  if name not in RULES:continue
  kind,size,colors,method=RULES[name];path=source_path(source,entry['source']);original=Image.open(path).convert('RGBA')
  if original.getchannel('A').getextrema()!=(255,255):raise ValueError('masked assets must not enter this treatment')
  reduced=linear_reduce(original,size);stroke_count=0
  art=reduced.convert('RGBA')if treatment=='resample'else clusters(reduced,kind,colors)
  if treatment=='strokes':art,stroke_count=strokes(reduced,art,kind)
  digest=sha(struct.pack('<II',*art.size)+art.tobytes());file='styled/'+digest+'.png';target=output/file;target.parent.mkdir(exist_ok=True);art.save(target)
  styles.append({'source':entry['source'],'file':file,'kind':kind})
  ledger.append({'source':entry['source'],'source_sha256':sha(path.read_bytes()),'original_size':list(original.size),'derived_size':list(art.size),'file':file,'sha256':sha(target.read_bytes()),'base_palette_budget':colors,'output_unique_rgb_colors':len(np.unique(np.asarray(art)[:,:,:3].reshape(-1,3),axis=0)),'source_guided_strokes':stroke_count,'kind':kind,'method':method if treatment!='resample'else'linear-light resize only; art-treatment control','uv':'unchanged','source_edited':False})
 pack['texture_styles']=styles;(output/'pack.json').write_text(json.dumps(pack,indent=2))
 extra=sum(x['derived_size'][0]*x['derived_size'][1]*4 for x in ledger)
 report={'revision':1,'method':'linear-light resample control only'if treatment=='resample'else'source-guided palette medoid plus ridge-traced short material strokes; no RNG'if treatment=='strokes'else'source-guided palette medoid only','files':ledger,'source_files_verified':len(inventory['files']),'extra_cached_base_bytes':extra,'treatment':treatment,'original_balanced_base_bytes':base_bytes,'comparison_cache_base_bytes':base_bytes+extra,'alpha_sensitive_art':'exact balanced/source bytes, unchanged','not_hand_authored':True,'production_approval':False}
 (output/'MATERIAL_PROVENANCE.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('balanced',type=Path);p.add_argument('output',type=Path);p.add_argument('--treatment',choices=['resample','cluster','strokes'],default='strokes');a=p.parse_args();build(a.source,a.balanced,a.output,a.treatment)
