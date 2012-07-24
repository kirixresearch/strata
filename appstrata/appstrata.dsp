# Microsoft Developer Studio Project File - Name="appstrata" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=appstrata - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "appstrata.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "appstrata.mak" CFG="appstrata - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "appstrata - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "appstrata - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "appstrata - Win32 Debug Unicode" (based on "Win32 (x86) Application")
!MESSAGE "appstrata - Win32 Release Unicode" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/appstrata", CAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "appstrata - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/msw" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/src/zlib" /I "../supp/freetype/include" /I "../tango/include" /D appstrata_BUILD_TIER=2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "CURL_STATICLIB" /YX"appstrata.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "../supp/wxWidgets/include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase26.lib wxbase26_net.lib wxmsw26_core.lib wxmsw26_adv.lib wxmsw26_propgrid.lib wxmsw26_stc.lib wxjpeg.lib wxpng.lib wxzlib.lib wxaui.lib freetype219MT.lib ../kl/lib/kl.lib ../kcl/lib/kcl.lib ../cfw/lib/cfw.lib ../paladin/lib/paladin.lib ../kscript/lib/kscript.lib ../kcanvas/lib/kcanvas.lib ../webconnect/lib/webconnect.lib /nologo /subsystem:windows /machine:I386 /out:"../release/appstrata.exe" /libpath:"../supp/wxWidgets/lib/vc_lib" /libpath:"../supp/wxpdfdoc/lib"

!ELSEIF  "$(CFG)" == "appstrata - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "appstrata___Win32_Debug0"
# PROP BASE Intermediate_Dir "appstrata___Win32_Debug0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswd" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/src/zlib" /I "../supp/freetype/include" /I "../tango/include" /D appstrata_BUILD_TIER=2 /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "CURL_STATICLIB" /YX"appstrata.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "../supp/wxWidgets/include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase26d.lib wxbase26d_net.lib wxmsw26d_core.lib wxmsw26d_adv.lib wxmsw26d_propgrid.lib wxmsw26d_stc.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxauid.lib freetype219MT_D.lib ../kl/lib/kld.lib ../kcl/lib/kcld.lib ../cfw/lib/cfwd.lib ../paladin/lib/paladind.lib ../kscript/lib/kscriptd.lib ../kcanvas/lib/kcanvasd.lib ../webconnect/lib/webconnectd.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../debug/appstrata.exe" /pdbtype:sept /libpath:"../supp/wxWidgets/lib/vc_lib" /libpath:"../supp/wxpdfdoc/lib"

!ELSEIF  "$(CFG)" == "appstrata - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "appstrata___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "appstrata___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /Zi /Od /I "./config" /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswud" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/src/zlib" /I "../supp/wxpdfdoc/include" /I "../tango/include" /I "../supp/curl/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "_UNICODE" /D "UNICODE" /D "CURL_STATICLIB" /YX"appstrata.h" /FD /Zm500 /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "../supp/wxWidgets/include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wxmswd.lib comctl32.lib wsock32.lib rpcrt4.lib flud.lib pngd.lib zlibd.lib freetype214MT_D.lib ../../kcl/lib/kcld.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../bin/appstrata.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase28ud.lib wxbase28ud_net.lib wxmsw28ud_core.lib wxmsw28ud_adv.lib wxmsw28ud_html.lib wxmsw28ud_aui.lib wxbase28ud_xml.lib wxmsw28ud_stc.lib wxpdfdocud.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxexpatd.lib ../kl/lib/klud.lib ../kcl/lib/kclud.lib ../cfw/lib/cfwud.lib ../paladin/lib/paladinud.lib ../kscript/lib/kscriptud.lib ../kcanvas/lib/kcanvasud.lib ../webconnect/lib/webconnectud.lib ../supp/haru/lib/harud.lib ../supp/curl/lib/debug-ssl/libcurld.lib ../supp/libssh2/win32/Debug_lib/libssh2d.lib ../supp/openssl/out32.dbg/libeay32.lib ../supp/openssl/out32.dbg/ssleay32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../debugu/appmain.exe" /pdbtype:sept /libpath:"../supp/wxWidgets/lib/vc_lib" /libpath:"../supp/wxpdfdoc/lib"

