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
INCLUDES = -I../include -I../../xcm/include -I../../kl/include -I../../supp/postgres/src/interfaces/libpq -I../../supp/postgres/src/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
LINKFLAGS = --pipe -mdll
LIBDIRS = -L../../kscript -L../../kl 
LIBS = ../xdcommonsql/libxdcommonsql.a ../xdcommon/libxdcommon.a -lkscript -lkl  ../../supp/postgres/src/interfaces/libpq/libpq.a -lpthread

ifeq ($(X_ARCH), LINUX)
LINKFLAGS = -shared -Wl,-soname,libxdpgsql.so.1 -fPIC
endif

ifeq ($(X_ARCH), MAC)
LINKFLAGS = -dynamiclib
endif


OBJECTS = database.o \
	inserter.o \
	iterator.o \
	stream.o \
	util.o \
	xdpgsql.o

VPATH=$(OUTPUT_DIR)

all: xdpgsql.so

xdpgsql.so:	$(OBJECTS) $(COBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdpgsql.so

clean:
	rm -f *.o xdpgsql.so $(OUTPUT_DIR)/xdpgsql.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

$(COBJECTS): %.o : %.c
	@echo $<
	gcc -fPIC $(INCLUDES) $(DEFINES) -c $<

