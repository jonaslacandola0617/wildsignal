@echo off
setlocal
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools/hardware.ps1" -Root "%~dp0." -Resolution 480 -Cap 30
if errorlevel 1 pause
