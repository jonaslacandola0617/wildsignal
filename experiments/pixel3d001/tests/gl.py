"""Actual OpenGL synthetic checks, explicitly SOFTWARE diagnostics in cloud."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
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
        print('PASS: three real GL modes, both resolutions, exact culling equality, frame query association, 1px outline and exact 3x nearest default-framebuffer readback')

if __name__=='__main__':main(sys.argv[1])