!ELSEIF  "$(CFG)" == "appstrata - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "appstrata___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "appstrata___Win32_Release_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zd /O2 /I "./config" /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswu" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/src/zlib" /I "../tango/include" /I "../supp/curl/include" /I "../supp/wxpdfdoc/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "_UNICODE" /D "UNICODE" /D "CURL_STATICLIB" /YX"appstrata.h" /FD /Zm500 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "./config" /i "../supp/wxWidgets/include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxmsw.lib fl.lib png.lib zlib.lib freetype214MT.lib ../../kcl/lib/kcl.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase28u.lib wxbase28u_net.lib wxmsw28u_core.lib wxmsw28u_adv.lib wxmsw28u_html.lib wxmsw28u_aui.lib wxbase28u_xml.lib wxmsw28u_stc.lib wxpdfdocu.lib wxjpeg.lib wxpng.lib wxzlib.lib wxexpat.lib ../kl/lib/klu.lib ../kcl/lib/kclu.lib ../cfw/lib/cfwu.lib ../paladin/lib/paladinu.lib ../kscript/lib/kscriptu.lib ../kcanvas/lib/kcanvasu.lib ../webconnect/lib/webconnectu.lib ../supp/haru/lib/haru.lib ../supp/curl/lib/release-ssl/libcurl.lib ../supp/libssh2/win32/Release_lib/libssh.lib ../supp/openssl/out32/libeay32.lib ../supp/openssl/out32/ssleay32.lib /nologo /subsystem:windows /map /machine:I386 /out:"../releaseu/appmain.exe" /libpath:"../supp/wxWidgets/lib/vc_lib" /libpath:"../supp/wxpdfdoc/lib" /MAPINFO:LINES /MAPINFO:EXPORTS
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "appstrata - Win32 Release"
# Name "appstrata - Win32 Debug"
# Name "appstrata - Win32 Debug Unicode"
# Name "appstrata - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\appmain\app.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\appcontroller.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\appmain.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\appprefs.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\appprint.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\apputil.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\bitmapmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\bookmark.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\bookmarkfs.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\commandmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\commands.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\connectionmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\connectionpages.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\connectionwizard.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\curlutil.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dbcombo.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dbdoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgauth.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgdatabasefile.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dlglinkprops.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgpagesetup.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgpassword.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgprojectmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgreportprops.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\editordoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\exportpages.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\exportwizard.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\exprbuilder.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\extensionmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\extensionpkg.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\feedparser.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\fieldlistcontrol.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\fieldtypechoice.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\importpages.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\importtemplate.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\importwizard.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\inetauth.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobappend.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobcopy.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobexport.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobexportpkg.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobfindinfiles.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobftp.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobgroup.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobimport.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobimportpkg.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobindex.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobmodifystruct.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobquery.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobscheduler.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobscript.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jobsplit.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\jsonconfig.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\linkbar.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\moduleremoveduprec.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\mongoose_vc6.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelcolordoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelcolprops.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelcolumnlist.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelconsole.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\paneldatabaseinfo.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelexprbuilderdoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelextensionmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelfileinfo.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelfind.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelgroup.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelindex.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\paneljobmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\paneljobscheduler.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelmarkmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelmerge.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\paneloptions.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelrelationship.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelreplacerows.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelsplit.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\panelview.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\pkgfile.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\querydoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\querytemplate.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\relationdiagram.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\reportdesign.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\reportdoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\reportengine.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\reportlayout.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\reportmodel.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\reportstore.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptapp.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptbanner.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptbase64.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptbitmap.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptbutton.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptcombo.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptcrypt.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptdb.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptenvironment.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptfile.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptfiletransfer.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptfont.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptgraphics.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptgui.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scripthash.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scripthost.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scripthostapp.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scripthttp.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptlayout.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptlist.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptlistview.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptmemory.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptnative.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptprocess.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptprogress.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptreport.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptslider.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptspin.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptstatusbar.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptsystem.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scripttable.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scripttext.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scripttoolbar.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scripttreeview.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptwebbrowser.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptwebdom.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptxml.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\sqldoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\structuredoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\structurevalidator.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\tabledoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\tabledocapi.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\tabledocmodel.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\tangogridmodel.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\textdoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\textview.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\toolbars.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\transformationdoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\updater.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\webdoc.cpp
# End Source File
# Begin Source File

