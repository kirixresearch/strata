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


#include "sdsvc.h"
#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <vector>
#include <kl/file.h>
#include <kl/string.h>
#include <kl/json.h>
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


        std::string instance;

        const char* uri = request_info->uri;
        if (uri)
        {
            if (*uri=='/') ++uri;
            const char* slash = strchr(uri,'/');
            if (slash)
            {
                instance.assign(uri, slash-uri);
                uri = slash;
            }
             else
            {
                instance = uri;
                uri = "/";
            }
        }

        if (instance.empty() || instance == "favicon.ico")
            return L"processed";


        int sock;
        char* ipaddress = "127.0.0.1";
        int port = 0;
        struct sockaddr_in serveraddr;
        std::string request;


        port = g_service.getServerPort(instance);

        if (port == 0)
        {
            mg_write(conn, "HTTP/1.0 404 Not Found\r\n", 24);
            return "processed";
        }


        //request += "GET / HTTP/1.0\r\n";
        request += "GET ";
        request += uri;
        request += " HTTP/1.0\r\n";

        request += "Host: localhost\r\n";
        request += "Connection: close\r\n";
        request += "\r\n";

        printf("\n\n-----------------------------------------------------\n");
        printf("instance: %s\n", instance.c_str());
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
        #define BUFFERSIZE 16384
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
    m_running = false;
    m_next_port = 28000;
}

Service::~Service()
{
}


void Service::readConfig()
{
}

int Service::getServerPort(const std::string& instance)
{
    XCM_AUTO_LOCK(m_mutex);


    SdServer info;
    info.port = 0;

    std::map<std::string, SdServer>::iterator it = m_servers.find(instance);
    if (it != m_servers.end())
        info = it->second;


    if (info.port == 0)
    {
        // find a free port
        info.instance = instance;
        info.process = NULL;
        info.thread = NULL;
        info.port = m_next_port++;

        // get our path
        TCHAR temps[MAX_PATH];
        if (!GetModuleFileName(NULL, temps, MAX_PATH))
            return 0;
        std::wstring exepath = kl::beforeLast(temps, '\\');
        exepath += L"\\sdserv.exe";

        // start the appropriate server
        wchar_t cmdline[255];
        swprintf(cmdline, 255, L"%ls -d %hs -p %d", exepath.c_str(), instance.c_str(), info.port);

        DWORD result = 0;
        STARTUPINFO startup_info;
        PROCESS_INFORMATION process_info;
        memset(&startup_info, 0, sizeof(STARTUPINFO));
        memset(&process_info, 0, sizeof(PROCESS_INFORMATION));
        startup_info.cb = sizeof(STARTUPINFO);

        if (CreateProcess(NULL,
                          cmdline,
                          0, 0, false,
                          0, // CREATE_DEFAULT_ERROR_MODE,
                          0, 0,
                          &startup_info,
                          &process_info) != false)
        {
            result = ERROR_SUCCESS;
        }
         else
        {
            result = GetLastError();
        }
    
        info.process = process_info.hProcess;
        info.thread = process_info.hThread;
        m_servers[instance] = info;

        ::Sleep(1000);

        return info.port;
    }

    return info.port;
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
    
    m_running = true;

    while (m_running)
        ::Sleep(1000);
    
    mg_stop(ctx);
}

