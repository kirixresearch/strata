# Microsoft Developer Studio Project File - Name="webconnect" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=webconnect - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "webconnect.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "webconnect.mak" CFG="webconnect - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "webconnect - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "webconnect - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "webconnect - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "webconnect - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "webconnect - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/msw" /I "../supp/gecko-sdk/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\webconnect.lib"

!ELSEIF  "$(CFG)" == "webconnect - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswd" /I "../supp/gecko-sdk/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\webconnectd.lib"

!ELSEIF  "$(CFG)" == "webconnect - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "webconnect___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "webconnect___Win32_Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswud" /I "../supp/gecko-sdk/include" /D "WIN32" /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\webconnectud.lib"

!ELSEIF  "$(CFG)" == "webconnect - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "webconnect___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "webconnect___Win32_Release_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswu" /I "../supp/gecko-sdk/include" /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\webconnectu.lib"

!ENDIF 

# Begin Target

# Name "webconnect - Win32 Release"
# Name "webconnect - Win32 Debug"
# Name "webconnect - Win32 Debug Unicode"
# Name "webconnect - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dom.cpp
# End Source File
# Begin Source File

SOURCE=.\nsimpl.cpp
# End Source File
# Begin Source File

SOURCE=.\promptservice.cpp
# End Source File
# Begin Source File

SOURCE=.\webcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\webframe.cpp
# End Source File
# Begin Source File

SOURCE=.\webprefs.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\domprivate.h
# End Source File
# Begin Source File

SOURCE=.\nsbase.h
# End Source File
# Begin Source File

SOURCE=.\nsinclude.h
# End Source File
# Begin Source File

SOURCE=.\nsptr.h
# End Source File
# Begin Source File

SOURCE=.\nsstr.h
# End Source File
# Begin Source File

SOURCE=.\nsutils.h
# End Source File
# Begin Source File

SOURCE=.\nsweak.h
# End Source File
# Begin Source File

SOURCE=.\promptservice.h
# End Source File
# Begin Source File

SOURCE=.\webcontrol.h
# End Source File
# Begin Source File

SOURCE=.\webframe.h
# End Source File
# End Group
# End Target
# End Project
