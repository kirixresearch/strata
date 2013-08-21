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
#include "mongoose.h"
#include "request.h"
#include "controller.h"



Sdserv g_sdserv;
Controller c;


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


Sdserv::Sdserv()
{
    m_options[0] = 0;
}

Sdserv::~Sdserv()
{
}


std::wstring Sdserv::getDatabaseConnectionString(const std::wstring& database_name)
{
    kl::JsonNode config = getJsonNodeFromFile(m_config_file);
    if (config.isNull())
        return L"";

    kl::JsonNode databases = config["databases"];
    kl::JsonNode database = databases[database_name];
    kl::JsonNode connection_string = database["connection_string"];
    
    return connection_string.getString();
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

static void* request_callback(enum mg_event evt,
                              struct mg_connection* conn,
                              const struct mg_request_info* request_info)
{
    if (evt == MG_NEW_REQUEST)
    {
        RequestInfo req(conn, request_info);

        req.read();

        if (!c.onRequest(req))
        {
            req.setStatusCode(404);
            req.setContentType("text/html");
            req.write("<html><body><h2>Not found</h2></body></html>");
        }
    }
     else if (evt == MG_EVENT_LOG)
    {
        printf("ERROR: %s\n", request_info->log_message);
    }

    return "processed";
}


int Sdserv::runServer()
{
    struct mg_context *ctx;

    if (m_options[0] == 0)
        return 0;
        
    ctx = mg_start(request_callback, NULL, m_options);
    
    while (1)
        ::Sleep(1000);
        
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
    int port = 0;
    bool ssl = false;

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
            port = atoi(argv[i+1]);
        if (0 == strcmp(argv[i], "-s"))
            ssl = true;
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





    if (port != 0)
    {
        int i = 0;
        for (i = 0; i < sizeof(m_options); ++i)
        {
            if (!m_options[i])
                break;

            if (0 == strcmp(m_options[i], "listening_ports"))
            {
                if (ssl)
                    sprintf((char*)m_options[i+1], "%ds", port);
                     else
                    sprintf((char*)m_options[i+1], "%d", port);
            }
        }
    }

    if (database.length() > 0)
    {
        std::wstring cstr = g_sdserv.getDatabaseConnectionString(database);
        if (cstr.empty())
        {
            printf("Unknown database '%ls'.  Exiting...\n", database.c_str());
            return 1;
        }

        c.setConnectionString(cstr);
    }


    return true;
}


int main(int argc, const char** argv)
{
    if (!g_sdserv.initOptions(argc, argv))
        return 0;

    g_sdserv.runServer();

    return 0;
}

