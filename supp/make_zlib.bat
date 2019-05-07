@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir zlib /s /q
unzip zip\zlib.zip

cd zlib

nmake -f win32\Makefile.msc clean all
nmake -f win32\Makefile_Debug.msc clean all

popd

:end
