# Microsoft Developer Studio Project File - Name="kxinter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=kxinter - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kxinter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kxinter.mak" CFG="kxinter - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kxinter - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "kxinter - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "kxinter - Win32 Debug Unicode" (based on "Win32 (x86) Application")
!MESSAGE "kxinter - Win32 Release Unicode" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/kxinter", ARBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kxinter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../xcm/include" /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/msw" /I "../supp/wxWidgets/contrib/include" /I "../tango/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX"kxinter.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib wxbase26.lib wxbase26_net.lib wxmsw26_core.lib wxmsw26_adv.lib wxmsw26_fl.lib wxmsw26_stc.lib wxpng.lib wxzlib.lib ../kcl/lib/kcl.lib ../cfw/lib/cfw.lib ../kappcmn/lib/kappcmn.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../release/kxinter.exe" /libpath:"../supp/wxWidgets/lib/vc_lib"

!ELSEIF  "$(CFG)" == "kxinter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswd" /I "../supp/wxWidgets/contrib/include" /I "../tango/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX"kxinter.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase26d.lib wxbase26d_net.lib wxmsw26d_core.lib wxmsw26d_adv.lib wxmsw26d_fl.lib wxmsw26d_stc.lib wxpngd.lib wxzlibd.lib ../kcl/lib/kcld.lib ../cfw/lib/cfwd.lib ../kappcmn/lib/kappcmnd.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../debug/kxinter.exe" /pdbtype:sept /libpath:"../supp/wxWidgets/lib/vc_lib"

!ELSEIF  "$(CFG)" == "kxinter - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "kxinter___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "kxinter___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX"kxinter.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswud" /I "../supp/wxWidgets/contrib/include" /I "../tango/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX"kxinter.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wxmswd.lib comctl32.lib wsock32.lib rpcrt4.lib fld.lib pngd.lib zlibd.lib ../kcl/lib/kcld.lib ../cfw/lib/cfwd.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../bin/kxinter.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase26ud.lib wxbase26ud_net.lib wxmsw26ud_core.lib wxmsw26ud_adv.lib wxmsw26ud_fl.lib wxmsw26ud_stc.lib wxpngd.lib wxzlibd.lib ../kcl/lib/kclud.lib ../cfw/lib/cfwud.lib ../kappcmn/lib/kappcmnud.lib ../paladin/lib/paladinud.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../debugu/kxinter.exe" /pdbtype:sept /libpath:"../supp/wxWidgets/lib/vc_lib"

!ELSEIF  "$(CFG)" == "kxinter - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "kxinter___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "kxinter___Win32_Release_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /I "../xcm/include" /I "../kl/include" /I "../tango/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX"kxinter.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswud" /I "../supp/wxWidgets/contrib/include" /I "../tango/include" /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX"kxinter.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib wxbase25.lib wxbase25_net.lib wxmsw25_core.lib wxmsw25_adv.lib wxmsw25_fl.lib wxmsw25_stc.lib wxpng.lib wxzlib.lib ../kcl/lib/kcl.lib ../cfw/lib/cfw.lib ../kappcmn/lib/kappcmn.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../release/kxinter.exe"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib wxbase26u.lib wxbase26u_net.lib wxmsw26u_core.lib wxmsw26u_adv.lib wxmsw26u_fl.lib wxmsw26u_stc.lib wxpng.lib wxzlib.lib ../kcl/lib/kclu.lib ../cfw/lib/cfwu.lib ../kappcmn/lib/kappcmnu.lib ../paladin/lib/paladinu.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../releaseu/kxinter.exe" /libpath:"../supp/wxWidgets/lib/vc_lib"

!ENDIF 

# Begin Target

# Name "kxinter - Win32 Release"
# Name "kxinter - Win32 Debug"
# Name "kxinter - Win32 Debug Unicode"
# Name "kxinter - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\app.cpp
# End Source File
# Begin Source File

SOURCE=.\appcontroller.cpp
# End Source File
# Begin Source File

SOURCE=.\copybookparse.cpp
# End Source File
# Begin Source File

SOURCE=.\datadef.cpp
# End Source File
# Begin Source File

SOURCE=.\datadoc.cpp
# End Source File
# Begin Source File

SOURCE=.\datastream.cpp
# End Source File
# Begin Source File

SOURCE=.\griddoc.cpp
# End Source File
# Begin Source File

SOURCE=.\hexview.cpp
# End Source File
# Begin Source File

SOURCE=.\jobconvert.cpp
# End Source File
# Begin Source File

SOURCE=.\jobmodifystruct.cpp
# End Source File
# Begin Source File

SOURCE=.\panelconsole.cpp
# End Source File
# Begin Source File

SOURCE=.\panelcopytables.cpp
# End Source File
# Begin Source File

SOURCE=.\paneldataformat.cpp
# End Source File
# Begin Source File

SOURCE=.\structwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\textview.cpp
# End Source File
# Begin Source File

SOURCE=.\treecontroller.cpp
# End Source File
# Begin Source File

SOURCE=.\treemodel.cpp
# End Source File
# Begin Source File

SOURCE=.\xpmregister.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\app.h
# End Source File
# Begin Source File

SOURCE=.\appcontroller.h
# End Source File
# Begin Source File

SOURCE=.\bitmapmgr.h
# End Source File
# Begin Source File

SOURCE=.\copybookparse.h
# End Source File
# Begin Source File

SOURCE=.\datadef.h
# End Source File
# Begin Source File

SOURCE=.\datadoc.h
# End Source File
# Begin Source File

SOURCE=.\datastream.h
# End Source File
# Begin Source File

SOURCE=.\dataviewbase.h
# End Source File
# Begin Source File

SOURCE=.\griddoc.h
# End Source File
# Begin Source File

SOURCE=.\hexview.h
# End Source File
# Begin Source File

SOURCE=.\jobconvert.h
# End Source File
# Begin Source File

SOURCE=.\jobmodifystruct.h
# End Source File
# Begin Source File

SOURCE=.\kxinter.h
# End Source File
# Begin Source File

SOURCE=.\panelconsole.h
# End Source File
# Begin Source File

SOURCE=.\panelcopytables.h
# End Source File
# Begin Source File

SOURCE=.\paneldataformat.h
# End Source File
# Begin Source File

SOURCE=..\kl\include\kl\regex.h
# End Source File
# Begin Source File

SOURCE=.\structwindow.h
# End Source File
# Begin Source File

SOURCE=.\textview.h
# End Source File
# Begin Source File

SOURCE=.\treecontroller.h
# End Source File
# Begin Source File

SOURCE=.\treemodel.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\xpmregister.h
# End Source File
# End Group
# End Target
# End Project
