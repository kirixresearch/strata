@echo off

set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%download.bat"

pushd "%~dp0\util"
rmdir cmakewin32 /s/q 2>nul
rmdir perlwin32 /s/q 2>nul
..\..\unzip cmakewin32.zip

mkdir perl
pushd perl
..\..\..\unzip ..\..\zip\strawberry-perl-5.40.0.1-64bit-portable.zip
popd

popd

