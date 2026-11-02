@echo off
setlocal
cd /d "%~dp0"
set "RUN_ID=%RANDOM%-%RANDOM%"
call PLAY_GAME.cmd --new-game --save "%~dp0saves\benchmark-reference.json" --replay "%~dp0data\performance_walk.txt" --frames 7200 --render-reference --cap 0 --pacing off --output "%~dp0runs\reference-%RUN_ID%"
if errorlevel 1 exit /b 1
call PLAY_GAME.cmd --new-game --save "%~dp0saves\benchmark-optimized.json" --replay "%~dp0data\performance_walk.txt" --frames 7200 --cap 0 --pacing off --output "%~dp0runs\optimized-%RUN_ID%"
pause
