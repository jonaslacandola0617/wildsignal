"""Real executable and SDL input path, private authorized models; no gameplay mocks.
Cloud run is Mesa software only. This is event replay, not a physical Windows playtest.
"""
import csv,json,os,subprocess,sys,tempfile
from pathlib import Path
exe=Path(sys.argv[1]).resolve();assets=Path(sys.argv[2]).resolve();env=dict(os.environ);env.setdefault('SDL_VIDEODRIVER','offscreen');env.setdefault('LIBGL_ALWAYS_SOFTWARE','1')
with tempfile.TemporaryDirectory(prefix='wildsignal-gameloop-')as tmp:
 r=Path(tmp);save=r/'saves/game.json'
 prefix=[(0,'down','W'),(50,'up','W'),(51,'down','A'),(95,'up','A'),(96,'down','E'),(97,'up','E'),(100,'down','W'),(125,'up','W'),(130,'down','D'),(210,'up','D'),(211,'down','W'),(230,'up','W'),(232,'down','E'),(233,'up','E')]
 end=[(345,'down','Return'),(346,'up','Return'),(350,'down','F5'),(351,'up','F5'),(360,'down','Escape'),(361,'up','Escape'),(365,'down','Q')]
 def script(name,actions):
  p=r/(name+'.txt');p.write_text('\n'.join(f'{f} {a} {b}'for f,a,b in prefix+actions+end)+'\n');return p
 def run(name,extra=()):
  out=r/name;command=[str(exe),'--assets',str(assets),'--allow-software','--hidden','--frames','400','--cap','0','--save',str(save),'--output',str(out)]+list(extra)
  result=subprocess.run(command,env=env,capture_output=True,text=True,timeout=180);assert result.returncode==0,(name,result.stderr);return out
 actions=[(240,'down','2'),(241,'up','2'),(265,'down','1'),(266,'up','1'),(290,'down','1'),(291,'up','1'),(315,'down','3'),(316,'up','3')]
 out=run('full-loop',['--new-game','--replay',str(script('loop',actions))]);events=list(csv.DictReader((out/'events.csv').open()));names={e['event']for e in events};assert {'cottage_rest','encounter','defend','attack','befriend','return_explore','save','save_quit'}<=names,names
 state=json.loads(save.read_text());assert state['hp']==12 and state['recruited']==1 and state['visited']==1,state
 assert abs(state['x']-2.64)<.02 and abs(state['z']+1.87333)<.02,state
 frames=list(csv.DictReader((out/'frames.csv').open()));assert any(f['follower_moving']=='1'for f in frames),'follower static';assert any(f['phase']=='BATTLE'for f in frames),'no actual battle frames';assert any(int(f['grass_draws'])>0 for f in frames),'no real instancing';assert max(int(f['shadow_draws'])for f in frames[3:])==0,'moving actor invalidates static shadow cache'
 reopened=run('reopen',['--frames','3']);assert 'loaded=1 recovered=0'in(reopened/'run.log').read_text();assert json.loads(save.read_text())==state,'process restart lost progress'
 save.write_text('corrupt primary');recovered=run('recover',['--frames','3']);assert 'loaded=1 recovered=1'in(recovered/'run.log').read_text();assert json.loads(save.read_text())['recruited']==1
 save.write_text('bad');Path(str(save)+'.bak').write_text('bad');fresh=run('both-corrupt',['--frames','3']);assert 'loaded=0'in(fresh/'run.log').read_text();assert json.loads(save.read_text())['hp']==30
 loss=[]
 for frame in[240,265,290,315]:loss.extend([(frame,'down','3'),(frame+1,'up','3')])
 lost=run('lose-recover',['--new-game','--replay',str(script('loss',loss))]);events=list(csv.DictReader((lost/'events.csv').open()));assert any(e['phase']=='REST NEEDED'for e in events);state=json.loads(save.read_text());assert state['hp']==30 and state['recruited']==0 and abs(state['z']-4.8)<.02,state
 print('PASS actual SDL-control exploration/cottage/collision/follower/battle/attack/defend/recruit/restart/backup/corrupt-save/loss recovery. Software GL only.')
