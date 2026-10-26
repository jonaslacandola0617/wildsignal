param(
 [int]$Frames=600,[int]$Warmup=120,[int]$Repeats=1,
 [switch]$IncludeVsync,[switch]$SkipUncapped,
 [ValidateSet('default','0','1')][string]$TimerResolution='default',
 [ValidateSet('native','sdl')][string]$WaitBackend='native',
 [ValidateRange(0,1000)][int]$FineTailUs=500,
 [string]$SimpleRoot=''
)
$ErrorActionPreference='Stop'
if($Frames-lt($Warmup+480)-or $Warmup-lt0-or $Repeats-lt1-or $Repeats-gt4){throw 'Need >=480 measured samples, warmup>=0, repeats1..4.'}
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'wildsignal_holland004.exe'
if(-not(Test-Path -LiteralPath $exe)){throw "Missing $exe"}
if(-not $SimpleRoot){$SimpleRoot=Join-Path $root 'fixtures/simple'}
$dest=Join-Path $root ('runs/pacing-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $dest | Out-Null
$system=[ordered]@{utc=(Get-Date).ToUniversalTime().ToString('o');scope='API frame pacing; no scanout or thermals measurement';requested_timer_resolution=$TimerResolution;fine_tail_us=$FineTailUs;wait_backend=$WaitBackend}
try{
 $system.os=@(Get-CimInstance Win32_OperatingSystem|Select-Object Caption,Version,BuildNumber,TotalVisibleMemorySize)
 $system.cpu=@(Get-CimInstance Win32_Processor|Select-Object Name,NumberOfCores,NumberOfLogicalProcessors)
 $system.gpu=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM)
 $system.active_power_scheme=(& powercfg.exe /getactivescheme | Out-String).Trim()
}catch{$system.query_error=$_.Exception.Message}
$system|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $dest 'SYSTEM.json') -Encoding UTF8
$results=[Collections.Generic.List[object]]::new()
function Save-Results{
 $columns=@($results | ForEach-Object { $_.PSObject.Properties.Name } | Select-Object -Unique)
 $results|Select-Object -Property $columns|Export-Csv -LiteralPath (Join-Path $dest 'RESULTS.csv') -NoTypeInformation -Encoding UTF8
 ConvertTo-Json -InputObject @($results.ToArray()) -Depth 8|Set-Content -LiteralPath (Join-Path $dest 'RESULTS.json') -Encoding UTF8
}
$fixtures=@(
 @{Name='garden';Pack=(Join-Path $root 'assets/pack.json');Scene=(Join-Path $root 'data/garden.json');Origin='unchanged seven Wordline model garden'},
 @{Name='simple';Pack=(Join-Path $SimpleRoot 'assets/pack.json');Scene=(Join-Path $SimpleRoot 'data/simple_shapes.json');Origin='supplied five primitives; compatibility aliases, NOT Wordline art'}
)
$fixtureRecords=[Collections.Generic.List[object]]::new()
foreach($f in $fixtures){
 $f.Available=(Test-Path -LiteralPath $f.Pack)-and(Test-Path -LiteralPath $f.Scene)
 if(-not $f.Available){
  Write-Warning "SKIPPED_MISSING_FIXTURE: $($f.Name); no substitution."
  $results.Add([pscustomobject]@{case=$f.Name;status='SKIPPED_MISSING_FIXTURE';pack=$f.Pack;scene=$f.Scene});Save-Results;continue
 }
 $scene=Get-Content -LiteralPath $f.Scene -Raw|ConvertFrom-Json
 if(($f.Name-eq'garden'-and $scene.instances.Count-ne19)-or($f.Name-eq'simple'-and $scene.instances.Count-ne5)){throw "Unexpected $($f.Name) fixture; refusing silent replacement."}
 $pack=Get-Content -LiteralPath $f.Pack -Raw|ConvertFrom-Json
 $packRoot=Split-Path -Parent $f.Pack
 $hashes=@(Get-FileHash -LiteralPath $f.Pack,$f.Scene -Algorithm SHA256)
 foreach($model in $pack.models){
  $path=Join-Path $packRoot $model.path
  if(-not(Test-Path -LiteralPath $path)){throw "Missing model in $($f.Name): $path"}
  $hashes+=Get-FileHash -LiteralPath $path -Algorithm SHA256
 }
 if($f.Name-eq'simple'){
  $manifest=Join-Path $SimpleRoot 'MANIFEST.json'
  if(-not(Test-Path -LiteralPath $manifest)){throw 'Missing original simple-fixture MANIFEST.json'}
  $m=Get-Content -LiteralPath $manifest -Raw|ConvertFrom-Json
  foreach($entry in $m.assets.PSObject.Properties){
   $model=Join-Path $packRoot $entry.Name
   if((Get-FileHash -LiteralPath $model -Algorithm SHA256).Hash-ne$entry.Value.sha256){throw "Supplied primitive hash mismatch: $model"}
  }
 }
 $fixtureRecords.Add([pscustomobject]@{fixture=$f.Name;origin=$f.Origin;instances=$scene.instances.Count;hashes=$hashes})
}
$fixtureRecords|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $dest 'FIXTURES.json') -Encoding UTF8
function Invoke-Run($fixture,[int]$width,[int]$cap,[string]$pacer,[bool]$vsync,[int]$repeat){
 $name="$($fixture.Name)-${width}-cap${cap}-${pacer}-vsync${vsync}-r${repeat}"
 $out=Join-Path $dest $name
 $cmd=@('--pack',$fixture.Pack,'--scene',$fixture.Scene,'--fixture-id',$fixture.Name,
  '--look',(Join-Path $root 'data/TOON_DIAGNOSTIC.json'),'--style','C','--no-art-materials','--no-smooth-lighting','--mode','PIXEL3D',
  '--study','B1','--pixel-camera','off','--edge-mode','combined','--shadow-proof','stable','--shadow','cached','--shadow-size','512','--shafts','off',
  '--route','strafe','--present-scale','2','--resolution',[string]$width,'--cap',[string]$cap,'--pacing',$pacer,
  '--timer-resolution',$TimerResolution,'--wait-backend',$WaitBackend,'--fine-tail-us',[string]$FineTailUs,
  '--frames',[string]$Frames,'--warmup',[string]$Warmup,'--output',$out)
 if($vsync){$cmd+='--vsync'}
 $quoted=@($cmd|ForEach-Object{'"'+$_+'"'})-join' '
 Write-Host "Running $name; visible actual GL, no recording."
 $proc=Start-Process -FilePath $exe -ArgumentList $quoted -WorkingDirectory $root -NoNewWindow -Wait -PassThru -RedirectStandardOutput (Join-Path $dest ($name+'-stdout.txt')) -RedirectStandardError (Join-Path $dest ($name+'-stderr.txt'))
 if($proc.ExitCode-ne0){throw "Run failed: $name ($($proc.ExitCode)). Inspect stderr. No software fallback allowed."}
 $s=Get-Content -LiteralPath (Join-Path $out 'summary.json') -Raw|ConvertFrom-Json
 $p=Get-Content -LiteralPath (Join-Path $out 'pacing.json') -Raw|ConvertFrom-Json
 $rows=@(Import-Csv -LiteralPath (Join-Path $out 'frames.csv')|Where-Object{[int]$_.frame-ge$Warmup})
 $target=if($width-eq480){33.34}else{16.67}
 $observed=$s.hardware_gpu_tested-and($s.vendor-match'Intel')-and($s.renderer-match'UHD')-and($s.os_build-match'^Windows')
 $valid=$observed-and($rows.Count-ge480)-and(-not $s.capture_enabled)-and(-not $p.mixed_controls)-and($s.fixture_id-eq$fixture.Name)
 $gate='NOT_APPLICABLE_UNCAPPED'
 if($cap-gt0){
  if(-not $valid){$gate='NOT_VERIFIED_INTEL_UHD'}
  elseif($s.loop_p95_ms-lt$target-and$s.swap_completion_interval_p95_ms-lt$target){$gate='PASS_API_PACING_NOT_SCANOUT'}
  else{$gate='FAIL_API_FRAME_PACING'}
 }
 $r=[pscustomobject][ordered]@{
  case=$name;status='EXECUTED';fixture=$fixture.Name;internal_width=$width;cap=$cap;pacing=$pacer;vsync_requested=$vsync;repeat=$repeat
  vendor=$s.vendor;renderer=$s.renderer;GL_driver_version=$s.gl_version;OS_build=$s.os_build;hardware_Intel_UHD_observed=[bool]$observed
  drawable_width=$s.drawable_width;drawable_height=$s.drawable_height;desktop_width=$s.desktop_width;desktop_height=$s.desktop_height
  timed_samples=$rows.Count;gate=$gate;strict_target_ms=$target;loop_violation_ms=[Math]::Max(0,$s.loop_p95_ms-$target);API_interval_violation_ms=[Math]::Max(0,$s.swap_completion_interval_p95_ms-$target)
  loop_p50_ms=$p.loop_p50_ms;loop_p95_ms=$p.loop_p95_ms;loop_p99_ms=$p.loop_p99_ms
  API_interval_p50_ms=$p.API_present_interval_p50_ms;API_interval_p95_ms=$p.API_present_interval_p95_ms;API_interval_p99_ms=$p.API_present_interval_p99_ms
  complete_interval_p50_ms=$p.complete_interval_p50_ms;complete_interval_p95_ms=$p.complete_interval_p95_ms;complete_interval_p99_ms=$p.complete_interval_p99_ms
  CPU_work_p50_ms=$p.CPU_work_wall_p50_ms;CPU_work_p95_ms=$p.CPU_work_wall_p95_ms;CPU_work_p99_ms=$p.CPU_work_wall_p99_ms
  GPU_query_p50_ms=$p.GPU_or_software_query_p50_ms;GPU_query_p95_ms=$p.GPU_or_software_query_p95_ms;GPU_query_missing=$s.gpu_query_missing_samples
  wait_error_p50_ms=$p.wait_error_p50_ms;wait_error_p95_ms=$p.wait_error_p95_ms;wait_error_p99_ms=$p.wait_error_p99_ms
  sleep_call_wall_p50_ms=$p.sleep_call_wall_p50_ms;active_wait_wall_p95_ms=$p.active_wait_wall_p95_ms;CPU_core_percent=$p.process_CPU_core_percent;CPU_system_percent=$p.process_CPU_system_percent
  peak_process_working_set_bytes=$s.process_peak_rss_bytes;wait_backend=$p.wait_backend;timer_hint=$p.timer_resolution_hint;resyncs=$p.phase_resyncs_after_warmup;missed_deadlines=$p.missed_deadlines_after_warmup
  draw_calls_mean=($rows|Measure-Object -Property draw_calls -Average).Average;view_triangles_mean=($rows|Measure-Object -Property triangles_submitted -Average).Average
  swap_interval_known=$s.swap_interval_known;swap_interval_actual=$s.swap_interval_actual;scanout_verified=$false;physical_GPU_VRAM='NOT_MEASURED';thermal_power='NOT_MEASURED'
 }
 $results.Add($r);Save-Results
 ConvertTo-Json -InputObject @($cmd) -Depth 3|Set-Content -LiteralPath (Join-Path $out 'COMMAND.json') -Encoding UTF8
}
foreach($fixture in $fixtures){
 if(-not$fixture.Available){continue}
 foreach($width in @(480,640)){
  $cap=if($width-eq480){30}else{60}
  $swaps=@($false);if($IncludeVsync){$swaps+= $true}
  foreach($swap in $swaps){
   for($repeat=0;$repeat-lt$Repeats;$repeat++){
    $order=@('legacy','precise');if($repeat%2){[array]::Reverse($order)}
    foreach($pacer in $order){Invoke-Run $fixture $width $cap $pacer $swap $repeat}
   }
  }
  if(-not$SkipUncapped){Invoke-Run $fixture $width 0 'precise' $false 0}
 }
}
Write-Host "Done: $dest"
Write-Host 'Strict original loop/API P95 gates are unchanged. Displayed FPS, thermals and scanout remain unverified.'
