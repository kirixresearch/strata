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
#include <kl/file.h>



int main(int argc, const char** argv)
{
    Sdserv sdserv;

    if (!sdserv.initOptionsFromCommandLine(argc, argv))
    {
        sdserv.signalServerNotReady();
        return 0;
    }


    std::wstring config_file = sdserv.getOption(L"sdserv.config_file");

    if (config_file.empty())
    {
        std::wstring home_cfg_file;

        // no config file specified; look in default places

        #ifdef WIN32
        home_cfg_file  = _wgetenv(L"HOMEDRIVE");
        home_cfg_file += _wgetenv(L"HOMEPATH");
        home_cfg_file += L"\\sdserv.conf";
        #else
        home_cfg_file  = _wgetenv(L"HOME");
        home_cfg_file += L"/.sdservrc";
        #endif

        if (xf_get_file_exist(home_cfg_file))
        {
            sdserv.setOption(L"sdserv.config_file", home_cfg_file);
        }
    }



    sdserv.runServer();

    return 0;
}

