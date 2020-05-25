@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir libzip /s /q
unzip zip\libzip.zip

cd libzip

if "%PLATFORM%"=="X64" goto x64
:x86
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 16 2019" -DZLIB_LIBRARY:FILEPATH="%~dp0/zlib/zlib" -DZLIB_INCLUDE_DIR:PATH="%~dp0/zlib" -DCMAKE_GENERATOR_PLATFORM=Win32 .
SET MSBUILD_PLATFORM=Win32
goto done
:x64
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 16 2019" -DZLIB_LIBRARY:FILEPATH="%~dp0/zlib/zlib" -DZLIB_INCLUDE_DIR:PATH="%~dp0/zlib" -DCMAKE_GENERATOR_PLATFORM=x64 .
SET MSBUILD_PLATFORM=x64
goto done
:done

msbuild libzip.sln /t:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%
msbuild libzip.sln /t:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%


popd

:end
