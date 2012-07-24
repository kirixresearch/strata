# Microsoft Developer Studio Project File - Name="kl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=kl - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kl.mak" CFG="kl - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kl - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "kl - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "kl - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "kl - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kl - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\kl.lib"

!ELSEIF  "$(CFG)" == "kl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "kl___Win32_Debug"
# PROP BASE Intermediate_Dir "kl___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\kld.lib"

!ELSEIF  "$(CFG)" == "kl - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug Unicode"
# PROP BASE Intermediate_Dir "Debug Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "include" /D "WIN32" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\klud.lib"

!ELSEIF  "$(CFG)" == "kl - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release Unicode"
# PROP BASE Intermediate_Dir "Release Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Target_Dir ""
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "include" /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\klu.lib"

!ENDIF 

# Begin Target

# Name "kl - Win32 Release"
# Name "kl - Win32 Debug"
# Name "kl - Win32 Debug Unicode"
# Name "kl - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\base64.cpp
# End Source File
# Begin Source File

SOURCE=.\src\crypt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\file_ix.cpp

!IF  "$(CFG)" == "kl - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "kl - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "kl - Win32 Debug Unicode"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "kl - Win32 Release Unicode"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\file_win32.cpp
# End Source File
# Begin Source File

SOURCE=.\src\math.cpp
# End Source File
# Begin Source File

SOURCE=.\src\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\src\memory.cpp
# End Source File
# Begin Source File

SOURCE=.\src\url.cpp
# End Source File
# Begin Source File

SOURCE=.\src\utf8.cpp
# End Source File
# Begin Source File

SOURCE=.\src\xcm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\xml.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\include\kl\base64.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\crypt.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\file.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\klib.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\math.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\md5.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\memory.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\portable.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\regex.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\string.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\utf8.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\win32.h
# End Source File
# Begin Source File

SOURCE=.\include\kl\xml.h
# End Source File
# End Group
# Begin Group "XCM Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\xcm\create.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\iobject.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\macro.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\signal.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\smartptr.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\typeinfo.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\xcm.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\xcmerror.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\xcmthread.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\xcmvalue.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\xcmvector.h
# End Source File
# Begin Source File

SOURCE=.\include\xcm\xcmwin32.h
# End Source File
# End Group
# End Target
# End Project
