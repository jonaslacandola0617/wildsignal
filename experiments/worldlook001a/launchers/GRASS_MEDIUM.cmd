@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
"%~dp0wildsignal_worldlook001a.exe" --pack "%~dp0assets/pack.json" --scene "%~dp0data/garden.json" --look "%~dp0data/TOON_DIAGNOSTIC.json" --style C --mode PIXEL3D --no-art-materials --no-smooth-lighting --study B1 --pixel-camera off --edge-mode combined --shadow-proof stable --shadow cached --shadow-size 512 --shafts off --grass-kit "%~dp0assets/grass/grass_kit.json" --grass-facing view --grass-seed 17001 --present-scale 2 --grass medium --resolution 480 --pose 14 --cap 30 --pacing precise --output "%~dp0runs/MEDIUM-%RUN_ID%" %*
if errorlevel 1 pause
