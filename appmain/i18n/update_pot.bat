@echo off
rem ------------------------------------------------------------
rem  generate messages.pot for the Strata64 application (sorted)
rem  compatible with older gettext on Windows; avoids BOM
rem ------------------------------------------------------------

setlocal enabledelayedexpansion

set SRC_DIR=C:\src\strata64\appmain
set OUTPUT_DIR=%SRC_DIR%\i18n
set OUTPUT_FILE=%OUTPUT_DIR%\messages.pot
set TMP_FILE=%OUTPUT_FILE%.tmp

echo creating %OUTPUT_FILE%...

if exist "%TMP_FILE%" del "%TMP_FILE%"
if exist "%OUTPUT_FILE%" del "%OUTPUT_FILE%"

set FILES=
for /r "%SRC_DIR%" %%f in (*.cpp *.h *.hpp *.cxx) do (
    set FILES=!FILES! "%%f"
)

where xgettext >nul 2>&1
if errorlevel 1 (
    echo [error] xgettext not found in PATH.
    exit /b 1
)

rem extract .pot (older xgettext; no --output-charset)
xgettext ^
  --from-code=UTF-8 ^
  --language=C++ ^
  --keyword=_ ^
  --no-location ^
  --output="%TMP_FILE%" ^
  %FILES%

if errorlevel 1 (
    echo [error] failed to generate %TMP_FILE%.
    if exist "%TMP_FILE%" del "%TMP_FILE%"
    exit /b 1
)

rem patch header to UTF-8 WITHOUT BOM using .NET APIs
powershell -NoProfile -Command ^
  "$p='%TMP_FILE%';" ^
  "$text=[IO.File]::ReadAllText($p,[Text.Encoding]::UTF8);" ^
  "$text=$text -replace 'charset=CHARSET','charset=UTF-8';" ^
  "[IO.File]::WriteAllText($p,$text,(New-Object System.Text.UTF8Encoding($false)))"

where msgcat >nul 2>&1
if errorlevel 1 (
    echo [warn] msgcat not found; output may not be sorted.
    move /y "%TMP_FILE%" "%OUTPUT_FILE%" >nul
) else (
    msgcat --sort-output -o "%OUTPUT_FILE%" "%TMP_FILE%"
    if errorlevel 1 (
        echo [warn] msgcat failed; using unsorted output.
        move /y "%TMP_FILE%" "%OUTPUT_FILE%" >nul
    ) else (
        del "%TMP_FILE%"
    )
)

if exist "%OUTPUT_FILE%" (
    echo successfully generated: %OUTPUT_FILE%
    exit /b 0
) else (
    echo [error] %OUTPUT_FILE% not found after generation.
    exit /b 1
)

endlocal
