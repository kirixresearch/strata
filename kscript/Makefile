#/*!
# *
# * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  ECMA Script Library
# * Author:   Benjamin I. Williams
# * Created:  2004-03-31
# *
# */


include ../top.mak


INCLUDES = -I../kl/include
DEFINES =
CFLAGS = -fPIC $(TOP_CFLAGS)

OBJECTS = functions.o \
	jsarray.o \
	jsboolean.o \
	jsdate.o \
	jserror.o \
	jsfunction.o \
	jsmath.o \
	jsnumber.o \
	jsregexp.o \
	jsstring.o \
	jsobject.o \
	json.o \
	parse.o \
	util.o \
	validate.o \
	value.o 

all: libkscript.a

clean:
	rm -f *.o libkscript.a

libkscript.a:	$(OBJECTS)
	ar rc libkscript.a $(OBJECTS)
	ranlib libkscript.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


