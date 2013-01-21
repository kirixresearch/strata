#/*!
# *
# * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  wxWebConnect Test Application
# * Author:   Benjamin I. Williams
# * Created:  2009-05-09
# *
# */


# These two settings need to be adjusted to your build environment
WX_DIR := ../wxWidgets
WX_CONFIG := ${WX_DIR}/lib/wx/config/inplace-gtk2-unicode-release-static-2.8 


WX_CFLAGS := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --cppflags)
WX_LIBS := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --libs)

EXENAME = testapp
INCLUDES = 
CPP = g++
CFLAGS = -ggdb $(WX_CFLAGS)
LIBS = -L../webconnect -lwebconnect $(WX_LIBS)
DEPENDLIBS = ../webconnect/libwebconnect.a 

OBJECTS = testapp.o

all: $(EXENAME)
    
$(EXENAME): $(OBJECTS) $(DEPENDLIBS)
		$(CPP) $(OBJECTS) $(LIBDIRS) $(LIBS) -o $(EXENAME)

clean:
	rm -f *.o *.gch $(EXENAME)

$(OBJECTS): %.o : %.cpp
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<

