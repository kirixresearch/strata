@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir libharu /s /q
unzip zip\libharu.zip

pushd libharu

if "%VisualStudioVersion%"=="16.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 16 2019
if "%VisualStudioVersion%"=="17.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 17 2022

if "%PLATFORM%"=="X64" goto x64
:x86
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DCMAKE_GENERATOR_PLATFORM=Win32 .
SET MSBUILD_PLATFORM=Win32
goto done
:x64
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DCMAKE_GENERATOR_PLATFORM=x64 .
SET MSBUILD_PLATFORM=x64
goto done
:done

msbuild libharu.sln /t:libhpdfs:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%
msbuild libharu.sln /t:libhpdfs:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%

popd
popd

:end
