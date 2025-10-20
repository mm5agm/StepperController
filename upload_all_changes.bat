@echo off
REM Simple batch script to add, commit, and push all changes to your GitHub repository

REM Change to your project directory (optional, if not already there)
REM cd /d "C:\Users\colin\OneDrive\Documents\PlatformIO\Projects\StepperController"

git add -A

set /p msg="Enter commit message: "

git commit -m "%msg%"

git push origin master

pause
