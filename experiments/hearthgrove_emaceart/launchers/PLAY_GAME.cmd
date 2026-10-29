@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove_emaceart.exe" --assets "%~dp0assets" --save "%~dp0saves/hearthgrove_emaceart.json" --pacing precise --cap 30 --output "%~dp0runs/OWNER-GRASS-%RUN_ID%" --resolution 480 --grass-style owner --grass-density high --shaft-profile streaks
if errorlevel 1 pause
