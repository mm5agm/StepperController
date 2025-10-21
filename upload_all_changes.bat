@echo off
REM Simple batch script to add, commit, and push all changes to your GitHub repository

REM Change to your project directory (optional, if not already there)
REM cd /d "C:\Users\colin\OneDrive\Documents\PlatformIO\Projects\StepperController"

git branch --show-current | findstr /i "main" >nul
if errorlevel 1 (
    echo Switching local branch to main...
    git branch -m master main
    git fetch origin
    git branch --set-upstream-to=origin/main main
)

git add -A

echo Enter commit message: 
set /p msg=

git commit -m "%msg%"

git push origin main

pause
