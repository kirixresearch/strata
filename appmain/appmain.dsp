# Microsoft Developer Studio Project File - Name="appmain" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=appmain - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "appmain.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "appmain.mak" CFG="appmain - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "appmain - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "appmain - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "appmain - Win32 Debug Unicode" (based on "Win32 (x86) Application")
!MESSAGE "appmain - Win32 Release Unicode" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/appmain", CAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "appmain - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/msw" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/src/zlib" /I "../supp/freetype/include" /I "../tango/include" /D appmain_BUILD_TIER=2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "CURL_STATICLIB" /YX"appmain.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "../supp/wxWidgets/include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase26.lib wxbase26_net.lib wxmsw26_core.lib wxmsw26_adv.lib wxmsw26_propgrid.lib wxmsw26_stc.lib wxjpeg.lib wxpng.lib wxzlib.lib wxaui.lib freetype219MT.lib ../kl/lib/kl.lib ../kcl/lib/kcl.lib ../cfw/lib/cfw.lib ../paladin/lib/paladin.lib ../kscript/lib/kscript.lib ../kcanvas/lib/kcanvas.lib ../webconnect/lib/webconnect.lib /nologo /subsystem:windows /machine:I386 /out:"../release/appmain.exe" /libpath:"../supp/wxWidgets/lib/vc_lib" /libpath:"../supp/wxpdfdoc/lib"

!ELSEIF  "$(CFG)" == "appmain - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "appmain___Win32_Debug0"
# PROP BASE Intermediate_Dir "appmain___Win32_Debug0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswd" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/src/zlib" /I "../supp/freetype/include" /I "../tango/include" /D appmain_BUILD_TIER=2 /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "CURL_STATICLIB" /YX"appmain.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "../supp/wxWidgets/include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase26d.lib wxbase26d_net.lib wxmsw26d_core.lib wxmsw26d_adv.lib wxmsw26d_propgrid.lib wxmsw26d_stc.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxauid.lib freetype219MT_D.lib ../kl/lib/kld.lib ../kcl/lib/kcld.lib ../cfw/lib/cfwd.lib ../paladin/lib/paladind.lib ../kscript/lib/kscriptd.lib ../kcanvas/lib/kcanvasd.lib ../webconnect/lib/webconnectd.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../debug/appmain.exe" /pdbtype:sept /libpath:"../supp/wxWidgets/lib/vc_lib" /libpath:"../supp/wxpdfdoc/lib"

!ELSEIF  "$(CFG)" == "appmain - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "appmain___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "appmain___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /Zi /Od /I "./config" /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswud" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/src/zlib" /I "../supp/wxpdfdoc/include" /I "../tango/include" /I "../supp/curl/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "_UNICODE" /D "UNICODE" /D "CURL_STATICLIB" /YX"appmain.h" /FD /Zm500 /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "../supp/wxWidgets/include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wxmswd.lib comctl32.lib wsock32.lib rpcrt4.lib flud.lib pngd.lib zlibd.lib freetype214MT_D.lib ../../kcl/lib/kcld.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../bin/appmain.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase28ud.lib wxbase28ud_net.lib wxmsw28ud_core.lib wxmsw28ud_adv.lib wxmsw28ud_html.lib wxmsw28ud_aui.lib wxbase28ud_xml.lib wxmsw28ud_stc.lib wxpdfdocud.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxexpatd.lib ../kl/lib/klud.lib ../kcl/lib/kclud.lib ../cfw/lib/cfwud.lib ../paladin/lib/paladinud.lib ../kscript/lib/kscriptud.lib ../kcanvas/lib/kcanvasud.lib ../webconnect/lib/webconnectud.lib ../supp/haru/lib/harud.lib ../supp/curl/lib/debug-ssl/libcurld.lib ../supp/libssh2/win32/Debug_lib/libssh2d.lib ../supp/openssl/out32.dbg/libeay32.lib ../supp/openssl/out32.dbg/ssleay32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../debugu/appmain.exe" /pdbtype:sept /libpath:"../supp/wxWidgets/lib/vc_lib" /libpath:"../supp/wxpdfdoc/lib"

