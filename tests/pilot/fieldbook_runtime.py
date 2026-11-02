"""Actual UI regression: pointer navigation and modal input at two window sizes.
Private folio assets are supplied externally; no source artwork is embedded.
"""
import csv
import subprocess
import sys
from pathlib import Path

exe, assets, output = [Path(p).resolve() for p in sys.argv[1:4]]
output.mkdir(parents=True, exist_ok=True)
for width in [640,1280]:
    scale=width//640; dest=output/str(width);dest.mkdir(exist_ok=True)
    events=[f'5 click {610*scale} {335*scale}',f'25 click {584*scale} {185*scale}',
            '45 mouse 100 20','50 down d','52 up d','75 down m','77 up m',
            '79 down w',f'90 click {510*scale} {345*scale}',
            '110 down w','130 up w','140 down b','142 up b','170 down b','172 up b','180 down Escape','182 up Escape',
            f'200 click {320*scale} {183*scale}','220 down Escape','222 up Escape','230 down q','232 up q']
    replay=dest/'input.txt';replay.write_text('\n'.join(events)+'\n')
    subprocess.run([str(exe),'--assets',str(assets),'--pilot',str(assets/'pilot/pilot_warm3d.json'),
                    '--camera','raised','--window',str(width),'--save',str(dest/'save.json'),
                    '--output',str(dest/'run'),'--new-game','--allow-software','--hidden',
                    '--frames','250','--capture-every','10','--replay',str(replay),
                    '--cap','0','--pacing','off','--hud','off','--shafts','off','--haze','off'],check=True)
    rows=list(csv.DictReader(open(dest/'run/frames.csv')))
    recorded=list(csv.DictReader(open(dest/'run/events.csv')))
    names=[r['event'] for r in recorded]
    assert names.count('book_open')==2 and names.count('book_close')==2,names
    assert 'book_page_3' in names and 'book_page_4' in names,names
    for i in [30,60,85,91,100]:
        assert abs(float(rows[i]['player_z'])-8)<.001,(i,rows[i]['player_z'])
        assert abs(float(rows[i]['player_yaw']))<.001,(i,rows[i]['player_yaw'])
    assert float(rows[135]['player_z'])<7,'Movement did not resume after closing'
    assert 'resume' in names and 'save_quit' in names,names
    assert 'process_end'==names[-1]
print('Field-book pointer, keyboard, motion option, camera/movement isolation and resume passed at 640/1280')
