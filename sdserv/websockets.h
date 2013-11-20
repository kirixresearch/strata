/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2013-11-15
 *
 */

 
#ifndef __SDSERV_WEBSOCKETS_H
#define __SDSERV_WEBSOCKETS_H

#include <queue>

struct libwebsocket;
struct libwebsocket_context;



class WebSocketsClient
{
public:

    bool run(const std::string& server, int port = 80, bool ssl = false);

public:

    void onMessage(const std::string& msg);
    void send(const std::string& msg);

public:

    struct libwebsocket_context* m_context;
    struct libwebsocket* m_wsi;
    std::queue<std::string> m_write_bufs;
};



#endif
