#/*!
# *
# * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Benjamin I. Williams
# * Created:  2007-07-03
# *
# */


include ../../top.mak


INCLUDES = -I../include -I../../kl/include  -I../../supp/mysql/include
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC $(TOP_CFLAGS)
LIBDIRS = -L../../kscript -L../../kl
LIBS = ../xdcommon/libxdcommon.a ../../supp/mysql/libmysql/.libs/libmysqlclient.a -lkscript -lkl -lz -lpthread
LINKFLAGS = -fPIC -shared -Wl,-soname,libxdmysql.so.1

OBJECTS = database.o \
	iterator.o \
	set.o \
	xdmysql.o


VPATH=$(OUTPUT_DIR)

all: xdmysql.so

xdmysql.so:	$(OBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdmysql.so

clean:
	rm -f *.o xdmysql.so $(OUTPUT_DIR)/xdmysql.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

