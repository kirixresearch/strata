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


INCLUDES = -I../include -I../../xcm/include -I../../kl/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC $(TOP_CFLAGS) $(GLIB_CFLAGS)
LIBDIRS = -L../../kscript -L../../kl
LIBS = ../xdcommonsql/libxdcommonsql.a ../xdcommon/libxdcommon.a -lkscript -lkl -lpthread -lz
LINKFLAGS = -shared -fPIC -Wl,-soname,libxdkpg.so.1

OBJECTS = \
	database.o \
	inserter.o \
	iterator.o \
	pkgfile.o \
	stream.o \
	util.o \
	xdkpg.o

VPATH=$(OUTPUT_DIR)

all: xdkpg.so

xdkpg.so:	$(OBJECTS)
		$(CPP) $(LINKFLAGS) $^ -o $(OUTPUT_DIR)/xdkpg.so $(LIBDIRS) $(LIBS)

clean:
	rm -f *.o xdpgsql.so $(OUTPUT_DIR)/xdkpg.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

