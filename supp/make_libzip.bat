@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir libzip /s /q
tar zxvf zip/libzip-1.10.1.tar.gz
ren libzip-1.10.1 libzip
cd libzip



if "%VisualStudioVersion%"=="16.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 16 2019
if "%VisualStudioVersion%"=="17.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 17 2022

if "%PLATFORM%"=="x64" goto x64
:x86
SET MSBUILD_PLATFORM=Win32
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DBUILD_SHARED_LIBS=OFF -DZLIB_LIBRARY:FILEPATH="%~dp0/zlib/Debug/zlib.lib" -DZLIB_INCLUDE_DIR:PATH="%~dp0/zlib" -DCMAKE_GENERATOR_PLATFORM=Win32 .
msbuild libzip.sln /t:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DBUILD_SHARED_LIBS=OFF -DZLIB_LIBRARY:FILEPATH="%~dp0/zlib/Release/zlib.lib" -DZLIB_INCLUDE_DIR:PATH="%~dp0/zlib" -DCMAKE_GENERATOR_PLATFORM=Win32 .
msbuild libzip.sln /t:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%
goto done
:x64
SET MSBUILD_PLATFORM=x64
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DBUILD_SHARED_LIBS=OFF -DZLIB_LIBRARY:FILEPATH="%~dp0/zlib/Debug/zlib.lib" -DZLIB_INCLUDE_DIR:PATH="%~dp0/zlib" -DCMAKE_GENERATOR_PLATFORM=x64 .
msbuild libzip.sln /t:ALL_BUILD:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DBUILD_SHARED_LIBS=OFF -DZLIB_LIBRARY:FILEPATH="%~dp0/zlib/Release/zlib.lib" -DZLIB_INCLUDE_DIR:PATH="%~dp0/zlib" -DCMAKE_GENERATOR_PLATFORM=x64 .
msbuild libzip.sln /t:ALL_BUILD:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%
goto done
:done




popd

:end
