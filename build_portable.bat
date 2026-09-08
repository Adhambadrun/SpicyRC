@echo off
setlocal EnableExtensions
chcp 65001 >nul
title SpicyLamar v1.0 Integrated - Portable One-File Build

echo ==========================================================
echo  SPICY LAMAR v1.0 Integrated - PORTABLE ONE-FILE BUILD
echo  Single window 980x620 | TURBO 200Hz | Keypad docked inside
echo ==========================================================
echo.

call "%~dp0build\build.bat"
if errorlevel 1 exit /b %errorlevel%

echo.
echo Packaging portable zip...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build\package_portable.ps1"
exit /b %errorlevel%
