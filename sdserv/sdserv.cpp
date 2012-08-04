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


Controller c;

#define LISTENING_PORT "23456"

static void* callback(enum mg_event evt,
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


int main(int argc, const char** argv)
{
    struct mg_context *ctx;
    const char *options[] = { "listening_ports",   LISTENING_PORT,
                              //"enable_keep_alive", "yes",
                              NULL };

    ctx = mg_start(callback, NULL, options);
    
    while (1)
    {
        ::Sleep(1000);
    }

	return 0;
}

