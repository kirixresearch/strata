# Microsoft Developer Studio Project File - Name="kcanvas" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=kcanvas - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kcanvas.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kcanvas.mak" CFG="kcanvas - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kcanvas - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "kcanvas - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "kcanvas - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "kcanvas - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/kcanvas", NSCAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kcanvas - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxpdfdoc/include" /I "../supp/wxWidgets/lib/vc_lib/msw" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\kcanvas.lib"

!ELSEIF  "$(CFG)" == "kcanvas - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxpdfdoc/include" /I "../supp/wxWidgets/lib/vc_lib/mswd" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\kcanvasd.lib"

!ELSEIF  "$(CFG)" == "kcanvas - Win32 Debug Unicode"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "../xcm/include" /I "../kl/include" /I "../supp/wxWindows/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxpdfdoc/include" /I "../supp/wxWidgets/lib/vc_lib/mswud" /D "WIN32" /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\kcanvasud.lib"

!ELSEIF  "$(CFG)" == "kcanvas - Win32 Release Unicode"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../xcm/include" /I "../kl/include" /I "../supp/wxWindows/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxpdfdoc/include" /I "../supp/wxWidgets/lib/vc_lib/mswu" /D "WIN32" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\kcanvasu.lib"

!ENDIF 

# Begin Target

# Name "kcanvas - Win32 Release"
# Name "kcanvas - Win32 Debug"
# Name "kcanvas - Win32 Debug Unicode"
# Name "kcanvas - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\canvas.cpp
# End Source File
# Begin Source File

SOURCE=.\canvascontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\canvasdc.cpp
# End Source File
# Begin Source File

SOURCE=.\canvashistory.cpp
# End Source File
# Begin Source File

SOURCE=.\canvaspdf.cpp
# End Source File
# Begin Source File

SOURCE=.\canvasprintout.cpp
# End Source File
# Begin Source File

SOURCE=.\component.cpp
# End Source File
# Begin Source File

SOURCE=.\componentbox.cpp
# End Source File
# Begin Source File

SOURCE=.\componentcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\componentellipse.cpp
# End Source File
# Begin Source File

SOURCE=.\componentgraph.cpp
# End Source File
# Begin Source File

SOURCE=.\componentimage.cpp
# End Source File
# Begin Source File

SOURCE=.\componentline.cpp
# End Source File
# Begin Source File

SOURCE=.\componentpage.cpp
# End Source File
# Begin Source File

SOURCE=.\componenttable.cpp
# End Source File
# Begin Source File

SOURCE=.\componenttextbox.cpp
# End Source File
# Begin Source File

SOURCE=.\dnd.cpp
# End Source File
# Begin Source File

SOURCE=.\event.cpp
# End Source File
# Begin Source File

SOURCE=.\graphicsobj.cpp
# End Source File
# Begin Source File

SOURCE=.\property.cpp
# End Source File
# Begin Source File

SOURCE=.\range.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\canvas.h
# End Source File
# Begin Source File

SOURCE=.\canvascontrol.h
# End Source File
# Begin Source File

SOURCE=.\canvasdc.h
# End Source File
# Begin Source File

SOURCE=.\canvashistory.h
# End Source File
# Begin Source File

SOURCE=.\canvaspdf.h
# End Source File
# Begin Source File

SOURCE=.\canvasprintout.h
# End Source File
# Begin Source File

SOURCE=.\component.h
# End Source File
# Begin Source File

SOURCE=.\componentbox.h
# End Source File
# Begin Source File

SOURCE=.\componentcontrol.h
# End Source File
# Begin Source File

SOURCE=.\componentellipse.h
# End Source File
# Begin Source File

SOURCE=.\componentgraph.h
# End Source File
# Begin Source File

SOURCE=.\componentgrid.h
# End Source File
# Begin Source File

SOURCE=.\componentimage.h
# End Source File
# Begin Source File

SOURCE=.\componentline.h
# End Source File
# Begin Source File

SOURCE=.\componentpage.h
# End Source File
# Begin Source File

SOURCE=.\componenttable.h
# End Source File
# Begin Source File

SOURCE=.\componenttextbox.h
# End Source File
# Begin Source File

SOURCE=.\dnd.h
# End Source File
# Begin Source File

SOURCE=.\event.h
# End Source File
# Begin Source File

SOURCE=.\graphicsobj.h
# End Source File
# Begin Source File

SOURCE=.\kcanvas.h
# End Source File
# Begin Source File

SOURCE=.\kcanvas_int.h
# End Source File
# Begin Source File

SOURCE=.\model.h
# End Source File
# Begin Source File

SOURCE=.\property.h
# End Source File
# Begin Source File

SOURCE=.\range.h
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
