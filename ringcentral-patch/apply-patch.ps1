#Requires -Version 5.1
<#
  Put Spicy Lamar inside RingCentral and launch the patched RingCentral app.

  The patch installs a renderer-local control and a narrow Electron bridge. All
  interactions are scoped to RingCentral's own BrowserWindow: no standalone
  SpicyLamar.exe, global hotkeys, synthetic desktop input, or other desktop windows.

  Usage:
    .\ringcentral-patch\apply-patch.ps1
    .\ringcentral-patch\apply-patch.ps1 -Source C:\path\to\RingCentral.zip
    .\ringcentral-patch\apply-patch.ps1 -Source C:\path\to\resources\app.asar

  Exit codes: 0 patched (+ launched unless -SkipLaunch), 1 error, 2 no source.
#>
[CmdletBinding()]
param(
  [string]$Source = '',
  [switch]$SkipLaunch,
  [switch]$NoInstallAsar,
  [switch]$KeepWorkingDir
)

$ErrorActionPreference = 'Stop'
$scriptDir  = $PSScriptRoot
$rootDir    = Split-Path -Parent $scriptDir
$engineDir  = Join-Path $scriptDir 'spicy-engine'
$styleCss   = Join-Path $scriptDir 'styles\spicy-button.css'
$extractDir = Join-Path $scriptDir 'RingCentral-patched'

function Log($message)  { Write-Host "[SpicyLamar] $message" -ForegroundColor Cyan }
function Ok($message)   { Write-Host "[OK]  $message" -ForegroundColor Green }
function Warn($message) { Write-Host "[WARN] $message" -ForegroundColor Yellow }
function Err($message)  { Write-Host "[ERR]  $message" -ForegroundColor Red }

function Find-Asar([string]$Directory) {
  if (-not (Test-Path -LiteralPath $Directory)) { return $null }
  return Get-ChildItem -LiteralPath $Directory -Filter 'app.asar' -Recurse -File -ErrorAction SilentlyContinue |
    Select-Object -First 1
}

function Get-RequirePath([string]$FromDirectory, [string]$ToFile) {
  # [IO.Path]::GetRelativePath is not available in Windows PowerShell 5.1.
  $basePath = [System.IO.Path]::GetFullPath($FromDirectory).TrimEnd('\') + '\'
  $targetPath = [System.IO.Path]::GetFullPath($ToFile)
  $base = [Uri]$basePath
  $target = [Uri]$targetPath
  $relative = [Uri]::UnescapeDataString($base.MakeRelativeUri($target).ToString()).Replace('\', '/')
  if (-not $relative.StartsWith('.')) { $relative = './' + $relative }
  return $relative
}

function Find-MainEntry([string]$AppSource) {
  $packagePath = Join-Path $AppSource 'package.json'
  if (Test-Path -LiteralPath $packagePath) {
    try {
      $package = Get-Content -LiteralPath $packagePath -Raw -Encoding UTF8 | ConvertFrom-Json
      if ($package.main) {
        $candidate = Join-Path $AppSource ([string]$package.main)
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return Get-Item -LiteralPath $candidate }
      }
    } catch { Warn "Could not read package.json main entry: $($_.Exception.Message)" }
  }

  foreach ($name in @('main.js', 'background.js', 'index.js', 'app.js', 'main.cjs', 'background.cjs')) {
    $candidate = Join-Path $AppSource $name
    if (Test-Path -LiteralPath $candidate -PathType Leaf) { return Get-Item -LiteralPath $candidate }
  }

  # Last resort: prefer a shallow main/background candidate over arbitrary code.
  return Get-ChildItem -LiteralPath $AppSource -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -in @('main.js', 'background.js', 'main.cjs', 'background.cjs') } |
    Sort-Object @{ Expression = { $_.FullName.Length } } |
    Select-Object -First 1
}

