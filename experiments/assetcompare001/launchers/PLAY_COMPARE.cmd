@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_assetcompare001.exe" --assets "%~dp0assets" --style source --resolution 640 --cap 30 --output "%~dp0runs/PLAY-%RUN_ID%"
if errorlevel 1 pause
