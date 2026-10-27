param([string]$Root='.',[ValidateSet(480,640)][int]$Resolution=480,[ValidateSet(30,60)][int]$Cap=30,[int]$Frames=360)
$ErrorActionPreference='Stop'
$Root=(Resolve-Path -LiteralPath $Root).Path
$run=Join-Path $Root ('runs/HARDWARE-'+$Resolution+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $run -Force | Out-Null
$machine=@{Label='Physical machine evidence; swap-completion intervals are not verified displayed FPS';OS=Get-CimInstance Win32_OperatingSystem|Select-Object Caption,Version,BuildNumber,TotalVisibleMemorySize;CPU=Get-CimInstance Win32_Processor|Select-Object Name;GPU=Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM;RAM=Get-CimInstance Win32_ComputerSystem|Select-Object TotalPhysicalMemory}
$machine|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'MACHINE.json') -Encoding UTF8
$summary=@()
function Quantile($values,[double]$q){$v=@($values|Sort-Object);if(!$v.Count){return $null};return $v[[int][math]::Floor(($v.Count-1)*$q)]}
$configs=@(
 @{Grass='off';House='off';Cap=0;Pacing='off'},@{Grass='medium';House='off';Cap=0;Pacing='off'},@{Grass='off';House='on';Cap=0;Pacing='off'},@{Grass='medium';House='on';Cap=0;Pacing='off'},@{Grass='low';House='on';Cap=0;Pacing='off'},@{Grass='high';House='on';Cap=0;Pacing='off'},@{Grass='medium';House='on';Cap=$Cap;Pacing='precise'},@{Grass='medium';House='on';Cap=$Cap;Pacing='legacy'},@{Grass='medium';House='on';Cap=0;Pacing='off';Layout='legacy';Style='source'},@{Grass='medium';House='on';Cap=0;Pacing='off';Layout='continuous';Style='source'})
foreach($c in $configs){
 $layout=if($c.Layout){$c.Layout}else{'continuous'}; $style=if($c.Style){$c.Style}else{'pixel'}
 $name='layout-'+$layout+'-style-'+$style+'-grass-'+$c.Grass+'-house-'+$c.House+'-cap-'+$c.Cap+'-'+$c.Pacing
 $out=Join-Path $run $name
 $a=@('--assets',(Join-Path $Root 'assets'),'--benchmark','--resolution',"$Resolution",'--frames',"$Frames",'--grass-density',$c.Grass,'--megakit-house',$c.House,'--cap',"$($c.Cap)",'--pacing',$c.Pacing,'--output',$out,'--grass-layout',$layout,'--worldstyle',$style)
 # No captures/readback during timing. Every configuration follows the same route.
 & (Join-Path $Root 'wildsignal_gameloop002.exe') @a
 if($LASTEXITCODE -ne 0){throw "Runtime failed: $name. See its error.txt/run.log"}
 $rows=@(Import-Csv -LiteralPath (Join-Path $out 'frames.csv') | Where-Object {[int]$_.frame -ge 60})
 $gpu=@(Import-Csv -LiteralPath (Join-Path $out 'gpu.csv') | Where-Object {[int]$_.frame -ge 60})
 $parse=[System.Globalization.CultureInfo]::InvariantCulture
 $summary+=@{Name=$name;WarmupExcludedFrames=60;Samples=$rows.Count;ValidGpuSamples=$gpu.Count;AppLoopMedian_ms=(Quantile @($rows|ForEach-Object {[double]::Parse($_.app_loop_ms,$parse)}) .5);AppLoopP95_ms=(Quantile @($rows|ForEach-Object {[double]::Parse($_.app_loop_ms,$parse)}) .95);SwapCompletionP95_ms=(Quantile @($rows|ForEach-Object {[double]::Parse($_.swap_completion_interval_ms,$parse)}) .95);SubmitMedian_ms=(Quantile @($rows|ForEach-Object {[double]::Parse($_.cpu_submit_ms,$parse)}) .5);AnimationMedian_ms=(Quantile @($rows|ForEach-Object {[double]::Parse($_.animation_ms,$parse)}) .5);GpuMedian_ms=(Quantile @($gpu|ForEach-Object {[double]::Parse($_.gpu_elapsed_ms,$parse)}) .5);PeakWorkingSetMiB=(Quantile @($rows|ForEach-Object {[double]::Parse($_.peak_rss_bytes,$parse)/1MB}) 1);DrawsMedian=(Quantile @($rows|ForEach-Object {[int]$_.draws}) .5)}
}
$summary|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'SUMMARY.json') -Encoding UTF8
Write-Host "Finished. Return this entire folder privately: $run"
Write-Host 'Read frames.csv and gpu.csv by frame ID. Warmup/shader compilation must be excluded. No displayed-FPS certification is implied.'
Read-Host 'Press Enter to close'
