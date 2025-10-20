@echo off
REM This script deletes all local and remote branches except master, merges your current branch into master, and pushes all changes to GitHub.

REM Step 1: Switch to master branch

git checkout master

REM Step 2: Merge current branch (docs/nvs-migration) into master

git merge docs/nvs-migration

REM Step 3: Delete all local branches except master
for /f "tokens=*" %%b in ('git branch ^| findstr /v "* master"') do git branch -D %%b

REM Step 4: Delete all remote branches except master
for /f "tokens=*" %%b in ('git branch -r ^| findstr /v "origin/master"') do git push origin --delete %%b:~7

REM Step 5: Push master to GitHub

git push origin master

echo All old branches deleted, master updated and pushed to GitHub.
pause
