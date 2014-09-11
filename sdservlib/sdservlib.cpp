/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2012-07-30
 *
 */


#include "sdservlib.h"
#include "http.h"
#include "controller.h"
#include "websockets.h"
#include <kl/json.h>
#include <kl/file.h>

#ifdef WIN32
#include <Windows.h>
#endif


Sdserv::Sdserv()
{
    m_ws_client = NULL;

    // default settings
    setOption(L"sdserv.server_type", L"http");
    setOption(L"http.port", L"80,443s");
    setOption(L"websockets.ssl", L"true");

    //setOption(L"sdserv.config_file", L"");
    //setOption(L"sdserv.database", L"");
    //setOption(L"sdserv.win32evt_ready", L"");
    //setOption(L"sdserv.win32evt_notready", L"");
    //setOption(L"sdserv.idle_quit", L"");


    m_last_access = time(NULL);
    m_idle_quit = 0;
    m_controller = new Controller(this);
}

Sdserv::~Sdserv()
{
    delete m_controller;
}



static kl::JsonNode getJsonNodeFromFile(const std::wstring& filename)
{
    xf_off_t size = xf_get_file_size(filename);
    if (size < 0 || size > 1000000)
        return kl::JsonNode();
    
    char* buf = new char[((int)size)+1];
    
    xf_file_t f = xf_open(filename, xfOpen, xfRead, xfShareNone);
    if (!f)
    {
        printf("Could not open config file for reading.\n");
        
        kl::JsonNode null_return;
        return null_return;
    }
    
    buf[0] = 0;
    xf_off_t read_bytes = xf_read(f, buf, 1, (unsigned int)size);
    buf[read_bytes] = 0;
    xf_close(f);
    
    kl::JsonNode config;
    config.fromString(kl::towstring(buf));
    delete[] buf;
    
    return config;
}


std::wstring Sdserv::getDatabaseConnectionString(const std::wstring& database_name)
{
    // if database_name is a connection string itself, use it
    if (kl::icontains(database_name, L"xdprovider="))
        return database_name;

    // first, look for option "database.<dbname>"

    std::wstring res = getOption(L"database." + database_name);
    if (res.length() > 0)
    {
        kl::replaceStr(res, L"%database%", database_name);
        return res;
    }


    // then, check a config file, if available

    std::wstring config_file = getOption(L"sdserv.config_file");

    kl::JsonNode config = getJsonNodeFromFile(config_file);
    if (config.isNull())
        return L"";

    if (database_name.empty())
        return L"";

    kl::JsonNode databases = config["databases"];
    if (databases.childExists(database_name))
    {
        kl::JsonNode database = databases[database_name];
        kl::JsonNode connection_string = database["connection_string"];
        return connection_string.getString();
    }
     else
    {
        // check for wild card entry
        if (databases.childExists("*"))
        {
            kl::JsonNode wildcard = databases["*"];
            std::wstring cstr = wildcard["connection_string"].getString();
            if (cstr.empty())
                return L"";

            kl::replaceStr(cstr, L"%database%", database_name);
            return cstr;
        }

    }

    return L"";
}


void Sdserv::signalServerReady()
{
    std::wstring ready_evtid = getOption(L"sdserv.win32evt_ready");

    if (ready_evtid.length() > 0)
    {
        HANDLE evt_ready = CreateEvent(NULL, FALSE, FALSE, ready_evtid.c_str());
        if (evt_ready)
        {
            SetEvent(evt_ready);
            CloseHandle(evt_ready);
        }
    }

    printf("*** sdserv ready\n");
}


void Sdserv::signalServerNotReady()
{
    std::wstring notready_evtid = getOption(L"sdserv.win32evt_notready");

    if (notready_evtid.length() > 0)
    {
        HANDLE evt_notready = CreateEvent(NULL, FALSE, FALSE, notready_evtid.c_str());
        if (evt_notready)
        {
            SetEvent(evt_notready);
            CloseHandle(evt_notready);
        }
    }
}

void Sdserv::updateLastAccessTimestamp()
{
    m_last_access_mutex.lock();
    m_last_access = time(NULL);
    m_last_access_mutex.unlock();
}

