@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)


call cmd /c make_openssl
call make_curl
call make_postgres32
call make_postgres
call make_libmariadb
call make_wxwidgets
call make_libharu
call make_zlib
call make_libzip
REM call make_libwebsockets
call make_expat
call make_xlnt
call make_openxlsx
call make_oracle


:end
