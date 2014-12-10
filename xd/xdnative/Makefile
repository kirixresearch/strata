#/*!
# *
# * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Benjamin I. Williams
# * Created:  2002-12-16
# *
# */


include ../../top.mak


INCLUDES = -I../include -I../../kl/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC $(TOP_CFLAGS)
LIBDIRS = -L../../kscript -L../../kl
LIBS = ../xdcommon/libxdcommon.a -lkscript -lkl -lpthread


ifeq ($(X_ARCH),LINUX)
LINKFLAGS = -shared -Wl,-soname,xdnative.so -fpic
endif

ifeq ($(X_ARCH),MAC)
LINKFLAGS = -dynamiclib
endif

OBJECTS = baseiterator.o \
	baseset.o \
	bitmapfile.o \
	bufiterator.o \
	database.o \
	dbmgr.o \
	nativetbl.o \
	nodefilestream.o \
	ofs.o \
	query.o \
	tableset.o \
	tablesetmodify.o \
	util.o \
	xdnative.o

VPATH=$(OUTPUT_DIR)

all: xdnative.so

xdnative.so:	$(OBJECTS)
		$(CPP)  $(LINKFLAGS) $^ $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdnative.so

clean:
	rm -f *.o xdnative.so $(OUTPUT_DIR)/xdnative.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

