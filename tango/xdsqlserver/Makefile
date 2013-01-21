#/*!
# *
# * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Aaron L. Williams
# * Created:  2004-12-24
# *
# */


include ../../top.mak


CFLAGS = $(TOP_CFLAGS)
INCLUDES = -I../include -I../../xcm/include -I../../kl/include -I${SUPP_DIR}/freetds/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CPPFLAGS = -fPIC -g
LIBDIRS = -L../../kscript -L../../kl -L${SUPP_DIR}/freetds/src/tds/.libs
LIBS = ../xdcommon/libxdcommon.a -lkscript -lkl -ltds -lpthread
LINKFLAGS = -fPIC -shared -Wl,-soname,libxdsqlserver.so.1 -Wl,--rpath -Wl,.:../lib

OBJECTS = database.o \
	iterator.o \
	set.o \
	xdsqlserver.o


VPATH=$(OUTPUT_DIR)

all: xdsqlserver.so

xdsqlserver.so:	$(OBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdsqlserver.so

clean:
	rm -f *.o xdsqlserver.so $(OUTPUT_DIR)/xdsqlserver.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