!ELSEIF  "$(CFG)" == "appmain - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "appmain___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "appmain___Win32_Release_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zd /O2 /I "./config" /I "../kl/include" /I "../supp/wxWidgets/include" /I "../supp/wxWidgets/lib/vc_lib/mswu" /I "../supp/wxWidgets/contrib/include" /I "../supp/wxWidgets/src/zlib" /I "../tango/include" /I "../supp/curl/include" /I "../supp/wxpdfdoc/include" /D appmain_BUILD_TIER=3 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D "_UNICODE" /D "UNICODE" /D "CURL_STATICLIB" /YX"appmain.h" /FD /Zm500 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "../supp/wxWidgets/include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxmsw.lib fl.lib png.lib zlib.lib freetype214MT.lib ../../kcl/lib/kcl.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wsock32.lib rpcrt4.lib wxbase28u.lib wxbase28u_net.lib wxmsw28u_core.lib wxmsw28u_adv.lib wxmsw28u_html.lib wxmsw28u_aui.lib wxbase28u_xml.lib wxmsw28u_stc.lib wxpdfdocu.lib wxjpeg.lib wxpng.lib wxzlib.lib wxexpat.lib ../kl/lib/klu.lib ../kcl/lib/kclu.lib ../cfw/lib/cfwu.lib ../paladin/lib/paladinu.lib ../kscript/lib/kscriptu.lib ../kcanvas/lib/kcanvasu.lib ../webconnect/lib/webconnectu.lib ../supp/haru/lib/haru.lib ../supp/curl/lib/release-ssl/libcurl.lib ../supp/libssh2/win32/Release_lib/libssh.lib ../supp/openssl/out32/libeay32.lib ../supp/openssl/out32/ssleay32.lib /nologo /subsystem:windows /map /machine:I386 /out:"../releaseu/appmain.exe" /libpath:"../supp/wxWidgets/lib/vc_lib" /libpath:"../supp/wxpdfdoc/lib" /MAPINFO:LINES /MAPINFO:EXPORTS
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "appmain - Win32 Release"
# Name "appmain - Win32 Debug"
# Name "appmain - Win32 Debug Unicode"
# Name "appmain - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\app.cpp
# End Source File
# Begin Source File

SOURCE=.\appcontroller.cpp
# End Source File
# Begin Source File

SOURCE=.\appmain.cpp
# End Source File
# Begin Source File

SOURCE=.\appprefs.cpp
# End Source File
# Begin Source File

SOURCE=.\appprint.cpp
# End Source File
# Begin Source File

SOURCE=.\apputil.cpp
# End Source File
# Begin Source File

SOURCE=.\bitmapmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\bookmark.cpp
# End Source File
# Begin Source File

SOURCE=.\bookmarkfs.cpp
# End Source File
# Begin Source File

SOURCE=.\commandmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\commands.cpp
# End Source File
# Begin Source File

SOURCE=.\connectionmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\connectionpages.cpp
# End Source File
# Begin Source File

SOURCE=.\connectionwizard.cpp
# End Source File
# Begin Source File

SOURCE=.\curlutil.cpp
# End Source File
# Begin Source File

SOURCE=.\dbcombo.cpp
# End Source File
# Begin Source File

SOURCE=.\dbdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgauth.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgdatabasefile.cpp
# End Source File
# Begin Source File

SOURCE=.\dlglinkprops.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgpagesetup.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgpassword.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgprojectmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgreportprops.cpp
# End Source File
# Begin Source File

SOURCE=.\editordoc.cpp
# End Source File
# Begin Source File

SOURCE=.\exportpages.cpp
# End Source File
# Begin Source File

SOURCE=.\exportwizard.cpp
# End Source File
# Begin Source File

SOURCE=.\exprbuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\extensionmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\extensionpkg.cpp
# End Source File
# Begin Source File

SOURCE=.\feedparser.cpp
# End Source File
# Begin Source File

SOURCE=.\fieldlistcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\fieldtypechoice.cpp
# End Source File
# Begin Source File

SOURCE=.\importpages.cpp
# End Source File
# Begin Source File

SOURCE=.\importtemplate.cpp
# End Source File
# Begin Source File

SOURCE=.\importwizard.cpp
# End Source File
# Begin Source File

