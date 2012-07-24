@echo off

set APPLICATION_NAME=Kirix Strata

call "c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat"
set WIX_PATH="c:\Program Files (x86)\Windows Installer XML v3.5\bin"
set SIGNCMD=d:\build32\cert\signtool sign /d "%APPLICATION_NAME%" /f d:\build32\cert\signcert.p12 /t http://timestamp.comodoca.com/authenticode
set MSGFMT="c:\Program Files (x86)\Poedit\bin\msgfmt"
set MSGCAT="c:\Program Files (x86)\Poedit\bin\msgcat"
SET YEAR=%DATE:~10,4%
SET MONTH=%DATE:~4,2%
SET DAY=%DATE:~7,2%

set BUILD_DRIVE=d:
set BUILD_BASE=d:\build32
set SOURCE_PATH=d:\build32\src\trunk
set CONFIG_PATH=d:\build32\src\trunk\appstrata\config
set VC_OUTPUT_PATH=d:\build32\src\trunk\releaseu
set SETUP_PATH=%SOURCE_PATH%\appstrata\setup
set WXS_NAME=strata
set WEBRES_DIR=%SOURCE_PATH%\appstrata\webres
set BUILD_PROJECT=appstrata
set BUILD_SLN=build32.sln
set BUILDUTIL=cscript /nologo %SOURCE_PATH%\appmain\buildutil.vbs %CONFIG_PATH% 

REM -- these variables are used by the WIX source files --
set BUILDSRC=d:\build32\buildsrc
set BUILDSRCXR=d:\build32\buildsrc\xr

REM -- first, put the current build number in the BUILD_CURRENT variable --
for /f "delims=" %%A in ('%BUILDUTIL% show_buildnum') do set BUILD_CURRENT=%%A
echo Current build number is '%BUILD_CURRENT%'
echo Log will be placed in file: %SOURCE_PATH%\build.log

set BUILD_OUTPUT_PATH=%BUILD_BASE%\builds\%BUILD_CURRENT%

REM -- s3 bucket variables
set S3BUCKET=builds.kirix.com/kirix-strata
set S3NAME=kirix-strata-%YEAR%-%MONTH%-%DAY%-build-%BUILD_CURRENT%.msi


REM -- make sure that the output path doesn't already exist --

if exist %BUILD_OUTPUT_PATH% (
    echo Error Build Output Path '%BUILD_OUTPUT_PATH%' already exists.
    goto end
)


REM -- change to the drive and driectory where the build will be made --

REM %BUILD_DRIVE%
REM cd %SOURCE_PATH%



REM -- get the latest from source control --

echo Getting latest from source control
svn update >nul



REM -- update the version info --

%BUILDUTIL% write_config %CONFIG_PATH%\build_config.h
%BUILDUTIL% write_config %SOURCE_PATH%\tango\xdcommon\build_config.h


REM -- clean --

erase %VC_OUTPUT_PATH%\appstrata.exe /f/q
erase %VC_OUTPUT_PATH%\gpasvc.exe /f/q
erase %VC_OUTPUT_PATH%\xdnative.dll /f/q
erase %VC_OUTPUT_PATH%\xdodbc.dll /f/q
erase %VC_OUTPUT_PATH%\xdoracle.dll /f/q
erase %VC_OUTPUT_PATH%\xdfs.dll /f/q

REM -- build the source tree --

echo Building...
REM %MSDEV% %BUILD_SLN% /MAKE "%BUILD_PROJECT% - Win32 Release Unicode"  /REBUILD /OUT build.log
msbuild %SOURCE_PATH%\%BUILD_SLN% /t:Rebuild /p:Configuration=Release


IF NOT EXIST %VC_OUTPUT_PATH%\appstrata.exe goto err
IF NOT EXIST %VC_OUTPUT_PATH%\gpasvc.exe goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdnative.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdodbc.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdoracle.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdfs.dll goto err

goto ok


:err

echo A build error occurred...The error log will be displayed.
pause
notepad build.log
goto end


:ok

REM -- zip up the graphics images --

IF EXIST %VC_OUTPUT_PATH%\imgres.zip erase %VC_OUTPUT_PATH%\imgres.zip
%SOURCE_PATH%\zip -0 -j %VC_OUTPUT_PATH%\imgres.zip %SOURCE_PATH%\img\*.xpm %SOURCE_PATH%\img\*.png

REM -- zip up the webres files --

