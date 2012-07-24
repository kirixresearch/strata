#/*!
# *
# * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Benjamin I. Williams
# * Created:  2007-07-03
# *
# */


include ../../top.mak


INCLUDES = -I../include -I../../kl/include  -I../../supp/drizzle -I../../supp/drizzle/libdrizzle 
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC $(TOP_CFLAGS)
LIBDIRS = -L../../kscript -L../../kl
LIBS = ../xdcommon/libxdcommon.a ../../supp/drizzle/libdrizzle/.libs/libdrizzle.a -lkscript -lkl -lz -lpthread
LINKFLAGS = -fPIC -shared -Wl,-soname,libxddrizzle.so.1

OBJECTS = database.o \
	iterator.o \
	set.o \
	xddrizzle.o


VPATH=$(OUTPUT_DIR)

all: xddrizzle.so

xddrizzle.so:	$(OBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xddrizzle.so

clean:
	rm -f *.o xddrizzle.so $(OUTPUT_DIR)/xddrizzle.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

