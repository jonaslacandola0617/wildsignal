@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_holland001.exe" --pack "%~dp0assets/pack.json" --scene "%~dp0data/garden.json" --look "%~dp0data/sunlight_look.json" --style C --no-art-materials --mode PIXEL3D --shadow cached --shadow-size 512 --shafts off --resolution 640 --pose 14 --cap 30 --capture-presented --study B2 --pixel-camera comp --resolution 480 --scene "%~dp0data/garden_sparse.json" --cap 30 --output "%~dp0runs/RUN_LOW_480-%RUN_ID%"
if errorlevel 1 pause
