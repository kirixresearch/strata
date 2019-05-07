@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir wxWidgets /s /q
unzip zip\wxwidgets.zip

cd wxWidgets\build\msw

msbuild wx_vc12.sln /t:Rebuild /p:Configuration=Debug
msbuild wx_vc12.sln /t:Rebuild /p:Configuration=Release

popd

:end
