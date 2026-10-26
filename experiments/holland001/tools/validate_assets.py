"""Validate actual source-derived models through the compiled engine importer."""
import argparse,collections,hashlib,json,subprocess
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('audit',type=Path);p.add_argument('pack',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
 source=json.loads((a.pack.parent/'CONVERSION.json').read_text());records=[]
 for r in source['records']:
  path=a.pack.parent/(r['id']+'.glb')
  assert hashlib.sha256(path.read_bytes()).hexdigest()==r['glb_sha256']
  loaded=json.loads(subprocess.check_output([str(a.audit.resolve()),str(path.resolve())],text=True))
  assert loaded['triangles']==r['triangles'],r['id']
  assert max(abs(x-y)for aa,bb in zip(loaded['bounds'],r['bounds'])for x,y in zip(aa,bb))<.0004,r['id']
  groups=collections.Counter()
  for g in loaded['groups']:groups[g['name']]+=g['triangles']
  assert dict(groups)==r['source_groups'],(r['id'],groups,r['source_groups'])
  for m in loaded['material_data']:
   authored=r['materials'][m['name']]
   assert max(abs(x-y)for x,y in zip(m['color'],authored['Kd']+[authored['d']]))<1e-6
   assert max(abs(x-y)for x,y in zip(m['emissive'],authored['Ke']))<1e-6
   assert m['alpha']==(2 if authored['d']<1 else 0)
  records.append({'id':r['id'],'pass':True,'loaded':loaded})
 a.output.write_text(json.dumps({'status':'PASS_ACTUAL_ENGINE_IMPORTER','upstream_commit':source['upstream_commit'],'records':records},indent=2)+'\n')
 print('PASS',len(records),'actual GLB source-derived models: bounds, triangles, group/material boundaries, Kd/Ke/opacity')
if __name__=='__main__':main()
