# Microsoft Developer Studio Project File - Name="cfw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=cfw - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cfw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cfw.mak" CFG="cfw - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cfw - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "cfw - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "cfw - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "cfw - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/cfw", UQBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cfw - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/msw" /I "../wxaui/include" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\cfw.lib"

!ELSEIF  "$(CFG)" == "cfw - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswd" /I "../wxaui/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\cfwd.lib"

!ELSEIF  "$(CFG)" == "cfw - Win32 Debug Unicode"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/lib/vc_lib/mswud" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "_UNICODE" /D "UNICODE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib\cfwd.lib"
# ADD LIB32 /nologo /out:"lib\cfwud.lib"

!ELSEIF  "$(CFG)" == "cfw - Win32 Release Unicode"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/lib/vc_lib/mswu" /I "../wxaui/include" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "_UNICODE" /D "UNICODE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib\cfw.lib"
# ADD LIB32 /nologo /out:"lib\cfwu.lib"

!ENDIF 

# Begin Target

# Name "cfw - Win32 Release"
# Name "cfw - Win32 Debug"
# Name "cfw - Win32 Debug Unicode"
# Name "cfw - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\artprovider.cpp
# End Source File
# Begin Source File

SOURCE=.\cfw.cpp
# End Source File
# Begin Source File

SOURCE=.\framework.cpp
# End Source File
# Begin Source File

SOURCE=.\frameworkprops.cpp
# End Source File
# Begin Source File

SOURCE=.\fsitems.cpp
# End Source File
# Begin Source File

SOURCE=.\fspanel.cpp
# End Source File
# Begin Source File

SOURCE=.\fspanellistview.cpp
# End Source File
# Begin Source File

SOURCE=.\fspaneltreeview.cpp
# End Source File
# Begin Source File

SOURCE=.\jobinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\jobqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\prefs.cpp
# End Source File
# Begin Source File

SOURCE=.\propertiespanel.cpp
# End Source File
# Begin Source File

SOURCE=.\statusbar.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\artprovider.h
# End Source File
# Begin Source File

SOURCE=.\cfw.h
# End Source File
# Begin Source File

SOURCE=.\framework.h
# End Source File
# Begin Source File

SOURCE=.\framework_private.h
# End Source File
# Begin Source File

SOURCE=.\frameworkprops.h
# End Source File
# Begin Source File

SOURCE=.\fsitems.h
# End Source File
# Begin Source File

SOURCE=.\fsitems_private.h
# End Source File
# Begin Source File

SOURCE=.\fspanel.h
# End Source File
# Begin Source File

SOURCE=.\fspanel_private.h
# End Source File
# Begin Source File

SOURCE=.\fspanellistview.h
# End Source File
# Begin Source File

SOURCE=.\fspaneltreeview.h
# End Source File
# Begin Source File

SOURCE=.\jobqueue.h
# End Source File
# Begin Source File

SOURCE=.\jobqueue_private.h
# End Source File
# Begin Source File

SOURCE=.\prefs.h
# End Source File
# Begin Source File

SOURCE=.\prefs_private.h
# End Source File
# Begin Source File

SOURCE=.\propertiespanel.h
# End Source File
# Begin Source File

SOURCE=..\kl\include\kl\regex.h
# End Source File
# Begin Source File

SOURCE=.\statusbar.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Group "Xpm"

# PROP Default_Filter "xpm"
# Begin Source File

SOURCE=.\xpm_cancel.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_checkmark.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_closewindow.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_delete.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_error.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_job.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_progress1.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_progress2.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_progress3.xpm
# End Source File
# Begin Source File

SOURCE=.\xpm_progress4.xpm
# End Source File
# End Group
# Begin Source File

SOURCE=.\Makefile
# End Source File
# End Target
# End Project
