@echo off
setlocal
cd /d "%~dp0"
if not exist "%~dp0assets/pack.json" (
 echo SKIPPED_MISSING_FIXTURE: garden. No substitute is permitted.
 pause
 exit /b 2
)
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_holland004.exe" --pack "%~dp0assets/pack.json" --scene "%~dp0data/garden.json" --fixture-id garden --look "%~dp0data/TOON_DIAGNOSTIC.json" --style C --no-art-materials --no-smooth-lighting --mode PIXEL3D --study B1 --pixel-camera off --edge-mode combined --shadow-proof stable --shadow cached --shadow-size 512 --shafts off --present-scale 2 --resolution 640 --cap 60 --pacing precise --output "%~dp0runs/GARDEN_640_CAP60_PRECISE-%RUN_ID%"
if errorlevel 1 pause
