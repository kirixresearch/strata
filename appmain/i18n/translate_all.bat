@echo off
setlocal enabledelayedexpansion

rem Usage: translate_all LANG_CODE
rem Example: translate_all es

if "%~1"=="" (
    echo Usage: %~n0 LANG_CODE
    echo Example: %~n0 es
    exit /b 1
)

set "LANG=%~1"

rem Change to the directory of this script
pushd "%~dp0"

rem Load configuration from config.env if it exists
if exist "config.env" (
    echo Loading configuration from config.env...
    for /f "usebackq tokens=1,2 delims==" %%a in ("config.env") do (
        if not "%%a"=="" if not "%%a:~0,1%"=="#" (
            set "%%a=%%b"
        )
    )
) else (
    echo Warning: config.env not found. Using environment variables only.
    echo Create config.env from config.env.example and set your API keys.
)

rem Check if OPENAI_API_KEY is set
if "%OPENAI_API_KEY%"=="" (
    echo.
    echo ERROR: OPENAI_API_KEY is not set!
    echo.
    echo Please either:
    echo 1. Set the OPENAI_API_KEY environment variable, or
    echo 2. Create config.env from config.env.example and set your API key there
    echo.
    echo Get your API key from: https://platform.openai.com/api-keys
    echo.
    popd
    exit /b 1
)

rem Check if the API key is not just placeholder text
if "%OPENAI_API_KEY%"=="your_openai_api_key_here" (
    echo.
    echo ERROR: OPENAI_API_KEY is set to placeholder value!
    echo.
    echo Please set your actual OpenAI API key in config.env
    echo Get your API key from: https://platform.openai.com/api-keys
    echo.
    popd
    exit /b 1
)

rem Change to the directory of this script
pushd "%~dp0"

rem Create messages.pot if it doesn't exist
if not exist "messages.pot" (
    echo messages.pot not found, creating it...
    call "%~dp0update_pot.bat"
    if errorlevel 1 (
        echo Error: failed to create messages.pot
        popd
        exit /b 1
    )
)

rem Create workdir if it doesn't exist
if not exist "workdir" (
    mkdir "workdir"
    if errorlevel 1 (
        echo Error: failed to create workdir directory
        popd
        exit /b 1
    )
)

rem Create language directory in workdir if it doesn't exist
if not exist "workdir\%LANG%" (
    mkdir "workdir\%LANG%"
    if errorlevel 1 (
        echo Error: failed to create workdir\%LANG% directory
        popd
        exit /b 1
    )
)

rem Copy messages.pot to workdir\{lang}\messages.po
copy /y "messages.pot" "workdir\%LANG%\messages.po" >nul
if errorlevel 1 (
    echo Error: failed to copy messages.pot to workdir\%LANG%\messages.po
    popd
    exit /b 1
)

rem Change to the language directory
pushd "workdir\%LANG%"

rem Set the language header using our Python script
echo Setting language header to %LANG%...
python "..\..\set_po_header.py" "messages.po" --header "Language: %LANG%"
if errorlevel 1 (
    echo Error: failed to set language header
    popd
    popd
    exit /b 1
)

rem Get the full path to workdir for gpt-po-translator
for %%i in ("%~dp0workdir") do set "WORKDIR_FULL_PATH=%%~fi"

rem Set formal register language details
set "DETAIL_LANG="
if /I "%LANG%"=="de" set "DETAIL_LANG=German (Germany) with formal Sie"
if /I "%LANG%"=="es" set "DETAIL_LANG=Spanish (Spain) with formal Usted or neutral form"
if /I "%LANG%"=="fr" set "DETAIL_LANG=French (France) with formal Vous"
if /I "%LANG%"=="it" set "DETAIL_LANG=Italian (Italy) with formal Lei"
if /I "%LANG%"=="pt" set "DETAIL_LANG=Portuguese (Portugal) with professional tone"

rem Get the path to gpt-po-translator.cmd in the supp directory
set "GPT_PO_TRANSLATOR=%~dp0..\..\supp\gpt-po-translator\gpt-po-translator.cmd"

rem Check if the translator exists
if not exist "%GPT_PO_TRANSLATOR%" (
    echo Error: gpt-po-translator not found at %GPT_PO_TRANSLATOR%
    echo Please run supp\make_gpt_po_tranlator.bat first to set up the translator
    popd
    popd
    exit /b 1
)

rem Run gpt-po-translator
echo Running gpt-po-translator for %LANG%...
if defined DETAIL_LANG (
    echo call "%GPT_PO_TRANSLATOR%" --folder "%WORKDIR_FULL_PATH%" --lang "%LANG%" --bulk --no-ai-comment--detail-lang "%DETAIL_LANG%"
    call "%GPT_PO_TRANSLATOR%" --folder "%WORKDIR_FULL_PATH%" --lang "%LANG%" --bulk --no-ai-comment --detail-lang "%DETAIL_LANG%"
) else (
    echo call "%GPT_PO_TRANSLATOR%" --folder "%WORKDIR_FULL_PATH%" --lang "%LANG%" --bulk --no-ai-comment
    call "%GPT_PO_TRANSLATOR%" --folder "%WORKDIR_FULL_PATH%" --lang "%LANG%" --bulk --no-ai-comment
)
set "ERR=%ERRORLEVEL%"

rem Return to original directory
popd
popd

if "%ERR%"=="0" (
    echo Translation completed successfully for %LANG%
) else (
    echo Translation failed for %LANG% with error code %ERR%
)

exit /b %ERR%
