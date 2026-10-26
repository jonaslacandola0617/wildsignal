# CI orchestration test. MOCK DATA ONLY: no GPU, assets, FPS or hardware claim.
$ErrorActionPreference = 'Stop'
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('h4-launcher-mock-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $testRoot 'tools'),(Join-Path $testRoot 'assets'),(Join-Path $testRoot 'data'),(Join-Path $testRoot 'fixtures/simple/assets'),(Join-Path $testRoot 'fixtures/simple/data') | Out-Null
Copy-Item (Join-Path $PSScriptRoot '../tools/hardware.ps1') (Join-Path $testRoot 'tools/hardware.ps1')
[IO.File]::WriteAllText((Join-Path $testRoot 'wildsignal_holland004.exe'),'MOCK_ONLY_NOT_AN_EXECUTABLE')
$mockCalls = [Collections.Generic.List[object]]::new()
function Start-Process {
 param($FilePath,$ArgumentList,$WorkingDirectory,[switch]$NoNewWindow,[switch]$Wait,[switch]$PassThru,$RedirectStandardOutput,$RedirectStandardError)
 $argsList = @([regex]::Matches($ArgumentList,'"([^"]*)"') | ForEach-Object { $_.Groups[1].Value })
 $options = @{}
 for ($i=0; $i -lt $argsList.Count; $i++) {
  $key=$argsList[$i]
  if ($i+1 -lt $argsList.Count -and -not $argsList[$i+1].StartsWith('--')) { $options[$key]=$argsList[++$i] } else { $options[$key]=$true }
 }
 $mockCalls.Add($options)
 $out = $options['--output']; New-Item -ItemType Directory -Path $out | Out-Null
 $width = [int]$options['--resolution']; $period = if ($width -eq 480) { 33.333 } else { 16.666 }
 $measured=if ($options['--pacing'] -eq 'legacy') {$period+1} else {$period}
 $s = @{source='MOCK_NOT_HARDWARE';hardware_gpu_tested=$true;vendor='Intel';renderer='MOCK UHD';os_build='Windows MOCK';capture_enabled=$false;fixture_id=$options['--fixture-id'];loop_p95_ms=$measured;swap_completion_interval_p95_ms=$measured}
 $s | ConvertTo-Json | Set-Content (Join-Path $out 'summary.json')
 @{mixed_controls=$false;loop_p50_ms=$period;loop_p95_ms=$period;API_present_interval_p95_ms=$period} | ConvertTo-Json | Set-Content (Join-Path $out 'pacing.json')
 @(0..([int]$options['--frames']-1) | ForEach-Object { [pscustomobject]@{frame=$_;draw_calls=5;triangles_submitted=10} }) | Export-Csv (Join-Path $out 'frames.csv') -NoTypeInformation
 return [pscustomobject]@{ExitCode=0}
}
try {
 foreach ($fixture in @('garden','simple')) {
  $base = if ($fixture -eq 'garden') { $testRoot } else { Join-Path $testRoot 'fixtures/simple' }
  $models=@(); $hashes=@{}
  foreach ($name in @('BLOCK','SPHERE','PLANE')) {
   $path=Join-Path $base "assets/$name.glb"
   [IO.File]::WriteAllText($path,"CI_MOCK_HASH_TEST_ONLY_$name")
   $models+=@{id=$name;path="$name.glb"}; $hashes["$name.glb"]=@{sha256=(Get-FileHash $path -Algorithm SHA256).Hash}
  }
  @{models=$models} | ConvertTo-Json | Set-Content (Join-Path $base 'assets/pack.json')
  @{assets=$hashes} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $base 'MANIFEST.json')
  $count=if ($fixture -eq 'garden') {19} else {5}; $scene=if ($fixture -eq 'garden') {'garden'} else {'simple_shapes'}
  @{instances=@(1..$count | ForEach-Object {@{id="MOCK_$_"}})} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $base "data/$scene.json")
 }
 & (Join-Path $testRoot 'tools/hardware.ps1')
 if ($mockCalls.Count -ne 12) {throw 'Expected eight paced and four uncapped controls'}
 $output=Get-ChildItem (Join-Path $testRoot 'runs') -Directory | Select-Object -First 1
 $rows=@(Import-Csv (Join-Path $output.FullName 'RESULTS.csv'))
 if ($rows.Count -ne 12 -or @($rows | Where-Object {$_.timed_samples -ne '480'}).Count) {throw 'Measured sample filtering failed'}
 if (@($rows | Where-Object {$_.gate -eq 'PASS_API_PACING_NOT_SCANOUT'}).Count -ne 4 -or @($rows | Where-Object {$_.gate -eq 'FAIL_API_FRAME_PACING'}).Count -ne 4) {throw 'Original strict-gate orchestration failed'}
 Remove-Item (Join-Path $testRoot 'fixtures/simple') -Recurse -Force
 & (Join-Path $testRoot 'tools/hardware.ps1')
 $latest=Get-ChildItem (Join-Path $testRoot 'runs') -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1
 $missing=@(Import-Csv (Join-Path $latest.FullName 'RESULTS.csv'))
 if (@($missing | Where-Object {$_.status -eq 'SKIPPED_MISSING_FIXTURE'}).Count -ne 1 -or @($missing | Where-Object {$_.status -eq 'EXECUTED'}).Count -ne 6) {throw 'Missing fixture was not explicitly skipped'}
 if (-not $missing[1].PSObject.Properties['gate']) {throw 'Mixed CSV schema lost benchmark fields'}
 Write-Host 'PASS MOCK ONLY: command pairs, fixture hashes, 480 samples, strict gate labels, missing fixture and CSV union. No GPU executed.'
} finally { Remove-Item -LiteralPath $testRoot -Recurse -Force }
