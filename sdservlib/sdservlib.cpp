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



Sdserv::Sdserv()
{
    m_options[0] = 0;
    m_last_access = time(NULL);
    m_idle_quit = 0;
    m_server_type = serverHttp;
    m_websockets_ssl = false;
    m_controller = new Controller;
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
    kl::JsonNode config = getJsonNodeFromFile(m_config_file);
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


bool Sdserv::useConfigFile(const std::wstring& config_file)
{
    static char s_ports[255];
    size_t i;
    size_t options_arr_size = 0;
    
    
    kl::JsonNode config = getJsonNodeFromFile(config_file);
    if (config.isNull())
        return L"";
    
    kl::JsonNode server = config["server"];
    if (server.isNull())
    {
        printf("Missing server node in configuration file.\n");
        return false;
    }
    


    // handle 'ports' and 'ssl_ports'
    std::string tmps;
    
    kl::JsonNode ports_node = server["ports"];
    for (i = 0; i < ports_node.getChildCount(); ++i)
    {
        if (tmps.length() > 0)
            tmps += ",";
        tmps += kl::itostring(ports_node[i].getInteger());
    }
    
    kl::JsonNode ssl_ports_node = server["ssl_ports"];
    for (i = 0; i < ssl_ports_node.getChildCount(); ++i)
    {
        if (tmps.length() > 0)
            tmps += ",";
        tmps += kl::itostring(ssl_ports_node[i].getInteger());
        tmps += "s";
    }
    
    if (tmps.length() == 0 || tmps.length() >= sizeof(s_ports)-1)
    {
        printf("Please specify at least one port or ssl_port.\n");
        return false;
    }
    strcpy(s_ports, tmps.c_str());
    
    
    m_options[options_arr_size++] = "listening_ports";
    m_options[options_arr_size++] = s_ports;
    
    // enable keep alive by default
    m_options[options_arr_size++] = "enable_keep_alive";
    m_options[options_arr_size++] = "yes";

    // enable keep alive by default
    m_options[options_arr_size++] = "num_threads";
    m_options[options_arr_size++] = "30";
    
    
    
    kl::JsonNode ssl_cert = server["ssl_cert"];
    if (ssl_cert.isOk())
    {
        std::wstring cert_file = ssl_cert.getString();
        if (!xf_get_file_exist(cert_file))
        {
            printf("Certificate %ls does not exist.\n", cert_file.c_str());
            return false;
        }
        
        std::string cert_file_asc = kl::tostring(cert_file);
        strcpy(m_cert_file_path, cert_file_asc.c_str());
        
        m_options[options_arr_size++] = "ssl_certificate";
        m_options[options_arr_size++] = m_cert_file_path;
    }
    
    
    // terminator
    m_options[options_arr_size++] = NULL;
    
    m_config_file = config_file;
    
    return true;
}

void Sdserv::signalServerReady()
{
    if (m_ready_evtid.length() > 0)
    {
        HANDLE evt_ready = CreateEvent(NULL, FALSE, FALSE, m_ready_evtid.c_str());
        if (evt_ready)
        {
            SetEvent(evt_ready);
            CloseHandle(evt_ready);
        }
    }
}


void Sdserv::signalServerNotReady()
{
    if (m_notready_evtid.length() > 0)
    {
        HANDLE evt_notready = CreateEvent(NULL, FALSE, FALSE, m_notready_evtid.c_str());
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

int Sdserv::runServer()
{
    if (m_server_type == serverHttp)
    {
        HttpServer http(this);
        http.run(m_options);
    }
     else if (m_server_type == serverWebSocketsClient)
    {
        int port = m_websockets_ssl ? 443 :  80;
        WebSocketsClient ws(this);
        ws.run(m_websockets_server, port, m_websockets_ssl);
    }

    return 0;
}



bool Sdserv::initOptions(int argc, const char* argv[])
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
            cfg_file = kl::towstring(argv[i+1]);
        if (0 == strcmp(argv[i], "-d") && i+1 < argc)
            database = kl::towstring(argv[i+1]);
        if (0 == strcmp(argv[i], "-p") && i+1 < argc)
            port = argv[i+1];
        if (0 == strcmp(argv[i], "--win32evt-ready") && i+1 < argc)
            m_ready_evtid = kl::towstring(argv[i+1]);
        if (0 == strcmp(argv[i], "--win32evt-notready") && i+1 < argc)
            m_notready_evtid = kl::towstring(argv[i+1]);
        if (0 == strcmp(argv[i], "--idle-quit") && i+1 < argc)
            m_idle_quit = atoi(argv[i+1]);
        if (0 == strcmp(argv[i], "--ws") && i+1 < argc)
        {
            m_server_type = serverWebSocketsClient;
            m_websockets_server = argv[i+1];
        }
        if (0 == strcmp(argv[i], "--wsssl"))
        {
            m_websockets_ssl = true;
        }
    }


    if (cfg_file.length() > 0)
    {
        if (!useConfigFile(cfg_file))
            return false;
    }
     else if (xf_get_file_exist(L"sdserv.conf"))
    {
        if (!useConfigFile(L"sdserv.conf"))
            return false;
    }
     else if (xf_get_file_exist(home_cfg_file))
    {
        if (!useConfigFile(home_cfg_file))
            return false;
    }


    if (port.length() > 0 && port.length() < 80)
    {
        int i = 0;
        for (i = 0; i < sizeof(m_options); ++i)
        {
            if (!m_options[i])
                break;

            if (0 == strcmp(m_options[i], "listening_ports"))
            {
                strcpy((char*)m_options[i+1], port.c_str());
            }
        }
    }

    if (database.length() > 0)
    {
        std::wstring cstr = getDatabaseConnectionString(database);
        if (cstr.empty())
        {
            printf("Unknown database '%ls'.  Exiting...\n", database.c_str());
            return 1;
        }

        m_controller->setConnectionString(cstr);
    }



    return true;
}
