@echo off
REM Download latest code from StepperController and MagLoop_Common_Files

echo Updating main StepperController repo...
git pull

echo Updating MagLoop_Common_Files submodule...
cd MagLoop_Common_Files
git fetch origin
git checkout origin/main
cd ..

echo Done. Both repositories are now up to date.
