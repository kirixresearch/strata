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
LIBDIRS = -L../../kscript
LIBS = ../xdcommon/libxdcommon.a -lkscript $(GLIB_LIBS) -lpthread
LINKFLAGS = -shared -fPIC -Wl,-soname,libxdpgsql.so.1

OBJECTS = \
	database.o \
	iterator.o \
	set.o \
	xdpgsql.o

VPATH=../../bin

all: xdpgsql.so

xdpgsql.so:	$(OBJECTS)
		$(CPP) $(LINKFLAGS) $^ -o ../../bin/xdpgsql.so $(LIBDIRS) $(LIBS)

clean:
	rm -f *.o xdpgsql.so ../../bin/xdpgsql.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

