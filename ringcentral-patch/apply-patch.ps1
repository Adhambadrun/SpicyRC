#Requires -Version 5.1
param(
  [string]$RingCentralZip = ".\RingCentral.zip",
  [string]$AddButton = "🌶 SPICY",
  [string]$OutputDir = ".\RingCentral-patched"
)
$ErrorActionPreference = 'Stop'
Write-Host "=== RingCentral — SpicyLamar button patch ===" -ForegroundColor Cyan

if (-not (Test-Path $RingCentralZip)) {
  Write-Host "[FAIL] RingCentral.zip not found at $RingCentralZip" -ForegroundColor Red
  Write-Host "       Put your 456MB zip next to this script, or pass -RingCentralZip <path>"
  Write-Host "       If you only have resources/app.asar, pass -RingCentralZip to that file's parent folder and edit $OutputDir/app-src manually."
  exit 1
}

# 1. Expand
if (Test-Path $OutputDir) { Remove-Item $OutputDir -Recurse -Force }
Expand-Archive -Path $RingCentralZip -DestinationPath $OutputDir -Force
Write-Host "[OK] Expanded to $OutputDir"

# Try to locate app.asar
$asar = Get-ChildItem -Path $OutputDir -Recurse -Filter "app.asar" | Select-Object -First 1
if (-not $asar) {
  Write-Host "[WARN] app.asar not found — your zip may be an installer, not unpacked app." -ForegroundColor Yellow
  Write-Host "       Look for RingCentral.exe and resources/app.asar inside $OutputDir"
  Write-Host "       If it's an installer (Squirrel/NSIS), install RingCentral once, then patch %LocalAppData%\RingCentral\app-*\resources\app.asar"
  exit 1
}
Write-Host "[OK] Found $($asar.FullName)"

# Ensure asar tool
try { asar --version | Out-Null } catch {
  Write-Host "[INFO] Installing asar globally via npm..."
  npm i -g asar
  if ($LASTEXITCODE -ne 0) { throw "npm i -g asar failed. Install Node.js first: https://nodejs.org" }
}

$appSrc = Join-Path $OutputDir "app-src"
Write-Host "[1/4] Extracting app.asar to $appSrc ..."
asar extract $asar.FullName $appSrc
if ($LASTEXITCODE -ne 0) { throw "asar extract failed" }

# 2. Patch
Write-Host "[2/4] Copying Spicy injection files..."
$patchSrc = $PSScriptRoot
Copy-Item "$patchSrc\inject-spicy-button.js" "$appSrc\" -Force
Copy-Item "$patchSrc\styles\spicy-button.css" "$appSrc\styles\" -Force -ErrorAction SilentlyContinue
if (-not (Test-Path "$appSrc\styles")) { New-Item -ItemType Directory -Path "$appSrc\styles" | Out-Null; Copy-Item "$patchSrc\styles\spicy-button.css" "$appSrc\styles\" -Force }

# Apply diff if git is available
try {
  Push-Location $appSrc
  git apply --whitespace=nowarn "$patchSrc\patch.diff" 2>$null
  if ($LASTEXITCODE -eq 0) { Write-Host "[OK] patch.diff applied" -ForegroundColor Green }
  else { Write-Host "[INFO] patch.diff not applied automatically — will rely on runtime injection (inject-spicy-button.js self-injects). Manual apply: git apply $patchSrc\patch.diff" -ForegroundColor Yellow }
  Pop-Location
} catch { Pop-Location; Write-Host "[INFO] Skipping git apply, using runtime injection" -ForegroundColor Yellow }

# Ensure renderer entry loads our script (inject if patch didn't)
$indexHtml = Get-ChildItem -Path $appSrc -Recurse -Filter "index.html" | Select-Object -First 1
if ($indexHtml) {
  $html = Get-Content $indexHtml.FullName -Raw
  if ($html -notlike "*inject-spicy-button*") {
    $html = $html -replace '</head>', "  <link rel=`"stylesheet`" href=`"./styles/spicy-button.css`"/>`n</head>"
    $html = $html -replace '<script src="./renderer.js"', '<script src="./inject-spicy-button.js"></script>`n  <script src="./renderer.js"'
    Set-Content -Path $indexHtml.FullName -Value $html -NoNewline
    Write-Host "[OK] Patched index.html to load injection"
  }
}

# 3. Repack
Write-Host "[3/4] Repacking app.asar ..."
asar pack $appSrc $asar.FullName
if ($LASTEXITCODE -ne 0) { throw "asar pack failed" }
Write-Host "[OK] Repacked $($asar.FullName)" -ForegroundColor Green

# 4. Summary
Write-Host ""
Write-Host "=== DONE ===" -ForegroundColor Green
Write-Host "Patched app at: $OutputDir"
Write-Host "Button '$AddButton' will appear:"
Write-Host "  - In dropdown after 'Phone settings' (image-1.png)"
Write-Host "  - Between 'Noteson' and green CALL button (image-2.png)"
Write-Host "Next: Run $OutputDir\RingCentral.exe (or the .exe inside) — you should see 🌶 SPICY."
Write-Host "If you use SpicyLamar-Integrated (our 980x620 single-window), no patch needed — the keypad is already docked right inside SpicyLamar.exe."
Write-Host "To revert: restore original app.asar from backup (app.asar.bak created by asar)."
if (Test-Path "$($asar.FullName).bak") { Write-Host "Backup exists: $($asar.FullName).bak" }
