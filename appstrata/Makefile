#/*!
# *
# * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Application Client
# * Author:   Benjamin I. Williams
# * Created:  2002-12-16
# *
# */


include ../top.mak 


# output image name
#
EXENAME = kstrata

# linker relative path info
#
ifeq ($(X_ARCH), LINUX)
LDINFO = -Wl,--rpath -Wl,.:../lib
else
LDINFO = 
endif


INCLUDES = -I./config -I../appmain -I../xcm/include -I../kl/include -I../tango/include -I${WX_DIR}/include -I${WX_DIR}/contrib/include 
CFLAGS = $(TOP_CFLAGS) $(WX_CFLAGS) $(CURL_CFLAGS) 
LIBS = -L../kcanvas -L../cfw -L../kcl -L../webconnect -L../kscript -L../paladin -L../kl -lkcanvas -lcfw -lkcl -lwebconnect -lkscript -lpaladin -lkl $(WXSTC_LIBS) $(WX_LIBS) $(CURL_LIBS) $(HARU_LIBS)



UI_DEFINE = -DAPP_CONSOLE=0 -DAPP_GUI=1
ifeq ($(UI_MODE), CONSOLE)
UI_DEFINE = -DAPP_CONSOLE=1 -DAPP_GUI=0
endif

DEPENDLIBS = \
	../paladin/libpaladin.a \
	../kcl/libkcl.a \
	../cfw/libcfw.a \
	../kcanvas/libkcanvas.a \
	../webconnect/libwebconnect.a

APPOBJECTS = \
	appstrata.o 

OBJECTS = \
	app.o \
	appcontroller.o \
	appprefs.o \
	appprint.o \
	apputil.o \
	bitmapmgr.o \
	bookmark.o \
	bookmarkfs.o \
	commandmgr.o \
	commands.o \
	connectionmgr.o \
	connectionpages.o \
	connectionwizard.o \
	curlutil.o \
	dbcombo.o \
	dbdoc.o \
	dlgauth.o \
	dlgdatabasefile.o \
	dlglinkprops.o \
	dlgpagesetup.o \
	dlgprojectmgr.o \
	dlgreportprops.o \
	editordoc.o \
	exportpages.o \
	exportwizard.o \
	exprbuilder.o \
	extensionmgr.o \
	extensionpkg.o \
	feedparser.o \
	fieldlistcontrol.o \
	fieldtypechoice.o \
	importpages.o \
	importtemplate.o \
	importwizard.o \
	inetauth.o \
	jobappend.o \
	jobcopy.o \
	jobexport.o \
	jobexportpkg.o \
	jobgroup.o \
	jobfindinfiles.o \
	jobftp.o \
	jobimport.o \
	jobimportpkg.o \
	jobindex.o \
	jobmodifystruct.o \
	jobquery.o \
	jobscheduler.o \
	jobscript.o \
	jobsplit.o \
	jsonconfig.o \
	linkbar.o \
	moduleremoveduprec.o \
	mongoose.o \
	panelcolordoc.o \
	panelcolprops.o \
	panelcolumnlist.o \
	panelconsole.o \
	paneldatabaseinfo.o \
	panelextensionmgr.o \
	panelexprbuilderdoc.o \
	panelfileinfo.o \
	panelfind.o \
	panelgroup.o \
	panelindex.o \
	paneljobmgr.o \
	paneljobscheduler.o \
	panelmarkmgr.o \
	panelmerge.o \
	panelrelationship.o \
	panelreplacerows.o \
	paneloptions.o \
	panelsplit.o \
	panelview.o \
	pkgfile.o \
	querydoc.o \
	querytemplate.o \
	relationdiagram.o \
	reportdesign.o \
	reportdoc.o \
	reportengine.o \
	reportlayout.o \
	reportmodel.o \
	reportstore.o \
	scriptapp.o \
	scriptbanner.o \
	scriptbase64.o \
	scriptbitmap.o \
	scriptbutton.o \
	scriptcombo.o \
	scriptcrypt.o \
	scriptdb.o \
	scriptdlg.o \
	scriptenvironment.o \
	scriptfile.o \
	scriptfiletransfer.o \
	scriptfont.o \
	scriptgraphics.o \
	scriptgui.o \
	scripthash.o \
	scripthost.o \
	scripthostapp.o \
	scripthttp.o \
	scriptlayout.o \
	scriptlist.o \
	scriptlistview.o \
	scriptmemory.o \
	scriptmenu.o \
	scriptnative.o \
	scriptprocess.o \
	scriptprogress.o \
	scriptreport.o \
	scriptslider.o \
	scriptspin.o \
	scriptstatusbar.o \
	scriptsystem.o \
	scripttable.o \
	scripttext.o \
	scripttoolbar.o \
	scripttreeview.o \
	scriptwebbrowser.o \
	scriptwebdom.o \
	scriptxml.o \
	sqldoc.o \
	structuredoc.o \
	structurevalidator.o \
	tabledoc.o \
	tabledocapi.o \
	tabledocmodel.o \
	tangogridmodel.o \
	textdoc.o \
	textview.o \
	toolbars.o \
	transformationdoc.o \
	updater.o \
	webdoc.o \
	webserver.o

ifeq ($(X_ARCH), MAC)
MACREZDEP=macrez
endif


VPATH=$(OUTPUT_DIR):

all: $(EXENAME) $(LIBAPPMAIN) $(MACREZDEP)

$(EXENAME): $(APPOBJECTS) $(OBJECTS) $(DEPENDLIBS)
		$(CPP) $(APPOBJECTS) $(OBJECTS) $(LIBDIRS) $(LIBS) $(LDINFO) -o $(OUTPUT_DIR)/$(EXENAME)

macrez:
	/Developer/Tools/Rez -d __DARWIN__ -t APPL -d __WXMAC -o $(OUTPUT_DIR)/$(EXENAME) ../build/appmain.r

appmain.h.gch: appmain.h
	$(CPP) $(CFLAGS) $(UI_DEFINE) $(INCLUDES) appmain.h

clean:
	rm -f *.o *.gch $(OUTPUT_DIR)/$(EXENAME)

$(OBJECTS): %.o : ../appmain/%.cpp
	$(CPP) $(CFLAGS) $(UI_DEFINE) $(INCLUDES) -c $<

$(APPOBJECTS): %.o : %.cpp
	$(CPP) $(CFLAGS) $(UI_DEFINE) $(INCLUDES) -c $<

