"""Fetch exact owner-authorized Wordline files into a NEW private directory."""
import argparse,concurrent.futures,hashlib,json,urllib.parse,urllib.request
from pathlib import Path
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);a=p.parse_args()
    repository=next((parent for parent in Path(__file__).resolve().parents if (parent/'.git').exists()),None)
    dest=a.output.resolve()
    if (repository and dest.is_relative_to(repository)) or dest.exists():raise ValueError('Use a new private directory outside the public game repository')
    source=json.loads((Path(__file__).parents[1]/'data/wordline_sources.json').read_text())
    dest.mkdir(parents=True);root=dest/'upstream'
    def fetch(f):
        url='https://raw.githubusercontent.com/'+source['repository']+'/'+source['commit']+'/'+urllib.parse.quote(f['path'])
        with urllib.request.urlopen(url,timeout=60) as response:data=response.read()
        if len(data)!=f['size'] or hashlib.sha256(data).hexdigest()!=f['sha256']:raise ValueError('Source SHA/size mismatch: '+f['path'])
        if hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()!=f['git_blob_sha']:raise ValueError('Git blob mismatch')
        file=root/f['relative'];file.parent.mkdir(parents=True,exist_ok=True);file.write_bytes(data)
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as e:list(e.map(fetch,source['files']))
    (dest/'UPSTREAM_VERIFIED.json').write_text(json.dumps(source,indent=2)+'\n')
    print('Verified',len(source['files']),'unchanged source files at',source['commit'])
if __name__=='__main__':main()
