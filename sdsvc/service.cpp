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
#include <kl/string.h>
#include "sdsvc.h"
#include "mongoose.h"


bool g_running = true;

bool StopApplicationProcess()
{
    g_running = false;
    return true;
}



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


DWORD ServiceExecutionThread(DWORD* param)
{
    struct mg_context *ctx;

    const char* options[40] = { "listening_ports",   "8080",
                                "enable_keep_alive", "yes",
                                "num_threads", "30",

                               NULL };
        
    ctx = mg_start(request_callback, NULL, options);
    
    while (g_running)
        ::Sleep(1000);
    
    mg_stop(ctx);

    return 0;
}

