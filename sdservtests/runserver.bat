@echo off

SET batchdir=%~dp0
SET batchdir=%batchdir:~0,-1%

IF NOT EXIST %batchdir%\..\releaseu\sdserv.exe GOTO notexist


%batchdir%\..\releaseu\sdserv -d xdprovider=xdfs;database=%batchdir% -p 28000
goto end


:notexist
echo Please make sure a release mode sdserv is compiled

:end