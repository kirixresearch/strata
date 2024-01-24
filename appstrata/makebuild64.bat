@echo off

set APPLICATION_NAME=Kirix Strata

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set WIX_PATH="c:\Program Files (x86)\WiX Toolset v3.11\bin"
REM set SIGNCMD=c:\build\cert\signtool sign /d "%APPLICATION_NAME%" /f c:\build\cert\signcert.p12 /t http://timestamp.comodoca.com/authenticode
set SIGNCMD=signtool sign /n "fopen GmbH" /d "%APPLICATION_NAME%" /tr http://time.certum.pl /td sha256 /fd sha256
set MSGFMT="c:\Program Files (x86)\Poedit\GettextTools\bin\msgfmt"
set MSGCAT="c:\Program Files (x86)\Poedit\GettextTools\bin\msgcat"
SET YEAR=%DATE:~10,4%
SET MONTH=%DATE:~4,2%
SET DAY=%DATE:~7,2%

set BUILD_DRIVE=c:
set BUILD_BASE=c:\build\x64
set SOURCE_PATH=c:\build\x64\src
set CONFIG_PATH=c:\build\x64\src\appstrata\config
set VC_OUTPUT_PATH=c:\build\x64\src\appstrata\releaseu
set SETUP_PATH=%SOURCE_PATH%\appstrata\setup
set WXS_NAME=strata64
set WEBRES_DIR=%SOURCE_PATH%\appstrata\webres
set BUILD_PROJECT=appstrata
set BUILD_SLN=%SOURCE_PATH%\appstrata\sln\build.sln
set BUILDUTIL=cscript /nologo c:\build\common\buildutil.vbs c:\build\common

REM -- these variables are used by the WIX source files --
set BUILDSRC=c:\build\x64\buildsrc
set BUILDSRCXR=c:\build\x64\buildsrc\xr

REM -- first, put the current build number in the BUILD_CURRENT variable --
for /f "delims=" %%A in ('%BUILDUTIL% show_buildnum') do set BUILD_CURRENT=%%A
echo Current build number is '%BUILD_CURRENT%'
echo Log will be placed in file: %SOURCE_PATH%\build.log

set BUILD_OUTPUT_PATH=%BUILD_BASE%\builds\%BUILD_CURRENT%

REM -- s3 bucket variables --
set S3BUCKET=s3://builds.kirix.com/kirix-strata/development/64-bit
set S3NAME=kirix-strata-64bit-%YEAR%-%MONTH%-%DAY%-build-%BUILD_CURRENT%.msi


REM -- make sure that the output path doesn't already exist --

if exist %BUILD_OUTPUT_PATH% (
    echo Error Build Output Path '%BUILD_OUTPUT_PATH%' already exists.
    goto end
)


REM -- change to the drive and driectory where the build will be made --

%BUILD_DRIVE%
cd %SOURCE_PATH%



REM -- get the latest from source control --

echo Getting latest from source control
git pull


REM -- update the version info --

%BUILDUTIL% write_config %CONFIG_PATH%\build_config.h
%BUILDUTIL% write_config %SOURCE_PATH%\xd\xdcommon\build_config.h


REM -- clean --

erase %VC_OUTPUT_PATH%\appstrata.exe /f/q 2> nul
erase %VC_OUTPUT_PATH%\gpasvc.exe /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdfs.dll /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdkpg.dll /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdexcel.dll /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdmysql.dll /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdnative.dll /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdodbc.dll /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdoracle.dll /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdpgsql.dll /f/q 2> nul
erase %VC_OUTPUT_PATH%\xdsqlite.dll /f/q 2> nul


REM -- build the source tree --

echo Building...
msbuild %BUILD_SLN% /t:Rebuild /p:Configuration=Release /p:Platform=x64


IF NOT EXIST %VC_OUTPUT_PATH%\appstrata.exe goto err
IF NOT EXIST %VC_OUTPUT_PATH%\gpasvc.exe goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdfs.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdkpg.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdexcel.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdmysql.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdnative.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdodbc.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdoracle.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdpgsql.dll goto err
IF NOT EXIST %VC_OUTPUT_PATH%\xdsqlite.dll goto err

goto ok


:err

echo A build error occurred...The error log will be displayed.
pause
notepad build.log
goto end


:ok

REM -- zip up the graphics images --

IF EXIST %VC_OUTPUT_PATH%\imgres.zip erase %VC_OUTPUT_PATH%\imgres.zip
%SOURCE_PATH%\zip -0 -j %VC_OUTPUT_PATH%\imgres.zip %SOURCE_PATH%\appstrata\img\*.svg %SOURCE_PATH%\appstrata\img\*.png

