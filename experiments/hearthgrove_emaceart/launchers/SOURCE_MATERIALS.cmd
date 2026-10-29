@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove_emaceart.exe" --assets "%~dp0assets" --save "%~dp0saves/hearthgrove_emaceart.json" --pacing precise --cap 30 --output "%~dp0runs/SOURCE_MATERIALS-%RUN_ID%" --resolution 480 --style-preset baseline --render-mode original --worldstyle source --shadow off --shafts off --haze off --hud compact
if errorlevel 1 pause