function Find-PreloadFiles([string]$AppSource, $MainFile, [string]$EngineDirectory) {
  $files = New-Object System.Collections.ArrayList
  $seen = @{}
  function Add-PreloadCandidate($candidate) {
    if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
      $full = (Get-Item -LiteralPath $candidate).FullName
      if (-not $seen.ContainsKey($full) -and -not $full.StartsWith($EngineDirectory, [System.StringComparison]::OrdinalIgnoreCase)) {
        $seen[$full] = $true
        [void]$files.Add((Get-Item -LiteralPath $full))
      }
    }
  }

  if ($MainFile) {
    try {
      $mainText = Get-Content -LiteralPath $MainFile.FullName -Raw -Encoding UTF8
      # Handles preload: 'preload.js' and preload: path.join(__dirname, 'preload.js').
      $preloadPattern = '(?is)preload\s*:\s*(?:path\.join\s*\(\s*__dirname\s*,\s*)?["''](?<path>[^"'']+\.(?:js|cjs))["'']'
      $matches = [regex]::Matches($mainText, $preloadPattern)
      foreach ($match in $matches) {
        $relative = $match.Groups['path'].Value
        Add-PreloadCandidate (Join-Path $MainFile.DirectoryName $relative)
        Add-PreloadCandidate (Join-Path $AppSource $relative)
      }
    } catch { Warn "Could not inspect main entry for preload: $($_.Exception.Message)" }
  }

  foreach ($name in @('preload.js', 'preload.cjs', 'renderer\preload.js', 'src\preload.js')) {
    Add-PreloadCandidate (Join-Path $AppSource $name)
  }

  # A named preload is much more likely to be active than a minified bundle.
  Get-ChildItem -LiteralPath $AppSource -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^preload(?:\..+)?\.(js|cjs)$' } |
    ForEach-Object { Add-PreloadCandidate $_.FullName }

  return @($files)
}

function Add-RequireHook($File, [string]$TargetFile, [string]$Marker, [string]$Description) {
  $text = Get-Content -LiteralPath $File.FullName -Raw -Encoding UTF8
  if ($text -like "*$Marker*") {
    Ok "$Description already wired: $($File.Name)"
    return $false
  }
  $relative = Get-RequirePath $File.DirectoryName $TargetFile
  $hook = "`n`n// $Marker`ntry { require('$relative'); } catch (e) { console.warn('[SpicyLamar] $Description hook failed', e); }`n"
  Add-Content -LiteralPath $File.FullName -Value $hook -Encoding UTF8
  Ok "$Description wired: $($File.FullName)"
  return $true
}

Log '=== RingCentral — Spicy Lamar IN-APP patch ==='

# Validate the patch before making a backup or touching app.asar.
$requiredEngine = @('spicy-config.js', 'spicy-renderer.js', 'spicy-main.js', 'spicy-preload.js')
foreach ($file in $requiredEngine) {
  if (-not (Test-Path -LiteralPath (Join-Path $engineDir $file) -PathType Leaf)) {
    Err "Patch is incomplete: missing ringcentral-patch\spicy-engine\$file"
    exit 1
  }
}

# ---------------------------------------------------------------- Locate source
$asarPath = $null
$zipPath = $null
$kind = $null
if ($Source) {
  $resolved = Resolve-Path -LiteralPath $Source -ErrorAction SilentlyContinue
  if (-not $resolved) { Err "Source not found: $Source"; exit 1 }
  $item = Get-Item -LiteralPath $resolved.Path
  if ($item.PSIsContainer) {
    $asar = Find-Asar $item.FullName
    if (-not $asar) { Err "No app.asar found inside $($item.FullName)"; exit 1 }
    $asarPath = $asar.FullName; $kind = 'folder'
  } elseif ($item.Name -ieq 'app.asar') {
    $asarPath = $item.FullName; $kind = 'bare'
  } elseif ($item.Extension -ieq '.zip') {
    $zipPath = $item.FullName
  } else {
    Err "Unsupported source type: $($item.FullName)"
    exit 1
  }
}

if (-not $asarPath -and -not $zipPath -and -not $Source) {
  foreach ($candidate in @((Join-Path $scriptDir 'RingCentral.zip'), (Join-Path $rootDir 'RingCentral.zip'))) {
    if (Test-Path -LiteralPath $candidate -PathType Leaf) { $zipPath = $candidate; break }
  }
}

if ($zipPath -and -not $asarPath) {
  if (Test-Path -LiteralPath $extractDir) { Remove-Item -LiteralPath $extractDir -Recurse -Force }
  Log "Expanding $zipPath ..."
  Expand-Archive -LiteralPath $zipPath -DestinationPath $extractDir -Force
  $asar = Find-Asar $extractDir
  if (-not $asar) { Err "No app.asar found after expanding $zipPath"; exit 1 }
  $asarPath = $asar.FullName; $kind = 'folder'
}

