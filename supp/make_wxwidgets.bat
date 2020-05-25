@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir wxWidgets /s /q
unzip zip\wxwidgets.zip

cd wxWidgets\build\msw


if "%PLATFORM%"=="X64" goto x64
:x86
SET MSBUILD_PLATFORM=Win32
goto done
:x64
SET MSBUILD_PLATFORM=x64
goto done
:done

msbuild wx_vc16.sln /t:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%
msbuild wx_vc16.sln /t:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%

popd

:end
