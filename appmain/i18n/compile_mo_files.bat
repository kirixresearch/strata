@echo off
setlocal enabledelayedexpansion

rem Compile all .po files to .mo files in the i18n folder
rem This script finds all language directories and compiles messages.po to messages.mo

echo Compiling .po files to .mo files...

rem Change to the directory of this script
pushd "%~dp0"

rem Counter for processed files
set /a PROCESSED=0
set /a SUCCESS=0
set /a FAILED=0

rem Check if msgfmt is available
where msgfmt >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: msgfmt not found in PATH
    echo Please install gettext tools or add them to your PATH
    echo Download from: https://mlocati.github.io/articles/gettext-iconv-windows.html
    popd
    exit /b 1
)

rem Loop through all directories in the current folder
for /d %%d in (*) do (
    rem Check if it's a language directory (contains messages.po)
    if exist "%%d\messages.po" (
        echo Processing %%d...
        set /a PROCESSED+=1
        
        rem Compile the .po file to .mo file
        msgfmt -o "%%d\messages.mo" "%%d\messages.po"
        
        if %ERRORLEVEL% equ 0 (
            echo   [OK] Successfully compiled %%d\messages.po to %%d\messages.mo
            set /a SUCCESS+=1
        ) else (
            echo   [ERROR] Failed to compile %%d\messages.po
            set /a FAILED+=1
        )
    )
)

rem Check if any .po files were found
if %PROCESSED% equ 0 (
    echo No .po files found in language directories
    echo Expected structure: lang_code\messages.po
    popd
    exit /b 1
)

rem Show summary
echo.
echo ========================================
echo COMPILATION SUMMARY
echo ========================================
echo Total directories processed: %PROCESSED%
echo Successfully compiled: %SUCCESS%
echo Failed: %FAILED%

if %FAILED% gtr 0 (
    echo.
    echo Some files failed to compile. Check the error messages above.
    popd
    exit /b 1
) else (
    echo.
    echo All .po files compiled successfully!
)

popd
exit /b 0
