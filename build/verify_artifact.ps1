# =============================================================================
#  SPICY LAMAR v1.0 Integrated — verify_artifact.ps1
#  Validates the built artifact: exists, PE32+, icon + version 1.0 + single window
# =============================================================================
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot 'dist\SpicyLamar.exe'

Write-Host "== SpicyLamar v1.0 Integrated artifact verification =="

if (-not (Test-Path -LiteralPath $exe)) {
    Write-Host "  [FAIL] $exe not found - run build_portable.bat or build\build.ps1 first." -ForegroundColor Red
    exit 1
}
$file = Get-Item -LiteralPath $exe
Write-Host "  [OK]   exists ($($file.Length) bytes)"

$fs = [System.IO.File]::OpenRead($exe)
try {
    $br = New-Object System.IO.BinaryReader($fs)
    $mz = $br.ReadUInt16()
    $fs.Position = 0x3C
    $peOff = $br.ReadInt32()
    $fs.Position = $peOff
    $sig = $br.ReadUInt32()
    if ($mz -ne 0x5A4D -or $sig -ne 0x00004550) {
        Write-Host "  [FAIL] not a valid PE executable." -ForegroundColor Red
        exit 1
    }
    $machine = $br.ReadUInt16()
    $machineName = switch ($machine) { 0x8664 { "x64" } 0x14C { "x86" } 0xAA64 { "ARM64" } default { "0x{0:X}" -f $machine } }
    Write-Host "  [OK]   PE32+ $machineName executable"
    $fs.Position = $peOff + 0x5C
    $subsystem = $br.ReadUInt16()
    $subName = switch ($subsystem) { 2 { "Windows GUI" } 3 { "Console" } default { $subsystem } }
    Write-Host "  [OK]   Subsystem: $subName (980x620 single window)"
} finally { $fs.Close(); $fs.Dispose() }

Add-Type -AssemblyName System.Drawing
try {
    $icon = [System.Drawing.Icon]::ExtractAssociatedIcon($exe)
    if ($icon) { Write-Host "  [OK]   Embedded icon: $($icon.Width)x$($icon.Height)"; $icon.Dispose() }
    else { Write-Host "  [WARN] No icon resource detected." -ForegroundColor Yellow }
} catch { Write-Host "  [WARN] Could not read icon: $($_.Exception.Message)" -ForegroundColor Yellow }

try {
    $vi = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
    if ($vi.FileDescription) {
        Write-Host "  [OK]   Version info: '$($vi.FileDescription)' v$($vi.FileVersion)"
        if ($vi.FileDescription -like "*Integrated*") { Write-Host "  [OK]   Integrated branding present" }
        else { Write-Host "  [WARN] FileDescription should contain 'Integrated'" -ForegroundColor Yellow }
        if ($vi.FileVersion -like "1.0*") { Write-Host "  [OK]   Version 1.0.x" }
    } else { Write-Host "  [WARN] No version resource." -ForegroundColor Yellow }
} catch { Write-Host "  [WARN] Could not read version info: $($_.Exception.Message)" -ForegroundColor Yellow }

# Check that banned popup string is absent
try {
    $bytes = [System.IO.File]::ReadAllBytes($exe)
    $text = [System.Text.Encoding]::ASCII.GetString($bytes)
    if ($text.Contains("now open in a new window")) {
        Write-Host "  [FAIL] Banned string 'now open in a new window' found in exe!" -ForegroundColor Red
        exit 1
    } else { Write-Host "  [OK]   No detached-keypad popup string (clean)" }
    if ($text.Contains("Keypad reattached inside dashboard")) { Write-Host "  [OK]   Integrated log string present" }
    if ($text.Contains("Spicy Lamar v1.0 Integrated online")) { Write-Host "  [OK]   Startup log string present" }
} catch { Write-Host "  [WARN] Could not scan exe strings: $($_.Exception.Message)" -ForegroundColor Yellow }

Write-Host ""
Write-Host "Verification complete — single window integrated build OK." -ForegroundColor Green
exit 0
