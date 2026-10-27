@echo off
setlocal
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools/hardware.ps1" -Root "%~dp0." -Resolution 640 -Cap 60
if errorlevel 1 pause
