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


INCLUDES = -I./include
DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC
LIBS = 


OBJECTS = \
	base64.o \
	config.o \
	crc.o \
	crypt.o \
	file_common.o \
	file_ix.o \
	hex.o \
	json.o \
	math.o \
	md5.o \
	memory.o \
	string.o \
	system.o \
	thread.o \
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


