#!/usr/bin/env python3
"""Build a PRIVATE runtime from original owned/licensed files; never write assets into engine Git."""
import argparse
import hashlib
import json
import shutil
import zipfile
from pathlib import Path

SHA = 'b7b228ca232da9958f191b01db87764dddb67610116151e06e8dd8ade6de94b8'
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pixel-crawler', type=Path, required=True)
    parser.add_argument('--art', type=Path, required=True)
    parser.add_argument('--creatures', type=Path, required=True, help='Converted original WS004/005/008 GLBs with their external PNGs')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    out = args.output.resolve()
    if out.is_relative_to(repo):
        raise SystemExit('Private output must be outside the public engine checkout')
    if out.exists():
        raise SystemExit('Choose a new output directory; existing runtime edits are preserved')
    archive = args.pixel_crawler.read_bytes()
    if hashlib.sha256(archive).hexdigest() != SHA:
        raise SystemExit('Pixel Crawler source checksum mismatch')
    house = args.art/'HOUSES_023-028/WS_HOUSE_023'
    sources = [house/'WS_HOUSE_023.glb', house/'WS_HOUSE_023_atlas.png']
    for species in ('WS004', 'WS005', 'WS008'):
        if not (args.creatures/(species+'.glb')).is_file():
            raise SystemExit('Missing original model: '+species)
        sources.extend(args.creatures.glob(species+'*'))
    if any(not p.is_file() for p in sources):
        raise SystemExit('Missing approved house or original model files')
    assets = out/'assets'
    sheets = assets/'pixel_crawler'
    sheets.mkdir(parents=True)
    with zipfile.ZipFile(args.pixel_crawler) as z:
        if z.testzip() is not None:
            raise SystemExit('Damaged source ZIP')
        for name in z.namelist():
            if not name.endswith('.png'):
                continue
            parts = Path(name).parts
            dest = parts[-1]
            if '/Trees/' in name:
                dest = 'Trees_'+parts[-2]+'_'+dest
            elif '/Body_A/Animations/' in name:
                if parts[-2] not in ('Idle_Base', 'Walk_Base'):
                    continue
                dest = 'Body_A_'+dest
            else:
                if parts[-1] not in ('Vegetation.png', 'Rocks.png', 'Floors_Tiles.png', 'Wall_Tiles.png'):
                    continue
            (sheets/dest).write_bytes(z.read(name))
        terms = [n for n in z.namelist() if n.lower().endswith('terms.txt')]
        if not terms:
            raise SystemExit('Missing source licence')
        (out/'PIXEL_CRAWLER_TERMS.txt').write_bytes(z.read(terms[0]))
    for source in sources:
        shutil.copy2(source, assets/source.name)
    shutil.copytree(repo/'data/content', out/'content')
    shutil.copy2(repo/'data/journey/layout.json', out/'content/journey.json')
    manifest = {'status':'Fresh HD-2D foundation; not a completed game or owner-approved visual result',
                'pixel_crawler_source_sha256':SHA, 'source':'Anokolisa Pixel Crawler Free Pack 2.11; owned house023 and WS004/005/008 models',
                'signari_animation':'Static original models in this foundation; runtime rigs/animation integration remains outstanding',
                'files':{str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.is_file()}}
    (out/'MANIFEST.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('Prepared private runtime:',out)
if __name__ == '__main__':
    main()
