/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
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




Server g_server;
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


Server::Server()
{
    m_options[0] = 0;
}

Server::~Server()
{
}


std::wstring Server::getDatabaseConnectionString(const std::wstring& database_name)
{
    kl::JsonNode config = getJsonNodeFromFile(m_config_file);
    if (config.isNull())
        return L"";

    kl::JsonNode databases = config["databases"];
    kl::JsonNode database = databases[database_name];
    kl::JsonNode connection_string = database["connection_string"];
    
    return connection_string.getString();
}


bool Server::useConfigFile(const std::wstring& config_file)
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
        tmps += kl::itostring(ports_node[i].getInteger());
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
    m_options[options_arr_size++] = "50";
    
       // terminator
    m_options[options_arr_size++] = NULL;
    
    m_config_file = config_file;
    
    return true;
}

//static
void* Server::callback(enum mg_event evt,
                       struct mg_connection* conn,
                       const struct mg_request_info* request_info)
{
    if (evt == MG_NEW_REQUEST)
    {
        RequestInfo req(conn, request_info);
    
        if (!c.onRequest(req))
        {
            req.setStatusCode(404);
            req.setContentType("text/html");
            req.write("<html><body><h2>Not found</h2></body></html>");
        }
    }

    return "processed";
}


int Server::runServer()
{
    struct mg_context *ctx;

    if (m_options[0] == 0)
        return 0;
        
    ctx = mg_start(&Server::callback, NULL, m_options);
    
    while (1)
        ::Sleep(1000);
        
    return 0;
}



bool Server::initOptions(int argc, const char* argv[])
{
    //const char *options[40] = { "listening_ports",   LISTENING_PORT,
    //                            "enable_keep_alive", "yes",
    //                            NULL };
    
    const char* options[255];
    options[0] = NULL;
    

    std::wstring cfg_file;
    
#ifdef WIN32
    cfg_file = _wgetenv(L"HOMEDRIVE");
    cfg_file += _wgetenv(L"HOMEPATH");
    cfg_file += L"\\sdserv.conf";
#endif

              
    if (argc >= 3 && 0 == strcmp(argv[1], "-f"))
    {
        if (!useConfigFile(kl::towstring(argv[2])))
            return false;
    }
     else if (xf_get_file_exist(L"sdserv.conf"))
    {
        if (!useConfigFile(L"sdserv.conf"))
            return false;
    }
     else if (cfg_file.length() > 0 && xf_get_file_exist(cfg_file))
    {
        if (!useConfigFile(cfg_file))
            return false;
    }
     else
    {
        printf("Missing config file.  Please specify a config file with the -f option");
        return false;
    }
    
    return true;
}


int main(int argc, const char** argv)
{
    if (!g_server.initOptions(argc, argv))
        return 0;

    return g_server.runServer();
}

