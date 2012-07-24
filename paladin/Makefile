#/*!
# *
# * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Paladin Security Library
# * Author:   Benjamin I. Williams
# * Created:  2003-05-06
# *
# */


include ../top.mak


INCLUDES = -I../kl/include
CFLAGS = $(TOP_CFLAGS) -fPIC

OBJECTS = auth.o \
		crc.o \
		crypt.o \
		hostid_unix.o \

all: libpaladin.a

libpaladin.a:	$(OBJECTS)
		ar rc libpaladin.a $(OBJECTS)
		ranlib libpaladin.a

clean:
	rm -f *.o libpaladin.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) -c $<

