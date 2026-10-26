@echo off
setlocal
cd /d "%~dp0"
if not exist "%~dp0fixtures/simple/assets/pack.json" (
 echo SKIPPED_MISSING_FIXTURE: simple. No substitute is permitted.
 pause
 exit /b 2
)
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_holland004.exe" --pack "%~dp0fixtures/simple/assets/pack.json" --scene "%~dp0fixtures/simple/data/simple_shapes.json" --fixture-id simple --look "%~dp0data/TOON_DIAGNOSTIC.json" --style C --no-art-materials --no-smooth-lighting --mode PIXEL3D --study B1 --pixel-camera off --edge-mode combined --shadow-proof stable --shadow cached --shadow-size 512 --shafts off --present-scale 2 --resolution 480 --cap 30 --pacing legacy --output "%~dp0runs/SIMPLE_480_CAP30_LEGACY-%RUN_ID%"
if errorlevel 1 pause
