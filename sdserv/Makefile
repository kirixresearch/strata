#/*!
# *
# * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  wxWebConnect Test Application
# * Author:   Benjamin I. Williams
# * Created:  2009-05-09
# *
# */


# These two settings need to be adjusted to your build environment

EXENAME = ../debugu/sdserv 
INCLUDES = -I../kl/include -I../xd/include
CPP = g++
CFLAGS = 
LIBS = -L../sdservlib -L../jobs -L../kl -lsdserv -ljobs -lkl ../supp/libwebsockets/build/lib/libwebsockets.a -lssl -lcrypto -lpthread -ldl
DEPENDLIBS = ../sdservlib/libsdserv.a 

OBJECTS = sdserv.o

all: $(EXENAME)
    
$(EXENAME): $(OBJECTS) $(DEPENDLIBS)
		$(CPP) $(OBJECTS) $(LIBDIRS) $(LIBS) -o $(EXENAME)

clean:
	rm -f *.o *.gch $(EXENAME)

$(OBJECTS): %.o : %.cpp
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<