SOURCE=..\appmain\webserver.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\appmain\app.h
# End Source File
# Begin Source File

SOURCE=..\appmain\appcontroller.h
# End Source File
# Begin Source File

SOURCE=..\appmain\apphook.h
# End Source File
# Begin Source File

SOURCE=..\appmain\appmain.h
# End Source File
# Begin Source File

SOURCE=..\appmain\appprefs.h
# End Source File
# Begin Source File

SOURCE=..\appmain\appprint.h
# End Source File
# Begin Source File

SOURCE=..\appmain\apputil.h
# End Source File
# Begin Source File

SOURCE=..\appmain\bitmapmgr.h
# End Source File
# Begin Source File

SOURCE=..\appmain\bookmark.h
# End Source File
# Begin Source File

SOURCE=..\appmain\bookmarkfs.h
# End Source File
# Begin Source File

SOURCE=..\appmain\commandmgr.h
# End Source File
# Begin Source File

SOURCE=..\appmain\commands.h
# End Source File
# Begin Source File

SOURCE=..\appmain\connectionmgr.h
# End Source File
# Begin Source File

SOURCE=..\appmain\connectionpages.h
# End Source File
# Begin Source File

SOURCE=..\appmain\connectionwizard.h
# End Source File
# Begin Source File

SOURCE=..\appmain\curlutil.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dbcombo.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dbdoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgauth.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgdatabasefile.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dlglinkprops.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgpagesetup.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgpassword.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgprojectmgr.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dlgreportprops.h
# End Source File
# Begin Source File

SOURCE=..\appmain\dociface.h
# End Source File
# Begin Source File

SOURCE=..\appmain\editordoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\exportpages.h
# End Source File
# Begin Source File

SOURCE=..\appmain\exportwizard.h
# End Source File
# Begin Source File

SOURCE=..\appmain\exprbuilder.h
# End Source File
# Begin Source File

SOURCE=..\appmain\extensionmgr.h
# End Source File
# Begin Source File

SOURCE=..\appmain\extensionpkg.h
# End Source File
# Begin Source File

SOURCE=..\appmain\feedparser.h
# End Source File
# Begin Source File

SOURCE=..\appmain\fieldlistcontrol.h
# End Source File
# Begin Source File

SOURCE=..\appmain\fieldtypechoice.h
# End Source File
# Begin Source File

SOURCE=..\appmain\importpages.h
# End Source File
# Begin Source File

SOURCE=..\appmain\importtemplate.h
# End Source File
# Begin Source File

SOURCE=..\appmain\importwizard.h
# End Source File
# Begin Source File

SOURCE=..\appmain\inetauth.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobappend.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobcopy.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobexport.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobexportpkg.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobfindinfiles.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobftp.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobgroup.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobimport.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobimportpkg.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobindex.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobmodifystruct.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobquery.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobscheduler.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobscript.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jobsplit.h
# End Source File
# Begin Source File

SOURCE=..\appmain\jsonconfig.h
# End Source File
# Begin Source File

SOURCE=..\appmain\linkbar.h
# End Source File
# Begin Source File

SOURCE=..\appmain\moduleremoveduprec.h
# End Source File
# Begin Source File

SOURCE=..\appmain\mongoose.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelcolordoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelcolprops.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelcolumnlist.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelconsole.h
# End Source File
# Begin Source File

SOURCE=..\appmain\paneldatabaseinfo.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelexprbuilderdoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelextensionmgr.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelfileinfo.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelfind.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelgroup.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelindex.h
# End Source File
# Begin Source File

SOURCE=..\appmain\paneljobmgr.h
# End Source File
# Begin Source File

