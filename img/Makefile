#/*!
# *
# * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Application Images
# * Author:   Benjamin I. Williams
# * Created:  2007-07-27
# *
# */


include ../top.mak


ifeq ($(X_ARCH),WIN32)
ZIP = ../zip
else
ZIP = zip
endif

all: 
	@echo Packaging up images...
	rm -f imgres.zip
	$(ZIP) imgres.zip *.png *.xpm *.gif > /dev/null
	cp imgres.zip $(OUTPUT_DIR)/imgres.zip

clean:
	rm -f imgres.zip $(OUTPUT_DIR)/imgres.zip

