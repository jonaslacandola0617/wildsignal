param(
    [int]$Frames = 600,
    [int]$Warmup = 120,
    [switch]$Vsync,
    [switch]$SkipEffects
)
$ErrorActionPreference = 'Stop'
if ($Frames -lt ($Warmup + 120) -or $Warmup -lt 0) {
    throw 'Require at least 120 timed frames after warmup.'
}
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root 'wildsignal_holland003.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw "Missing executable: $exe" }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$output = Join-Path $root "runs/hardware-$stamp"
New-Item -ItemType Directory -Path $output | Out-Null
$system = [ordered]@{ timestamp_utc = (Get-Date).ToUniversalTime().ToString('o'); hardware_capture = 'Actual owner machine; no scanout verification' }
try {
    $system.os = @(Get-CimInstance Win32_OperatingSystem | Select-Object Caption, Version, BuildNumber, TotalVisibleMemorySize)
    $system.cpu = @(Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors)
    $system.gpu = @(Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion, AdapterRAM)
} catch { $system.cim_error = $_.Exception.Message }
$system | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'SYSTEM.json') -Encoding UTF8
$results = [Collections.Generic.List[object]]::new()
function Percentile($values, [double]$q) {
    $sorted = @($values | Sort-Object)
    if ($sorted.Count -eq 0) { return -1 }
    return [double]$sorted[[int][Math]::Floor(($sorted.Count - 1) * $q)]
}
function Invoke-Case([string]$name, $profile, [string]$study, [string]$camera, [int]$cap, [string[]]$extra) {
    $dest = Join-Path $output $name
    $command = @('--pack', (Join-Path $root 'assets/pack.json'), '--scene', (Join-Path $root ('data/' + $profile.Scene)),
        '--look', (Join-Path $root 'data/TOON_DIAGNOSTIC.json'), '--mode', 'PIXEL3D', '--style', 'C', '--no-art-materials', '--no-smooth-lighting', '--present-scale', '2',
        '--study', $study, '--pixel-camera', $camera, '--resolution', [string]$profile.Width,
        '--shadow', 'cached', '--shadow-size', '512', '--shafts', 'off', '--route', 'strafe',
        '--frames', [string]$Frames, '--warmup', [string]$Warmup, '--cap', [string]$cap, '--output', $dest)
    if ($Vsync) { $command += '--vsync' }
    $command += $extra
    # Start-Process avoids PowerShell 5's native-stderr-as-error behavior.
    # Windows file names cannot contain quotes; quote every argument for spaces.
    $quoted = @($command | ForEach-Object { '"' + $_ + '"' }) -join ' '
    Write-Host "Running $name (visible real GL window)"
    $proc = Start-Process -FilePath $exe -ArgumentList $quoted -WorkingDirectory $root -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput (Join-Path $output ($name + '-stdout.txt')) `
        -RedirectStandardError (Join-Path $output ($name + '-stderr.txt'))
    if ($proc.ExitCode -ne 0) { throw "$name failed ($($proc.ExitCode)); inspect its stderr file. No software fallback is allowed in this hardware suite." }
    $s = Get-Content -LiteralPath (Join-Path $dest 'summary.json') -Raw | ConvertFrom-Json
    $rows = @(Import-Csv -LiteralPath (Join-Path $dest 'frames.csv') | Where-Object { [int]$_.frame -ge $Warmup })
    $intel = $s.hardware_gpu_tested -and ($s.vendor -match 'Intel') -and ($s.renderer -match 'UHD') -and ($s.os_build -match '^Windows')
    $valid = $intel -and ($rows.Count -ge 120) -and (-not $s.capture_enabled)
    $gate = 'NOT_EVALUATED_UNCAPPED_COST_RUN'
    if ($cap -gt 0) {
        if (-not $valid) { $gate = 'INTEL_UHD_NOT_VERIFIED' }
        elseif ($s.loop_p95_ms -lt $profile.TargetMs -and $s.swap_completion_interval_p95_ms -lt $profile.TargetMs) {
            $gate = 'PASS_API_FRAME_PACING_NOT_SCANOUT_FPS'
        } else { $gate = 'FAIL_API_FRAME_PACING' }
    }
    $r = [pscustomobject][ordered]@{
        edge_mode = $s.edge_mode; stable_shadow = $s.stable_shadow; bounded_air = $s.bounded_air
        atmosphere_width = $s.atmosphere_width_actual; atmosphere_height = $s.atmosphere_height_actual
        effect_logical_bytes = $s.atmosphere_logical_allocated_bytes
        shadow_CPU_median_ms = (Percentile @($rows | ForEach-Object { [double]$_.shadow_submit_ms }) .5)
        air_CPU_median_ms = (Percentile @($rows | ForEach-Object { [double]$_.shaft_submit_ms }) .5)
        case = $name; profile = $profile.Name; width = $profile.Width; scene = $profile.Scene
        study = $study; pixel_camera = $camera; cap_fps = $cap; vsync_requested = [bool]$Vsync
        vendor = $s.vendor; renderer = $s.renderer; driver_gl_version = $s.gl_version; os_build = $s.os_build
        intel_uhd_observed = [bool]$intel; software = -not $s.hardware_gpu_tested
        timed_samples = $s.timed_samples; loop_median_ms = $s.loop_median_ms; loop_p95_ms = $s.loop_p95_ms
        swap_return_p95_ms = $s.swap_completion_interval_p95_ms; target_interval_ms = $profile.TargetMs
        p95_interval_violation_ms = [Math]::Max(0, $s.swap_completion_interval_p95_ms - $profile.TargetMs)
        cpu_update_median_ms = (Percentile @($rows | ForEach-Object { [double]$_.update_ms }) .5)
        cpu_submit_median_ms = $s.submit_median_ms; device_query_median_ms = $s.gpu_query_median_ms
        query_valid = $s.gpu_query_valid_samples; query_missing = $s.gpu_query_missing_samples
        draw_calls_mean = ($rows | Measure-Object -Property draw_calls -Average).Average
        triangles_mean = ($rows | Measure-Object -Property triangles_submitted -Average).Average
        texture_base_bytes = $s.texture_base_level_bytes; process_peak_working_set_bytes = $s.process_peak_rss_bytes
        display_scanout_verified = $false; pacing_gate = $gate; extra_flags = ($extra -join ' ')
    }
    $results.Add($r)
    $results | Export-Csv -LiteralPath (Join-Path $output 'RESULTS.csv') -NoTypeInformation -Encoding UTF8
    ConvertTo-Json -InputObject @($results.ToArray()) -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'RESULTS.json') -Encoding UTF8
    $command | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $dest 'COMMAND.json') -Encoding UTF8
}
$profiles = @(
    @{ Name='low480'; Width=480; Scene='garden.json'; TargetMs=33.34; Cap=30 },
    @{ Name='balanced640'; Width=640; Scene='garden.json'; TargetMs=16.67; Cap=60 }
)
$paced = @(
    @{Name='owner-toon'; Flags=@('--shadow','off','--edge-mode','legacy')},
    @{Name='combined512'; Flags=@('--edge-mode','combined','--shadow-proof','stable')},
    @{Name='optional-air8'; Flags=@('--edge-mode','combined','--shadow-proof','stable','--sunlight-profile','low')}
)
$costs = @(
    @{Name='edges-off'; Flags=@('--shadow','off','--edge-mode','off')},
    @{Name='old-B1'; Flags=@('--shadow','off','--edge-mode','legacy')},
    @{Name='highlight-only'; Flags=@('--shadow','off','--edge-mode','only')},
    @{Name='combined-edges'; Flags=@('--shadow','off','--edge-mode','combined')},
    @{Name='stable512'; Flags=@('--edge-mode','combined','--shadow-proof','stable')},
    @{Name='stable1024'; Flags=@('--edge-mode','combined','--shadow-proof','stable','--shadow-size','1024')},
    @{Name='live512'; Flags=@('--edge-mode','combined','--shadow-proof','stable','--shadow','live')},
    @{Name='contacts'; Flags=@('--edge-mode','combined','--shadow-proof','stable','--wordline-contacts')},
    @{Name='air8quarter'; Flags=@('--edge-mode','combined','--shadow-proof','stable','--sunlight-profile','low')},
    @{Name='air12half'; Flags=@('--edge-mode','combined','--shadow-proof','stable','--sunlight-profile','standard')},
    @{Name='haze-only'; Flags=@('--edge-mode','combined','--shadow-proof','stable','--sunlight-profile','haze')}
)
foreach ($profile in $profiles) {
    for ($repeat=0; $repeat -lt 2; $repeat++) {
        $order = @(0,1,2)
        if ($repeat -eq 1) { [array]::Reverse($order) }
        foreach ($i in $order) {
            Invoke-Case ($profile.Name + "-paced-$repeat-" + $paced[$i].Name) $profile 'B1' 'off' $profile.Cap $paced[$i].Flags
        }
    }
    if (-not $SkipEffects) {
        foreach ($effect in $costs) {
            Invoke-Case ($profile.Name + '-uncapped-cost-' + $effect.Name) $profile 'B1' 'off' 0 $effect.Flags
        }
    }
}
Write-Host "Finished. Results: $output"
Write-Host 'Both resolutions use the SAME full seven-model scene. Sparse fallback is a separate visual launcher.'
Write-Host 'API loop and swap-return P95 must BOTH be <33.34ms low / <16.67ms stretch. This is not scanout/displayed FPS.'
