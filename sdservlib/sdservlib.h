/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2012-07-30
 *
 */

 
#ifndef __SDSERVLIB_SDSERVLIB_H
#define __SDSERVLIB_SDSERVLIB_H

#include <vector>
#include <map>
#include <string>
#include <xcm/xcmthread.h>
#include <kl/string.h>

class Controller;
class Sdserv
{
    friend class HttpServer;
    friend class WebSocketsClient;

public:

    enum
    {
        serverHttp = 1,
        serverWebSocketsClient = 2
    };


public:
    
    Sdserv();
    ~Sdserv();
    
    bool initOptionsFromCommandLine(int argc, const char* argv[]);
    void setOption(const std::wstring& option, const std::wstring& value);
    std::wstring getOption(const std::wstring& option);

    int runServer();
    std::wstring getDatabaseConnectionString(const std::wstring& database = L"");

    void signalServerReady();
    void signalServerNotReady();
    void updateLastAccessTimestamp();

private:

    Controller* m_controller;

    int m_idle_quit;
    xcm::mutex m_last_access_mutex;
    time_t m_last_access;

    std::map<std::wstring, std::wstring> m_options;
    xcm::mutex m_options_mutex;
};


#endif
