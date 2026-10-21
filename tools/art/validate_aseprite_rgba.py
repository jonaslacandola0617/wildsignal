"""Validate a private single-frame RGBA Aseprite file against a PNG.
Structural/pixel validation only; never claims native Aseprite runtime validation.
Requires Pillow. Does not upload or modify artwork.
"""
import argparse,struct,zlib,json
from pathlib import Path
from PIL import Image,ImageChops

def validate(source,png):
 b=Path(source).read_bytes()
 def u16(off):return struct.unpack_from('<H',b,off)[0]
 assert len(b)>=144 and struct.unpack_from('<I',b)[0]==len(b),'File length'
 assert u16(4)==0xa5e0 and u16(6)==1 and u16(12)==32,'Only single-frame RGBA supported'
 w,h=u16(8),u16(10);end=128+struct.unpack_from('<I',b,128)[0]
 assert end==len(b) and u16(132)==0xf1fa,'Frame header'
 count=u16(134) or struct.unpack_from('<I',b,140)[0]
 off=144;layers=[];cels=[]
 for _ in range(count):
  length,typ=struct.unpack_from('<IH',b,off);assert length>=6 and off+length<=end,'Chunk bounds'
  p=b[off+6:off+length]
  if typ==0x2004:
   flags,kind,child,_,_,blend,opacity=struct.unpack_from('<HHHHHHB',p)
   n=struct.unpack_from('<H',p,16)[0];name=p[18:18+n].decode('utf8')
   assert len(p)>=18+n and kind==0 and child==0 and blend==0,'Unsupported layer type/blend'
   layers.append({'name':name,'visible':bool(flags&1),'opacity':opacity})
  elif typ==0x2005:
   idx,x,y,opacity,kind=struct.unpack_from('<HhhBH',p)
   assert kind in (0,2),'Unsupported linked or tilemap cel'
   cw,ch=struct.unpack_from('<HH',p,16);raw=zlib.decompress(p[20:]) if kind==2 else p[20:]
   assert len(raw)==cw*ch*4,'Cel byte length'
   cels.append((idx,x,y,opacity,Image.frombytes('RGBA',(cw,ch),raw)))
  off+=length
 assert off==end,'Chunk count/size mismatch'
 out=Image.new('RGBA',(w,h));seen=[]
 for idx,x,y,opacity,im in sorted(cels,key=lambda t:t[0]):
  assert 0<=idx<len(layers),'Cel layer reference'
  layer=layers[idx]
  assert opacity==255 and layer['opacity']==255,'Opacity conversion outside validator scope'
  if layer['visible']:out.alpha_composite(im,(x,y))
  seen.append(idx)
 assert len(set(seen))==len(seen),'Multiple cels on one layer'
 expected=Image.open(png).convert('RGBA');assert expected.size==out.size,'PNG size mismatch'
 # Compare all channels, including RGB under transparent pixels.
 exact=out.tobytes()==expected.tobytes()
 # Visible comparison ignores hidden RGB, which some exporters retain.
 visible=all((a[3]==e[3] and (a[3]==0 or a[:3]==e[:3])) for a,e in zip(out.getdata(),expected.getdata()))
 assert visible,'Visible pixel mismatch'
 return {'file':Path(source).name,'size':[w,h],'layers':[l['name'] for l in layers], 'cel_count':len(cels),'structure':'PASS','visible_pixels_match_png':visible,'all_rgba_bytes_match_png':exact,'aseprite_runtime':'NOT_RUN'}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('png');a=p.parse_args();print(json.dumps(validate(a.source,a.png),indent=2))
