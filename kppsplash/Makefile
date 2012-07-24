#/*!
# *
# * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Splash Screen Application
# * Author:   Aaron L. Williams
# * Created:  2005-01-21
# *
# */


include ../top.mak 

EXENAME = kppsplash
INCLUDES = -I../xcm/include -I../kl/include -I${WX_DIR}/include -I${WX_DIR}/contrib/include
#CFLAGS = $(TOP_CFLAGS) $(WX_CFLAGS)
CFLAGS = $(TOP_CFLAGS) $(WX_CFLAGS) $(FREETYPE_CFLAGS)
LIBDIRS = -L../kcl -L../cfw -L../kappcmn
LIBS = -lwx_gtk2ud_stc-2.5 $(WX_LIBS) $(FREETYPE_LIBS) -lfontconfig -lcfw -lkcl -lkappcmn -lwx_gtk2ud_fl-2.5

OBJECTS = \
	kppsplash.o \


VPATH=../bin:

all: $(EXENAME)
    
$(EXENAME): $(OBJECTS)
		$(CPP) $(OBJECTS) $(LIBDIRS) $(LIBS) -o ../bin/$(EXENAME)
		cp instinfo.xml ../bin/instinfo.xml

clean:
	rm -f *.o *.gch ../bin/$(EXENAME)

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) -c $<
