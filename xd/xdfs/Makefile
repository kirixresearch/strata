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
INCLUDES = -I../include -I../../xcm/include -I../../kl/include -I../../supp/libzip/lib
DEFINES =  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
LINKFLAGS = --pipe -mdll
LIBDIRS = -L../../kscript -L../../kl 
LIBS = ../xdcommonsql/libxdcommonsql.a ../xdcommon/libxdcommon.a ../../supp/libzip/lib/.libs/libzip.a -lkscript -lkl -lpthread -lz -lexpat

ifeq ($(X_ARCH), LINUX)
LINKFLAGS = -shared -Wl,-soname,libxdfs.so.1 -fPIC
endif

ifeq ($(X_ARCH), MAC)
LINKFLAGS = -dynamiclib
endif


OBJECTS = baseset.o \
	database.o \
	delimitedtext.o \
	delimitedtextiterator.o \
	delimitedtextset.o \
	fixedlength.o \
	fixedlengthtextiterator.o \
	fixedlengthtextset.o \
	rawtext.o \
	ttbfile.o \
	ttbiterator.o \
	ttbset.o \
	ttbsetmodify.o \
	xbase.o \
	xbaseiterator.o \
	xbaseset.o \
	xdfs.o \
	xlsx.o \
	xlsxiterator.o \
	xlsxset.o 

COBJECTS = sqlite3.o


VPATH=$(OUTPUT_DIR)

all: xdfs.so

xdfs.so:	$(OBJECTS) $(COBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/xdfs.so

clean:
	rm -f *.o xdfs.so $(OUTPUT_DIR)/xdfs.so

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

$(COBJECTS): %.o : %.c
	@echo $<
	gcc -fPIC $(INCLUDES) $(DEFINES) -c $<

