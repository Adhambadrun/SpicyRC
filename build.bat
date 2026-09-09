@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul
title Spicy Lamar - Patch & Launch RingCentral (feature inside RC)
pushd "%~dp0" >nul
cd /d "%~dp0"

echo ==========================================================
echo  SPICY LAMAR v1.0 - In-App Build for RingCentral
echo  Puts the feature INSIDE RingCentral's own app, then opens it.
echo ==========================================================
echo.
echo  What this does:
echo   1. Locates your RingCentral (zip / installed app).
echo   2. Unpacks resources\app.asar and injects the Spicy engine:
echo        - a  [Chili] SPICY ON/OFF button in the dialer row
echo        - Auto-Answer + Pin RingCentral on top items in the Settings menu
echo        - in-app controls only: no popup, alt-exe, global hotkey, or PC-wide input
echo   3. Repacks app.asar and LAUNCHES the patched RingCentral.
echo.
echo ==========================================================

set "PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if not exist "%PS%" (
    echo [ERROR] Windows PowerShell 5.1 not found.
    pause
    exit /b 1
)

"%PS%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0ringcentral-patch\apply-patch.ps1"
set "RC=%ERRORLEVEL%"

if "%RC%"=="2" (
    echo.
    echo ==========================================================
    echo  NO RINGCENTRAL FOUND - nothing could be patched/launched.
    echo ==========================================================
    echo  Open the guide below (launched in your browser) for the
    echo  exact one thing you need to drop in, then re-run build.bat.
    echo.
    start "" "%~dp0docs\INAPP_GUIDE.html"
    echo  Guide opened:  docs\INAPP_GUIDE.html
    echo.
    pause
    exit /b 2
)

if "%RC%" NEQ "0" (
    echo.
    echo [ERROR] Patch & launch failed - see messages above.
    pause
    exit /b 1
)

echo.
echo ==========================================================
echo  DONE - RingCentral was launched with Spicy Lamar inside it.
echo  Open the dialer: you should see [Chili] SPICY ON/OFF beside Call
echo  and Auto-Answer + Pin RingCentral on top in Settings.
echo ==========================================================
timeout /t 5 >nul 2>&1 || pause
exit /b 0
