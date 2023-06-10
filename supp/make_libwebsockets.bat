@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir libwebsockets /s /q
unzip zip\libwebsockets.zip

cd libwebsockets

if "%VisualStudioVersion%"=="16.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 16 2019
if "%VisualStudioVersion%"=="17.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 17 2022

if "%PLATFORM%"=="x64" goto x64
:x86
SET OPENSSL_ROOT_DIR=%~dp0\openssl
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DOPENSSL_ROOT_DIR=%~dp0\openssl -DOPENSSL_INCLUDE_DIR=%~dp0\openssl\inc32 -DWITHOUT_DAEMONIZE=ON -DCMAKE_GENERATOR_PLATFORM=Win32 .
SET MSBUILD_PLATFORM=Win32
goto done
:x64
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DOPENSSL_ROOT_DIR=%~dp0\openssl -DOPENSSL_INCLUDE_DIR=%~dp0\openssl\inc32 -DWITHOUT_DAEMONIZE=ON -DCMAKE_GENERATOR_PLATFORM=x64 .
SET MSBUILD_PLATFORM=x64
goto done
:done


msbuild libwebsockets.sln /t:websockets:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%
msbuild libwebsockets.sln /t:websockets:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%

popd

:end
