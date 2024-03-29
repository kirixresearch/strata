/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2012-07-30
 *
 */


#include "sdserv.h"
#include "../sdservlib/sdservlib.h"
#include <cstdio>
#include <kl/file.h>

#ifdef __linux__
#include <sys/prctl.h>
#include <sys/signal.h>
#endif



int main(int argc, const char** argv)
{
    Sdserv sdserv;

    if (!sdserv.initOptionsFromCommandLine(argc, argv))
    {
        sdserv.signalServerNotReady();
        return 0;
    }


    #ifdef __linux__
    // on linux, the following line causes this
    // process to be sent a hangup signal when
    // its parent process dies
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    #endif

    std::wstring config_file = sdserv.getOption(L"sdserv.config_file");
    std::wstring host = sdserv.getOption(L"sdserv.database.host");

    if (host.empty() && config_file.empty())
    {
        std::wstring home_cfg_file;

        // no config file specified; look in default places

        #ifdef WIN32
        home_cfg_file  = _wgetenv(L"HOMEDRIVE");
        home_cfg_file += _wgetenv(L"HOMEPATH");
        home_cfg_file += L"\\sdserv.conf";
        #else
        home_cfg_file = L"/etc/sdserv.conf";
        #endif

        if (xf_get_file_exist(home_cfg_file))
        {
            config_file = home_cfg_file;
            sdserv.setOption(L"sdserv.config_file", home_cfg_file);
        }
    }

    if (!config_file.empty())
        printf("reading configuration file %ls\n", config_file.c_str());

    sdserv.runServer();

    return 0;
}

