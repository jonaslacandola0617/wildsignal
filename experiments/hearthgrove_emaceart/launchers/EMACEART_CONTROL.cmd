@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove_emaceart.exe" --assets "%~dp0assets" --save "%~dp0saves/hearthgrove_emaceart.json" --pacing precise --cap 30 --output "%~dp0runs/EMACEART-SPARSE-%RUN_ID%" --resolution 480 --grass-style emaceart --grass-density low --shaft-profile glow
if errorlevel 1 pause
