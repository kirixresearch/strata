/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Sdserv Data Service
 * Author:   Benjamin I. Williams
 * Created:  2013-08-20
 *
 */


#define _CRT_SECURE_NO_WARNINGS 1


#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <vector>
#include <kl/file.h>
#include <kl/string.h>
#include <kl/json.h>
#include "sdsvc.h"
#include "service.h"
#include "mongoose.h"



Service g_service;



static void* request_callback(enum mg_event evt,
                              struct mg_connection* conn,
                              const struct mg_request_info* request_info)
{
    if (evt == MG_NEW_REQUEST)
    {
        mg_force_close(conn);

        int sock;
        char* ipaddress = "127.0.0.1";
        int port = 4800;
        struct sockaddr_in serveraddr;
        std::string request;

        //request += "GET / HTTP/1.0\r\n";
        request += "GET ";
        request += request_info->uri;
        request += " HTTP/1.0\r\n";

        request += "Host: localhost\r\n";
        request += "Connection: close\r\n";
        request += "\r\n";

        printf("\n\n-----------------------------------------------------\n");
        printf("%s\n\n", request.c_str());

        // open socket
        if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
            return "processed";  // socket() failed

        // connect
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = inet_addr(ipaddress);
        serveraddr.sin_port = htons((unsigned short) port);
        if (connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
            return "processed"; // connect() failed

        // send request
        if (send(sock, request.c_str(), request.length(), 0) != request.length())
            return "processed"; // sent bytes mismatched request bytes

        // get response
        #define BUFFERSIZE 4096
        char buf[BUFFERSIZE+1];
        int len;
        while (true)
        {
            len = recv(sock, (char*)&buf, BUFFERSIZE, 0);
            if (len <= 0)
                break;

            buf[len] = 0;
            
            //printf("%s",buf);

            mg_write(conn, buf, (size_t)len);
        }

        closesocket(sock);

    }
     else if (evt == MG_EVENT_LOG)
    {
        printf("ERROR: %s\n", request_info->log_message);
    }

    return "processed";
}




static kl::JsonNode getJsonNodeFromFile(const std::wstring& filename)
{
    xf_off_t size = xf_get_file_size(filename);
    if (size < 0 || size > 1000000)
        return kl::JsonNode();
    
    xf_file_t f = xf_open(filename, xfOpen, xfRead, xfShareNone);
    if (!f)
    {
        printf("Could not open config file for reading.\n");
        
        kl::JsonNode null_return;
        return null_return;
    }
    
    char* buf = new char[((int)size)+1];
    buf[0] = 0;
    xf_off_t read_bytes = xf_read(f, buf, 1, (unsigned int)size);
    buf[read_bytes] = 0;
    xf_close(f);
    
    kl::JsonNode config;
    config.fromString(kl::towstring(buf));
    delete[] buf;
    
    return config;
}




Service::Service()
{
}

Service::~Service()
{
}


void Service::readConfig()
{
}


void Service::stop()
{
    m_running = false;
}

void Service::run()
{
    readConfig();

    struct mg_context* ctx;

    const char* options[40] = { "listening_ports",   "8080",
                                "enable_keep_alive", "yes",
                                "num_threads", "30",

                               NULL };
        
    ctx = mg_start(request_callback, NULL, options);
    
    while (m_running)
        ::Sleep(1000);
    
    mg_stop(ctx);
}

