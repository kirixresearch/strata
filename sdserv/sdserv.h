/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2012-07-30
 *
 */

 
#ifndef __SDSERV_SDSERV_H
#define __SDSERV_SDSERV_H

#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <cwchar>
#include <ctime>
#include <windows.h>
#include <tango.h>
#include <kl/string.h>
#include <kl/md5.h>
#include <kl/base64.h>
#include <kl/portable.h>
#include <kl/memory.h>
#include <kl/file.h>
#include <kl/json.h>
#include "../jobs/jobs.h"


class Sdserv
{
public:
    
    Sdserv();
    ~Sdserv();
    
    bool initOptions(int argc, const char* argv[]);
    const char** getOptions() { return m_options; }

    int runServer();
    std::wstring getDatabaseConnectionString(const std::wstring& database = L"");

    void signalServerReady();
    void signalServerNotReady();
    
private:

    bool useConfigFile(const std::wstring& config_file);

private:

    std::wstring m_config_file;

    std::wstring m_ready_evtid;
    std::wstring m_notready_evtid;

    char m_cert_file_path[255];
    
    const char* m_options[255];
};


extern Sdserv g_sdserv;


#endif
