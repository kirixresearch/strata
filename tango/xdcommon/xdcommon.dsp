# Microsoft Developer Studio Project File - Name="xdcommon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=xdcommon - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xdcommon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xdcommon.mak" CFG="xdcommon - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xdcommon - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "xdcommon - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "xdcommon - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "xdcommon - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/tango/xdcommon", FTAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xdcommon - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../kl/include" /I "../xdregex/include" /I "../include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\xdcommon.lib"

!ELSEIF  "$(CFG)" == "xdcommon - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../kl/include" /I "../xdregex/include" /I "../include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\xdcommond.lib"

!ELSEIF  "$(CFG)" == "xdcommon - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "xdcommon___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "xdcommon___Win32_Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../xcm/include" /I "../../kl/include" /I "../include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "../../kl/include" /I "../xdregex/include" /I "../include" /D "WIN32" /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib\xdcommond.lib"
# ADD LIB32 /nologo /out:"lib\xdcommonud.lib"

!ELSEIF  "$(CFG)" == "xdcommon - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "xdcommon___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "xdcommon___Win32_Release_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "../../xcm/include" /I "../../kl/include" /I "../include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../kl/include" /I "../xdregex/include" /I "../include" /D "WIN32" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib\xdcommon.lib"
# ADD LIB32 /nologo /out:"lib\xdcommonu.lib"

!ENDIF 

# Begin Target

# Name "xdcommon - Win32 Release"
# Name "xdcommon - Win32 Debug"
# Name "xdcommon - Win32 Debug Unicode"
# Name "xdcommon - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\block.cpp
# End Source File
# Begin Source File

SOURCE=.\cmnbaseiterator.cpp
# End Source File
# Begin Source File

SOURCE=.\cmnbaseset.cpp
# End Source File
# Begin Source File

SOURCE=.\cmndynamicset.cpp
# End Source File
# Begin Source File

SOURCE=.\columninfo.cpp
# End Source File
# Begin Source File

SOURCE=.\connectionstr.cpp
# End Source File
# Begin Source File

SOURCE=.\dbattr.cpp
# End Source File
# Begin Source File

SOURCE=.\dbentry.cpp
# End Source File
# Begin Source File

SOURCE=.\dbfuncs.cpp
# End Source File
# Begin Source File

SOURCE=.\errorinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\exindex.cpp
# End Source File
# Begin Source File

SOURCE=.\exkeypool.cpp
# End Source File
# Begin Source File

SOURCE=.\exprfuncs.cpp
# End Source File
# Begin Source File

SOURCE=.\extfileinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\fileinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\filestream.cpp
# End Source File
# Begin Source File

SOURCE=.\idxutil.cpp
# End Source File
# Begin Source File

SOURCE=.\jobinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\keylayout.cpp
# End Source File
# Begin Source File

SOURCE=.\localrowcache.cpp
# End Source File
# Begin Source File

SOURCE=.\rowidarray.cpp
# End Source File
# Begin Source File

SOURCE=.\setconfigfile.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlalter.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlcommon.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlcreate.cpp
# End Source File
# Begin Source File

SOURCE=.\sqldelete.cpp
# End Source File
# Begin Source File

SOURCE=.\sqldrop.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlinsert.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlmove.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlselect.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlupdate.cpp
# End Source File
# Begin Source File

SOURCE=.\structure.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\xdcommon.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\block.h
# End Source File
# Begin Source File

SOURCE=.\cmnbaseiterator.h
# End Source File
# Begin Source File

SOURCE=.\cmnbaseset.h
# End Source File
# Begin Source File

SOURCE=.\cmndynamicset.h
# End Source File
# Begin Source File

SOURCE=.\columninfo.h
# End Source File
# Begin Source File

SOURCE=.\connectionstr.h
# End Source File
# Begin Source File

SOURCE=.\dbattr.h
# End Source File
# Begin Source File

SOURCE=.\dbentry.h
# End Source File
# Begin Source File

SOURCE=.\dbfuncs.h
# End Source File
# Begin Source File

SOURCE=.\errorinfo.h
# End Source File
# Begin Source File

SOURCE=.\exindex.h
# End Source File
# Begin Source File

SOURCE=.\exkeypool.h
# End Source File
# Begin Source File

SOURCE=.\exprfuncs.h
# End Source File
# Begin Source File

SOURCE=.\extfileinfo.h
# End Source File
# Begin Source File

SOURCE=.\fileinfo.h
# End Source File
# Begin Source File

SOURCE=.\filestream.h
# End Source File
# Begin Source File

SOURCE=.\idxutil.h
# End Source File
# Begin Source File

SOURCE=.\jobinfo.h
# End Source File
# Begin Source File

SOURCE=.\keylayout.h
# End Source File
# Begin Source File

SOURCE=.\localrowcache.h
# End Source File
# Begin Source File

SOURCE=.\rowidarray.h
# End Source File
# Begin Source File

SOURCE=.\setconfigfile.h
# End Source File
# Begin Source File

SOURCE=.\sqlcommon.h
# End Source File
# Begin Source File

SOURCE=.\structure.h
# End Source File
# Begin Source File

SOURCE=.\tango_private.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\xdcommon.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Makefile
# End Source File
# Begin Source File

SOURCE=.\modversion.h
# End Source File
# Begin Source File

SOURCE=..\include\tango.h
# End Source File
# End Target
# End Project
