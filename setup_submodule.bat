@echo off
setlocal enabledelayedexpansion

REM ===== CONFIG =====
set VISIBILITY=--public
set GITHUB_USER=YOSRA-BS
set BRANCH=main

REM Get parent folder name
for %%I in (.) do set PARENT_NAME=%%~nxI

echo Parent repo: %PARENT_NAME%
echo GitHub user: %GITHUB_USER%
echo.

REM ===== STEP 1: Create repos for subfolders =====
for /d %%D in (*) do (
    if not "%%D"=="." (
        echo Processing subfolder: %%D
        pushd %%D

        if not exist ".git" (
            git init
            git add .
            git commit -m "Initial commit"
            gh repo create %%D %VISIBILITY% --source=. --remote=origin --push
        )

        popd
    )
)

REM ===== STEP 2: Remove embedded git dirs =====
for /d %%D in (*) do (
    if exist "%%D\.git" (
        rmdir /s /q "%%D\.git"
    )
)

REM ===== STEP 3: Init parent repo =====
if not exist ".git" (
    git init
    gh repo create %PARENT_NAME% %VISIBILITY% --source=. --remote=origin
)

REM ===== STEP 4: Add submodules =====
for /d %%D in (*) do (
    if not "%%D"=="." (
        echo Adding submodule %%D
        git submodule add https://github.com/%GITHUB_USER%/%%D.git %%D
    )
)

REM ===== STEP 5: Commit & push =====
git add .
git commit -m "Add submodules"
git branch -M %BRANCH%
git push -u origin %BRANCH%

echo.
echo ✅ Done. Parent repo and submodules are now public on GitHub.
pause