SOURCE=.\inetauth.cpp
# End Source File
# Begin Source File

SOURCE=.\jobappend.cpp
# End Source File
# Begin Source File

SOURCE=.\jobcopy.cpp
# End Source File
# Begin Source File

SOURCE=.\jobexport.cpp
# End Source File
# Begin Source File

SOURCE=.\jobexportpkg.cpp
# End Source File
# Begin Source File

SOURCE=.\jobfindinfiles.cpp
# End Source File
# Begin Source File

SOURCE=.\jobftp.cpp
# End Source File
# Begin Source File

SOURCE=.\jobgroup.cpp
# End Source File
# Begin Source File

SOURCE=.\jobimport.cpp
# End Source File
# Begin Source File

SOURCE=.\jobimportpkg.cpp
# End Source File
# Begin Source File

SOURCE=.\jobindex.cpp
# End Source File
# Begin Source File

SOURCE=.\jobmodifystruct.cpp
# End Source File
# Begin Source File

SOURCE=.\jobquery.cpp
# End Source File
# Begin Source File

SOURCE=.\jobscheduler.cpp
# End Source File
# Begin Source File

SOURCE=.\jobscript.cpp
# End Source File
# Begin Source File

SOURCE=.\jobsplit.cpp
# End Source File
# Begin Source File

SOURCE=.\jsonconfig.cpp
# End Source File
# Begin Source File

SOURCE=.\linkbar.cpp
# End Source File
# Begin Source File

SOURCE=.\moduleremoveduprec.cpp
# End Source File
# Begin Source File

SOURCE=.\mongoose_vc6.cpp
# End Source File
# Begin Source File

SOURCE=.\panelcolordoc.cpp
# End Source File
# Begin Source File

SOURCE=.\panelcolprops.cpp
# End Source File
# Begin Source File

SOURCE=.\panelcolumnlist.cpp
# End Source File
# Begin Source File

SOURCE=.\panelconsole.cpp
# End Source File
# Begin Source File

SOURCE=.\paneldatabaseinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\panelexprbuilderdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\panelextensionmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\panelfileinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\panelfind.cpp
# End Source File
# Begin Source File

SOURCE=.\panelgroup.cpp
# End Source File
# Begin Source File

SOURCE=.\panelindex.cpp
# End Source File
# Begin Source File

SOURCE=.\paneljobmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\paneljobscheduler.cpp
# End Source File
# Begin Source File

SOURCE=.\panelmarkmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\panelmerge.cpp
# End Source File
# Begin Source File

SOURCE=.\paneloptions.cpp
# End Source File
# Begin Source File

SOURCE=.\panelrelationship.cpp
# End Source File
# Begin Source File

SOURCE=.\panelreplacerows.cpp
# End Source File
# Begin Source File

SOURCE=.\panelsplit.cpp
# End Source File
# Begin Source File

SOURCE=.\panelview.cpp
# End Source File
# Begin Source File

SOURCE=.\pkgfile.cpp
# End Source File
# Begin Source File

SOURCE=.\querydoc.cpp
# End Source File
# Begin Source File

SOURCE=.\querytemplate.cpp
# End Source File
# Begin Source File

SOURCE=.\relationdiagram.cpp
# End Source File
# Begin Source File

SOURCE=.\reportdesign.cpp
# End Source File
# Begin Source File

SOURCE=.\reportdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\reportengine.cpp
# End Source File
# Begin Source File

SOURCE=.\reportlayout.cpp
# End Source File
# Begin Source File

SOURCE=.\reportmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\reportstore.cpp
# End Source File
# Begin Source File

SOURCE=.\config\resources.rc
# End Source File
# Begin Source File

SOURCE=.\scriptapp.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptbanner.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptbase64.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptbitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptbutton.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptcombo.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptcrypt.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptdb.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptenvironment.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptfile.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptfiletransfer.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptfont.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptgraphics.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptgui.cpp
# End Source File
# Begin Source File

SOURCE=.\scripthash.cpp
# End Source File
# Begin Source File

SOURCE=.\scripthost.cpp
# End Source File
# Begin Source File

SOURCE=.\scripthostapp.cpp
# End Source File
# Begin Source File

