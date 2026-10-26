# Genuine fixed-view captures made on the owner's machine, not benchmark runs.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'wildsignal_worldlook001a.exe'
$dest=Join-Path $root ('runs/visual-'+[guid]::NewGuid().ToString('N').Substring(0,8))
foreach ($width in @(480,640)) {
 foreach ($density in @('off','sparse','medium','dense')) {
  $cmd=@('--pack',(Join-Path $root 'assets/pack.json'),'--scene',(Join-Path $root 'data/garden.json'),'--look',(Join-Path $root 'data/TOON_DIAGNOSTIC.json'),'--style','C','--mode','PIXEL3D','--no-art-materials','--no-smooth-lighting','--study','B1','--pixel-camera','off','--edge-mode','combined','--shadow-proof','stable','--shadow','cached','--shafts','off','--grass-kit',(Join-Path $root 'assets/grass/grass_kit.json'),'--grass',$density,'--resolution',[string]$width,'--pose','14','--cap','0','--frames','3','--warmup','0','--present-scale','2','--capture-presented','--output',(Join-Path $dest "$width-$density"))
  $quoted=@($cmd | ForEach-Object {'"'+$_+'"'}) -join ' '
  $p=Start-Process -FilePath $exe -ArgumentList $quoted -WorkingDirectory $root -NoNewWindow -Wait -PassThru
  if ($p.ExitCode -ne 0) {throw 'Capture failed; no fallback'}
 }
}
Write-Host "Actual native/presented PPM captures: $dest. These captured runs are not performance measurements."
Invoke-Item $dest
