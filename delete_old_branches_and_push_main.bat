@echo off
REM This script deletes all local and remote branches except main, merges your current branch into main, and pushes all changes to GitHub.

REM Step 1: Switch to main branch

git checkout main

REM Step 2: Merge current branch (docs/nvs-migration) into main

git merge docs/nvs-migration

REM Step 3: Delete all local branches except main
for /f "tokens=*" %%b in ('git branch ^| findstr /v "* main"') do git branch -D %%b

REM Step 4: Delete all remote branches except main
for /f "tokens=*" %%b in ('git branch -r ^| findstr /v "origin/main"') do (
    setlocal enabledelayedexpansion
    set branch=%%b
    set branch=!branch:origin/=!
    git push origin --delete !branch!
    endlocal
)

REM Step 5: Push main to GitHub

git push origin main

echo All old branches deleted, main updated and pushed to GitHub.
pause
