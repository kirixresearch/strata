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


class SdServers
{
public:

    std::wstring name;
    HANDLE process;
};

class Service
{
public:

    Service();
    ~Service();

    void run();
    void stop();

protected:

    void readConfig();

public:

    std::vector<SdServers> m_servers;
    bool m_running;
};

extern Service g_service;



#endif

