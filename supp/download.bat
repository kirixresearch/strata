@echo off

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

if not exist "%SCRIPT_DIR%\zip\strawberry-perl-5.38.2.2-64bit-portable.zip" (
    curl -L -o "%SCRIPT_DIR%\zip\strawberry-perl-5.38.2.2-64bit-portable.zip" https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_53822_64bit/strawberry-perl-5.38.2.2-64bit-portable.zip
)

if not exist "%SCRIPT_DIR%\zip\strawberry-perl-5.40.0.1-64bit-portable.zip" (
    curl -L -o "%SCRIPT_DIR%\zip\strawberry-perl-5.40.0.1-64bit-portable.zip" https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_54001_64bit_UCRT/strawberry-perl-5.40.0.1-64bit-portable.zip
)


if not exist "%SCRIPT_DIR%\zip\postgres-15.10.zip" (
    curl -L -o "%SCRIPT_DIR%\zip\postgres-15.10.zip" https://github.com/postgres/postgres/archive/refs/tags/REL_15_10.zip
)

