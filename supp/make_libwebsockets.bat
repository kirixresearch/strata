@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir libwebsockets /s /q
unzip zip\libwebsockets.zip

cd libwebsockets

..\util\cmakewin32\bin\cmake.exe -G "Visual Studio 14 2015" -DOPENSSL_ROOT_DIR=..\openssl -DOPENSSL_INCLUDE_DIR=..\openssl\inc32 -DWITHOUT_DAEMONIZE=ON .
msbuild libwebsockets.sln /t:websockets:Rebuild /p:Configuration=Release
msbuild libwebsockets.sln /t:websockets:Rebuild /p:Configuration=Debug

popd

:end
