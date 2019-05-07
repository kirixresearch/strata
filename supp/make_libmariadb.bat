@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir libmariadb /s /q
unzip zip\libmariadb.zip

cd libmariadb

if "%PLATFORM%"=="X64" goto x64

:x86
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 14 2015" .
goto done

:x64
%~dp0\util\cmakewin32\bin\cmake -G "Visual Studio 14 2015" -DCMAKE_GENERATOR_PLATFORM=x64 .
goto done

:done


cscript %~dp0\util\replace.vbs %~dp0\libmariadb\libmariadb\mariadbclient.vcxproj ">MultiThreadedDebug<" ">MultiThreadedDebugDll<"
cscript %~dp0\util\replace.vbs %~dp0\libmariadb\libmariadb\mariadbclient.vcxproj ">MultiThreaded<" ">MultiThreadedDll<"

rem following was causing problems in vc2015
rem cscript %~dp0\util\replace.vbs %~dp0\libmariadb\include\my_pthread.h "#ifndef _TIMESPEC_DEFINED" "#ifdef XXXXXXXX"

msbuild mariadb-connector-c.sln /t:mariadbclient:Rebuild /p:Configuration=Release
msbuild mariadb-connector-c.sln /t:mariadbclient:Rebuild /p:Configuration=Debug
msbuild mariadb-connector-c.sln /t:libmariadb:Rebuild /p:Configuration=Release
msbuild mariadb-connector-c.sln /t:libmariadb:Rebuild /p:Configuration=Debug
mkdir %~dp0\..\debugu
mkdir %~dp0\..\releaseu
copy %~dp0\libmariadb\libmariadb\Debug\libmariadb.dll %~dp0\..\debugu
copy %~dp0\libmariadb\libmariadb\Release\libmariadb.dll %~dp0\..\releaseu


popd

:end
