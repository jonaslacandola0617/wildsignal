@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_holland003.exe" --pack "%~dp0assets/pack.json" --scene "%~dp0data/garden.json" --look "%~dp0data/TOON_DIAGNOSTIC.json" --style C --no-art-materials --mode PIXEL3D --no-smooth-lighting --study B1 --pixel-camera off --shadow off --shafts off --resolution 640 --pose 14 --cap 30 --capture-presented --edge-mode off --shadow-proof stable --shadow cached --diagnostic shadow --debug-effects --pose 12 --output "%~dp0runs/RUN_SHADOW_DIAGNOSTIC-%RUN_ID%"
if errorlevel 1 pause
