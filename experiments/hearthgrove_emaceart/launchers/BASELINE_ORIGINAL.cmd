@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove_emaceart.exe" --assets "%~dp0assets_baseline" --save "%~dp0saves/baseline_compare.json" --pacing precise --cap 30 --output "%~dp0runs/BASELINE_ORIGINAL-%RUN_ID%" --resolution 480 --style-preset baseline --grass-density medium --hud debug
if errorlevel 1 pause
