#/*!
# *
# * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  XD Database Library
# * Author:   Benjamin I. Williams
# * Created:  2013-04-18
# *
# */


include ../../top.mak


CPP = g++
INCLUDES = -I../../xcm/include -I../../kl/include -I../include 
DEFINES = 
CFLAGS = -fPIC $(TOP_CFLAGS)

OBJECTS = \
	sqlalter.o \
	sqlcreate.o \
	sqldelete.o \
	sqldrop.o \
	sqlinsert.o \
	sqlmove.o \
	sqlselect.o \
	sqlupdate.o \
	xdcommonsql.o 

all: libxdcommonsql.a

libxdcommonsql.a:	$(OBJECTS)
	ar rc libxdcommonsql.a $(OBJECTS)
	ranlib libxdcommonsql.a

clean:
	rm -f *.o libxdcommonsql.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


