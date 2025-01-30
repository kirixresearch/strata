@echo off

if "%FrameworkDir%"=="" (
    echo error: please run this script from the Visual Studio Command Prompt environment
	goto end
)

pushd "%~dp0"
rmdir postgres32 /s /q
rmdir postgres-REL_15_10 /s /q
unzip zip\postgres-15.10.zip
ren postgres-REL_15_10 postgres32

rem if "%PLATFORM%"=="x64" (
rem     perl -i -e "$/ = undef; $_ = <>; s/sub DeterminePlatform.*?shift;/sub DeterminePlatform\n{\n\tmy \$self = shift;\n\n\t\$self->{platform} = 'x64';\n\treturn;/s; print" postgres32/src/tools/msvc/Solution.pm
rem ) else (
rem     perl -i -e "$/ = undef; $_ = <>; s/sub DeterminePlatform.*?shift;/sub DeterminePlatform\n{\n\tmy \$self = shift;\n\n\t\$self->{platform} = 'Win32';\n\treturn;/s; print" postgres32/src/tools/msvc/Solution.pm
rem )

perl -i -e "$/ = undef; $_ = <>; s/\$libpq = \$solution->AddProject\('libpq', 'dll', 'interfaces',/\$libpq = \$solution->AddProject\('libpq', 'lib', 'interfaces',/s; print" postgres32/src/tools/msvc/Mkvcbuild.pm

pushd postgres32\src\tools\msvc
call build libpq
call build DEBUG libpq
popd

:done
popd

:end
