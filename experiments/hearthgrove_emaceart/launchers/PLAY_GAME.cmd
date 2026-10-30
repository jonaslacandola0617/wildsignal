@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove_emaceart.exe" --assets "%~dp0assets" --save "%~dp0saves/hearthgrove_emaceart.json" --style-preset balanced --ink balanced --pacing precise --cap 30 --grass-density high --highlights on --outlines on --vegetation-edges on --haze on --shafts on --quality medium --air-proof bounded --grass-lod on --meadow-patches on --meadow-lighting on --shaft-profile defined --output "%~dp0runs/PLAY_GAME-%RUN_ID%" --resolution 480 --shadow on --shadow-policy clean --grass-cast off
if errorlevel 1 pause
