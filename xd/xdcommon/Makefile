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
CC = gcc
INCLUDES = -I../../xcm/include -I../../kl/include -I../include 
DEFINES = 
CFLAGS = -fPIC $(TOP_CFLAGS)

OBJECTS = block.o \
	cmnbaseiterator.o \
	cmnbaseset.o \
	cmndynamicset.o \
	cmniteratorset.o \
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
	formatdefinition.o \
	groupquery.o \
	idxutil.o \
	jobinfo.o \
	keylayout.o \
	localrowcache.o \
	localrowcache2.o \
	rowidarray.o \
	util.o \
	xdcommon.o \

COBJECTS = sqlite3.o

all: libxdcommon.a

libxdcommon.a:	$(OBJECTS) $(COBJECTS)
	ar rc libxdcommon.a $(OBJECTS) $(COBJECTS)
	ranlib libxdcommon.a

clean:
	rm -f *.o libxdcommon.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

$(COBJECTS): %.o : %.c
	@echo $<
	@$(CC) $(INCLUDES) $(DEFINES) -c $<