void Sdserv::updateAssetInformation()
{
    if (m_ws_client)
    {
        m_ws_client->updateAssetInformation();
    }
}

int Sdserv::runServer()
{
    std::wstring database = getOption(L"sdserv.database");
    if (database.length() > 0)
    {
        m_database.clear();

        std::wstring cstr = getDatabaseConnectionString(database);
        if (cstr.empty())
        {
            printf("Unknown database '%ls'.  Exiting...\n", database.c_str());
            signalServerNotReady();
            return 1;
        }


        xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
        if (dbmgr)
        {
            m_database = dbmgr->open(cstr);
        }

        m_controller->setConnectionString(cstr);
    }



    std::wstring idle_quit = getOption(L"sdserv.idle_quit");
    if (idle_quit.length() > 0)
        m_idle_quit = kl::wtoi(idle_quit);



    std::wstring server_type = getOption(L"sdserv.server_type");
    if (server_type == L"http")
    {
        HttpServer http(this);
        http.run();
    }
     else if (server_type == L"websockets")
    {
        bool ssl = false;

        if (getOption(L"websockets.ssl") == L"true")
            ssl = true;

        std::string server = kl::tostring(getOption(L"websockets.server"));
        std::string path = kl::tostring(getOption(L"websockets.path"));
        if (path.empty())
            path = "/";

        int port = ssl  ? 443 :  80;
        WebSocketsClient ws(this);
        m_ws_client = &ws;
        ws.run(server, port, ssl, path);
        m_ws_client = NULL;
    }
     else
    {
        printf("unknown sdserv server mode\n");
        return 1;
    }

    return 0;
}



bool Sdserv::initOptionsFromCommandLine(int argc, const char* argv[])
{
    //const char *options[40] = { "listening_ports",   LISTENING_PORT,
    //                            "enable_keep_alive", "yes",
    //                            NULL };
    


    std::wstring cfg_file;
    std::wstring database;
    std::wstring home_cfg_file;
    std::string port;

#ifdef WIN32
    home_cfg_file  = _wgetenv(L"HOMEDRIVE");
    home_cfg_file += _wgetenv(L"HOMEPATH");
    home_cfg_file += L"\\sdserv.conf";
#endif

    int i;
    for (i = 1; i < argc; ++i)
    {
        if (0 == strcmp(argv[i], "-f") && i+1 < argc)
        {
            setOption(L"sdserv.config_file", kl::towstring(argv[i+1]));
        }
         else if (0 == strcmp(argv[i], "-d") && i+1 < argc)
        {
            setOption(L"sdserv.database", kl::towstring(argv[i+1]));
        }
         else if (0 == strcmp(argv[i], "--win32evt-ready") && i+1 < argc)
        {
            setOption(L"sdserv.win32evt_ready", kl::towstring(argv[i+1]));
        }
         else if (0 == strcmp(argv[i], "--win32evt-notready") && i+1 < argc)
        {
            setOption(L"sdserv.win32evt_notready", kl::towstring(argv[i+1]));
        }
         else if (0 == strcmp(argv[i], "--idle-quit") && i+1 < argc)
        {
            setOption(L"sdserv.idle_quit", kl::towstring(argv[i+1]));
        }
         else if (0 == strcmp(argv[i], "--ws") && i+1 < argc)
        {
            setOption(L"sdserv.server_type", L"websockets");
            setOption(L"websockets.server", kl::towstring(argv[i+1]));
        }
         else if (0 == strcmp(argv[i], "--wsssl"))
        {
            setOption(L"websockets.ssl", L"true");
        }
         else if (0 == strcmp(argv[i], "-p") && i+1 < argc)
        {
            setOption(L"http.port", kl::towstring(argv[i+1]));
        }
    }


    return true;
}



void Sdserv::setOption(const std::wstring& option, const std::wstring& value)
{
    m_options_mutex.lock();
    m_options[option] = value;
    m_options_mutex.unlock();
}


std::wstring Sdserv::getOption(const std::wstring& option)
{
    m_options_mutex.lock();
    std::wstring res = m_options[option];
    m_options_mutex.unlock();

    return res;
}


std::wstring Sdserv::getWebSocketsConnectionId()
{
    if (m_ws_client)
        return m_ws_client->m_connection;
         else
        return L"";
}
