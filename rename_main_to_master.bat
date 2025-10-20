@echo off
REM Rename 'main' branch to 'master' locally and on GitHub

REM Step 1: Checkout main branch

git checkout main

REM Step 2: Rename main to master locally

git branch -m main master

REM Step 3: Push master branch to GitHub

git push origin master

REM Step 4: Set upstream for master

git push --set-upstream origin master

REM Step 5: Delete main branch on GitHub

git push origin --delete main

echo Branch 'main' renamed to 'master' locally and on GitHub.
pause