IF EXIST %VC_OUTPUT_PATH%\webres.jar erase %VC_OUTPUT_PATH%\webres.jar
%SOURCE_PATH%\zip -j %VC_OUTPUT_PATH%\webres.jar %WEBRES_DIR%\*.*

REM -- make .mo translation files from the .po files
%MSGFMT% %SOURCE_PATH%\appmain\i18n\de\messages.po -o %SOURCE_PATH%\appmain\i18n\de\messages.mo


REM -- copy the files to build source --

copy %VC_OUTPUT_PATH%\appstrata.exe %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\gpasvc.exe %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdnative.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdodbc.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdoracle.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdfs.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\imgres.zip %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\webres.jar %BUILDSRC%\bin /Y
mkdir %BUILDSRC%\i18n\de >nul
copy %SOURCE_PATH%\appmain\i18n\de\messages.mo %BUILDSRC%\i18n\de



REM -- sign the files --

echo Signing build output files...
%SIGNCMD% %BUILDSRC%\bin\appstrata.exe
%SIGNCMD% %BUILDSRC%\bin\gpasvc.exe
%SIGNCMD% %BUILDSRC%\bin\xdnative.dll
%SIGNCMD% %BUILDSRC%\bin\xdfs.dll
%SIGNCMD% %BUILDSRC%\bin\xdoracle.dll
%SIGNCMD% %BUILDSRC%\bin\xdodbc.dll



if "%2"=="build_only" goto end


REM -- make the setup exe --

:build

echo Generating Installation...
erase %SETUP_PATH%\%WXS_NAME%_tmp.wxs /f /q 2>nul
erase %SETUP_PATH%\%WXS_NAME%.exe /f /q 2>nul
erase %SETUP_PATH%\%WXS_NAME%.msi /f /q 2>nul
erase %SETUP_PATH%\*.wixobj /f /q >nul
%BUILDUTIL% process_wix %SETUP_PATH%\%WXS_NAME%.wxs %SETUP_PATH%\%WXS_NAME%_tmp.wxs

heat dir %BUILDSRCXR% -var %BUILDSRCXR% -ke -gg -sreg -dr INSTALLDIR -cg xr -var env.buildsrcxr -out %SETUP_PATH%\xr.wxs
candle %SETUP_PATH%\xr.wxs -o %SETUP_PATH%\xr.wixobj
candle %SETUP_PATH%\%WXS_NAME%_tmp.wxs -o %SETUP_PATH%\%WXS_NAME%.wixobj

light -ext WixUIExtension %SETUP_PATH%\%WXS_NAME%.wixobj %SETUP_PATH%\xr.wixobj -out %SETUP_PATH%\%WXS_NAME%.msi
if not exist %SETUP_PATH%\%WXS_NAME%.msi (
    echo The MSI Build step failed -- .msi setup file missing
    goto end
)

erase %SETUP_PATH%\%WXS_NAME%_tmp.wxs /f /q 2>nul
erase %SETUP_PATH%\*.wixobj /f /q 2>nul

%SIGNCMD% %SETUP_PATH%\%WXS_NAME%.msi

REM -- make sure the setup output file exists --


if not "%S3BUCKET%"=="" (
copy %SETUP_PATH%\%WXS_NAME%.msi %TEMP%\%S3NAME%
s3 put %S3BUCKET%/ %TEMP%\%S3NAME%
erase %TEMP%\%S3NAME%
)


mkdir %BUILD_OUTPUT_PATH%
mkdir %BUILD_OUTPUT_PATH%\info
copy %SETUP_PATH%\%WXS_NAME%.msi %BUILD_OUTPUT_PATH%
copy %SOURCE_PATH%\appstrata\ReleaseUnicode\*.map %BUILD_OUTPUT_PATH%\info
copy %SOURCE_PATH%\tango\xdnative\ReleaseUnicode\*.map %BUILD_OUTPUT_PATH%\info
copy %SOURCE_PATH%\tango\xdfs\ReleaseUnicode\*.map %BUILD_OUTPUT_PATH%\info
copy %SOURCE_PATH%\tango\xdoracle\ReleaseUnicode\*.map %BUILD_OUTPUT_PATH%\info
copy %SOURCE_PATH%\tango\xdodbc\ReleaseUnicode\*.map %BUILD_OUTPUT_PATH%\info



REM -- increment the build number for the next build --

%BUILDUTIL% increment

REM -- open the build folder in explorer --
echo Build %BUILD_CURRENT% Complete at:
date /t
time /t
%BUILDUTIL% open %BUILD_OUTPUT_PATH%

pause

:end