SOURCE=.\scripthttp.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptlayout.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptlist.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptlistview.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptmemory.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptnative.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptprocess.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptprogress.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptreport.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptslider.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptspin.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptstatusbar.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptsystem.cpp
# End Source File
# Begin Source File

SOURCE=.\scripttable.cpp
# End Source File
# Begin Source File

SOURCE=.\scripttext.cpp
# End Source File
# Begin Source File

SOURCE=.\scripttoolbar.cpp
# End Source File
# Begin Source File

SOURCE=.\scripttreeview.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptwebbrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptwebdom.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptxml.cpp
# End Source File
# Begin Source File

SOURCE=.\sqldoc.cpp
# End Source File
# Begin Source File

SOURCE=.\structuredoc.cpp
# End Source File
# Begin Source File

SOURCE=.\structurevalidator.cpp
# End Source File
# Begin Source File

SOURCE=.\tabledoc.cpp
# End Source File
# Begin Source File

SOURCE=.\tabledocapi.cpp
# End Source File
# Begin Source File

SOURCE=.\tabledocmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\tangogridmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\textdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\textview.cpp
# End Source File
# Begin Source File

SOURCE=.\toolbars.cpp
# End Source File
# Begin Source File

SOURCE=.\transformationdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\updater.cpp
# End Source File
# Begin Source File

SOURCE=.\webdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\webserver.cpp
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

SOURCE=.\appmain.h
# End Source File
# Begin Source File

SOURCE=.\appprefs.h
# End Source File
# Begin Source File

SOURCE=.\appprint.h
# End Source File
# Begin Source File

SOURCE=.\apputil.h
# End Source File
# Begin Source File

SOURCE=.\config\appversion.h
# End Source File
# Begin Source File

SOURCE=.\bitmapmgr.h
# End Source File
# Begin Source File

SOURCE=.\bookmark.h
# End Source File
# Begin Source File

SOURCE=.\bookmarkfs.h
# End Source File
# Begin Source File

SOURCE=.\commandmgr.h
# End Source File
# Begin Source File

SOURCE=.\commands.h
# End Source File
# Begin Source File

SOURCE=.\connectionmgr.h
# End Source File
# Begin Source File

SOURCE=.\connectionpages.h
# End Source File
# Begin Source File

SOURCE=.\connectionwizard.h
# End Source File
# Begin Source File

SOURCE=.\curlutil.h
# End Source File
# Begin Source File

SOURCE=.\dbcombo.h
# End Source File
# Begin Source File

SOURCE=.\dbdoc.h
# End Source File
# Begin Source File

SOURCE=.\dlgauth.h
# End Source File
# Begin Source File

SOURCE=.\dlgdatabasefile.h
# End Source File
# Begin Source File

SOURCE=.\dlgexpired.h
# End Source File
# Begin Source File

SOURCE=.\dlglinkprops.h
# End Source File
# Begin Source File

SOURCE=.\dlgpagesetup.h
# End Source File
# Begin Source File

SOURCE=.\dlgpassword.h
# End Source File
# Begin Source File

SOURCE=.\dlgprojectmgr.h
# End Source File
# Begin Source File

SOURCE=.\dlgreportprops.h
# End Source File
# Begin Source File

SOURCE=.\editordoc.h
# End Source File
# Begin Source File

SOURCE=.\exportpages.h
# End Source File
# Begin Source File

SOURCE=.\exportwizard.h
# End Source File
# Begin Source File

SOURCE=.\exprbuilder.h
# End Source File
# Begin Source File

SOURCE=.\extensionmgr.h
# End Source File
# Begin Source File

SOURCE=.\extensionpkg.h
# End Source File
# Begin Source File

SOURCE=.\feedparser.h
# End Source File
# Begin Source File

SOURCE=.\fieldlistcontrol.h
# End Source File
# Begin Source File

SOURCE=.\fieldtypechoice.h
# End Source File
# Begin Source File

SOURCE=.\importpages.h
# End Source File
# Begin Source File

SOURCE=.\importtemplate.h
# End Source File
# Begin Source File

SOURCE=.\importwizard.h
# End Source File
# Begin Source File

