#Requires -Version 5.1
<#
  apply-patch.ps1 — Put the Spicy Lamar feature INSIDE RingCentral and launch it.

  No longer builds a separate SpicyLamar.exe. It patches RingCentral's own
  resources/app.asar so that:

    1. The dialer gets a "🌶 SPICY" button + a "Spicy Lamar — Auto-Answer" entry
       in the ⚙ menu (injected by ringcentral-patch/spicy-engine/spicy-renderer.js).
    2. Incoming calls are auto-answered inside RC by clicking RC's own Answer
       control (no popup, no external helper window).
    3. Pin-on-top + toggle state handled by spicy-main.js / spicy-preload.js.

  Then it REPACKS app.asar and LAUNCHES the patched RingCentral so something
  actually opens.

  USAGE
    # Option A: explicit source (zip / unpacked app folder / app.asar)
    .\ringcentral-patch\apply-patch.ps1 -Source C:\path\to\RingCentral.zip

    # Option B: auto-detect the app you already run (%LocalAppData%\RingCentral)
    .\ringcentral-patch\apply-patch.ps1

  EXIT CODES
    0 = patched + launched         1 = hard failure
    2 = no RingCentral found (nothing to patch) — caller may show a guide.
#>
param(
  [string]$Source = "",                 # zip | unpacked folder | app.asar
  [switch]$SkipLaunch,
  [switch]$NoInstallAsar,               # don't auto npm i -g asar
  [switch]$KeepWorkingDir               # don't delete app-src after repack
)

$ErrorActionPreference = 'Stop'
$scriptDir = $PSScriptRoot
$rootDir   = Split-Path -Parent $scriptDir
$engineDir = Join-Path $scriptDir 'spicy-engine'
$styleCss  = Join-Path $scriptDir 'styles\spicy-button.css'
$extractDir= Join-Path $scriptDir 'RingCentral-patched'

function Log($m)  { Write-Host "[SpicyLamar] $m" -ForegroundColor Cyan }
function Ok($m)   { Write-Host "[OK]  $m" -ForegroundColor Green }
function Warn($m) { Write-Host "[WARN] $m" -ForegroundColor Yellow }
function Err($m)  { Write-Host "[ERR]  $m" -ForegroundColor Red }
function Find-Asar($dir) { Get-ChildItem -Path $dir -Filter 'app.asar' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 }

Log '=== RingCentral - Spicy Lamar IN-APP patch & launch ==='

# ---------------------------------------------------------------- 1. LOCATE SOURCE
# Resolution ends with: $asarPath (app.asar to patch) and $kind = 'folder'|'bare'|'installed'
$asarPath = $null
$zipPath  = $null
$kind     = $null

# 1a. explicit -Source
if ($Source) {
  $res = Resolve-Path $Source -ErrorAction SilentlyContinue
  if (-not $res) { Err "Source not found: $Source"; exit 1 }
  $s = (Get-Item $res.Path).FullName
  if ((Get-Item $s).PSIsContainer) {
    $a = Find-Asar $s
    if (-not $a) { Err "No app.asar found inside $s"; exit 1 }
    $asarPath = $a.FullName; $kind = 'folder'
  } elseif ($s -like '*app.asar') { $asarPath = $s; $kind = 'bare' }
  elseif ($s -like '*.zip') { $zipPath = $s }
  else { Err "Unsupported source type: $s"; exit 1 }
}

# 1b. auto-discover a RingCentral.zip next to the script / repo root
if (-not $asarPath -and -not $zipPath -and -not $Source) {
  foreach ($cand in @((Join-Path $scriptDir 'RingCentral.zip'), (Join-Path $rootDir 'RingCentral.zip'))) {
    if (Test-Path $cand) { $zipPath = $cand; break }
  }
}

# 1c. expand the zip into a working folder
if ($zipPath -and -not $asarPath) {
  if (Test-Path $extractDir) { Remove-Item $extractDir -Recurse -Force }
  Log "Expanding $zipPath ..."
  Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force
  $a = Find-Asar $extractDir
  if (-not $a) { Err "No app.asar found after expanding $zipPath (is it the unpacked app, not an installer?)."; exit 1 }
  $asarPath = $a.FullName; $kind = 'folder'
}

