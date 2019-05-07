@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir libharu /s /q
unzip zip\libharu.zip

pushd libharu

if "%PLATFORM%"=="X64" goto x64

:x86
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 14 2015" .
goto done

:x64
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 14 2015" -DCMAKE_GENERATOR_PLATFORM=x64 .
goto done

:done


msbuild libharu.sln /t:libhpdfs:Rebuild /p:Configuration=Release
msbuild libharu.sln /t:libhpdfs:Rebuild /p:Configuration=Debug

popd
popd

:end
