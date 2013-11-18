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





class WebSocketsClient
{
public:

    bool run();

public:

    void onMessage(const std::string& msg);
};



#endif
