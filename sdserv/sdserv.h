/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2012-07-30
 *
 */


#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <cwchar>
#include <ctime>
#include <windows.h>
#include <tango.h>
#include <kl/string.h>
#include <kl/md5.h>
#include <kl/base64.h>
#include <kl/portable.h>
#include <kl/memory.h>
#include <kl/file.h>


struct mg_connection;
struct mg_request_info;

class Server
{
public:
    
    Server();
    ~Server();
    
    bool initOptions(int argc, const char* argv[]);
    int runServer();
    std::wstring getDatabaseConnectionString(const std::wstring& database);
    
private:

    bool useConfigFile(const std::wstring& config_file);
    
    static void* callback(enum mg_event evt,
                          struct mg_connection* conn,
                          const struct mg_request_info* request_info);
                       
private:

    std::wstring m_config_file;
    
    const char* m_options[255];
};


extern Server g_server;