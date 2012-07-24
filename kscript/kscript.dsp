# Microsoft Developer Studio Project File - Name="kscript" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=kscript - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kscript.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kscript.mak" CFG="kscript - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kscript - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "kscript - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "kscript - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "kscript - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/kscript", UXBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kscript - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib/kscript.lib"

!ELSEIF  "$(CFG)" == "kscript - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../kl/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib/kscriptd.lib"

!ELSEIF  "$(CFG)" == "kscript - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "kscript___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "kscript___Win32_Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "../kl/include" /D "WIN32" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib/kscriptd.lib"
# ADD LIB32 /nologo /out:"lib/kscriptud.lib"

!ELSEIF  "$(CFG)" == "kscript - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "kscript___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "kscript___Win32_Release_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "../xcm/include" /I "../kl/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib/kscript.lib"
# ADD LIB32 /nologo /out:"lib/kscriptu.lib"

!ENDIF 

# Begin Target

# Name "kscript - Win32 Release"
# Name "kscript - Win32 Debug"
# Name "kscript - Win32 Debug Unicode"
# Name "kscript - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\functions.cpp
# End Source File
# Begin Source File

SOURCE=.\jsarray.cpp
# End Source File
# Begin Source File

SOURCE=.\jsboolean.cpp
# End Source File
# Begin Source File

SOURCE=.\jsdate.cpp
# End Source File
# Begin Source File

SOURCE=.\jserror.cpp
# End Source File
# Begin Source File

SOURCE=.\jsfunction.cpp
# End Source File
# Begin Source File

SOURCE=.\jsmath.cpp
# End Source File
# Begin Source File

SOURCE=.\jsnumber.cpp
# End Source File
# Begin Source File

SOURCE=.\jsobject.cpp
# End Source File
# Begin Source File

SOURCE=.\json.cpp
# End Source File
# Begin Source File

SOURCE=.\jsregexp.cpp
# End Source File
# Begin Source File

SOURCE=.\jsstring.cpp
# End Source File
# Begin Source File

SOURCE=.\parse.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\validate.cpp
# End Source File
# Begin Source File

SOURCE=.\value.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\jsarray.h
# End Source File
# Begin Source File

SOURCE=.\jsboolean.h
# End Source File
# Begin Source File

SOURCE=.\jsdate.h
# End Source File
# Begin Source File

SOURCE=.\jserror.h
# End Source File
# Begin Source File

SOURCE=.\jsfunction.h
# End Source File
# Begin Source File

SOURCE=.\jsmath.h
# End Source File
# Begin Source File

SOURCE=.\jsnumber.h
# End Source File
# Begin Source File

SOURCE=.\jsobject.h
# End Source File
# Begin Source File

SOURCE=.\json.h
# End Source File
# Begin Source File

SOURCE=.\jsregexp.h
# End Source File
# Begin Source File

SOURCE=.\jsstring.h
# End Source File
# Begin Source File

SOURCE=.\kscript.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Makefile
# End Source File
# End Target
# End Project
