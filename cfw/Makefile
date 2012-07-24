#/*!
# *
# * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Application Client Framework
# * Author:   Benjamin I. Williams
# * Created:  2003-05-06
# *
# */


include ../top.mak

INCLUDES = -I../xcm/include -I../kl/include -I${WXDIR}/include -I${WXDIR}/contrib/include 
DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = $(TOP_CFLAGS) $(WX_CFLAGS) $(GTK_CFLAGS)
LIBS = ${WX_LIBS)


OBJECTS = \
	artprovider.o \
	cfw.o \
	framework.o \
	frameworkprops.o \
	fsitems.o \
	fspanel.o \
	fspanellistview.o \
	fspaneltreeview.o \
	jobinfo.o \
	jobqueue.o \
	prefs.o \
	propertiespanel.o \
	statusbar.o \
	util.o

all: libcfw.a

libcfw.a:	$(OBJECTS)
	ar rc libcfw.a $(OBJECTS)
	ranlib libcfw.a

clean:
	rm -f *.o libcfw.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


