@echo off
setlocal EnableExtensions
chcp 65001 >nul
title Spicy Lamar - RingCentral In-App Installer

echo ==========================================================
echo  SPICY LAMAR - RINGCENTRAL IN-APP INSTALLER
echo ==========================================================
echo.
echo  The separate WinForms dashboard is no longer a supported control
echo path. Redirecting to the RingCentral in-app installer so controls
echo never apply outside RingCentral.
echo.

call "%~dp0build.bat"
exit /b %errorlevel%
