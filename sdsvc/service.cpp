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
#include <kl/portable.h>
#include "service.h"
#include "mongoose.h"



Service g_service;


static int request_callback( struct mg_connection* conn)
{
    const struct mg_request_info* request_info = mg_get_request_info(conn);


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
        return 1;


    int sock;
    char* ipaddress = "127.0.0.1";
    int port = 0;
    struct sockaddr_in serveraddr;
    std::string request;


    port = g_service.getServerPort(instance);

    if (port == 0)
    {
        mg_write(conn, "HTTP/1.0 404 Not Found\r\n", 24);
        return 1;
    }


    //request += "GET / HTTP/1.0\r\n";
    request += request_info->request_method;
    request += ' ';
    request += uri;
    if (request_info->query_string && *(request_info->query_string))
    {
        request += '?';
        request += request_info->query_string;
    }
    request += " HTTP/1.0\r\n";

    request += "Host: localhost\r\n";
    request += "Connection: close\r\n";

    for (int h = 0; h < request_info->num_headers; ++h)
    {
        if (0 == strcasecmp(request_info->http_headers[h].name, "Connection") ||
            0 == strcasecmp(request_info->http_headers[h].name, "Host"))
        {
            continue;
        }

        request += request_info->http_headers[h].name;
        request += ": ";
        request += request_info->http_headers[h].value;
        request += "\r\n";
    }

    request += "\r\n";

    printf("\n\n-----------------------------------------------------\n");
    printf("instance: %s\n", instance.c_str());
    printf("%s\n\n", request.c_str());

    // open socket
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        return 1;  // socket() failed

    // connect
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ipaddress);
    serveraddr.sin_port = htons((unsigned short) port);
    if (connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return 1; // connect() failed

    // send request
    if (send(sock, request.c_str(), request.length(), 0) != request.length())
        return 1; // sent bytes mismatched request bytes


    #define BUFFERSIZE 16384
    char buf[BUFFERSIZE+1];


    if (*(request_info->request_method) == 'P')
    {
        int h;
        int content_length = -1;
        for (h = 0; h < request_info->num_headers; ++h)
        {
            if (0 == strncasecmp("Content-Length", request_info->http_headers[h].name, 14))
                content_length = atoi(request_info->http_headers[h].value);
        }

        printf("expected content length: %d\n", content_length);

        int buf_len;
        int received_bytes = 0;

        while (true)
        {
            buf_len = mg_read(conn, buf, BUFFERSIZE);
                
            send(sock, buf, buf_len, 0);
                
            received_bytes += buf_len;

            if (buf_len != BUFFERSIZE)
                break;
        }

        printf("received bytes: %d\n", received_bytes);

        if (content_length != -1 && received_bytes != content_length)
        {
            closesocket(sock);
            return 1;
        }
    }


    // get response
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


    return 1;
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
    m_options[0] = NULL;
}

Service::~Service()
{
}


bool Service::readConfig(const std::wstring& config_file)
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
    m_options[options_arr_size++] = "50";
    
    
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

    return true;
}

int Service::getServerPort(const std::string& instance)
{
    SdServer info;
    info.port = 0;
    info.process = NULL;

    m_mutex.lock();
    std::map<std::string, SdServer>::iterator it = m_servers.find(instance);
    if (it != m_servers.end())
    {
        info = it->second;
    }
     else
    {
        info.instance = instance;
        info.port = 0;
        info.process = NULL;
        info.thread = NULL;
        m_servers[instance] = info;
    }
    m_mutex.unlock();


    if (info.process)
    {
        DWORD exit_code = 0;
        GetExitCodeProcess(info.process, &exit_code);
        if (exit_code != STILL_ACTIVE)
        {
            CloseHandle(info.process);
            CloseHandle(info.thread);
            info.port = 0;
            info.process = NULL;
            info.thread = NULL;

            // process no longer appears to be running.  Restart it
            m_mutex.lock();
            std::map<std::string, SdServer>::iterator it = m_servers.find(instance);
            it->second.port = 0;
            it->second.process = NULL;
            it->second.thread = NULL;
            m_mutex.unlock();
        }
    }


    if (info.port == 0)
    {
        // find a free port
        info.instance = instance;
        info.process = NULL;
        info.thread = NULL;
        info.port = m_next_port++;

        wchar_t port_str[128];
        swprintf(port_str, 128, L"127.0.0.1:%d", info.port);

        // get our path
        TCHAR temps[MAX_PATH];
        if (!GetModuleFileName(NULL, temps, MAX_PATH))
            return 0;
        std::wstring exepath = kl::beforeLast(temps, '\\');
        exepath += L"\\sdserv.exe";


        // create two events which will help us synchronize the server starting
        std::wstring ready_evtid = kl::getUniqueString();
        std::wstring notready_evtid = kl::getUniqueString();
        HANDLE ready_evt = CreateEvent(NULL, FALSE, FALSE, ready_evtid.c_str());
        HANDLE notready_evt = CreateEvent(NULL, FALSE, FALSE, notready_evtid.c_str());

        // start the appropriate server
        wchar_t cmdline[255];
        swprintf(cmdline, 255, L"%ls -d %hs -p %ls --win32evt-ready %ls --win32evt-notready %ls --idle-quit 3000", exepath.c_str(), instance.c_str(), port_str, ready_evtid.c_str(), notready_evtid.c_str());

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
            CloseHandle(ready_evt);
            CloseHandle(notready_evt);
            result = GetLastError();
            return 0;
        }
    
        // wait until the server is running
        HANDLE events[2];
        events[0] = ready_evt;
        events[1] = notready_evt;
        DWORD dwresult = WaitForMultipleObjects(2, events, false, INFINITE);
        CloseHandle(ready_evt);
        CloseHandle(notready_evt);

        if (dwresult == WAIT_OBJECT_0)
        {
            info.process = process_info.hProcess;
            info.thread = process_info.hThread;
            m_servers[instance] = info;
        }
         else
        {
            // failure
        }

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
    std::wstring home_cfg_file;

#ifdef WIN32
    home_cfg_file  = _wgetenv(L"HOMEDRIVE");
    home_cfg_file += _wgetenv(L"HOMEPATH");
    home_cfg_file += L"\\sdserv.conf";
#endif

    if (xf_get_file_exist(home_cfg_file))
    {
        if (!readConfig(home_cfg_file))
            return;
    }

    printf("Server started. Options are: \n\n");
    const char** options = m_options;
    while (*options)
    {
        printf("%-22s: %s\n", *options, *(options+1));
        options += 2;
    }


    struct mg_context* ctx;
    struct mg_callbacks callbacks;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.begin_request = request_callback;


    ctx = mg_start(&callbacks, NULL, m_options);
    
    m_running = true;

    while (m_running)
        ::Sleep(1000);
    
    mg_stop(ctx);
}

