@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove_emaceart.exe" --assets "%~dp0assets" --save "%~dp0saves/hearthgrove_emaceart.json" --style-preset balanced --ink balanced --pacing precise --cap 30 --grass-density medium --haze off --shafts off --output "%~dp0runs/LOW_SPEC-%RUN_ID%" --resolution 480 --shadow off --grass-density low --quality low
if errorlevel 1 pause
