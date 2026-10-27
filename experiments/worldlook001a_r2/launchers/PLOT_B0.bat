@echo off
setlocal
cd /d "%~dp0"
"%~dp0wildsignal_worldlook001a_r2.exe" --r2-kit "%~dp0assets/mesh/mesh_kit.json" --r2-model B0 --r2-count 24 --r2-ground patches --r2-placement clustered --look "%~dp0data/TOON_DIAGNOSTIC.json" --no-art-materials --no-smooth-lighting --study B1 --shadow cached --shadow-proof stable --shafts off --resolution 480 --pose 0 --pacing precise --cap 30 --output "%~dp0runs/PLOT_B0-%RANDOM%-%RANDOM%" --grass-kit "%~dp0assets/grass/grass_kit.json"
if errorlevel 1 pause
