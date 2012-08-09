#/*!
# *
# * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Standard Library
# * Author:   Benjamin I. Williams
# * Created:  2006-03-31
# *
# */


include ../top.mak


INCLUDES = -I./include
DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC
LIBS = 


OBJECTS = \
	base64.o \
	crypt.o \
	file_ix.o \
	json.o \
	math.o \
	md5.o \
	memory.o \
	string.h \
	url.o \
	utf8.o \
	xcm.o \
	xml.o


VPATH=src:lib

all: libkl.a

libkl.a:	$(OBJECTS)
	ar rc libkl.a $(OBJECTS)
	ranlib libkl.a

clean:
	rm -f *.o libkl.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


