#/*!
# *
# * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Benjamin I. Williams
# * Created:  2004-03-31
# *
# */


include ../../top.mak


CPP = g++
INCLUDES = -I../../xcm/include -I../../kl/include -I../include 
DEFINES = 
CFLAGS = -fPIC $(TOP_CFLAGS)

OBJECTS = block.o \
	cmnbaseiterator.o \
	cmnbaseset.o \
	cmndynamicset.o \
	connectionstr.o \
	dbattr.o \
	dbfuncs.o \
	errorinfo.o \
	exindex.o \
	exkeypool.o \
	exprfuncs.o \
	extfileinfo.o \
	fileinfo.o \
	filestream.o \
	idxutil.o \
	indexinfo.o \
	jobinfo.o \
	keylayout.o \
	localrowcache.o \
	rowidarray.o \
	setconfigfile.o \
	structure.o \
	util.o \
	xdcommon.o \

all: libxdcommon.a

libxdcommon.a:	$(OBJECTS)
	ar rc libxdcommon.a $(OBJECTS)
	ranlib libxdcommon.a

clean:
	rm -f *.o libxdcommon.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


