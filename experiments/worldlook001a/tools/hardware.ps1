param(
 [int]$Frames=600,[int]$Warmup=120,[ValidateRange(1,4)][int]$Repeats=1,
 [ValidateSet('all','cost','pacing')][string]$Suite='all',
 [switch]$Vsync,[ValidateSet('native','sdl')][string]$WaitBackend='native',
 [ValidateRange(0,1000)][int]$FineTailUs=500,
 [ValidateSet('default','0','1')][string]$TimerResolution='default'
)
$ErrorActionPreference='Stop'
if ($Warmup -lt 0 -or $Frames -lt ($Warmup+480)) {throw 'Need at least480 measured noncapture frames after warmup'}
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'wildsignal_worldlook001a.exe'
$kit=Join-Path $root 'assets/grass/grass_kit.json'
$pack=Join-Path $root 'assets/pack.json'
$scene=Join-Path $root 'data/garden.json'
foreach ($path in @($exe,$kit,$pack,$scene)) {if (-not(Test-Path -LiteralPath $path)) {throw "Required private review input missing: $path. No substitute or software fallback."}}
$m=Get-Content -LiteralPath $kit -Raw | ConvertFrom-Json
$atlas=Join-Path (Split-Path -Parent $kit) $m.atlas
if ((Get-FileHash -LiteralPath $atlas -Algorithm SHA256).Hash -ne $m.atlas_sha256) {throw 'Original private grass atlas SHA256 mismatch'}
if ((Get-Content -LiteralPath $scene -Raw | ConvertFrom-Json).instances.Count -ne 19) {throw 'Expected unchanged19-instance garden'}
$dest=Join-Path $root ('runs/grass-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $dest | Out-Null
$system=[ordered]@{utc=(Get-Date).ToUniversalTime().ToString('o');scope='Actual API timings; scanout, energy and thermals NOT measured';Suite=$Suite;vsync_requested=[bool]$Vsync}
try {
 $system.os=@(Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber,TotalVisibleMemorySize)
 $system.cpu=@(Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors)
 $system.gpu=@(Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion,AdapterRAM)
 $system.power_scheme=(& powercfg.exe /getactivescheme | Out-String).Trim()
} catch {$system.query_error=$_.Exception.Message}
$system | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $dest 'SYSTEM.json') -Encoding UTF8
$hashes=@(Get-FileHash -LiteralPath $exe,$pack,$scene,$kit,$atlas -Algorithm SHA256)
$p=Get-Content -LiteralPath $pack -Raw | ConvertFrom-Json
foreach ($model in $p.models) {$hashes+=Get-FileHash -LiteralPath (Join-Path (Split-Path -Parent $pack) $model.path) -Algorithm SHA256}
$hashes | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $dest 'INPUT_SHA256.json') -Encoding UTF8
$records=[Collections.Generic.List[object]]::new()
function Quantile($values,[double]$q) {$sorted=@($values | Sort-Object);if (-not $sorted.Count) {return -1};return [double]$sorted[[int][Math]::Floor(($sorted.Count-1)*$q)]}
function Run-Case([int]$width,[string]$density,[int]$cap,[string]$pacer,[int]$repeat) {
 $name="garden-${width}-${density}-cap${cap}-${pacer}-r${repeat}"
 $out=Join-Path $dest $name
 $cmd=@('--pack',$pack,'--scene',$scene,'--fixture-id','garden','--look',(Join-Path $root 'data/TOON_DIAGNOSTIC.json'),
  '--style','C','--mode','PIXEL3D','--no-art-materials','--no-smooth-lighting','--study','B1','--pixel-camera','off',
  '--edge-mode','combined','--shadow-proof','stable','--shadow','cached','--shadow-size','512','--shafts','off',
  '--grass-kit',$kit,'--grass',$density,'--grass-facing','view','--grass-seed','17001','--grass-frustum','on',
  '--route','strafe','--present-scale','2','--resolution',[string]$width,'--cap',[string]$cap,'--pacing',$pacer,
  '--wait-backend',$WaitBackend,'--fine-tail-us',[string]$FineTailUs,'--timer-resolution',$TimerResolution,
  '--frames',[string]$Frames,'--warmup',[string]$Warmup,'--output',$out)
 if ($Vsync) {$cmd+='--vsync'}
 $quoted=@($cmd | ForEach-Object {'"'+$_+'"'}) -join ' '
 Write-Host "Running $($name): actual visible GL, no capture"
 $proc=Start-Process -FilePath $exe -ArgumentList $quoted -WorkingDirectory $root -NoNewWindow -Wait -PassThru -RedirectStandardOutput (Join-Path $dest ($name+'-stdout.txt')) -RedirectStandardError (Join-Path $dest ($name+'-stderr.txt'))
 if ($proc.ExitCode -ne 0) {throw "Failed $name; inspect stderr. No substitution/fallback."}
 $s=Get-Content (Join-Path $out 'summary.json') -Raw | ConvertFrom-Json
 $g=Get-Content (Join-Path $out 'grass.json') -Raw | ConvertFrom-Json
 $t=Get-Content (Join-Path $out 'pacing.json') -Raw | ConvertFrom-Json
 $rows=@(Import-Csv (Join-Path $out 'frames.csv') | Where-Object {[int]$_.frame -ge $Warmup})
 $gr=@(Import-Csv (Join-Path $out 'grass_frames.csv') | Where-Object {[int]$_.frame -ge $Warmup})
 if ($s.capture_enabled -or $g.shafts_enabled -or $rows.Count -lt 480 -or $t.mixed_controls) {throw 'Invalid noncapture comparison conditions'}
 $intel=$s.hardware_gpu_tested -and ($s.vendor -match 'Intel') -and ($s.renderer -match 'UHD') -and ($s.os_build -match '^Windows')
 $target=if ($width -eq 480) {33.34} else {16.67}
 $gate='UNCAPPED_COST_NOT_DISPLAY_FPS'
 if ($cap) {
  if (-not $intel) {$gate='NOT_VERIFIED_OWNER_INTEL_UHD'}
  elseif ($s.loop_p95_ms -lt $target -and $s.swap_completion_interval_p95_ms -lt $target) {$gate='PASS_ORIGINAL_API_PACING_NOT_SCANOUT'}
  else {$gate='FAIL_ORIGINAL_API_FRAME_PACING'}
 }
 $visible=($gr | Measure-Object visible -Average).Average
 $draws=($gr | Measure-Object grass_draw_calls -Average).Average
 $record=[pscustomobject][ordered]@{
  run=$name;source=$s.source;density=$density;count=$g.count;width=$width;cap=$cap;pacing=$pacer;vsync_requested=[bool]$Vsync;timed_samples=$rows.Count
  vendor=$s.vendor;renderer=$s.renderer;GL_driver=$s.gl_version;OS=$s.os_build;Intel_UHD=[bool]$intel;gate=$gate;target_ms=$target;scanout_verified=$false
  drawable_width=$s.drawable_width;drawable_height=$s.drawable_height;loop_P50_ms=$t.loop_p50_ms;loop_P95_ms=$t.loop_p95_ms;loop_P99_ms=$t.loop_p99_ms
  API_interval_P50_ms=$t.API_present_interval_p50_ms;API_interval_P95_ms=$t.API_present_interval_p95_ms;API_interval_P99_ms=$t.API_present_interval_p99_ms
  CPU_work_P50_ms=$t.CPU_work_wall_p50_ms;CPU_work_P95_ms=$t.CPU_work_wall_p95_ms;CPU_work_P99_ms=$t.CPU_work_wall_p99_ms
  GPU_query_P50_ms=$s.gpu_query_median_ms;GPU_query_P95_ms=$s.gpu_query_p95_ms;GPU_query_valid=$s.gpu_query_valid_samples;GPU_query_missing=$s.gpu_query_missing_samples
  CPU_grass_P50_ms=(Quantile @($gr | ForEach-Object {[double]$_.CPU_grass_ms}) .5);CPU_grass_P95_ms=(Quantile @($gr | ForEach-Object {[double]$_.CPU_grass_ms}) .95)
  visible_clumps_mean=$visible;near_mean=($gr | Measure-Object near -Average).Average;mid_mean=($gr | Measure-Object mid -Average).Average
  frustum_culled_mean=($gr | Measure-Object frustum_culled -Average).Average;distance_culled_mean=($gr | Measure-Object distance_culled -Average).Average
  grass_draw_calls_mean=$draws;draws_saved_vs_one_per_clump=($visible-$draws);grass_triangles_mean=($gr | Measure-Object grass_triangles -Average).Average
  total_draw_calls_mean=($rows | Measure-Object draw_calls -Average).Average;total_scene_triangles_mean=($rows | Measure-Object total_scene_triangles_submitted -Average).Average
  atlas_bytes=$g.atlas_bytes;grass_buffer_bytes=$g.buffer_bytes;original_texture_bytes=$s.texture_base_level_bytes;peak_process_working_set_bytes=$s.process_peak_rss_bytes
  process_CPU_core_percent=$t.process_CPU_core_percent;process_CPU_system_percent=$t.process_CPU_system_percent;active_tail_P95_ms=$t.active_wait_wall_p95_ms
  wait_backend=$t.wait_backend;timer_hint=$t.timer_resolution_hint;energy_thermals='NOT_MEASURED';physical_VRAM='NOT_MEASURED';grass_sha256=$g.atlas_sha256_declared
 }
 $records.Add($record)
 $records | Export-Csv (Join-Path $dest 'RESULTS.csv') -NoTypeInformation -Encoding UTF8
 ConvertTo-Json -InputObject @($records.ToArray()) -Depth 6 | Set-Content (Join-Path $dest 'RESULTS.json') -Encoding UTF8
 ConvertTo-Json -InputObject @($cmd) | Set-Content (Join-Path $out 'COMMAND.json') -Encoding UTF8
}
foreach ($width in @(480,640)) {
 for ($repeat=0;$repeat -lt $Repeats;$repeat++) {
  $order=@('off','sparse','medium','dense');if ($repeat%2) {[array]::Reverse($order)}
  if ($Suite -ne 'pacing') {foreach ($density in $order) {Run-Case $width $density 0 'legacy' $repeat}}
  if ($Suite -ne 'cost') {
   $cap=if ($width -eq 480) {30} else {60}
   foreach ($density in $order) {foreach ($pacer in @('legacy','precise')) {Run-Case $width $density $cap $pacer $repeat}}
  }
 }
}
Write-Host "Results: $dest"
Write-Host 'Separate uncapped grass GPU cost from legacy/precise API pacing. Original strict gate unchanged; displayed FPS and energy are not measured.'
