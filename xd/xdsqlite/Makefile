#/*!
# *
# * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Benjamin I. Williams
# * Created:  2008-08-13
# *
# */


include ../../top.mak

UNAME := $(shell uname)
SONAME=$(if $(filter $(UNAME),Linux),-soname,-install_name)
INCLUDES = -I../include -I../../xcm/include -I../../kl/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC $(TOP_CFLAGS) $(GLIB_CFLAGS)
LIBDIRS = -L../../kscript -L../../kl
LIBS = ../xdcommon/libxdcommon.a -lkscript -lkl $(GLIB_LIBS) -lpthread
LINKFLAGS = -shared -fPIC -Wl,$(SONAME),libxdsqlite.so.1

OBJECTS = \
	database.o \
	inserter.o \
	iterator.o \
	stream.o \
	util.o \
	xdsqlite.o

VPATH=$(OUTPUT_DIR)

all: xdsqlite.so

xdsqlite.so:	$(OBJECTS)
		$(CPP) $(LINKFLAGS) $^ -o $(OUTPUT_DIR)/xdsqlite.so $(LIBDIRS) $(LIBS)

clean:
	rm -f *.o xdsqlite.so ../../bin/xdsqlite.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

