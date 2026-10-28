@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove001.exe" --assets "%~dp0assets" --save "%~dp0saves/hearthgrove.json" --cap 30 --pacing precise --worldstyle pixel --render-mode toon --resolution 480 --output "%~dp0runs/B_PIXEL_TOON_480-%RUN_ID%"
if errorlevel 1 pause
