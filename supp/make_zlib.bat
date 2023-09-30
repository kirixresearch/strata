@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir zlib /s /q
tar zxvf zip/zlib-1.3.tar.gz
ren zlib-1.3 zlib

cd zlib

copy %~dp0\zlib\win32\Makefile.msc %~dp0\zlib\win32\Makefile_Debug.msc
cscript %~dp0\util\replace.vbs %~dp0\zlib\win32\Makefile_Debug.msc "-nologo -MD" "-nologo -MDd"
cscript %~dp0\util\replace.vbs %~dp0\zlib\win32\Makefile.msc "-nologo -debug -incremental:no" "-nologo -incremental:no"

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

