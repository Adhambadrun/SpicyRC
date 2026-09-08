@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul
title SpicyLamar v1.0 Integrated - C# Instant Build

pushd "%~dp0" >nul
cd /d "%~dp0"

echo ==========================================================
echo  SPICY LAMAR v1.0 Integrated - C# INSTANT BUILD
echo  980x620 single window | WinForms mirror | docked keypad
echo ==========================================================

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

set "MANIFEST_PARAM="
if exist "resources\app.manifest" set "MANIFEST_PARAM=/win32manifest:resources\app.manifest"

echo [1/2] Compiling SpicyLamar.cs (Integrated, Panel layout: settingsBar Top + keypad Right + left Fill)...
"%CSC%" /nologo /target:winexe /optimize+ /platform:anycpu /utf8output ^
    /codepage:65001 ^
    /r:System.dll,System.Drawing.dll,System.Windows.Forms.dll,System.Core.dll ^
    !ICON_PARAM! !MANIFEST_PARAM! /out:"%OUT%" SpicyLamar.cs

if errorlevel 1 (
    echo [ERROR] Compilation failed - see messages above.
    pause
    exit /b 1
)

echo.
echo ==========================================================
echo  SUCCESS - portable executable created at:
echo  "%OUT%"
echo  Single window 980x620 | Keypad docked inside
echo ==========================================================
timeout /t 5 >nul 2>&1 || pause
exit /b 0
