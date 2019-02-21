/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-04-27
 *
 */


#ifndef __APP_CONNECTIONINFO_H
#define __APP_CONNECTIONINFO_H



class ConnectionInfo
{
public:

    ConnectionInfo()
    {
        type = xd::dbtypeUndefined;
        last_type = xd::dbtypeUndefined;
        description = L"";

        server = L"";
        port = 0;
        database = L"";
        username = L"";
        password = L"";
        save_password = false;

        path = L"";
        filter = L"";

        kpg_compressed = true;
        kpg_version = 2;

        binary_copy = false;
    }

    ConnectionInfo(const ConnectionInfo& c)
    {
        type = c.type;
        last_type = c.last_type;
        description = c.description;

        server = c.server;
        port = c.port;
        database = c.database;
        username = c.username;
        password = c.password;
        save_password = c.save_password;

        path = c.path;
        filter = c.filter;

        kpg_compressed = c.kpg_compressed;
        kpg_version = c.kpg_version;

        binary_copy = c.binary_copy;

        m_conn_ptr = c.m_conn_ptr;
    }

    ConnectionInfo& operator=(const ConnectionInfo& c)
    {
        type = c.type;
        last_type = c.last_type;
        description = c.description;

        server = c.server;
        port = c.port;
        database = c.database;
        username = c.username;
        password = c.password;
        save_password = c.save_password;

        path = c.path;
        filter = c.filter;

        kpg_compressed = c.kpg_compressed;
        kpg_version = c.kpg_version;

        binary_copy = c.binary_copy;

        m_conn_ptr = c.m_conn_ptr;

        return *this;
    }
    
public:

    int type;                 // xd database type
    int last_type;            // if the user is changing the connection type,
                              // this is the connection type before the change
    std::wstring description;

    // used for databases
    std::wstring server;
    int port;
    std::wstring database;
    std::wstring username;
    std::wstring password;
    bool save_password;

    // used for directories and data projects
    std::wstring path;
    std::wstring filter;

    // used for package files only
    bool kpg_compressed;
    int kpg_version;

    // option for binary-copy import
    bool binary_copy;

    // live connection ptr
    IConnectionPtr m_conn_ptr;
};


#endif

