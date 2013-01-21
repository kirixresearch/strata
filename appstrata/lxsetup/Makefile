#/*!
# *
# * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Application Installation Utility
# * Author:   Benjamin I. Williams
# * Created:  2002-12-16
# *
# */


include ../top.mak 

EXENAME = lxsetup
INCLUDES = -I../xcm/include -I../kl/include -I../tango/include -I${WX_DIR}/include -I${WX_DIR}/contrib/include
#CFLAGS = $(TOP_CFLAGS) $(WX_CFLAGS)
CFLAGS = $(WX_CFLAGS)
LIBDIRS = -L../cfw -L../kl
LIBS = $(WX_LIBSNONET) -lcfw -lkl

OBJECTS = \
	lxsetup.o \


VPATH=$(OUTPUT_DIR):

all: $(EXENAME)
    
$(EXENAME): $(OBJECTS)
		$(CPP) $(OBJECTS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)/$(EXENAME)
		cp instinfo4.xml $(OUTPUT_DIR)/instinfo.xml
		#cp instinfo_eval.xml $(OUTPUT_DIR)/instinfo_eval.xml

clean:
	rm -f *.o *.gch $(OUTPUT_DIR)/$(EXENAME)

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) -c $<

