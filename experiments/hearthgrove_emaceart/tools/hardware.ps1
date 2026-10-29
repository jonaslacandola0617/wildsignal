param([string]$Root='.')
$ErrorActionPreference='Stop';$Root=(Resolve-Path -LiteralPath $Root).Path
$run=Join-Path $Root ('runs/HEARTHGROVE-HARDWARE-'+(Get-Date -Format 'yyyyMMdd-HHmmss'));New-Item -ItemType Directory -Path $run -Force|Out-Null
@{Label='Physical hardware application/API diagnostics; NOT verified displayed FPS';OS=Get-CimInstance Win32_OperatingSystem|Select-Object Caption,Version,BuildNumber;CPU=Get-CimInstance Win32_Processor|Select-Object Name;GPU=Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion;RAM=Get-CimInstance Win32_ComputerSystem|Select-Object TotalPhysicalMemory}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $run 'MACHINE.json') -Encoding UTF8
function Q($values,[double]$q){$v=@($values|Sort-Object);if(!$v.Count){return $null};if($q -eq .5 -and $v.Count%2 -eq 0){return ($v[$v.Count/2-1]+$v[$v.Count/2])/2};return $v[[int][math]::Max(0,[math]::Ceiling($v.Count*$q)-1)]}
$ci=[System.Globalization.CultureInfo]::InvariantCulture
function N($s){return [double]::Parse([string]$s,$ci)}
$summary=@()
foreach($res in @(480,640)){foreach($mode in @('OFF','GLOW','STREAKS')){
 $out=Join-Path $run ($mode+'_'+$res);$shafts=if($mode -eq 'OFF'){'off'}else{'on'};$profile=if($mode -eq 'GLOW'){'glow'}else{'streaks'};$extra=@('--shadow','on','--shafts',$shafts,'--shaft-profile',$profile,'--haze','off','--grass-style','owner','--grass-density','high','--quality','medium')
 $a=@('--assets',(Join-Path $Root 'assets'),'--benchmark','--pose','13','--resolution',"$res",'--frames','180','--cap','0','--pacing','off','--output',$out)+$extra
 & (Join-Path $Root 'wildsignal_hearthgrove_emaceart.exe') @a
 if($LASTEXITCODE -ne 0){throw "Runtime failed: $mode $res. Return error.txt and run.log."}
 $f=@(Import-Csv (Join-Path $out 'frames.csv')|Where-Object {[int]$_.frame -ge 30});$g=@(Import-Csv (Join-Path $out 'gpu.csv')|Where-Object {[int]$_.frame -ge 30});if($f.Count -ne 150){throw 'Incomplete frames'}
 $summary+=@{Mode=$mode;Resolution=$res;Measured=150;GpuValid=$g.Count;AppP50=(Q @($f|ForEach-Object {N $_.app_loop_ms}) .5);AppP95=(Q @($f|ForEach-Object {N $_.app_loop_ms}) .95);AppP99=(Q @($f|ForEach-Object {N $_.app_loop_ms}) .99);SubmitP50=(Q @($f|ForEach-Object {N $_.cpu_submit_ms}) .5);GpuP50=(Q @($g|ForEach-Object {N $_.gpu_elapsed_ms}) .5);WorkingSetMiB=(Q @($f|ForEach-Object {(N $_.peak_rss_bytes)/1MB}) 1);Draws=(Q @($f|ForEach-Object {N $_.draws}) .5);Triangles=(Q @($f|ForEach-Object {N $_.triangles}) .5);GrassShadowDraws=(Q @($f|ForEach-Object {N $_.grass_shadow_draws}) 1)}
}}
$summary|ConvertTo-Json -Depth 5|Set-Content (Join-Path $run 'SUMMARY.json') -Encoding UTF8
Write-Host "Return this complete folder privately: $run";Write-Host 'No capture/encoding in timed loop. Original textures; normal-map/PBR rendering is unsupported. This is not display-FPS certification.';Read-Host 'Enter to close'
