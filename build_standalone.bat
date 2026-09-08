@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul
title SpicyLamar - Standalone C# build (legacy mirror, auto-launch)

echo ==========================================================
echo  SPICY LAMAR - STANDALONE C# mirror build (980x620 demo)
echo  This is the LEGACY standalone SpicyLamar.exe.
echo  The recommended in-app path is build.bat (patches RingCentral
echo  so the feature lives INSIDE the RC app). This builds a separate
echo  demo window and then LAUNCHES it.
echo ==========================================================

pushd "%~dp0" >nul
cd /d "%~dp0"

set "CSC=%SystemRoot%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%CSC%" set "CSC=%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
if not exist "%CSC%" (
    echo [ERROR] .NET Framework 4.x C# compiler not found at %CSC%.
    pause
    exit /b 1
)

if not exist "%~dp0dist" mkdir "%~dp0dist"
set "OUT=%~dp0dist\SpicyLamar.exe"

set "ICON_PARAM="
if exist "resources\icon.ico" set "ICON_PARAM=/win32icon:resources\icon.ico"

echo [1/2] Compiling SpicyLamar.cs ...
"%CSC%" /nologo /target:winexe /optimize+ /platform:anycpu /utf8output ^
    /codepage:65001 ^
    /r:System.dll,System.Drawing.dll,System.Windows.Forms.dll,System.Core.dll ^
    !ICON_PARAM! /out:"%OUT%" SpicyLamar.cs

if errorlevel 1 (
    echo [ERROR] Compilation failed - see messages above.
    pause
    exit /b 1
)

echo.
echo ==========================================================
echo  SUCCESS - built: "%OUT%"
echo  Launching the standalone demo window now...
echo ==========================================================
start "" "%OUT%"
timeout /t 5 >nul 2>&1 || pause
exit /b 0