if (-not $asarPath) {
  $installRoots = @(
    (Join-Path $env:LOCALAPPDATA 'RingCentral'),
    (Join-Path $env:LOCALAPPDATA 'Programs\RingCentral'),
    (Join-Path $env:ProgramFiles 'RingCentral')
  )
  if (${env:ProgramFiles(x86)}) { $installRoots += (Join-Path ${env:ProgramFiles(x86)} 'RingCentral') }
  foreach ($root in $installRoots) {
    $asar = Find-Asar $root
    if ($asar) { $asarPath = $asar.FullName; $kind = 'installed'; break }
  }
}

if (-not $asarPath) {
  Err 'No RingCentral app.asar was found. Nothing was patched.'
  Write-Host 'Provide -Source <RingCentral.zip | app folder | resources\app.asar>, or install RingCentral first.'
  exit 2
}
Ok "Using app.asar: $asarPath"

# ---------------------------------------------------------------- Ensure asar CLI
try { $null = & asar --version 2>$null; $hasAsar = ($LASTEXITCODE -eq 0) } catch { $hasAsar = $false }
if (-not $hasAsar) {
  if ($NoInstallAsar) { Err 'asar is missing. Install it with: npm i -g @electron/asar'; exit 1 }
  Warn 'asar is missing; installing @electron/asar with npm ...'
  & npm i -g @electron/asar
  if ($LASTEXITCODE -ne 0) { Err 'npm could not install @electron/asar. Install Node.js, then retry.'; exit 1 }
  # npm's global bin directory may not be on this PowerShell process PATH yet.
  try {
    $npmPrefix = (& npm prefix -g 2>$null | Select-Object -First 1)
    if ($npmPrefix) { $env:PATH = "$npmPrefix;$env:PATH" }
  } catch {}
}

# ---------------------------------------------------------------- Extract + install modules
$asarDirectory = Split-Path -Parent $asarPath
$appSource = Join-Path $asarDirectory 'app-src'
if (Test-Path -LiteralPath $appSource) { Remove-Item -LiteralPath $appSource -Recurse -Force }
Log 'Extracting app.asar ...'
& asar extract $asarPath $appSource
if ($LASTEXITCODE -ne 0) { Err 'asar extract failed'; exit 1 }

