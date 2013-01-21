#/*!
# *
# * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Canvas Control
# * Author:   Benjamin I. Williams
# * Created:  2004-10-28
# *
# */


include ../top.mak


INCLUDES = \
	-I../xcm/include \
	-I../kl/include \
    $(HARU_INCLUDE) \
    $(WXPDFDOC_INCLUDE)
	
DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
CFLAGS = $(TOP_CFLAGS) $(WX_CFLAGS)
LIBS = $(WX_LIBS)



OBJECTS = \
	canvas.o \
	canvasdc.o \
	canvascontrol.o \
	canvashistory.o \
	canvaspdf.o \
	canvasprintout.o \
	component.o \
	componentbox.o\
	componentcontrol.o\
	componentellipse.o \
	componentgraph.o \
	componentimage.o \
	componentline.o \
	componentpage.o \
	componenttable.o \
	componenttextbox.o \
	dnd.o \
	event.o \
	graphicsobj.o \
	property.o \
	range.o \
	util.o \

all: libkcanvas.a

libkcanvas.a:	$(OBJECTS)
	ar rc libkcanvas.a $(OBJECTS)
	ranlib libkcanvas.a

clean:
	rm -f *.o libkcanvas.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


