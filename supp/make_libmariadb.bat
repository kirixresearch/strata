@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir libmariadb /s /q
util\unzip zip\libmariadb.zip

cd libmariadb

if "%VisualStudioVersion%"=="16.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 16 2019
if "%VisualStudioVersion%"=="17.0" SET CMAKE_VISUAL_STUDIO_VERSION=Visual Studio 17 2022

if "%PLATFORM%"=="x64" goto x64
:x86
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DCMAKE_GENERATOR_PLATFORM=Win32 .
SET MSBUILD_PLATFORM=Win32
goto done
:x64
%~dp0\util\cmakewin32\bin\cmake -G "%CMAKE_VISUAL_STUDIO_VERSION%" -DCMAKE_GENERATOR_PLATFORM=x64 .
SET MSBUILD_PLATFORM=x64
goto done
:done

cscript %~dp0\util\replace.vbs %~dp0\libmariadb\libmariadb\mariadbclient.vcxproj ">MultiThreadedDebug<" ">MultiThreadedDebugDll<"
cscript %~dp0\util\replace.vbs %~dp0\libmariadb\libmariadb\mariadbclient.vcxproj ">MultiThreaded<" ">MultiThreadedDll<"
msbuild mariadb-connector-c.sln /t:mariadbclient:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%
msbuild mariadb-connector-c.sln /t:mariadbclient:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%
msbuild mariadb-connector-c.sln /t:libmariadb:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%
msbuild mariadb-connector-c.sln /t:libmariadb:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%

rem following was causing problems in vc2015
rem cscript %~dp0\util\replace.vbs %~dp0\libmariadb\include\my_pthread.h "#ifndef _TIMESPEC_DEFINED" "#ifdef XXXXXXXX"


mkdir %~dp0\..\debugu
mkdir %~dp0\..\releaseu
copy %~dp0\libmariadb\libmariadb\Debug\libmariadb.dll %~dp0\..\debugu
copy %~dp0\libmariadb\libmariadb\Release\libmariadb.dll %~dp0\..\releaseu

popd

:end
