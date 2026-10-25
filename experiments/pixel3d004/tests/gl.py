"""Actual OpenGL synthetic checks, explicitly SOFTWARE diagnostics in cloud."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile
from PIL import Image

def main(exe):
    spec=importlib.util.spec_from_file_location('fixtures',Path(__file__).with_name('import.py'))
    fixtures=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixtures)
    with tempfile.TemporaryDirectory()as tmp:
        root=Path(tmp);d,b=fixtures.fixture(root)
        (root/'pack.json').write_text(json.dumps({'revision':1,'models':[{'id':'triangle','path':'mesh.gltf'}]}))
        (root/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':str(i),'model':'triangle','position':[x,0,z],'yaw':0,'scale':1}for i,(x,z)in enumerate([(0,0),(100,0),(-100,0),(0,-100)])]}))
        env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','offscreen')
        def run(name,flags):
            out=root/name
            cmd=[exe,'--pack',str(root/'pack.json'),'--scene',str(root/'scene.json'),'--output',str(out),'--hidden','--allow-software','--frames','4','--warmup','0','--cap','0',*flags]
            r=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=60)
            assert r.returncode==0,r.stderr
            return out,Image.open(out/'final.ppm').copy()
        for w in (480,640):
            for mode in ('BASE','TOON','PIXEL3D'):
                out,on=run(f'on_{w}_{mode}',['--resolution',str(w),'--mode',mode])
                _,off=run(f'off_{w}_{mode}',['--resolution',str(w),'--mode',mode,'--no-cull'])
                assert on.tobytes()==off.tobytes(),'culling changes framebuffer'
                s=json.loads((out/'summary.json').read_text())
                assert s['gpu_query_missing_samples']==0,'lost timer association'
                assert on.size==(w,w*9//16)
                assert len(on.getcolors(on.width*on.height))>1,'blank renderer'
            _,toon=run(f'toon_{w}',['--resolution',str(w),'--mode','TOON'])
            out,pixel=run(f'pixel_{w}',['--resolution',str(w),'--verify-upscale'])
            assert 'EXACT_3X_NEAREST_PRESENTATION_PASS' in (out/'run.log').read_text()
            # An isolated triangle has no internal normal creases: changed pixels
            # must lie one cardinal texel inside its BASE silhouette. This catches
            # thick dilation even when an outline looks plausible by eye.
            bg=toon.getpixel((0,0));changed=0
            for y in range(1,toon.height-1):
                for x in range(1,toon.width-1):
                    if toon.getpixel((x,y))!=pixel.getpixel((x,y)):
                        assert toon.getpixel((x,y))!=bg
                        assert any(toon.getpixel((x+dx,y+dy))==bg for dx,dy in [(-1,0),(1,0),(0,-1),(0,1)]),'outline exceeds 1-pixel silhouette'
                        changed+=1
            assert changed>20,'no actual outline'
        look=Path(__file__).parents[1]/'data/cozy_look.json'
        out,cozy=run('cozy',['--look',str(look),'--verify-upscale'])
        _,cozy_off=run('cozy_off',['--look',str(look),'--no-cull'])
        assert cozy.tobytes()==cozy_off.tobytes(),'cozy culling changes pixels'
        _,legacy=run('legacy_compare',[])
        assert cozy.tobytes()!=legacy.tobytes(),'cozy shader inactive'
        bad=json.loads(look.read_text());bad['sun_direction']=[0,0,0]
        (root/'badlook.json').write_text(json.dumps(bad))
        failed=subprocess.run([exe,'--pack',str(root/'pack.json'),'--output',str(root/'invalid'),'--look',str(root/'badlook.json')],capture_output=True,text=True,env=env,timeout=20)
        assert failed.returncode!=0,'zero sun direction accepted'
        # A real shader readback must respect an authored normalized COLOR_0.
        d,b=fixtures.fixture(root)
        colors=struct.pack('<12H',*([3276,65535,3276,65535]*3))
        (root/'mesh.bin').write_bytes(b+colors);d['buffers'][0]['byteLength']=len(b+colors)
        d['bufferViews'].append({'buffer':0,'byteOffset':len(b),'byteLength':len(colors)})
        d['accessors'].append({'bufferView':4,'componentType':5123,'normalized':True,'count':3,'type':'VEC4'})
        d['meshes'][0]['primitives'][0]['attributes']['COLOR_0']=4
        d['materials'][0]['pbrMetallicRoughness']['baseColorFactor']=[1,1,1,1]
        (root/'mesh.gltf').write_text(json.dumps(d))
        _,colored=run('authored_color',['--mode','TOON'])
        assert sum(g>2*r and g>2*b for r,g,b in colored.getdata())>1000,'shader ignores or corrupts COLOR_0'
        # A synthetic lit floor + real triangle occluder validates the new
        # actual depth-map pipeline independently of licensed/private artwork.
        d,b=fixtures.fixture(root)
        d['images']=[{'uri':'white.png'}];d['textures']=[{'source':0}]
        d['materials'][0]['pbrMetallicRoughness']['baseColorTexture']={'index':0}
        (root/'mesh.gltf').write_text(json.dumps(d));Image.new('RGBA',(1,1),'white').save(root/'white.png')
        pack={'revision':1,'models':[{'id':'triangle','path':'mesh.gltf'},{'id':'Grass_Common_Short','path':'mesh.gltf'}]}
        (root/'pack.json').write_text(json.dumps(pack))
        (root/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'caster','model':'triangle','position':[0,0,0],'yaw':0,'scale':1},{'id':'ground','model':'ground','position':[0,0,0],'yaw':0,'scale':1}]}))
        _,clear=run('shadow_off',['--look',str(look)])
        out,live=run('shadow_live',['--look',str(look),'--shadow','live'])
        _,cached=run('shadow_cached',['--look',str(look),'--shadow','cached'])
        _,ground=run('shadow_ground',['--look',str(look),'--shadow','ground'])
        assert live.tobytes()!=clear.tobytes(),'no real cast shadow'
        assert cached.tobytes()==live.tobytes(),'static cache changes cast shadows'
        assert ground.tobytes()!=clear.tobytes(),'ground bake inactive'
        assert json.loads((out/'summary.json').read_text())['atmosphere_logical_allocated_bytes']==512*512*4
        for method in ('screen','volume'):
            _,a=run('shaft_'+method,['--look',str(look),'--shafts',method,'--shadow','cached','--pose','6'])
            _,b=run('shaft_'+method+'_offcull',['--look',str(look),'--shafts',method,'--shadow','cached','--pose','6','--no-cull'])
            assert a.tobytes()==b.tobytes(),'shaft culling changes atmosphere'
        # A solid surface less than .25 units from the eye leaves no marchable
        # air segment. Neither method may leak radiance from behind that wall.
        d,b=fixtures.fixture(root)
        positions=[(-100.,-100.,12.8),(100.,-100.,12.8),(0.,100.,12.8)]
        raw=b''.join(struct.pack('<3f',*p)for p in positions)+b[36:]
        (root/'mesh.bin').write_bytes(raw);d['nodes'][0]={'mesh':0}
        (root/'mesh.gltf').write_text(json.dumps(d))
        (root/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'solid-near-wall','model':'triangle','position':[0,0,0],'yaw':0,'scale':1}]}))
        _,solid=run('near_solid',['--look',str(look),'--pose','0'])
        for method in ('screen','volume'):
            _,effect=run('near_solid_'+method,['--look',str(look),'--pose','0','--shafts',method])
            assert solid.tobytes()==effect.tobytes(),'shaft leaks through a near solid wall'
        # A private-pack style entry must genuinely switch a sampler. Missing
        # style art fails closed unless explicitly disabled for diagnostics.
        failed=subprocess.run([exe,'--pack',str(root/'pack.json'),'--scene',str(root/'scene.json'),'--look',str(look),'--style','B','--output',str(root/'missing_style'),'--hidden','--allow-software','--frames','1'],capture_output=True,text=True,env=env,timeout=60)
        assert failed.returncode!=0 and 'STYLE_ASSETS_MISSING' in failed.stderr
        d,b=fixtures.fixture(root)
        d['images']=[{'uri':'white.png'}];d['textures']=[{'source':0}];d['materials'][0]['pbrMetallicRoughness']={'baseColorFactor':[1,1,1,1],'baseColorTexture':{'index':0}}
        (root/'mesh.gltf').write_text(json.dumps(d));Image.new('RGBA',(2,2),(20,200,80,255)).save(root/'style.png')
        pack={'revision':1,'models':[{'id':'Wall_Test','path':'mesh.gltf'}],'texture_styles':[{'source':'white.png','file':'style.png','kind':2}]}
        (root/'pack.json').write_text(json.dumps(pack));(root/'scene.json').write_text(json.dumps({'revision':1,'instances':[{'id':'one','model':'Wall_Test','position':[0,0,0],'yaw':0,'scale':1}]}))
        _,original=run('artA',['--look',str(look),'--style','A'])
        _,material=run('artB',['--look',str(look),'--style','B'])
        assert original.tobytes()!=material.tobytes(),'material sampler does not change'
        flags=['--look',str(look),'--style','C','--shadow','cached','--shafts','volume']
        out,c=run('artC',flags+['--verify-upscale'])
        _,off=run('artC_nocull',flags+['--no-cull'])
        assert c.tobytes()==off.tobytes(),'styleC culling changes pixels'
        meta=json.loads((out/'summary.json').read_text());assert meta['atmosphere_width_actual']==320 and meta['styled_texture_cached_bytes']==16
        _,clearC=run('C_clear',['--look',str(look),'--style','C'])
        _,zeroC=run('C_zero_air',['--look',str(look),'--style','C','--shafts','volume','--shaft-strength','0'])
        assert clearC.tobytes()==zeroC.tobytes(),'air reconstruction changes material detail at zero radiance'
        # Unoccluded planar receiver remains lit under corrected depth comparison.
        positions=[(-100.,-100.,12.8),(100.,-100.,12.8),(0.,100.,12.8)]
        (root/'mesh.bin').write_bytes(b''.join(struct.pack('<3f',*p)for p in positions)+b[36:]);d['nodes'][0]={'mesh':0};(root/'mesh.gltf').write_text(json.dumps(d))
        _,solidC=run('C_solid',['--look',str(look),'--style','C','--pose','0'])
        _,shadowC=run('C_solid_shadow',['--look',str(look),'--style','C','--pose','0','--shadow','cached'])
        assert solidC.tobytes()==shadowC.tobytes(),'corrected flat receiver self shadows'
        _,volumeC=run('C_solid_volume',['--look',str(look),'--style','C','--pose','0','--shafts','volume'])
        assert solidC.tobytes()==volumeC.tobytes(),'filtered shafts leak behind a near solid'
        print('PASS: three real GL modes, both resolutions, exact culling equality, frame query association, 1px outline and exact 3x nearest default-framebuffer readback')

if __name__=='__main__':main(sys.argv[1])
