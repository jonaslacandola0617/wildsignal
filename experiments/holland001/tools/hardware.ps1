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
$exe = Join-Path $root 'wildsignal_holland001.exe'
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
        '--look', (Join-Path $root 'data/sunlight_look.json'), '--mode', 'PIXEL3D', '--style', 'C', '--no-art-materials',
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
        elseif ($s.loop_p95_ms -le $profile.TargetMs -and $s.swap_completion_interval_p95_ms -le $profile.TargetMs) {
            $gate = 'PASS_API_FRAME_PACING_NOT_SCANOUT_FPS'
        } else { $gate = 'FAIL_API_FRAME_PACING' }
    }
    $r = [pscustomobject][ordered]@{
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
    @{ Name='low480'; Width=480; Scene='garden_sparse.json'; TargetMs=33.3; Cap=30 },
    @{ Name='balanced640'; Width=640; Scene='garden.json'; TargetMs=16.7; Cap=60 }
)
$controls = @(@('B0','off'), @('B1','off'), @('B2','off'), @('B2','raw'), @('B2','comp'))
foreach ($profile in $profiles) {
    for ($repeat=0; $repeat -lt 2; $repeat++) {
        $order = @(0,1,2,3,4)
        if ($repeat -eq 1) { [array]::Reverse($order) }
        foreach ($i in $order) {
            Invoke-Case ($profile.Name + "-paced-$repeat-$i") $profile $controls[$i][0] $controls[$i][1] $profile.Cap @()
        }
    }
    if (-not $SkipEffects) {
        $effects = @(
            @{Name='cached'; Flags=@()}, @{Name='shadow-off'; Flags=@('--shadow','off')},
            @{Name='shadow-live'; Flags=@('--shadow','live')}, @{Name='outline-off'; Flags=@('--no-outline')},
            @{Name='vegetation-off'; Flags=@('--no-vegetation')},
            @{Name='shafts-screen'; Flags=@('--shafts','screen')}, @{Name='shafts-volume'; Flags=@('--shafts','volume')}
        )
        foreach ($effect in $effects) {
            Invoke-Case ($profile.Name + '-uncapped-cost-' + $effect.Name) $profile 'B1' 'off' 0 $effect.Flags
        }
    }
}
Write-Host "Finished. Results: $output"
Write-Host 'These are actual application/swap-return timings, not verified scanout/displayed FPS. 640/60 is a stretch target; do not hide exact P95 violations.'
Write-Host 'Low and balanced profiles have explicitly different vegetation density. Within a profile all shader/camera controls share identical scene, lighting and route.'
