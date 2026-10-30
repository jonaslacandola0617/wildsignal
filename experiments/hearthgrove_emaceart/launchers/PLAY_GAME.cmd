@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove_emaceart.exe" --assets "%~dp0assets" --save "%~dp0saves/tree_study.json" --tree-scene --tree-study billboards --leaf-color plain --style-preset balanced --ink balanced --pacing precise --cap 30 --grass-density low --highlights on --outlines on --vegetation-edges on --haze off --shafts off --quality medium --air-proof bounded --grass-lod on --meadow-patches on --meadow-lighting on --meadow-response beam --grass-outlines off --shaft-profile defined --output "%~dp0runs/PLAY_GAME-%RUN_ID%" --resolution 480 --shadow on --shadow-policy clean --grass-cast off
if errorlevel 1 pause
