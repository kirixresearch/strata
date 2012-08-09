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
#include "jsonconfig.h"



Server g_server;
Controller c;


bool Server::readConfigFile(const std::wstring& config_file)
{
    static char s_ports[255];
    size_t i;
    size_t options_arr_size = 0;
    
    
    char* buf = new char[16384];
    
    xf_file_t f = xf_open(config_file, xfOpen, xfRead, xfShareNone);
    if (!f)
    {
        printf("Could not open config file for reading.\n");
        return false;
    }
    
    buf[0] = 0;
    size_t read_bytes = xf_read(f, buf, 1, 16000);
    buf[read_bytes] = 0;
    xf_close(f);
    
    JsonNode config;
    config.fromString(kl::towstring(buf));
    delete[] buf;
    
    
    JsonNode server = config["server"];
    if (server.isNull())
    {
        printf("Missing server node in configuration file.\n");
        return false;
    }
    


    // handle 'ports' and 'ssl_ports'
    std::string tmps;
    
    JsonNode ports_node = server["ports"];
    for (i = 0; i < ports_node.getCount(); ++i)
    {
        if (tmps.length() > 0)
            tmps += ",";
        tmps += kl::itostring(ports_node[i].getInteger());
    }
    
    JsonNode ssl_ports_node = server["ssl_ports"];
    for (i = 0; i < ports_node.getCount(); ++i)
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
   
    // terminator
    m_options[options_arr_size++] = NULL;
    
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
                            
    if (argc >= 3 && 0 == strcmp(argv[1], "-f"))
    {
        if (!readConfigFile(kl::towstring(argv[2])))
            return false;
    }
     else if (xf_get_file_exist(L"sdserv.conf"))
    {
        if (!readConfigFile(kl::towstring(argv[2])))
            return false;
    }
     else
    {
        printf("Missing config file.  Please specify a config file with the -f option");
    }
    
    return true;
}


int main(int argc, const char** argv)
{
    if (!g_server.initOptions(argc, argv))
        return 0;

    return g_server.runServer();
}

