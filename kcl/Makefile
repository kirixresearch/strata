#/*!
# *
# * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  GUI Control Library
# * Author:   Benjamin I. Williams
# * Created:  2003-05-06
# *
# */


include ../top.mak


INCLUDES = -I../xcm/include \
	   -I../kl/include \
	   -I${WXDIR}/include \
	   -I${WXDIR}/contrib/include \
	   $(GTK_CFLAGS)

DEFINES = ${LARGEFILE_DEFINES}
CFLAGS = ${WX_CFLAGS} -g -ggdb
LIBS = ${WX_LIBS}


OBJECTS = \
	bannercontrol.o \
	bitmapcombo.o \
	borderpanel.o \
	button.o \
	buttonbar.o \
	colordropdown.o \
	colorpanel.o \
	colorselector.o \
	filepanel.o \
	grid.o \
	griddnd.o \
	gridmemmodel.o \
	gridvalidator.o \
	popupcontainer.o \
	rowselectiongrid.o \
	rulercontrol.o \
	scrolllistcontrol.o \
	util.o \
	validcontrol.o \
	wizard.o 

all: libkcl.a

libkcl.a: $(OBJECTS)
	ar rc libkcl.a $(OBJECTS)
	ranlib libkcl.a

clean:
	rm -f *.o libkcl.a

$(OBJECTS): %.o : %.cpp
	@echo $<
	@$(CPP) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $<


