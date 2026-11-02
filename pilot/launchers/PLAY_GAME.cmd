@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_pilot.exe" --assets "%~dp0assets" --save "%~dp0saves\cozy.json" --camera raised --style-preset balanced --ink balanced --pacing precise --cap 30 --grass-density high --highlights on --outlines on --vegetation-edges on --haze off --shafts off --quality medium --air-proof bounded --grass-lod on --meadow-patches on --meadow-lighting on --meadow-response beam --grass-outlines off --shaft-profile defined --shadow on --shadow-policy clean --grass-cast off --hud off --pilot "%~dp0assets\pilot\pilot_warm3d.json" --output "%~dp0runs\play-%RUN_ID%" %*
if errorlevel 1 pause
