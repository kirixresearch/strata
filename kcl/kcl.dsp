# Microsoft Developer Studio Project File - Name="kcl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=kcl - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kcl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kcl.mak" CFG="kcl - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kcl - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "kcl - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "kcl - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "kcl - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/kcl", XVAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kcl - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/msw" /I "../supp/wxWidgets/contrib/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\kcl.lib"

!ELSEIF  "$(CFG)" == "kcl - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswd" /I "../supp/wxWidgets/contrib/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\kcld.lib"

!ELSEIF  "$(CFG)" == "kcl - Win32 Debug Unicode"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswud" /I "../supp/wxWidgets/contrib/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "_UNICODE" /D "UNICODE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib\kcld.lib"
# ADD LIB32 /nologo /out:"lib\kclud.lib"

!ELSEIF  "$(CFG)" == "kcl - Win32 Release Unicode"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswu" /I "../supp/wxWidgets/contrib/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "_UNICODE" /D "UNICODE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib\kcl.lib"
# ADD LIB32 /nologo /out:"lib\kclu.lib"

!ENDIF 

# Begin Target

# Name "kcl - Win32 Release"
# Name "kcl - Win32 Debug"
# Name "kcl - Win32 Debug Unicode"
# Name "kcl - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bannercontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\bitmapcombo.cpp
# End Source File
# Begin Source File

SOURCE=.\borderpanel.cpp
# End Source File
# Begin Source File

SOURCE=.\button.cpp
# End Source File
# Begin Source File

SOURCE=.\buttonbar.cpp
# End Source File
# Begin Source File

SOURCE=.\colordropdown.cpp
# End Source File
# Begin Source File

SOURCE=.\colorpanel.cpp
# End Source File
# Begin Source File

SOURCE=.\colorselector.cpp
# End Source File
# Begin Source File

SOURCE=.\grid.cpp
# End Source File
# Begin Source File

SOURCE=.\griddnd.cpp
# End Source File
# Begin Source File

SOURCE=.\gridmemmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\gridvalidator.cpp
# End Source File
# Begin Source File

SOURCE=.\notebookcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\popupcontainer.cpp
# End Source File
# Begin Source File

SOURCE=.\rowselectiongrid.cpp
# End Source File
# Begin Source File

SOURCE=.\rulercontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\scrolllistcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\validcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\wizard.cpp
# End Source File
# Begin Source File

SOURCE=.\xmdi.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bannercontrol.h
# End Source File
# Begin Source File

SOURCE=.\bitmapcombo.h
# End Source File
# Begin Source File

SOURCE=.\borderpanel.h
# End Source File
# Begin Source File

SOURCE=.\button.h
# End Source File
# Begin Source File

SOURCE=.\buttonbar.h
# End Source File
# Begin Source File

SOURCE=.\colordropdown.h
# End Source File
# Begin Source File

SOURCE=.\colorpanel.h
# End Source File
# Begin Source File

SOURCE=.\colorselector.h
# End Source File
# Begin Source File

SOURCE=.\grid.h
# End Source File
# Begin Source File

SOURCE=.\griddnd.h
# End Source File
# Begin Source File

SOURCE=.\gridmemmodel.h
# End Source File
# Begin Source File

SOURCE=.\gridvalidator.h
# End Source File
# Begin Source File

SOURCE=.\kcl.h
# End Source File
# Begin Source File

SOURCE=.\notebookcontrol.h
# End Source File
# Begin Source File

SOURCE=.\popupcontainer.h
# End Source File
# Begin Source File

SOURCE=.\rowselectiongrid.h
# End Source File
# Begin Source File

SOURCE=.\rulercontrol.h
# End Source File
# Begin Source File

SOURCE=.\scrolllistcontrol.h
# End Source File
# Begin Source File

SOURCE=.\toolbar.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\validcontrol.h
# End Source File
# Begin Source File

SOURCE=.\wizard.h
# End Source File
# Begin Source File

SOURCE=.\xmdi.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Makefile
# End Source File
# End Target
# End Project
