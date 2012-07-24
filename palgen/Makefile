#/*!
# *
# * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Paladin Code Generator Client
# * Author:   Benjamin I. Williams
# * Created:  2004-04-01
# *
# */


include ../top.mak


EXENAME = palgen
INCLUDES = -I../xcm/include -I../kl/include -I${WX_DIR}/include -I${WX_DIR}/contrib/include 
DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D_wxUSE_GUI=1
CFLAGS = ${WX_CFLAGS} ${TOP_CFLAGS}
LIBDIRS = -L../cfw -L../paladin -L../kl
LIBS = -lcfw -lpaladin -lkl ${WX_LIBS}

OBJECTS = app.o \
	generatepage.o \
	gencode.o \
	infopage.o \
	mainframe.o

VPATH=$(OUTPUT_DIR):


all: $(EXENAME)

$(EXENAME):	$(OBJECTS)
		$(CPP) $^ $(LINKFLAGS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/$(EXENAME)

clean:
	rm -f *.o $(EXENAME)

$(OBJECTS): %.o : %.cpp
	$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<



