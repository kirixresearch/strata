#/*!
# *
# * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Application Client
# * Author:   Benjamin I. Williams
# * Created:  2002-12-16
# *
# */


include ../top.mak 


# output image name
#
EXENAME = appmain

# linker relative path info
#
ifeq ($(X_ARCH), LINUX)
LDINFO = -Wl,--rpath -Wl,.:../lib
else
LDINFO = 
endif


INCLUDES = -I./config -I../xcm/include -I../kl/include -I../xd/include -I${WX_DIR}/include -I${WX_DIR}/contrib/include 
CFLAGS = $(TOP_CFLAGS) $(WX_CFLAGS) $(GTK_CFLAGS) $(CURL_CFLAGS) 
LIBS = -L../kcanvas -L../kcl -L../webconnect -L../kscript -L../paladin -L../kl -lkcanvas -lkcl -lwebconnect -lkscript -lpaladin -lkl $(WXSTC_LIBS) $(WX_LIBS) $(CURL_LIBS) $(HARU_LIBS)



UI_DEFINE = -DAPP_CONSOLE=0 -DAPP_GUI=1
ifeq ($(UI_MODE), CONSOLE)
UI_DEFINE = -DAPP_CONSOLE=1 -DAPP_GUI=0
endif

DEPENDLIBS = \
	../paladin/libpaladin.a \
	../kcl/libkcl.a \
	../jobs/libjobs.a \
	../kcanvas/libkcanvas.a \
	../webconnect/libwebconnect.a

OBJECTS = \
	app.o \
	appcontroller.o \
	appmain.o \
	appprefs.o \
	appprint.o \
	bitmapmgr.o \
	bookmarkfs.o \
	commandmgr.o \
	commands.o \
	connectionmgr.o \
	curlutil.o \
	dbcombo.o \
	dbdoc.o \
	dlgauth.o \
	dlgconnection.o \
	dlgcustomprompt.o \
	dlgdatabasefile.o \
	dlglinkprops.o \
	dlgpagesetup.o \
	dlgpassword.o \
	dlgprojectmgr.o \
	dlgreportprops.o \
	dlgshareview.o \
	editordoc.o \
	exporttemplate.o \
	exprbuilder.o \
	extensionmgr.o \
	extensionpkg.o \
	feedparser.o \
	fieldlistcontrol.o \
	fieldtypechoice.o \
	importtemplate.o \
	inetauth.o \
	jobfindinfiles.o \
	jobqueue.o \
	jobscheduler.o \
	jobscript.o \
	jsonconfig.o \
	linkbar.o \
	moduleremoveduprec.o \
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
	querydoc.o \
	querytemplate.o \
	relationdiagram.o \
	reportdesign.o \
	reportdoc.o \
	reportengine.o \
	reportlayout.o \
	reportmodel.o \
	scriptapp.o \
	scriptbanner.o \
	scriptbitmap.o \
	scriptbutton.o \
	scriptcombo.o \
	scriptdlg.o \
	scriptfont.o \
	scriptgraphics.o \
	scriptgui.o \
	scripthost.o \
	scripthostapp.o \
	scriptlayout.o \
	scriptlist.o \
	scriptlistview.o \
	scriptmenu.o \
	scriptprocess.o \
	scriptprogress.o \
	scriptreport.o \
	scriptslider.o \
	scriptspin.o \
	scriptstatusbar.o \
	scriptsystemgui.o \
	scripttable.o \
	scripttext.o \
	scripttoolbar.o \
	scripttreeview.o \
	scriptwebbrowser.o \
	scriptwebdom.o \
	sqldoc.o \
	structuredoc.o \
	structurevalidator.o \
	tabledoc.o \
	tabledocapi.o \
	tabledocmodel.o \
	textdoc.o \
	textview.o \
	toolbars.o \
	transformationdoc.o \
	updater.o \
	util.o \
	webdoc.o \
	xdgridmodel.o


ifeq ($(X_ARCH), MAC)
MACREZDEP=macrez
endif


VPATH=$(OUTPUT_DIR):

all: $(EXENAME) $(MACREZDEP)

$(EXENAME): $(OBJECTS) $(DEPENDLIBS)
		$(CPP) $(OBJECTS) $(LIBDIRS) $(LIBS) $(LDINFO) -o $(OUTPUT_DIR)/$(EXENAME)

macrez:
	/Developer/Tools/Rez -d __DARWIN__ -t APPL -d __WXMAC -o $(OUTPUT_DIR)/$(EXENAME) ../build/appmain.r

appmain.h.gch: appmain.h
	$(CPP) $(CFLAGS) $(UI_DEFINE) $(INCLUDES) appmain.h

clean:
	rm -f *.o *.gch $(OUTPUT_DIR)/$(EXENAME)

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(UI_DEFINE) $(INCLUDES) -c $<

