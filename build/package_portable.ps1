# =============================================================================
#  SPICY LAMAR v1.0 Integrated — package_portable.ps1
#  Packages the built single-file portable executable into a zip:
#      dist\SpicyLamar.exe
#      dist\README.txt  -> packaged as README.txt
#  Output: dist\SpicyLamar-Portable.zip
#  Single window 980x620 | No detached keypad window
# =============================================================================
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$exe      = Join-Path $repoRoot 'dist\SpicyLamar.exe'
$readme   = Join-Path $repoRoot 'dist\README.txt'
$zipPath  = Join-Path $repoRoot 'dist\SpicyLamar-Portable.zip'

if (-not (Test-Path -LiteralPath $exe)) { throw "Build output missing: $exe" }
if (-not (Test-Path -LiteralPath $readme)) { throw "README missing: $readme" }

$staging = Join-Path $env:TEMP 'SpicyLamarPortableStaging'
if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
New-Item -ItemType Directory -Path $staging -Force | Out-Null
Copy-Item -LiteralPath $exe -Destination $staging
Copy-Item -LiteralPath $readme -Destination $staging
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zipPath -CompressionLevel Optimal
Write-Host "Packaged portable build (v1.0 Integrated, single window):"
Write-Host ("  Exe: {0:N0} bytes" -f (Get-Item -LiteralPath $exe).Length)
Write-Host ("  Zip: {0:N0} bytes" -f (Get-Item -LiteralPath $zipPath).Length)
Write-Host "  Ready artifact: $zipPath"
Write-Host "  Keypad is docked inside — no 'now open in a new window' popup"
