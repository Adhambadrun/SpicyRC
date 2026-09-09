@echo off
setlocal EnableExtensions
chcp 65001 >nul
title Spicy Lamar - RingCentral In-App Installer

echo ==========================================================
echo  SPICY LAMAR - RINGCENTRAL IN-APP INSTALLER
echo ==========================================================
echo.
echo  The old portable EXE created a separate desktop window and is
echo no longer a supported control path. Redirecting to the in-app
echo installer so every Spicy Lamar control remains inside RingCentral.
echo.

call "%~dp0build.bat"
exit /b %errorlevel%
