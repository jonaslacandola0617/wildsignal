param([string]$Root='.',[ValidateSet(480,640)][int]$Resolution=480,[ValidateSet(30,60)][int]$Cap=30,[int]$Frames=360)
$ErrorActionPreference='Stop'
$Root=(Resolve-Path -LiteralPath $Root).Path
$run=Join-Path $Root ('runs/HARDWARE-'+$Resolution+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $run -Force | Out-Null
$machine=@{Label='Physical machine evidence; swap-completion intervals are not verified displayed FPS';OS=Get-CimInstance Win32_OperatingSystem|Select-Object Caption,Version,BuildNumber,TotalVisibleMemorySize;CPU=Get-CimInstance Win32_Processor|Select-Object Name;GPU=Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM;RAM=Get-CimInstance Win32_ComputerSystem|Select-Object TotalPhysicalMemory}
$machine|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'MACHINE.json') -Encoding UTF8
$configs=@(
 @{Grass='off';House='off';Cap=0;Pacing='off'},@{Grass='medium';House='off';Cap=0;Pacing='off'},@{Grass='off';House='on';Cap=0;Pacing='off'},@{Grass='medium';House='on';Cap=0;Pacing='off'},@{Grass='low';House='on';Cap=0;Pacing='off'},@{Grass='high';House='on';Cap=0;Pacing='off'},@{Grass='medium';House='on';Cap=$Cap;Pacing='precise'},@{Grass='medium';House='on';Cap=$Cap;Pacing='legacy'})
foreach($c in $configs){
 $name='grass-'+$c.Grass+'-house-'+$c.House+'-cap-'+$c.Cap+'-'+$c.Pacing
 $out=Join-Path $run $name
 $a=@('--assets',(Join-Path $Root 'assets'),'--benchmark','--resolution',"$Resolution",'--frames',"$Frames",'--grass-density',$c.Grass,'--megakit-house',$c.House,'--cap',"$($c.Cap)",'--pacing',$c.Pacing,'--output',$out)
 # No captures/readback during timing. Every configuration follows the same route.
 & (Join-Path $Root 'wildsignal_gameloop001.exe') @a
 if($LASTEXITCODE -ne 0){throw "Runtime failed: $name. See its error.txt/run.log"}
}
Write-Host "Finished. Return this entire folder privately: $run"
Write-Host 'Read frames.csv and gpu.csv by frame ID. Warmup/shader compilation must be excluded. No displayed-FPS certification is implied.'
Read-Host 'Press Enter to close'
