/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Sdserv Data Service
 * Author:   Benjamin I. Williams
 * Created:  2013-08-20
 *
 */


 
#ifndef __SDSVC_SERVICE_H
#define __SDSVC_SERVICE_H


class SdServer
{
public:

    std::string instance;
    HANDLE process;
    HANDLE thread;
    int port;
};

class Service
{
public:

    Service();
    ~Service();

    void run();
    void stop();
    int getServerPort(const std::string& instance);

protected:

    void readConfig();

public:

    xcm::mutex m_mutex;
    std::map<std::string, SdServer> m_servers;
    int m_next_port;

    bool m_running;
};

extern Service g_service;



#endif

