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


INCLUDES = -I../include -I../../xcm/include -I../../kl/include -I$(SUPP_DIR)/oci/sdk/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fpic $(TOP_CFLAGS)
LIBDIRS = -L../../kscript 
LIBS = ../xdcommon/libxdcommon.a -lkscript $(SUPP_DIR)/oci/libclntsh.so.10.1
LINKFLAGS = -fPIC -shared -Wl,-soname,libxdoracle.so.1

OBJECTS = database.o \
	iterator.o \
	set.o \
	xdoracle.o


VPATH=$(OUTPUT_DIR)

all: xdoracle.so

xdoracle.so:	$(OBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdoracle.so

clean:
	rm -f *.o xdoracle.so $(OUTPUT_DIR)/xdoracle.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

