#/*!
# *
# * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Benjamin I. Williams
# * Created:
# *
# */


include ../../top.mak

INCLUDES = -I../include -I../../xcm/include -I../../kl/include -I${SUPP_DIR}/mdbtools/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC $(TOP_CFLAGS) $(GLIB_CFLAGS)
LIBDIRS = -L$(SUPP_DIR)/mdbtools/src/libmdb/.libs -L../../kscript
LIBS = ../xdcommon/libxdcommon.a -lkscript -lmdb $(GLIB_LIBS) -lpthread
LINKFLAGS = -shared -fPIC -Wl,-soname,libxdaccess.so.1

OBJECTS = \
	database.o \
	iterator.o \
	set.o \
	xdaccess.o

VPATH=../../bin

all: xdaccess.so

xdaccess.so:	$(OBJECTS)
		$(CPP) $(LINKFLAGS) $^ -o ../../bin/xdaccess.so $(LIBDIRS) $(LIBS)

clean:
	rm -f *.o xdaccess.so ../../bin/xdaccess.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

