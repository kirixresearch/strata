@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir xlnt /s /q
unzip zip\xlnt.zip

cd xlnt

if "%VisualStudioVersion%"=="16.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 16 2019
if "%VisualStudioVersion%"=="17.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 17 2022

if "%PLATFORM%"=="x64" goto x64
:x86
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DSTATIC=ON -DCMAKE_GENERATOR_PLATFORM=Win32 .
SET MSBUILD_PLATFORM=Win32
goto done
:x64
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DSTATIC=ON -DCMAKE_GENERATOR_PLATFORM=x64 .
SET MSBUILD_PLATFORM=x64
goto done

:done

msbuild xlnt_all.sln /t:ALL_BUILD:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%
msbuild xlnt_all.sln /t:ALL_BUILD:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%

popd

:end
