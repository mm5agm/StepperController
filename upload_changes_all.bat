@echo off
REM Upload changes to StepperController and MagLoop_Common_Files if there are changes

REM Check for changes in MagLoop_Common_Files
cd MagLoop_Common_Files
git status --porcelain > ..\_magloop_status.txt
cd ..

REM Check for changes in main repo
git status --porcelain > _main_status.txt

setlocal enabledelayedexpansion
set magloop_changes=
set main_changes=
for /f %%i in (_magloop_status.txt) do set magloop_changes=1
for /f %%i in (_main_status.txt) do set main_changes=1

del _magloop_status.txt
 del _main_status.txt

if not defined magloop_changes if not defined main_changes (
    echo No changes to upload in either repository.
    goto end
)

if defined magloop_changes (
    echo Changes detected in MagLoop_Common_Files. Pushing...
    cd MagLoop_Common_Files
    set /p commitMsg="Enter commit message for MagLoop_Common_Files: "
    git add .
    git commit -m "%commitMsg%"
    git push
    cd ..
)

if defined main_changes (
    echo Changes detected in main StepperController repo. Pushing...
    set /p commitMsg2="Enter commit message for StepperController: "
    git add .
    git commit -m "%commitMsg2%"
    git push
)

:end
echo Upload process complete.
