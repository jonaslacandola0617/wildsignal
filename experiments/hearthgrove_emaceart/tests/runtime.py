"""Actual SDL gameplay, two-companion camera Idle, reopen and corrupt-save recovery."""
import csv,json,os,subprocess,sys,tempfile
from pathlib import Path
from route import build
exe=Path(sys.argv[1]).resolve();assets=Path(sys.argv[2]).resolve()
env=dict(os.environ);env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('LIBGL_ALWAYS_SOFTWARE','1');env.setdefault('LP_NUM_THREADS','2')
cfg=json.loads((assets/'game.json').read_text());actions,count=build(cfg)
with tempfile.TemporaryDirectory(prefix='wildsignal-emacegrove-')as tmp:
 r=Path(tmp);save=r/'saves/game.json';replay=r/'loop.txt';replay.write_text('\n'.join(f'{f} {a} {v}'for f,a,v in actions)+'\n')
 def run(name,extra=()):
  out=r/name
  cmd=[str(exe),'--assets',str(assets),'--allow-software','--hidden','--frames','3','--cap','0','--grass-density','off','--shafts','off','--shadow','off','--haze','off','--save',str(save),'--output',str(out)]+list(extra)
  p=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=240)
  assert p.returncode==0,(name,p.stderr)
  return out
 out=run('full-loop',['--new-game','--frames',str(count),'--replay',str(replay)])
 events=list(csv.DictReader((out/'events.csv').open()));names={e['event']for e in events}
 assert {'cottage_rest','encounter','defend','attack','befriend','return_explore','save'}<=names,names
 state=json.loads(save.read_text());assert state['recruited']==1 and state['visited']==1 and state['hp']==12,state
 fs=list(csv.DictReader((out/'frames.csv').open()));assert max(float(f['player_z'])for f in fs)>20;assert any(f['follower_moving']=='1'for f in fs)
 reopened=run('reopen');assert 'loaded=1 recovered=0'in(reopened/'run.log').read_text();assert json.loads(save.read_text())==state
 yaw=r/'yaw.txt';yaw.write_text('\n'.join(f'{i} mouse 40 0'for i in range(20,100))+'\n')
 out=run('yaw',['--frames','150','--replay',str(yaw)]);fs=list(csv.DictReader((out/'frames.csv').open()));assert all(f['party']=='2'for f in fs)
 assert len({(f['follower_x'],f['follower_z'])for f in fs})==1
 assert all(f['follower_moving']=='0'and f['follower_state']=='IDLE'for f in fs),'camera-only orbit'
 save.write_text('corrupt primary');out=run('backup');assert 'loaded=1 recovered=1'in(out/'run.log').read_text();assert json.loads(save.read_text())['recruited']==1
 save.write_text('bad');Path(str(save)+'.bak').write_text('bad');out=run('both-corrupt');assert 'loaded=0'in(out/'run.log').read_text();assert json.loads(save.read_text())['hp']==30
 print('PASS actual SDL cottage/rest/battle/defend/attack/recruit/exploration/save/reopen, two-companion stationary-camera Idle and corrupt-save recovery. Effects OFF for logic test; Mesa software only.')