SOURCE=.\inetauth.h
# End Source File
# Begin Source File

SOURCE=.\jobappend.h
# End Source File
# Begin Source File

SOURCE=.\jobcopy.h
# End Source File
# Begin Source File

SOURCE=.\jobdelete.h
# End Source File
# Begin Source File

SOURCE=.\jobexport.h
# End Source File
# Begin Source File

SOURCE=.\jobexportpkg.h
# End Source File
# Begin Source File

SOURCE=.\jobfindinfiles.h
# End Source File
# Begin Source File

SOURCE=.\jobftp.h
# End Source File
# Begin Source File

SOURCE=.\jobgroup.h
# End Source File
# Begin Source File

SOURCE=.\jobimport.h
# End Source File
# Begin Source File

SOURCE=.\jobimportpkg.h
# End Source File
# Begin Source File

SOURCE=.\jobindex.h
# End Source File
# Begin Source File

SOURCE=.\jobmodifystruct.h
# End Source File
# Begin Source File

SOURCE=.\jobquery.h
# End Source File
# Begin Source File

SOURCE=.\jobreplace.h
# End Source File
# Begin Source File

SOURCE=.\jobscheduler.h
# End Source File
# Begin Source File

SOURCE=.\jobscript.h
# End Source File
# Begin Source File

SOURCE=.\jobsplit.h
# End Source File
# Begin Source File

SOURCE=.\jsonconfig.h
# End Source File
# Begin Source File

SOURCE=.\linkbar.h
# End Source File
# Begin Source File

SOURCE=.\moduleremoveduprec.h
# End Source File
# Begin Source File

SOURCE=.\mongoose.h
# End Source File
# Begin Source File

SOURCE=.\panelcolordoc.h
# End Source File
# Begin Source File

SOURCE=.\panelcolprops.h
# End Source File
# Begin Source File

SOURCE=.\panelcolumnlist.h
# End Source File
# Begin Source File

SOURCE=.\panelconsole.h
# End Source File
# Begin Source File

SOURCE=.\paneldatabaseinfo.h
# End Source File
# Begin Source File

SOURCE=.\panelexprbuilderdoc.h
# End Source File
# Begin Source File

SOURCE=.\panelextensionmgr.h
# End Source File
# Begin Source File

SOURCE=.\panelfileinfo.h
# End Source File
# Begin Source File

SOURCE=.\panelfind.h
# End Source File
# Begin Source File

SOURCE=.\panelgroup.h
# End Source File
# Begin Source File

SOURCE=.\panelindex.h
# End Source File
# Begin Source File

SOURCE=.\paneljobmgr.h
# End Source File
# Begin Source File

SOURCE=.\paneljobscheduler.h
# End Source File
# Begin Source File

SOURCE=.\panelmarkmgr.h
# End Source File
# Begin Source File

SOURCE=.\panelmerge.h
# End Source File
# Begin Source File

SOURCE=.\paneloptions.h
# End Source File
# Begin Source File

SOURCE=.\panelrelationship.h
# End Source File
# Begin Source File

SOURCE=.\panelreplacerows.h
# End Source File
# Begin Source File

SOURCE=.\panelreportgen.h
# End Source File
# Begin Source File

SOURCE=.\panelsplit.h
# End Source File
# Begin Source File

SOURCE=.\paneltabbar.h
# End Source File
# Begin Source File

SOURCE=.\panelview.h
# End Source File
# Begin Source File

SOURCE=.\pkgfile.h
# End Source File
# Begin Source File

SOURCE=.\querydoc.h
# End Source File
# Begin Source File

SOURCE=.\querytemplate.h
# End Source File
# Begin Source File

SOURCE=.\relationdiagram.h
# End Source File
# Begin Source File

SOURCE=.\relationdnd.h
# End Source File
# Begin Source File

SOURCE=.\reportdesign.h
# End Source File
# Begin Source File

SOURCE=.\reportdoc.h
# End Source File
# Begin Source File

SOURCE=.\reportengine.h
# End Source File
# Begin Source File

SOURCE=.\reportlayout.h
# End Source File
# Begin Source File

SOURCE=.\reportmodel.h
# End Source File
# Begin Source File

