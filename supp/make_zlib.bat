@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir zlib /s /q
unzip zip\zlib.zip

cd zlib

nmake -f win32\Makefile_Debug.msc clean all
mkdir Debug
copy *.lib Debug
copy *.dll Debug
copy *.pdb Debug

nmake -f win32\Makefile.msc clean all
mkdir Release
copy *.lib Release
copy *.dll Release
copy *.pdb Release

erase *.lib /f
erase *.pdb /f
erase *.dll /f

popd

:end
