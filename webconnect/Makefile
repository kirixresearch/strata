#/*!
# *
# * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  wxWebConnect Embedded Web Browser Control Library
# * Author:   Benjamin I. Williams
# * Created:  2006-10-10
# *
# */


include ../top.mak


INCLUDES = 
DEFINES = ${LARGEFILE_DEFINES}
CFLAGS = ${WX_CFLAGS} -g -ggdb -fno-rtti
LIBS = ${WX_LIBS} 


OBJECTS = \
	dom.o \
	nsimpl.o \
	promptservice.o \
	webcontrol.o \
	webframe.o \
	webprefs.o 

all: libwebconnect.a

libwebconnect.a: $(OBJECTS)
	ar rc libwebconnect.a $(OBJECTS)
	ranlib libwebconnect.a

clean:
	rm -f *.o libwebconnect.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


