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


INCLUDES = -I../include -I../../xcm/include -I../../kl/include 
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC $(TOP_CFLAGS) 
LIBDIRS = -L../../kscript -L../../kl
LIBS = ../xdcommon/libxdcommon.a -lodbc -lodbcinst -lkscript -lkl -lpthread -ldl
LINKFLAGS = -fPIC -shared -Wl,-soname,libxdodbc.so.1

OBJECTS = database.o \
	inserter.o \
	iterator.o \
	xdodbc.o


VPATH=$(OUTPUT_DIR)

all: xdodbc.so

xdodbc.so:	$(OBJECTS)
		$(CPP) $(LINKFLAGS) $^ $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdodbc.so

clean:
	rm -f *.o xdodbc.so $(OUTPUT_DIR)/xdodbc.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

