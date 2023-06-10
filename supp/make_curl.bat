@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir curl /s /q
unzip zip\curl.zip

rmdir deps /s /q 2> nul
mkdir deps\include\openssl
mkdir deps\lib
xcopy openssl\inc32\*.* deps\include /e
xcopy openssl\out32\*.lib deps\lib
cd curl\winbuild

if "%PLATFORM%"=="x64" goto x64

:x86
nmake /f makefile.vc mode=static VC=14 DEBUG=no ENABLE_IDN=no ENABLE_WINSSL=yes
nmake /f makefile.vc mode=static VC=14 DEBUG=yes ENABLE_IDN=no ENABLE_WINSSL=yes
goto done

:x64
nmake /f makefile.vc mode=static VC=14 DEBUG=no ENABLE_IDN=no ENABLE_WINSSL=yes MACHINE=x64
nmake /f makefile.vc mode=static VC=14 DEBUG=yes ENABLE_IDN=no ENABLE_WINSSL=yes MACHINE=x64
goto done

:done

popd

:end
