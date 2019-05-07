@echo off

pushd "%~dp0\util"
rmdir cmakewin32 /s/q 2>nul
rmdir perlwin32 /s/q 2>nul
..\..\unzip cmakewin32.zip
..\..\unzip perlwin32.zip
popd

