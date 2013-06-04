#/*!
# *
# * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Application Client
# * Author:   Benjamin I. Williams
# * Created:  2004-11-29
# *
# */


all:
	echo Making all.  Current configuration:
	cat app_config.mak
	cd kl && $(MAKE) -w
	cd paladin && $(MAKE) -w
	cd kscript && $(MAKE) -w
	cd tango/xdcommon && $(MAKE) -w
	cd tango/xdfs && $(MAKE) -w
	cd tango/xdnative && $(MAKE) -w
	cd tango/xdodbc && $(MAKE) -w
#	cd tango/xdoracle && $(MAKE) -w
#	cd tango/xddrizzle && $(MAKE) -w
	cd jobs && $(MAKE) -w
	cd kcl && $(MAKE) -w
	cd webconnect && $(MAKE) -w
	cd kcanvas && $(MAKE) -w
	cd img && $(MAKE) -w
	cd appmain && $(MAKE) -w

clean:
	cd kl && $(MAKE) -w clean
	cd paladin && $(MAKE) -w clean
	cd kscript && $(MAKE) -w clean
	cd tango/xdcommon && $(MAKE) -w clean
	cd tango/xdfs && $(MAKE) -w clean
	cd tango/xdnative && $(MAKE) -w clean
	cd tango/xdodbc && $(MAKE) -w clean
#	cd tango/xdoracle && $(MAKE) -w clean
#	cd tango/xddrizzle && $(MAKE) -w clean
	cd jobs && $(MAKE) -w clean
	cd kcl && $(MAKE) -w clean
	cd webconnect && $(MAKE) -w clean
	cd kcanvas && $(MAKE) -w clean
	cd webconnect && $(MAKE) -w clean
	cd img && $(MAKE) -w clean
	cd appmain && $(MAKE) -w clean

cleanwx:
	cd kcl && $(MAKE) -w clean
	cd webconnect && $(MAKE) -w clean
	cd kcanvas && $(MAKE) -w clean
	cd webconnect && $(MAKE) -w clean
	cd img && $(MAKE) -w clean
	cd appmain && $(MAKE) -w clean


