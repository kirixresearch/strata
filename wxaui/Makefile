#///////////////////////////////////////////////////////////////////////////////
#// Name:        Makefile
#// Purpose:     wxaui: wx advanced user interface
#// Author:      Benjamin I. Williams
#// Modified by:
#// Created:     2005-10-03
#// RCS-ID:      
#// Copyright:   Copyright (c) 2005, Kirix Research, LLC.  All rights reserved.
#// Licence:     wxWindows Library Licence, Version 3.1
#///////////////////////////////////////////////////////////////////////////////


# change these two to match your setup
WX_DIR := ../supp/wxGTK-2.6.3
WX_CONFIG := ${WX_DIR}/lib/wx/config/inplace-gtk2-unicode-debug-static-2.6

WX_CFLAGS := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --cppflags)
WX_LIBS := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --libs)

DEFINES = 
CPP = g++
CFLAGS = ${WX_CFLAGS} -g -ggdb
INCLUDES = -I${WXDIR}/include -I./include


OBJECTS = \
	manager.o

VPATH=src:lib

all: libwxaui.a sample

libwxaui.a: $(OBJECTS)
	ar rc lib/libwxaui.a $(OBJECTS)
	ranlib lib/libwxaui.a

sample: $(OBJECTS)
	cd sample && $(MAKE) -w -f Makefile

clean:
	rm -f *.o lib/libwxaui.a sample/*.o sample/wxauitest

$(OBJECTS): %.o : %.cpp
	$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


