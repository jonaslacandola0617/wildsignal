"""Analyze real exported frame intervals. Old and new files retain their scope.
No frame is dropped except the run's recorded warmup. No scanout/FPS inference.
"""
import argparse,csv,json,math
from pathlib import Path

def quantile(values,q):
 a=sorted(values);return a[int((len(a)-1)*q)]if a else None

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('input',type=Path);p.add_argument('output',type=Path);p.add_argument('--plots',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);records=[]
 for file in sorted(a.input.rglob('frames.csv')):
  meta=file.parent/'summary.json'
  if not meta.exists():continue
  s=json.loads(meta.read_text(encoding='utf-8-sig'));rows=[r for r in csv.DictReader(file.open(encoding='utf-8-sig'))if int(r['frame'])>=s['warmup_excluded']]
  if not rows:continue
  fields={'loop':'loop_ms','API_return_interval':'swap_completion_interval_ms','GPU_or_software_query':'gpu_query_ms','CPU_work_wall':'cpu_work_wall_ms','complete_interval':'complete_interval_ms','wait_error':'wait_deadline_error_ms','sleep_call_wall':'sleep_call_wall_ms','active_wait_wall':'active_wait_wall_ms'}
  rec={'run':file.parent.name,'source':s['source'],'scope':'SOFTWARE_GL_DIAGNOSTIC'if not s['hardware_gpu_tested']else'ACTUAL_DEVICE_API_NOT_SCANOUT','renderer':s['renderer'],'fixture':s.get('fixture_id','LEGACY_FIXTURE_SEE_COMMAND'),'pacing':s.get('pacing','legacy'),'width':int(rows[0]['width']),'drawable_width':s['drawable_width'],'drawable_height':s['drawable_height'],'cap':s['cap_fps'],'timed_samples':len(rows),'working_set_or_RSS_MiB':s['process_peak_rss_bytes']/1048576,'capture_enabled':s['capture_enabled'],'scanout_verified':False}
  data={}
  for name,key in fields.items():
   values=[float(r[key])for r in rows if key in r and math.isfinite(float(r[key]))and(name not in ['API_return_interval','complete_interval']or int(r['frame'])>0)and(name!='GPU_or_software_query'or float(r[key])>=0)]
   data[name]=values
   for q in [.5,.95,.99]:rec[name+'_p'+str(int(q*100))+'_ms']=quantile(values,q)
  cap=rec['cap'];rec['strict_original_gate']='NOT_APPLICABLE_UNCAPPED'
  if cap:
   target=33.34 if cap==30 else 16.67
   observed=s['hardware_gpu_tested']and'Intel'in s['vendor']and'UHD'in s['renderer']and s['os_build'].startswith('Windows')and len(rows)>=480 and not s['capture_enabled']and not s.get('pacing_controls_mixed',False)
   rec['strict_original_gate']='NOT_VERIFIED_OWNER_INTEL_UHD'
   if observed:rec['strict_original_gate']='PASS_API_NOT_SCANOUT'if rec['loop_p95_ms']<target and rec['API_return_interval_p95_ms']<target else'FAIL_API_FRAME_PACING'
   rec['loop_violation_ms']=max(0,rec['loop_p95_ms']-target);rec['API_interval_violation_ms']=max(0,rec['API_return_interval_p95_ms']-target)
   rec['relative_API_phase_drift_ms']=sum(data['API_return_interval'])-len(data['API_return_interval'])*1000/cap
  records.append(rec)
  if a.plots:
   import matplotlib;matplotlib.use('Agg')
   import matplotlib.pyplot as plt
   fig,axes=plt.subplots(3,1,figsize=(10,8),layout='constrained')
   axes[0].plot([int(r['frame'])for r in rows],data['loop'],label='loop wall');axes[0].plot([int(r['frame'])for r in rows if int(r['frame'])>0],data['API_return_interval'],label='API swap-return interval',alpha=.7)
   if cap:axes[0].axhline(1000/cap,color='black',linestyle='--',label='nominal target')
   axes[0].set_ylabel('ms');axes[0].legend()
   drift=[];total=0
   for v in data['API_return_interval']:total+=v-(1000/cap if cap else 0);drift.append(total)
   axes[1].plot(drift);axes[1].set_ylabel('relative API phase drift ms'if cap else'elapsed API intervals ms')
   if data['wait_error']:axes[2].plot(data['wait_error'],label='direct wait error');axes[2].legend()
   else:axes[2].text(.05,.5,'Legacy input: sleep/active wait not separately recorded',transform=axes[2].transAxes)
   axes[2].set_ylabel('ms');axes[2].set_xlabel('timed sample');fig.suptitle(rec['scope']+' | '+rec['run']+'\nNot scanout/displayed FPS');fig.savefig(a.output/(rec['run']+'.png'),dpi=120);plt.close(fig)
 (a.output/'ANALYSIS.json').write_text(json.dumps(records,indent=2)+'\n')
 if records:
  keys=list(dict.fromkeys(k for r in records for k in r))
  with (a.output/'ANALYSIS.csv').open('w',newline='')as f:w=csv.DictWriter(f,fieldnames=keys);w.writeheader();w.writerows(records)
 print('Analyzed',len(records),'real runs')
if __name__=='__main__':main()
