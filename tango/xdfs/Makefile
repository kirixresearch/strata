#/*!
# *
# * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Aaron L. Williams
# * Created:  2004-12-23
# *
# */


include ../../top.mak


CFLAGS = -fPIC $(TOP_CFLAGS)
INCLUDES = -I../include -I../../xcm/include -I../../kl/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
LINKFLAGS = --pipe -mdll
LIBDIRS = -L../../kscript -L../../kl
LIBS = ../xdcommon/libxdcommon.a -lkscript -lkl -lpthread

ifeq ($(X_ARCH), LINUX)
LINKFLAGS = -shared -Wl,-soname,libxdfs.so.1 -fPIC
endif

ifeq ($(X_ARCH), MAC)
LINKFLAGS = -dynamiclib
endif


OBJECTS = database.o \
	delimitedtext.o \
	fixedlengthtextiterator.o \
	fixedlengthtextset.o \
	delimitedtextiterator.o \
	delimitedtextset.o \
	rawtext.o \
	xbase.o \
	xbaseiterator.o \
	xbaseset.o \
	xdfs.o


VPATH=$(OUTPUT_DIR)

all: xdfs.so

xdfs.so:	$(OBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdfs.so

clean:
	rm -f *.o xdfs.so $(OUTPUT_DIR)/xdfs.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

