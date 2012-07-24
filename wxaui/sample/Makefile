#///////////////////////////////////////////////////////////////////////////////
#// Name:        Makefile
#// Purpose:     wxaui: wx advanced user interface - sample/test program
#// Author:      Benjamin I. Williams
#// Modified by:
#// Created:
#// RCS-ID:      
#// Copyright:   Copyright (c) 2005, Kirix Research, LLC.  All rights reserved.
#// Licence:     wxWindows Library Licence, Version 3.1
#///////////////////////////////////////////////////////////////////////////////


WX_DIR := ../../supp/wxGTK-2.6.2
WX_CONFIG := ${WX_DIR}/lib/wx/config/inplace-gtk2-unicode-debug-static-2.6

WX_CFLAGS := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --cppflags)
WX_LIBS := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --libs)



EXENAME = wxauitest
INCLUDES = -I../include
CPP = g++
CFLAGS = $(WX_CFLAGS)
LIBS = ../lib/libwxaui.a $(WX_LIBS)

OBJECTS = wxauitest.o


all: $(EXENAME)
    
$(EXENAME): $(OBJECTS) 
		$(CPP) $(OBJECTS) $(LIBDIRS) $(LIBS) -o $(EXENAME)

clean:
	rm -f *.o *.gch $(EXENAME)

$(OBJECTS): %.o : %.cpp
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<