SOURCE=..\appmain\paneljobscheduler.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelmarkmgr.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelmerge.h
# End Source File
# Begin Source File

SOURCE=..\appmain\paneloptions.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelrelationship.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelreplacerows.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelsplit.h
# End Source File
# Begin Source File

SOURCE=..\appmain\panelview.h
# End Source File
# Begin Source File

SOURCE=..\appmain\pkgfile.h
# End Source File
# Begin Source File

SOURCE=..\appmain\querydoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\querytemplate.h
# End Source File
# Begin Source File

SOURCE=..\appmain\relationdiagram.h
# End Source File
# Begin Source File

SOURCE=..\appmain\relationdnd.h
# End Source File
# Begin Source File

SOURCE=..\appmain\reportdesign.h
# End Source File
# Begin Source File

SOURCE=..\appmain\reportdoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\reportengine.h
# End Source File
# Begin Source File

SOURCE=..\appmain\reportlayout.h
# End Source File
# Begin Source File

SOURCE=..\appmain\reportmodel.h
# End Source File
# Begin Source File

SOURCE=..\appmain\reportstore.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptapp.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptbanner.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptbase64.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptbitmap.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptbutton.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptcombo.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptcrypt.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptdb.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptdlg.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptenvironment.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptfile.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptfiletransfer.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptfont.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptgraphics.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptgui.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scripthash.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scripthost.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scripthostapp.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scripthttp.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptlayout.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptlist.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptlistview.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptmemory.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptmenu.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptnative.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptprocess.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptprogress.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptreport.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptslider.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptspin.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptstatusbar.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptsystem.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scripttable.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scripttext.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scripttoolbar.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scripttreeview.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptwebbrowser.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptwebdom.h
# End Source File
# Begin Source File

SOURCE=..\appmain\scriptxml.h
# End Source File
# Begin Source File

SOURCE=..\appmain\sqldoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\structuredoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\structurevalidator.h
# End Source File
# Begin Source File

SOURCE=..\appmain\tabledoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\tabledoc_private.h
# End Source File
# Begin Source File

SOURCE=..\appmain\tabledocmodel.h
# End Source File
# Begin Source File

SOURCE=..\appmain\tangogridmodel.h
# End Source File
# Begin Source File

SOURCE=..\appmain\tangojobbase.h
# End Source File
# Begin Source File

SOURCE=..\appmain\textdoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\textview.h
# End Source File
# Begin Source File

SOURCE=..\appmain\toolbars.h
# End Source File
# Begin Source File

SOURCE=..\appmain\transformationdoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\updater.h
# End Source File
# Begin Source File

SOURCE=..\appmain\webdoc.h
# End Source File
# Begin Source File

SOURCE=..\appmain\webserver.h
# End Source File
# End Group
# Begin Group "Application Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\config\appversion.h
# End Source File
# Begin Source File

SOURCE=.\config\resources.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\wx\msw\blank.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\bullseye.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\cdrom.ico
# End Source File
# Begin Source File

SOURCE=.\wx\msw\computer.ico
# End Source File
# Begin Source File

SOURCE=.\wx\msw\cross.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\drive.ico
# End Source File
# Begin Source File

SOURCE=.\wx\msw\file1.ico
# End Source File
# Begin Source File

SOURCE=.\wx\msw\floppy.ico
# End Source File
# Begin Source File

SOURCE=.\wx\msw\folder1.ico
# End Source File
# Begin Source File

SOURCE=.\wx\msw\folder2.ico
# End Source File
# Begin Source File

SOURCE=.\wx\msw\hand.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\magnif1.cur
# End Source File
# Begin Source File

SOURCE=.\Makefile
# End Source File
# Begin Source File

SOURCE=.\wx\msw\noentry.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\pbrush.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\pencil.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\pntleft.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\pntright.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\query.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\removble.ico
# End Source File
# Begin Source File

SOURCE=.\wx\msw\rightarr.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\roller.cur
# End Source File
# Begin Source File

SOURCE=.\wx\msw\watch1.cur
# End Source File
# End Target
# End Project
