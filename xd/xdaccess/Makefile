#/*!
# *
# * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Benjamin I. Williams
# * Created:
# *
# */


include ../../top.mak

INCLUDES = -I../include -I../../kl/include -I${SUPP_DIR}/mdbtools/include 
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC $(TOP_CFLAGS) $(GLIB_CFLAGS)
MDBTOOLS_DIR = $(SUPP_DIR)/mdbtools/src/libmdb/.libs
LIBDIRS = -L$(MDBTOOLS_DIR) -L../../kscript -L../../kl
LIBS = ../xdcommon/libxdcommon.a -lkscript -lkl $(MDBTOOLS_DIR)/libmdb.0 $(GLIB_LIBS) -lpthread
LINKFLAGS = -shared -fPIC -Wl,-soname,libxdaccess.so.1 -Wl,--rpath -Wl,.:../lib

OBJECTS = \
	database.o \
	iterator.o \
	set.o \
	xdaccess.o

VPATH=$(OUTPUT_DIR)

all: xdaccess.so

xdaccess.so:	$(OBJECTS)
		$(CPP) $(LINKFLAGS) $^ -o $(OUTPUT_DIR)/xdaccess.so $(LIBDIRS) $(LIBS)

clean:
	rm -f *.o xdaccess.so $(OUTPUT_DIR)/xdaccess.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

