@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir expat /s /q
unzip zip\expat.zip

pushd expat

if "%PLATFORM%"=="X64" goto x64

:x86
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 14 2015" -DBUILD_shared=OFF .
msbuild expat.sln /t:expat:Rebuild /p:Configuration=Release
msbuild expat.sln /t:expat:Rebuild /p:Configuration=Debug
goto done

:x64
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 14 2015" -DBUILD_shared=OFF -DCMAKE_GENERATOR_PLATFORM=x64 .
msbuild expat.sln /t:expat:Rebuild /p:Configuration=Release
msbuild expat.sln /t:expat:Rebuild /p:Configuration=Debug
goto done

:done


popd
popd

:end
