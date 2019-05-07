@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir xlnt /s /q
unzip zip\xlnt.zip

cd xlnt

if "%PLATFORM%"=="X64" goto x64


:x86
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 14 2015" -DSTATIC=ON  .
goto done

:x64
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 14 2015" -DSTATIC=ON -DCMAKE_GENERATOR_PLATFORM=x64 .
goto done

:done

msbuild xlnt_all.sln /t:ALL_BUILD:Rebuild /p:Configuration=Release
msbuild xlnt_all.sln /t:ALL_BUILD:Rebuild /p:Configuration=Debug

popd

:end