try {
  $destEngine = Join-Path $appSource 'spicy-engine'
  New-Item -ItemType Directory -Path $destEngine -Force | Out-Null
  Copy-Item -Path (Join-Path $engineDir '*') -Destination $destEngine -Recurse -Force
  if (Test-Path -LiteralPath $styleCss -PathType Leaf) {
    $destStyles = Join-Path $appSource 'styles'
    New-Item -ItemType Directory -Path $destStyles -Force | Out-Null
    Copy-Item -LiteralPath $styleCss -Destination (Join-Path $destStyles 'spicy-button.css') -Force
  }
  Ok 'Installed in-app renderer, main bridge, and scoped controls'

  # Main injection is the primary route: it works for hashed/bundled renderer
  # entries and re-injects only after a RingCentral renderer navigation.
  $mainFile = Find-MainEntry $appSource
  if ($mainFile) {
    Add-RequireHook $mainFile (Join-Path $destEngine 'spicy-main.js') 'spicy-lamar-main-hook' 'SpicyLamar main integration' | Out-Null
  } else {
    Warn 'No Electron main entry was found. The index.html compatibility hook will be used if present.'
  }

  # Add the narrow contextBridge to the existing preload when discoverable.
  # It exposes only settings state and only to the RC renderer.
  $preloads = @(Find-PreloadFiles $appSource $mainFile $destEngine)
  if ($preloads.Count -gt 0) {
    foreach ($preload in $preloads) {
      Add-RequireHook $preload (Join-Path $destEngine 'spicy-preload.js') 'spicy-lamar-preload-hook' 'SpicyLamar preload bridge' | Out-Null
    }
  } else {
    Warn 'No existing preload was identified. Auto-answer remains in-app; pin control will report if its bridge is unavailable.'
  }

  # Compatibility route for versions with a normal index.html. The renderer is
  # idempotent, so this and the main injection cannot add duplicate controls.
  $indexHtml = Get-ChildItem -LiteralPath $appSource -Recurse -File -Filter 'index.html' -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($indexHtml) {
    $html = Get-Content -LiteralPath $indexHtml.FullName -Raw -Encoding UTF8
    if ($html -notlike '*spicy-engine/spicy-config.js*') {
      # index.html may be nested below app root, so calculate URLs relative to
      # this exact renderer entry rather than assuming ./spicy-engine.
      $configUrl = Get-RequirePath $indexHtml.DirectoryName (Join-Path $destEngine 'spicy-config.js')
      $rendererUrl = Get-RequirePath $indexHtml.DirectoryName (Join-Path $destEngine 'spicy-renderer.js')
      $styleUrl = Get-RequirePath $indexHtml.DirectoryName (Join-Path $appSource 'styles\spicy-button.css')
      $tags = "<script src='$configUrl'></script>`n<script src='$rendererUrl'></script>"
      if ($html -match '</body>') { $html = $html -replace '</body>', ($tags + "`n</body>") }
      else { $html += "`n$tags" }
      if ($html -notlike '*spicy-button.css*' -and $html -match '</head>') {
        $html = $html -replace '</head>', "<link rel='stylesheet' href='$styleUrl'>`n</head>"
      }
      Set-Content -LiteralPath $indexHtml.FullName -Value $html -NoNewline -Encoding UTF8
      Ok "Compatibility renderer hook wired: $($indexHtml.FullName)"
    } else { Ok 'Compatibility renderer hook already wired' }
  }

  # ---------------------------------------------------------------- Repack
  if (-not (Test-Path -LiteralPath "$asarPath.bak")) {
    Copy-Item -LiteralPath $asarPath -Destination "$asarPath.bak" -Force
    Ok "Backup created: $asarPath.bak"
  }
  Log 'Repacking app.asar ...'
  & asar pack $appSource $asarPath
  if ($LASTEXITCODE -ne 0) { throw 'asar pack failed' }
  Ok "Repacked $asarPath"
} catch {
  Err "Patch failed: $($_.Exception.Message)"
  exit 1
} finally {
  if (-not $KeepWorkingDir -and (Test-Path -LiteralPath $appSource)) {
    Remove-Item -LiteralPath $appSource -Recurse -Force -ErrorAction SilentlyContinue
  }
}

# ---------------------------------------------------------------- Launch patched RingCentral
$exe = $null
$appFolder = Split-Path -Parent $asarDirectory
if ($kind -ne 'bare') {
  foreach ($name in @('RingCentral.exe', 'RingCentral Phone.exe', 'Glip.exe', 'rcdesktop.exe')) {
    $candidate = Join-Path $appFolder $name
    if (Test-Path -LiteralPath $candidate -PathType Leaf) { $exe = Get-Item -LiteralPath $candidate; break }
  }
  if (-not $exe) {
    $exe = Get-ChildItem -LiteralPath $appFolder -File -Filter '*.exe' -ErrorAction SilentlyContinue |
      Where-Object { $_.Name -notmatch '^(unins|update)' } | Select-Object -First 1
  }
}

Write-Host ''
Log '=== PATCH COMPLETE ==='
Write-Host 'Spicy Lamar now runs inside RingCentral only:' -ForegroundColor Green
Write-Host '  • 🌶 SPICY ON/OFF is beside RingCentral''s CALL control.'
Write-Host '  • Auto-Answer and Pin RingCentral controls are in RingCentral''s settings menu.'
Write-Host '  • No standalone window, global hotkey, or PC-wide input is installed.'
if ($exe -and -not $SkipLaunch) {
  Log "Launching patched RingCentral: $($exe.FullName)"
  Start-Process -FilePath $exe.FullName
  Ok 'RingCentral launch requested. Open its dialer to use the in-app controls.'
} elseif ($exe) {
  Write-Host "Not launched (-SkipLaunch). Start RingCentral yourself: $($exe.FullName)"
} else {
  Warn 'Patched app.asar successfully, but an executable was not found next to it. Start RingCentral normally.'
}
exit 0
