@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir wxWidgets /s /q
unzip zip\wxwidgets.zip
unzip zip\microsoft.web.webview2.1.0.2210.55.nupkg -d wxWidgets\3rdparty\webview2

pushd wxWidgets\include\wx\msw
cscript %~dp0\util\replace.vbs setup.h "wxUSE_WEBVIEW_IE 1" "wxUSE_WEBVIEW_IE 0"
cscript %~dp0\util\replace.vbs setup.h "wxUSE_WEBVIEW_EDGE 0" "wxUSE_WEBVIEW_EDGE 1"
cscript %~dp0\util\replace.vbs setup.h "wxUSE_WEBVIEW_EDGE_STATIC 0" "wxUSE_WEBVIEW_EDGE_STATIC 1"
popd

if "%PLATFORM%"=="x64" goto x64
:x86
SET MSBUILD_PLATFORM=Win32
goto done
:x64
SET MSBUILD_PLATFORM=x64
goto done
:done

cd wxWidgets\build\msw

msbuild wx_vc17.sln /t:Rebuild /p:Configuration=Debug /p:Platform=%MSBUILD_PLATFORM%
msbuild wx_vc17.sln /t:Rebuild /p:Configuration=Release /p:Platform=%MSBUILD_PLATFORM%

popd

:end
