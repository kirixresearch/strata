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
#include "jobserver.h"
#include <cstdio>
#include <kl/json.h>
#include <kl/file.h>


#ifdef WIN32
#include <windows.h>
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
    std::wstring run_file = getOption(L"sdserv.run_file");
    if (run_file.length() > 0)
        xf_remove(run_file);

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
#ifdef WIN32
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
#endif

    std::wstring run_file_contents = L"";
    std::wstring run_file = getOption(L"sdserv.run_file");

    if (m_http)
    {
        const std::vector<HttpListeningPort>& ports = m_http->getListeningPorts();
        std::vector<HttpListeningPort>::const_iterator it;

        kl::JsonNode root;
        root["ports"].setArray();

        for (it = ports.begin(); it != ports.end(); ++it)
        {
            kl::JsonNode element = root["ports"].appendElement();
            element.setObject();
            element["port"] = it->port;
            element["ssl"].setBoolean(it->ssl);
        }

        run_file_contents = root.toString();
    }


    if (run_file.length() > 0)
    {
        xf_put_file_contents(run_file, run_file_contents);
        printf("%ls\n", run_file_contents.c_str());
    }



    printf("*** sdserv ready\n");
    fflush(stdout);

}


void Sdserv::signalServerNotReady()
{
#ifdef WIN32
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
#endif

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
    //kl::thread_sleep(12000);

    std::wstring cstr;

    std::wstring dbtype = getOption(L"sdserv.database.type");
    if (dbtype == L"postgres")
    {
        int port = kl::wtoi(getOption(L"sdserv.database.post"));
        std::wstring host = getOption(L"sdserv.database.host");
        std::wstring database = getOption(L"sdserv.database.database");
        std::wstring user = getOption(L"sdserv.database.user");
        std::wstring password = getOption(L"sdserv.database.password");

        xd::ConnectionString cs;
        cs.setParameters(xd::dbtypePostgres, host, port, database, user, password);

        cstr = cs.getConnectionString();
    }
     else if (dbtype == L"filesystem")
    {
        std::wstring path = getOption(L"sdserv.database.path");
        if (path.length() == 0)
            return 1;
        
        if (!xf_get_directory_exist(path))
            xf_mkdir(path);

        xd::ConnectionString cs;
        cs.setValue(L"xdprovider", L"xdfs");
        cs.setValue(L"database", path);

        cstr = cs.getConnectionString();
    }
     else
    {
        std::wstring database = getOption(L"sdserv.database.database");
        if (database.length() > 0)
        {
            m_database.clear();

            cstr = getDatabaseConnectionString(database);
            if (cstr.empty())
            {
                printf("Unknown database '%ls'.  Exiting...\n", database.c_str());
                signalServerNotReady();
                return 1;
            }
        }
    }

    m_controller->setConnectionString(cstr);

    xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
    if (dbmgr)
    {
        m_database = dbmgr->open(cstr);
    }


    std::wstring idle_quit = getOption(L"sdserv.idle_quit");
    if (idle_quit.length() > 0)
        m_idle_quit = kl::wtoi(idle_quit);


    std::wstring server_type = getOption(L"sdserv.server_type");
    if (server_type == L"http")
    {
        HttpServer http(this);
        m_http = &http;
        http.run();
        m_http = NULL;
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
     else if (server_type == L"job")
    {
        JobServer job_server(this);
        job_server.run();
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
    const wchar_t* penv = _wgetenv(L"HOMEDRIVE");
    if (penv)
    {
        home_cfg_file = penv;
        penv = _wgetenv(L"HOMEPATH");
        if (penv)
        {
            home_cfg_file += penv;
            home_cfg_file += L"\\sdserv.conf";
        }
         else
        {
            home_cfg_file = L"";
        }
    }
#endif

    int i;
    std::wstring value;

    for (i = 1; i < argc; ++i)
    {
        const char* eq = strchr(argv[i], '=');
        if (eq)
        {
            value = kl::afterFirst(kl::towstring(argv[i]), '=');
        }
         else
        {
            if (i+1 < argc)
            {
                value = kl::towstring(argv[i+1]);
                if (value.length() > 0 && value[0] == L'-')
                    value = L"";
            }
        }


        if (0 == strcmp(argv[i], "-f") && i+1 < argc)
        {
            setOption(L"sdserv.config_file", value);
        }
         else if (0 == strcmp(argv[i], "-t") || 0 == strncmp(argv[i], "--database-type", 15))
        {
            setOption(L"sdserv.database.type", value);
        }
         else if (0 == strcmp(argv[i], "-h") || 0 == strncmp(argv[i], "--host", 6))
        {
            setOption(L"sdserv.database.host", value);
        }
         else if (0 == strcmp(argv[i], "-P") || 0 == strncmp(argv[i], "--port", 6))
        {
            setOption(L"sdserv.database.port", value);
        }
         else if (0 == strcmp(argv[i], "-d") || 0 == strncmp(argv[i], "--database", 10))
        {
            setOption(L"sdserv.database.database", value);
        }
         else if (0 == strcmp(argv[i], "-u") || 0 == strncmp(argv[i], "--user", 6))
        {
            setOption(L"sdserv.database.user", value);
        }
         else if (0 == strcmp(argv[i], "-p") || 0 == strncmp(argv[i], "--password", 10))
        {
            setOption(L"sdserv.database.password", value);
        }
         else if (0 == strcmp(argv[i], "-P") || 0 == strncmp(argv[i], "--path", 6))
        {
            setOption(L"sdserv.database.path", value);
        }
         else if (0 == strcmp(argv[i], "--win32evt-ready") && i+1 < argc)
        {
            setOption(L"sdserv.win32evt_ready", value);
        }
         else if (0 == strcmp(argv[i], "--win32evt-notready") && i+1 < argc)
        {
            setOption(L"sdserv.win32evt_notready", value);
        }
         else if (0 == strcmp(argv[i], "-i") || 0 == strncmp(argv[i], "--idle-quit", 11))
        {
            setOption(L"sdserv.idle_quit", value);
        }
         else if (0 == strcmp(argv[i], "-r") || 0 == strncmp(argv[i], "--run-file", 10))
        {
            setOption(L"sdserv.run_file", value);
        }
         else if (0 == strcmp(argv[i], "-j") || 0 == strncmp(argv[i], "--job-file", 10))
        {
            setOption(L"sdserv.server_type", L"job");
            setOption(L"sdserv.job_file", value);
        }
         else if (0 == strcmp(argv[i], "--ws") && i+1 < argc)
        {
            setOption(L"sdserv.server_type", L"websockets");
            setOption(L"websockets.server", value);
        }
         else if (0 == strcmp(argv[i], "--wsssl"))
        {
            setOption(L"websockets.ssl", L"true");
        }
         else if (0 == strncmp(argv[i], "--http-port", 11))
        {
            setOption(L"http.port", value);
        }
         else if (0 == strcmp(argv[i], "-s") || 0 == strncmp(argv[i], "--strip-path", 12))
        {
            setOption(L"http.strip_path", value);
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
