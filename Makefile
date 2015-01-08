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
	cd xd/xdcommon && $(MAKE) -w
	cd xd/xdcommonsql && $(MAKE) -w
	cd xd/xdfs && $(MAKE) -w
	cd xd/xdkpg && $(MAKE) -w
	cd xd/xdnative && $(MAKE) -w
	cd xd/xdodbc && $(MAKE) -w
	cd xd/xdpgsql && $(MAKE) -w
#	cd xd/xdoracle && $(MAKE) -w
	cd jobs && $(MAKE) -w
	cd scripthost && $(MAKE) -w
	cd kcl && $(MAKE) -w
	cd webconnect && $(MAKE) -w
	cd kcanvas && $(MAKE) -w
	cd img && $(MAKE) -w
	cd appstrata && $(MAKE) -w

clean:
	cd kl && $(MAKE) -w clean
	cd paladin && $(MAKE) -w clean
	cd kscript && $(MAKE) -w clean
	cd xd/xdcommon && $(MAKE) -w clean
	cd xd/xdcommonsql && $(MAKE) -w clean
	cd xd/xdfs && $(MAKE) -w clean
	cd xd/xdkpg && $(MAKE) -w clean
	cd xd/xdnative && $(MAKE) -w clean
	cd xd/xdodbc && $(MAKE) -w clean
	cd xd/xdpgsql && $(MAKE) -w clean
#	cd xd/xdoracle && $(MAKE) -w clean
	cd jobs && $(MAKE) -w clean
	cd scripthost && $(MAKE) -w clean
	cd kcl && $(MAKE) -w clean
	cd webconnect && $(MAKE) -w clean
	cd kcanvas && $(MAKE) -w clean
	cd webconnect && $(MAKE) -w clean
	cd img && $(MAKE) -w clean
	cd appstrata && $(MAKE) -w clean

cleanwx:
	cd kcl && $(MAKE) -w clean
	cd webconnect && $(MAKE) -w clean
	cd kcanvas && $(MAKE) -w clean
	cd webconnect && $(MAKE) -w clean
	cd img && $(MAKE) -w clean
	cd appstrata && $(MAKE) -w clean