REM -- make .mo translation files from the .po files
%MSGFMT% %SOURCE_PATH%\appmain\i18n\de\messages.po -o %SOURCE_PATH%\appmain\i18n\de\messages.mo


REM -- copy the files to build source --

copy %VC_OUTPUT_PATH%\appstrata.exe %BUILDSRC%\bin\kstrata.exe /Y
copy %VC_OUTPUT_PATH%\gpasvc.exe %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdfs.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdkpg.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdexcel.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdmysql.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdnative.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdodbc.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdoracle.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdpgsql.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\xdsqlite.dll %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\imgres.zip %BUILDSRC%\bin /Y
copy %VC_OUTPUT_PATH%\webres.jar %BUILDSRC%\bin /Y

xcopy %WEBRES_DIR%\*.* %BUILDSRC%\webres /s/e/r/v/k/f/c/h

mkdir %BUILDSRC%\i18n\de >nul
copy %SOURCE_PATH%\appmain\i18n\de\messages.mo %BUILDSRC%\i18n\de



REM -- sign the files --

echo Signing build output files...
%SIGNCMD% %BUILDSRC%\bin\kstrata.exe
%SIGNCMD% %BUILDSRC%\bin\gpasvc.exe
%SIGNCMD% %BUILDSRC%\bin\xdfs.dll
%SIGNCMD% %BUILDSRC%\bin\xdkpg.dll
%SIGNCMD% %BUILDSRC%\bin\xdexcel.dll
%SIGNCMD% %BUILDSRC%\bin\xdmysql.dll
%SIGNCMD% %BUILDSRC%\bin\xdnative.dll
%SIGNCMD% %BUILDSRC%\bin\xdodbc.dll
%SIGNCMD% %BUILDSRC%\bin\xdoracle.dll
%SIGNCMD% %BUILDSRC%\bin\xdpgsql.dll
%SIGNCMD% %BUILDSRC%\bin\xdsqlite.dll




if "%2"=="build_only" goto end


REM -- make the setup exe --

:build

echo Generating Installation...
erase %SETUP_PATH%\%WXS_NAME%_tmp.wxs /f /q 2>nul
erase %SETUP_PATH%\%WXS_NAME%.exe /f /q 2>nul
erase %SETUP_PATH%\%WXS_NAME%.msi /f /q 2>nul
erase %SETUP_PATH%\*.wixobj /f /q >nul
%BUILDUTIL% process_wix %SETUP_PATH%\%WXS_NAME%.wxs %SETUP_PATH%\%WXS_NAME%_tmp.wxs

REM %WIX_PATH%\heat dir %BUILDSRCXR% -var %BUILDSRCXR% -ke -gg -sreg -dr INSTALLDIR -cg xr -var env.buildsrcxr -out %SETUP_PATH%\xr.wxs
REM %WIX_PATH%\candle %SETUP_PATH%\xr.wxs -arch x64 -o %SETUP_PATH%\xr.wixobj
%WIX_PATH%\candle %SETUP_PATH%\%WXS_NAME%_tmp.wxs -arch x64 -o %SETUP_PATH%\%WXS_NAME%.wixobj

%WIX_PATH%\light -ext WixUIExtension %SETUP_PATH%\%WXS_NAME%.wixobj -out %SETUP_PATH%\%WXS_NAME%.msi
if not exist %SETUP_PATH%\%WXS_NAME%.msi (
    echo The MSI Build step failed -- .msi setup file missing
    goto end
)

erase %SETUP_PATH%\%WXS_NAME%_tmp.wxs /f /q 2>nul
erase %SETUP_PATH%\*.wixobj /f /q 2>nul

%SIGNCMD% %SETUP_PATH%\%WXS_NAME%.msi

REM -- make sure the setup output file exists --


if not "%S3BUCKET%"=="" (
echo Uploading to S3...
copy %SETUP_PATH%\%WXS_NAME%.msi %TEMP%\%S3NAME%
REM s3 put /  /acl:public-read
aws s3 cp %TEMP%\%S3NAME% %S3BUCKET%/ --acl public-read
echo Build can be downloaded here: %S3BUCKET%/%S3NAME%
erase %TEMP%\%S3NAME%
)


mkdir %BUILD_OUTPUT_PATH%
mkdir %BUILD_OUTPUT_PATH%\info
copy %SETUP_PATH%\%WXS_NAME%.msi %BUILD_OUTPUT_PATH%
copy %VC_OUTPUT_PATH%\*.map %BUILD_OUTPUT_PATH%\info



REM -- open the build folder in explorer --
echo Build %BUILD_CURRENT% Complete at:
date /t
time /t
%BUILDUTIL% open %BUILD_OUTPUT_PATH%


:end
