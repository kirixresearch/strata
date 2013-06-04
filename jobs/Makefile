#/*!
# *
# * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Standard Library
# * Author:   Benjamin I. Williams
# * Created:  2006-03-31
# *
# */


include ../top.mak


INCLUDES = -I../kl/include -I../tango/include -I../xcm/include
DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC
LIBS = 


OBJECTS = \
	aggregate.o \
	alter.o \
	append.o \
	copy.o \
	delete.o \
	divide.o \
	execute.o \
	group.o \
	index.o \
	jobbase.o \
	jobinfo.o \
	jobs.o \
	jobspch.o \
	load.o \
	query.o \
	relationship.o \
	summarize.o \
	update.o \
	util.o 
 

VPATH=src:lib

all: libjobs.a

libjobs.a:	$(OBJECTS)
	ar rc libjobs.a $(OBJECTS)
	ranlib libjobs.a

clean:
	rm -f *.o libjobs.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


