@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd %~dp0
rmdir openssl /s /q
unzip zip\openssl.zip

cd openssl
mkdir build
mkdir build\release
mkdir build\debug

REM the replacement of Git\usr is to avoid rmdir.exe, which throws off openssl's makefile clean
set PATH=%~dp0\util\perlwin32\bin;%PATH:Git\usr=EMPTY%
set


perl Configure VC-WIN32 --prefix=%CD% no-asm
nmake -f makefile all
pushd %~dp0\openssl
move *.lib build\release
move *.dll build\release
popd


pushd %~dp0\openssl
nmake -f makefile clean
popd


perl Configure debug-VC-WIN32 --prefix=%CD% no-asm
nmake -f makefile all
pushd %~dp0\openssl
move *.lib build\debug
move *.dll build\debug
popd


REM this makes it easier for libwebsockets to find openssl's lib directory
rem mkdir lib
rem copy out32\*.* lib
popd

:end