# 1d. auto-detect the installed app
if (-not $asarPath) {
  $instDir = Join-Path $env:LOCALAPPDATA 'RingCentral'
  if (Test-Path $instDir) {
    $instApp = Get-ChildItem -Path $instDir -Directory -Filter 'app-*' -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($instApp) {
      $c = Join-Path $instApp.FullName 'resources\app.asar'
      if (Test-Path $c) { $asarPath = (Get-Item $c).FullName; $kind = 'installed' }
    }
  }
}

if (-not $asarPath) {
  Err 'No RingCentral found. Nothing to patch/launch.'
  Write-Host ''
  Write-Host 'Provide one of these and re-run (or run build.bat which opens this guide):'
  Write-Host '  1) Drop RingCentral.zip (or an unpacked app folder) into the repo root / ringcentral-patch\'
  Write-Host '  2)  .\ringcentral-patch\apply-patch.ps1 -Source <path>'
  Write-Host '  3) If RingCentral is installed under %LocalAppData%\RingCentral, run with no args.'
  exit 2
}
Ok "Using app.asar: $asarPath"

# ---------------------------------------------------------------- 2. TOOLS
try { $null = asar --version; $hasAsar = $true } catch { $hasAsar = $false }
if (-not $hasAsar) {
  if ($NoInstallAsar) { Err 'asar tool missing and -NoInstallAsar passed. Install:  npm i -g asar'; exit 1 }
  Warn 'asar tool missing - installing via npm (needs Node.js)...'
  npm i -g asar
  if ($LASTEXITCODE -ne 0) { Err 'npm i -g asar failed. Install Node.js: https://nodejs.org'; exit 1 }
  # Freshly installed global bins aren't on the current process PATH yet.
  $g = npm prefix -g 2>$null
  if ($g) { $env:PATH = $g + ';' + $env:PATH }
}

# ---------------------------------------------------------------- 3. UNPACK
$asarDir = Split-Path -Parent $asarPath
$appSrc  = Join-Path $asarDir 'app-src'
if (Test-Path $appSrc) { Remove-Item $appSrc -Recurse -Force }
Log 'Extracting app.asar to app-src ...'
asar extract $asarPath $appSrc
if ($LASTEXITCODE -ne 0) { Err 'asar extract failed'; exit 1 }

# ---------------------------------------------------------------- 4. COPY ENGINE
Log 'Copying in-app engine modules into RingCentral...'
$destEngine = Join-Path $appSrc 'spicy-engine'
New-Item -ItemType Directory -Path $destEngine -Force | Out-Null
Copy-Item (Join-Path $engineDir '*') $destEngine -Recurse -Force
if (Test-Path $styleCss) {
  $destStyles = Join-Path $appSrc 'styles'
  New-Item -ItemType Directory -Path $destStyles -Force | Out-Null
  Copy-Item $styleCss (Join-Path $destStyles 'spicy-button.css') -Force
}

# ---------------------------------------------------------------- 5. WIRE RENDERER
$indexHtml = Get-ChildItem -Path $appSrc -Recurse -Filter 'index.html' -ErrorAction SilentlyContinue | Select-Object -First 1
if ($indexHtml) {
  $html = Get-Content $indexHtml.FullName -Raw -Encoding UTF8
  if ($html -notlike '*spicy-engine/spicy-renderer.js*') {
    $tags = "<script src='./spicy-engine/spicy-config.js'></script>`n    <script src='./spicy-engine/spicy-renderer.js'></script>"
    if ($html -match '</body>') { $html = $html -replace '</body>', "$tags`n</body>" }
    else { $html = $html + "`n$tags" }
    if ($html -notlike '*spicy-button.css*') { $html = $html -replace '</head>', "<link rel='stylesheet' href='./styles/spicy-button.css'/>`n</head>" }
    Set-Content -Path $indexHtml.FullName -Value $html -NoNewline -Encoding UTF8
    Ok 'Renderer entry patched to load the Spicy Lamar engine'
  } else { Ok 'Renderer entry already wired' }
} else {
  Warn 'No index.html found - renderer injection must be done manually (see spicy-engine/README.md).'
}

