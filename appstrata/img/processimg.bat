 @echo off
 
 set IMG=%1
 set HANDLE=%IMG:.png=%
 set IDENTIFIER=%IMG:.=_%
 set SIZEIDENTIFIER=%IDENTIFIER%_size
 echo processing %IMG%
 
 ..\bin2c -o c\%1.c %1
 
 echo. >> c\%1.c
 echo #ifdef REGISTER_EMBEDDED_IMAGE >> c\%1.c
 echo REGISTER_EMBEDDED_IMAGE(%HANDLE%,%IDENTIFIER%,%SIZEIDENTIFIER%) >> c\%1.c
 echo #endif >> c\%1.c
 