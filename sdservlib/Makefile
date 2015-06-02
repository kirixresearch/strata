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


INCLUDES = -I../kl/include -I../xd/include -I../kl/include -I../supp/libwebsockets/lib
DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = -fPIC
LIBS = 


OBJECTS = controller.o \
	http.o \
	jobserver.o \
	sdservlib.o \
	websockets.o

COBJECTS = mongoose.o 

VPATH=src:lib

all: libsdserv.a

libsdserv.a:	$(OBJECTS) $(COBJECTS)
	ar rc libsdserv.a $(OBJECTS) $(COBJECTS)
	ranlib libsdserv.a

clean:
	rm -f *.o libsdserv.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<

$(COBJECTS): %.o : %.c
	@echo $<
	@gcc $(INCLUDES) $(DEFINES) -c $<

