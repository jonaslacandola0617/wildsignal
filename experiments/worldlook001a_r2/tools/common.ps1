$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
$Exe=Join-Path $Root 'wildsignal_worldlook001a_r2.exe'
$Kit=Join-Path $Root 'assets/mesh/mesh_kit.json'
$Legacy=Join-Path $Root 'assets/grass/grass_kit.json'
if(!(Test-Path $Exe) -or !(Test-Path $Kit)){throw 'Missing executable or PRIVATE mesh kit. No substitute art.'}
$Base=@('--r2-kit',$Kit,'--look',(Join-Path $Root 'data/TOON_DIAGNOSTIC.json'),'--no-art-materials','--no-smooth-lighting','--study','B1','--shadow-proof','stable','--shadow','cached','--shadow-size','512','--shafts','off','--r2-count','24','--present-scale','2','--pose','0')
function Invoke-Plot([string]$Label,[string[]]$Flags){
 $out=Join-Path $Root ('runs/'+$Label+'-'+[guid]::NewGuid().ToString('N'))
 & $Exe @Base @Flags --output $out
 if($LASTEXITCODE -ne 0){throw "Engine failed: $Label"}
 return $out
}
