#/*!
# *
# * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Aaron L. Williams
# * Created:  2009-12-22
# *
# */


include ../../top.mak

CFLAGS = -fPIC $(TOP_CFLAGS) $(CURL_CFLAGS)
INCLUDES = -I../include -I../../xcm/include -I../../kl/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
LINKFLAGS = --pipe -mdll
LIBDIRS = -L../../kscript -L../../kl
LIBS = ../xdcommon/libxdcommon.a -lkscript -lkl -lpthread $(CURL_LIBS)

ifeq ($(X_ARCH), LINUX)
LINKFLAGS = -shared -Wl,-soname,libxdclient.so.1 -fPIC
endif

ifeq ($(X_ARCH), MAC)
LINKFLAGS = -dynamiclib
endif


OBJECTS = database.o \
	iterator.o \
	request.o \
	set.o \
	xdclient.o


VPATH=$(OUTPUT_DIR)

all: xdclient.so

xdclient.so:	$(OBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdclient.so

clean:
	rm -f *.o xdclient.so $(OUTPUT_DIR)/xdclient.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

