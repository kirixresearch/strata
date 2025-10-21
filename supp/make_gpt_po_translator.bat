@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"

echo Cloning python-gpt-po repository...
rmdir gpt-po-translator /s /q 2> nul
git clone git@github.com:pescheckit/python-gpt-po.git gpt-po-translator

if %ERRORLEVEL% neq 0 (
    echo error: failed to clone repository
    goto end
)

echo Repository cloned successfully to gpt-po-translator directory

echo Modifying translation service for database application context...
powershell -Command "(Get-Content 'gpt-po-translator\python_gpt_po\services\translation_service.py') -replace '{target_lang_text}\.', '{target_lang_text}. Use terms associated with database applications.' | Set-Content 'gpt-po-translator\python_gpt_po\services\translation_service.py'"

echo Creating gpt-po-translator.cmd...
(
echo @echo off
echo REM GPT-PO Translator Runner
echo REM This script runs the GPT-PO translator and passes through all command line parameters
echo.
echo REM Get the directory where this script is located
echo set SCRIPT_DIR=%%~dp0
echo.
echo REM Run the translator with all passed parameters using the full path
echo python "%%SCRIPT_DIR%%python_gpt_po\main.py" %%*
echo.
echo REM Check if the command was successful
echo if %%ERRORLEVEL%% neq 0 ^(
echo     echo.
echo     echo Error: Translation failed with exit code %%ERRORLEVEL%%
echo     pause
echo     exit /b %%ERRORLEVEL%%
echo ^)
echo.
echo echo.
echo echo Translation completed successfully.
) > gpt-po-translator\gpt-po-translator.cmd

popd

:end