SOURCE=.\reportstore.h
# End Source File
# Begin Source File

SOURCE=.\scriptapp.h
# End Source File
# Begin Source File

SOURCE=.\scriptbanner.h
# End Source File
# Begin Source File

SOURCE=.\scriptbase64.h
# End Source File
# Begin Source File

SOURCE=.\scriptbitmap.h
# End Source File
# Begin Source File

SOURCE=.\scriptbutton.h
# End Source File
# Begin Source File

SOURCE=.\scriptcombo.h
# End Source File
# Begin Source File

SOURCE=.\scriptcrypt.h
# End Source File
# Begin Source File

SOURCE=.\scriptdb.h
# End Source File
# Begin Source File

SOURCE=.\scriptdlg.h
# End Source File
# Begin Source File

SOURCE=.\scriptenvironment.h
# End Source File
# Begin Source File

SOURCE=.\scriptfile.h
# End Source File
# Begin Source File

SOURCE=.\scriptfiletransfer.h
# End Source File
# Begin Source File

SOURCE=.\scriptfont.h
# End Source File
# Begin Source File

SOURCE=.\scriptgraphics.h
# End Source File
# Begin Source File

SOURCE=.\scriptgui.h
# End Source File
# Begin Source File

SOURCE=.\scripthash.h
# End Source File
# Begin Source File

SOURCE=.\scripthost.h
# End Source File
# Begin Source File

SOURCE=.\scripthostapp.h
# End Source File
# Begin Source File

SOURCE=.\scripthttp.h
# End Source File
# Begin Source File

SOURCE=.\scriptlayout.h
# End Source File
# Begin Source File

SOURCE=.\scriptlist.h
# End Source File
# Begin Source File

SOURCE=.\scriptlistview.h
# End Source File
# Begin Source File

SOURCE=.\scriptmemory.h
# End Source File
# Begin Source File

SOURCE=.\scriptmenu.h
# End Source File
# Begin Source File

SOURCE=.\scriptnative.h
# End Source File
# Begin Source File

SOURCE=.\scriptprocess.h
# End Source File
# Begin Source File

SOURCE=.\scriptprogress.h
# End Source File
# Begin Source File

SOURCE=.\scriptreport.h
# End Source File
# Begin Source File

SOURCE=.\scriptslider.h
# End Source File
# Begin Source File

SOURCE=.\scriptspin.h
# End Source File
# Begin Source File

SOURCE=.\scriptstatusbar.h
# End Source File
# Begin Source File

SOURCE=.\scriptsystem.h
# End Source File
# Begin Source File

SOURCE=.\scripttable.h
# End Source File
# Begin Source File

SOURCE=.\scripttext.h
# End Source File
# Begin Source File

SOURCE=.\scripttoolbar.h
# End Source File
# Begin Source File

SOURCE=.\scripttreeview.h
# End Source File
# Begin Source File

SOURCE=.\scriptwebbrowser.h
# End Source File
# Begin Source File

SOURCE=.\scriptwebdom.h
# End Source File
# Begin Source File

SOURCE=.\scriptxml.h
# End Source File
# Begin Source File

SOURCE=.\sqldoc.h
# End Source File
# Begin Source File

SOURCE=.\structuredoc.h
# End Source File
# Begin Source File

SOURCE=.\structurevalidator.h
# End Source File
# Begin Source File

SOURCE=.\tabledoc.h
# End Source File
# Begin Source File

SOURCE=.\tabledoc_private.h
# End Source File
# Begin Source File

SOURCE=.\tabledocmodel.h
# End Source File
# Begin Source File

SOURCE=.\tangogridmodel.h
# End Source File
# Begin Source File

SOURCE=.\tangojobbase.h
# End Source File
# Begin Source File

SOURCE=.\textdoc.h
# End Source File
# Begin Source File

SOURCE=.\textview.h
# End Source File
# Begin Source File

SOURCE=.\toolbars.h
# End Source File
# Begin Source File

SOURCE=.\transformationdoc.h
# End Source File
# Begin Source File

SOURCE=.\updater.h
# End Source File
# Begin Source File

SOURCE=.\webdoc.h
# End Source File
# Begin Source File

SOURCE=.\webserver.h
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
