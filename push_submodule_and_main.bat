@echo off
REM Commit and push submodule and main repo changes

git add MagLoop_Common_Files
REM Add other changes in main repo
git add .

set /p commitMsg="Enter commit message: "
git commit -m "%commitMsg%"
git push

echo Submodule and main repo changes pushed to remote.
