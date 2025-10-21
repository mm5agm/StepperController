@echo off
REM Batch script to delete all remote branches except 'main' from your GitHub repository
REM and clean up local branches as well.

REM List of branches to delete (add more if needed)
set branches_to_delete=master docs/nvs-migration

for %%B in (%branches_to_delete%) do (
    echo Deleting remote branch %%B ...
    git push origin --delete %%B
    echo Deleting local branch %%B ...
    git branch -d %%B
)

echo Only 'main' branch should remain on remote and local.
echo All old branches deleted, main updated and pushed to GitHub.
pause
