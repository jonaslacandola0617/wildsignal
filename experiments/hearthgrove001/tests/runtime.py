"""Actual native SDL route through Hearthgrove, battle/recruit/reopen/recovery.
No private art in this test; provide a validated private runtime directory.
"""
import csv,json,os,subprocess,sys,tempfile
from pathlib import Path
exe=Path(sys.argv[1]).resolve();assets=Path(sys.argv[2]).resolve();env=dict(os.environ);env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('LIBGL_ALWAYS_SOFTWARE','1');env.setdefault('LP_NUM_THREADS','2')
with tempfile.TemporaryDirectory(prefix='wildsignal-hearthgrove-')as tmp:
 r=Path(tmp);save=r/'saves/hearthgrove.json'
 prefix=[(0,'down','W'),(180,'up','W'),(181,'down','A'),(261,'up','A'),(262,'down','W'),(292,'up','W'),(293,'down','A'),(333,'up','A'),(334,'down','E'),(335,'up','E'),(340,'down','D'),(428,'up','D'),(430,'down','S'),(490,'up','S'),(492,'down','E'),(493,'up','E')]
 end=[(620,'down','Return'),(621,'up','Return'),(623,'down','F5'),(624,'up','F5'),(655,'down','Escape'),(656,'up','Escape'),(660,'down','Q')]
 def script(name,actions):
  p=r/(name+'.txt');p.write_text('\n'.join(f'{f} {a} {b}'for f,a,b in prefix+actions+end)+'\n');return p
 def run(name,extra=()):
  out=r/name;cmd=[str(exe),'--assets',str(assets),'--allow-software','--hidden','--frames','700','--cap','0','--save',str(save),'--output',str(out)]+list(extra);p=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=240);assert p.returncode==0,(name,p.stderr);return out
 actions=[(520,'down','2'),(521,'up','2'),(545,'down','1'),(546,'up','1'),(570,'down','1'),(571,'up','1'),(595,'down','3'),(596,'up','3')]
 out=run('full-loop',['--new-game','--replay',str(script('loop',actions))]);events=list(csv.DictReader((out/'events.csv').open()));names={e['event']for e in events};assert {'cottage_rest','encounter','defend','attack','befriend','return_explore','save','save_quit'}<=names,(names,events)
 state=json.loads(save.read_text());assert state['hp']==12 and state['recruited']==1 and state['visited']==1,state
 fs=list(csv.DictReader((out/'frames.csv').open()));assert max(float(f['player_z'])for f in fs)>12,'still trapped in12mglade';assert min(float(f['player_x'])for f in fs)<-8,'west cottage inaccessible';assert any(f['follower_moving']=='1'for f in fs);assert all(f['grass_shadow_draws']=='0'and f['grass_shadow_triangles']=='0'for f in fs);assert max(int(f['shadow_cache_invalidations'])for f in fs)==1,'camera/actors rebuild cached shadows'
 reopened=run('reopen',['--frames','3']);assert 'loaded=1 recovered=0'in(reopened/'run.log').read_text();assert json.loads(save.read_text())==state
 yaw=r/'yaw.txt';yaw.write_text('\n'.join(f'{i} mouse 40 0'for i in range(20,100))+'\n');turn=run('yaw-only',['--frames','150','--replay',str(yaw)]);fs=list(csv.DictReader((turn/'frames.csv').open()));assert all(f['party']=='2'for f in fs);assert len({(f['follower_x'],f['follower_z'])for f in fs})==1;assert all(f['follower_moving']=='0'and f['follower_state']=='IDLE'for f in fs),'camera caused follower orbit'
 save.write_text('corrupt primary');backup=run('recover',['--frames','3']);assert 'loaded=1 recovered=1'in(backup/'run.log').read_text();assert json.loads(save.read_text())['recruited']==1
 save.write_text('bad');Path(str(save)+'.bak').write_text('bad');fresh=run('both-corrupt',['--frames','3']);assert 'loaded=0'in(fresh/'run.log').read_text();assert json.loads(save.read_text())['hp']==30
 loss=[]
 for f in[520,545,570,595]:loss.extend([(f,'down','3'),(f+1,'up','3')])
 lost=run('loss',['--new-game','--replay',str(script('loss',loss))]);es=list(csv.DictReader((lost/'events.csv').open()));assert any(e['phase']=='REST NEEDED'for e in es);state=json.loads(save.read_text());assert state['hp']==30 and state['recruited']==0 and abs(state['z']-14)<.02,state
 print('PASS actual Hearthgrove SDL walking beyond old bounds, cottage rest, collision route, followers, battle/attack/defend/recruit, restart, corrupt-save recovery, loss return and camera-only two-companion Idle. Mesa software only.')