# ---------------------------------------------------------------- 6. WIRE MAIN (best effort)
$mainFile = Get-ChildItem -Path $appSrc -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -in @('main.js','background.js') } | Select-Object -First 1
if (-not $mainFile) {
  $mainFile = Get-ChildItem -Path $appSrc -File -ErrorAction SilentlyContinue |
              Where-Object { $_.Name -in @('app.js','index.js') } | Select-Object -First 1
}
if ($mainFile) {
  $txt = Get-Content $mainFile.FullName -Raw -Encoding UTF8
  if ($txt -notlike '*spicy-engine/spicy-main*') {
    $engFile = Join-Path $destEngine 'spicy-main.js'
    $rel = [System.IO.Path]::GetRelativePath($mainFile.DirectoryName, $engFile).Replace('\','/')
    if (-not $rel.StartsWith('.')) { $rel = './' + $rel }
    $stub = "`ntry { const sm = require('$rel'); if (sm && typeof sm.attachBestWindow === 'function') sm.attachBestWindow(); } catch (e) { console.warn('[SpicyLamar] main hook failed', e); }"
    Add-Content -Path $mainFile.FullName -Value $stub -Encoding UTF8
    Ok "Main-process hook added to $($mainFile.Name)"
  } else { Ok 'Main process already wired' }
} else {
  Warn 'No main-process entry found - pin-on-top will be renderer-only.'
}

# ---------------------------------------------------------------- 7. REPACK
if (-not (Test-Path "$asarPath.bak")) { Copy-Item $asarPath "$asarPath.bak" -Force; Ok "Backup kept: $asarPath.bak" }
Log 'Repacking app.asar ...'
asar pack $appSrc $asarPath
if ($LASTEXITCODE -ne 0) { Err 'asar pack failed'; exit 1 }
Ok "Repacked $asarPath"
if (-not $KeepWorkingDir) { Remove-Item $appSrc -Recurse -Force -ErrorAction SilentlyContinue }

# ---------------------------------------------------------------- 8. LAUNCH
$exe = $null
$appFolder = Split-Path -Parent $asarDir   # folder that holds resources\ + RingCentral.exe
if ($kind -ne 'bare') {
  foreach ($name in @('RingCentral.exe','Glip.exe','rcdesktop.exe')) {
    $c = Join-Path $appFolder $name
    if (Test-Path $c) { $exe = $c; break }
  }
  if (-not $exe) {
    $exe = Get-ChildItem -Path $appFolder -Filter '*.exe' -ErrorAction SilentlyContinue |
           Where-Object { $_.Name -notlike 'unins*' -and $_.Name -notlike 'Update*' } | Select-Object -First 1
  }
}

Write-Host ''
Log '=== PATCH COMPLETE ==='
Write-Host 'Spicy Lamar is now INSIDE RingCentral:'
Write-Host '   dialer row:  [Chili] SPICY button      Settings menu:  Spicy Lamar - Auto-Answer [ON]' -ForegroundColor Green
if ($exe -and -not $SkipLaunch) {
  Log "Launching patched RingCentral: $exe"
  Start-Process -FilePath $exe
  Ok 'RingCentral started - open the dialer to see the [Chili] SPICY button.'
} elseif ($exe) {
  Write-Host "Not launched (-SkipLaunch). Start it yourself:  $exe"
} else {
  Warn 'Could not locate RingCentral.exe - start your RingCentral app manually (patched app.asar is in place).'
}
Write-Host ''
Write-Host 'First-run note: if no [Chili] SPICY button appears, open the dialer DevTools'
Write-Host '(Ctrl+Shift+I) and read the [SpicyLamar] console logs, then tune the DOM'
Write-Host 'selectors in ringcentral-patch\spicy-engine\spicy-config.js to your RC build.'
exit 0
