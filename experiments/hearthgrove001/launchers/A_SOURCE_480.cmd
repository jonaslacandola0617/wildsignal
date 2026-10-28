@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_hearthgrove001.exe" --assets "%~dp0assets" --save "%~dp0saves/hearthgrove.json" --cap 30 --pacing precise --worldstyle source --render-mode original --resolution 480 --output "%~dp0runs/A_SOURCE_480-%RUN_ID%"
if errorlevel 1 pause
