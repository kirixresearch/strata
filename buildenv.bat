@echo off
set "SCRIPT_DIR=%~dp0"

if not exist "%SCRIPT_DIR%supp/util/perl/portableshell.bat" (
    echo supp directory has not been setup yet.
    echo Please run: cd supp ^&^& setup_supp.bat
    exit /b 1
)

if "%1"=="x64" (
    title x64 Build Environment
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && call "%SCRIPT_DIR%postgres/util/perl/portableshell.bat"
) else if "%1"=="x86" (
    title x86 Build Environment
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" && call "%SCRIPT_DIR%postgres/util/perl/portableshell.bat"
) else (
    echo Please specify architecture: x64 or x86
    exit /b 1
)

call "%SCRIPT_DIR%supp/util/perl/portableshell.bat"

set SUPP_ENV=true
