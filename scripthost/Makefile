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


INCLUDES = -I../kl/include -I../xd/include -I../kl/include
DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC
LIBS = 


OBJECTS = \
	base64.o \
	crypt.o \
	db.o \
	environment.o \
	event.o \
	file.o \
	filetransfer.o \
	hash.o \
	http.o \
	memory.o \
	native.o \
	scripthost.o \
	system.o \
	xml.o

VPATH=src:lib

all: libscripthost.a

libscripthost.a:	$(OBJECTS)
	ar rc libscripthost.a $(OBJECTS)
	ranlib libscripthost.a

clean:
	rm -f *.o libscripthost.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


