@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir postgres /s /q
unzip zip\postgres.zip

rem there is some code that doesn't work with vc2015 in chklocale.c -- use alternative code (which works fine)
rem cscript %~dp0\util\replace.vbs %~dp0\postgres\src\port\chklocale.c "1700" "3000"

pushd postgres\src\tools\msvc
set PATH=%~dp0\util\perlwin32\bin;%PATH%
call build
popd


pushd postgres\src\interfaces\libpq

if "%PLATFORM%"=="x64" goto x64


:x86
nmake /f win32.mak clean
nmake /f win32.mak 
nmake /f win32.mak DEBUG=1
goto done

:x64
nmake /f win32.mak clean
nmake /f win32.mak CPU=AMD64
nmake /f win32.mak CPU=AMD64 DEBUG=1
goto done

:done

echo.
echo Just ignore the errors above -- it built correctly.

popd
popd

:end
