@echo off
REM Update MagLoop_Common_Files submodule to latest remote commit
cd MagLoop_Common_Files

git fetch origin
REM Checkout latest commit from main branch
git checkout origin/main
cd ..
REM Stage submodule update in main repo
git add MagLoop_Common_Files

echo MagLoop_Common_Files submodule updated to latest remote commit.
