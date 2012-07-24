#/*!
# *
# * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Strata Setup Utility
# * Author:   Benjamin I. Williams
# * Created:  2007-07-03
# *
# */


include ../top.mak

BUILD_DIR = $(OUTPUT_DIR)
SETUP_DIR = $(BASE_DIR)/setup
WORK_DIR = $(BASE_DIR)/setup/strata_work
FINAL_DIR = $(BASE_DIR)/setup/strata_setup

PROGRAM_FILES = \
	$(BUILD_DIR)/appmain \
	$(BUILD_DIR)/xdfs.so \
	$(BUILD_DIR)/xdnative.so \
	$(BUILD_DIR)/xdodbc.so \
	$(BUILD_DIR)/xdoracle.so \
	$(BUILD_DIR)/webres.jar \
	$(BUILD_DIR)/imgres.zip

XULRUNNER_PATH = $(BASE_DIR)/xr

all:
	rm -f strata.zip
	rm -rf $(WORK_DIR)
	rm -rf $(FINAL_DIR)
	mkdir $(FINAL_DIR)
	mkdir $(WORK_DIR)
	mkdir $(WORK_DIR)/bin
	cp $(PROGRAM_FILES) $(WORK_DIR)/bin
	mv $(WORK_DIR)/bin/appmain $(WORK_DIR)/bin/kstrata
	chmod 644 $(WORK_DIR)/bin/*
	chmod 755 $(WORK_DIR)/bin/kstrata
	strip $(WORK_DIR)/bin/kstrata $(WORK_DIR)/bin/*.so
	cp -LpR $(XULRUNNER_PATH) $(WORK_DIR)
	-strip $(WORK_DIR)/xr/* 2> /dev/null
	cd $(WORK_DIR) && zip -r $(FINAL_DIR)/data.bin *
	rm -rf $(WORK_DIR)
	cp $(BASE_DIR)/lxsetup/instinfo4.xml $(FINAL_DIR)/instinfo.xml
	cp $(BASE_DIR)/lxsetup/eula.txt $(FINAL_DIR)/eula.txt
	cp $(BUILD_DIR)/lxsetup $(FINAL_DIR)/setup
	strip $(FINAL_DIR)/setup


