"""Real executable evidence for HOLLAND-001. No generated scene imagery.

All source assets, framebuffer pixels, benchmark data and clips stay private.
Video replay cadence is explicitly independent of measured application FPS.
"""
import argparse, collections, csv, hashlib, json, os, statistics, subprocess
from pathlib import Path
from PIL import Image, ImageDraw

CONTROLS = [('B0','off'), ('B1','off'), ('B2','off'), ('B2','raw'), ('B2','comp')]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('executable',type=Path);p.add_argument('pack',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--data',type=Path,default=Path(__file__).resolve().parents[1]/'data')
    p.add_argument('--baseline004',type=Path);p.add_argument('--quaternius-pack',type=Path)
    for name in ('stills','motion','benchmark','allow-software'):p.add_argument('--'+name,action='store_true')
    p.add_argument('--benchmark-frames',type=int,default=150)
    p.add_argument('--motion-frames',type=int,default=240)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    env=os.environ.copy()
    if os.name!='nt':env.setdefault('SDL_VIDEODRIVER','offscreen')
    env.setdefault('MESA_SHADER_CACHE_DIR',str(a.output/'mesa-cache'))
    runs=[];checks=[];metrics=[]
    def run(name,width=640,study='B1',camera='off',flags=(),frames=3,pose=14,route=None,capture=0,baseline=False,pack=None,scene='garden.json',presented=True):
        out=a.output/name
        cmd=[str((a.baseline004 if baseline else a.executable).resolve()),'--pack',str((pack or a.pack).resolve()),'--scene',str((a.data/scene).resolve()),'--look',str((a.data/'sunlight_look.json').resolve()),'--style','C','--no-art-materials','--mode','PIXEL3D','--resolution',str(width),'--pose',str(pose),'--shadow','cached','--shafts','off','--cap','0','--frames',str(frames),'--warmup',str(30 if route and not capture else 0),'--hidden','--output',str(out)]
        if not baseline:
            cmd+=['--study',study,'--pixel-camera',camera]
            if route:cmd+=['--route',route]
            cmd+=['--present-scale',str(3 if width==480 and presented else 2)]
            if presented:cmd+=['--capture-presented']
        if capture:cmd+=['--capture-every',str(capture)]
        if a.allow_software:cmd+=['--allow-software']
        cmd+=list(flags)
        result=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=600)
        if result.returncode:raise RuntimeError(name+'\n'+result.stdout+'\n'+result.stderr)
        (out/'stdout.txt').write_text(result.stdout);(out/'stderr.txt').write_text(result.stderr)
        for file in out.glob('*.ppm'):
            # Lossless format conversion only, original PPM remains available.
            Image.open(file).save(file.with_suffix('.png'))
        summary=json.loads((out/'summary.json').read_text())
        runs.append({'name':name,'command':cmd,'summary':summary,'native_sha256':hashlib.sha256((out/'final.ppm').read_bytes()).hexdigest()})
        print(name,flush=True)
        return out
    def sheet(name,items,presented=False):
        source='final_presented.png' if presented else 'final.png'
        imgs=[Image.open(path/source).convert('RGB') for label,path in items]
        width=sum(im.width for im in imgs);height=max(im.height for im in imgs)
        out=Image.new('RGB',(width,height+30),(24,30,36));draw=ImageDraw.Draw(out);x=0
        for (label,path),im in zip(items,imgs):
            draw.text((x+8,8),label,fill='white');out.paste(im,(x,30));x+=im.width
        out.save(a.output/name)
    def same(name,a0,b0,file='final.png'):
        equal=Image.open(a0/file).tobytes()==Image.open(b0/file).tobytes()
        checks.append({'name':name,'equal':equal});assert equal,name
    if a.stills:
        for width in (480,640):
            for pose in (13,14,12,1):
                items=[]
                for study,camera in CONTROLS:
                    out=run(f'still_{width}_p{pose}_{study}_{camera}',width,study,camera,pose=pose)
                    items.append((study+' '+camera.upper(),out))
                sheet(f'WORDLINE_NATIVE_{width}_p{pose}.png',items)
                sheet(f'WORDLINE_PRESENTED_{width}_p{pose}.png',items,presented=True)
                same('RAW_COMP_native_'+str((width,pose)),items[3][1],items[4][1])
                # Native close crops preserve actual framebuffer texels.
                for label,out in items:
                    im=Image.open(out/'final.png');box=(width//3,im.height//4,width*2//3,im.height*3//4)
                    im.crop(box).save(out/'detail_native.png')
            on=run('culling_on_'+str(width),width,'B2','comp',pose=14,presented=False)
            off=run('culling_off_'+str(width),width,'B2','comp',flags=['--no-cull'],pose=14,presented=False)
            same('conservative_culling_'+str(width),on,off)
        if a.baseline004:
            for width in (480,640):
                for pose in (0,1,2):
                    old=run(f'PR26_oracle_{width}_{pose}',width,pose=pose,baseline=True,pack=a.pack.parent/'oracle'/'pack.json',presented=False)
                    new=run(f'B0_parity_{width}_{pose}',width,'B0',pose=pose,presented=False)
                    same('actual_PR26_B0_'+str((width,pose)),old,new)
        # Independent effects, identical geometry/camera/sun and off-snap state.
        for width in (480,640):
            for name,flags in [('shadow_off',['--shadow','off']),('shadow_live',['--shadow','live']),('outline_off',['--no-outline']),('highlight_off',['--no-edge-highlights']),('shafts_screen',['--shafts','screen']),('shafts_volume',['--shafts','volume'])]:
                run(f'effect_{width}_{name}',width,flags=flags,pose=14)
        # Worldline versus the earlier Quaternius scene is a separate context
        # comparison, explicitly NOT a controlled shader or geometry test.
        if a.baseline004 and a.quaternius_pack:
            data004=Path(__file__).resolve().parents[2]/'pixel3d004'/'data'
            out=a.output/'quaternius_PR26_context'
            cmd=[str(a.baseline004.resolve()),'--pack',str(a.quaternius_pack.resolve()),'--scene',str(data004/'sunlit_cottage.json'),'--look',str(data004/'sunlight_look.json'),'--style','C','--mode','PIXEL3D','--resolution','640','--pose','1','--shadow','cached','--shafts','off','--cap','0','--frames','3','--warmup','0','--hidden','--output',str(out)]
            if a.allow_software:cmd+=['--allow-software']
            subprocess.run(cmd,env=env,capture_output=True,text=True,check=True,timeout=600)
            Image.open(out/'final.ppm').save(out/'final.png')
            runs.append({'name':'quaternius_PR26_context','command':cmd,'summary':json.loads((out/'summary.json').read_text()),'not_isolated_shader_comparison':True})
            sheet('CONTEXT_DIFFERENT_GEOMETRY_NOT_SHADER_AB.png',[('PR26 Quaternius - different assets/scene',out),('Wordline B1 - different assets/scene',a.output/'still_640_p1_B1_off')])
    if a.motion:
        for route in ('strafe','orbit'):
            dirs={}
            for study,camera in [('B0','off'),('B2','off'),('B2','raw'),('B2','comp')]:
                out=run(f'motion_{route}_{study}_{camera}',study=study,camera=camera,route=route,frames=a.motion_frames,capture=2)
                dirs[(study,camera)]=out
            def video(name,keys):
                cmd=['ffmpeg','-v','error','-y']
                for key in keys:cmd+=['-framerate','10','-pattern_type','glob','-i',str(dirs[key]/'presented_*.png')]
                count=len(keys);cmd+=['-filter_complex',''.join(f'[{i}:v]'for i in range(count))+f'hstack=inputs={count}[v]','-map','[v]','-c:v','libx264','-crf','19','-pix_fmt','yuv420p',str(a.output/name)]
                subprocess.run(cmd,check=True)
            video(f'B2_OFF_RAW_COMP_{route}.mp4',[('B2','off'),('B2','raw'),('B2','comp')])
            video(f'B0_VS_REFINED_OFF_{route}.mp4',[('B0','off'),('B2','off')])
            checks.append({'name':'matched_motion_'+route,'columns':['OFF','RAW','COMP'],'rendered_frames_per_run':a.motion_frames,'capture_every':2,'replay_fps':10,'verified_displayed_FPS':False})
    if a.benchmark:
        for width in (480,640):
            # Reverse ordering exposes some shared-host drift; no claim of
            # hardware significance from this software diagnostic sample.
            for index,(study,camera) in enumerate(CONTROLS+list(reversed(CONTROLS))):
                out=run(f'bench_{width}_{index}_{study}_{camera}',width,study,camera,route='strafe',frames=a.benchmark_frames,presented=False)
                summary=json.loads((out/'summary.json').read_text());rows=list(csv.DictReader((out/'frames.csv').open()))[30:]
                metrics.append({'name':out.name,'width':width,'study':study,'camera':camera,'loop_median_ms':summary['loop_median_ms'],'loop_p95_ms':summary['loop_p95_ms'],'present_p95_ms':summary['swap_completion_interval_p95_ms'],'cpu_submit_median_ms':summary['submit_median_ms'],'api_query_median_ms':summary['gpu_query_median_ms'],'api_query_missing':summary['gpu_query_missing_samples'],'draws_mean':statistics.mean(float(r['draw_calls'])for r in rows),'triangles_mean':statistics.mean(float(r['triangles_submitted'])for r in rows),'peak_RSS_MiB':summary['process_peak_rss_bytes']/1048576,'textures_bytes':summary['texture_base_level_bytes'],'software':not summary['hardware_gpu_tested']})
            for name,flags in [('cached',[]),('shadow_off',['--shadow','off']),('shadow_live',['--shadow','live']),('outline_off',['--no-outline']),('vegetation_off',['--no-vegetation']),('shafts_screen',['--shafts','screen']),('shafts_volume',['--shafts','volume'])]:
                out=run(f'cost_{width}_{name}',width,flags=flags,route='strafe',frames=a.benchmark_frames,presented=False)
                s=json.loads((out/'summary.json').read_text());rows=list(csv.DictReader((out/'frames.csv').open()))[30:]
                metrics.append({'name':out.name,'width':width,'study':'B1','camera':'off','loop_median_ms':s['loop_median_ms'],'loop_p95_ms':s['loop_p95_ms'],'present_p95_ms':s['swap_completion_interval_p95_ms'],'cpu_submit_median_ms':s['submit_median_ms'],'api_query_median_ms':s['gpu_query_median_ms'],'api_query_missing':s['gpu_query_missing_samples'],'draws_mean':statistics.mean(float(r['draw_calls'])for r in rows),'triangles_mean':statistics.mean(float(r['triangles_submitted'])for r in rows),'peak_RSS_MiB':s['process_peak_rss_bytes']/1048576,'textures_bytes':s['texture_base_level_bytes'],'software':not s['hardware_gpu_tested']})
    for name,data in [('RUNS.json',runs),('CHECKS.json',checks),('PERFORMANCE.json',metrics)]: (a.output/name).write_text(json.dumps(data,indent=2)+'\n')
    if metrics:
        with (a.output/'PERFORMANCE.csv').open('w',newline='') as f:
            writer=csv.DictWriter(f,fieldnames=list(metrics[0]));writer.writeheader();writer.writerows(metrics)
    (a.output/'REPLAY_NOTE.txt').write_text('Videos replay real default-back-buffer captures at 10 fps, one capture per two deterministic route steps. This cadence is not measured engine/display FPS. Native PPM/PNG pixels and separate timings are included. OFF/RAW/COMP columns share scene, sun, route, input and outline shader.\n')
    print('DONE',len(runs),'actual executable runs',flush=True)
if __name__=='__main__':main()
