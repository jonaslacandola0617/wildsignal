@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove001.exe" --assets "%~dp0assets" --save "%~dp0saves/hearthgrove.json" --resolution 480 --cap 30 --grass-density medium --megakit-house on --pacing precise --output "%~dp0runs/PLAY-%RUN_ID%"
if errorlevel 1 pause
