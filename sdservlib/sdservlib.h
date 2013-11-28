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
    
    bool initOptions(int argc, const char* argv[]);
    const char** getOptions() { return m_options; }

    int runServer();
    std::wstring getDatabaseConnectionString(const std::wstring& database = L"");

    void signalServerReady();
    void signalServerNotReady();
    void updateLastAccessTimestamp();

private:

    bool useConfigFile(const std::wstring& config_file);

private:

    Controller* m_controller;

    std::wstring m_config_file;

    std::wstring m_ready_evtid;
    std::wstring m_notready_evtid;

    char m_cert_file_path[255];
    const char* m_options[255];

    int m_idle_quit;
    xcm::mutex m_last_access_mutex;
    time_t m_last_access;

    int m_server_type;
    std::string m_websockets_server;
    bool m_websockets_ssl;
};


#endif
