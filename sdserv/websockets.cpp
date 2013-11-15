/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2013-11-15
 *
 */

#define _WINSOCKAPI_
#include "sdserv.h"
#include "request.h"
#include "controller.h"
#include "websockets.h"
#include "libwebsockets.h"


#define MAX_MESSAGE_PAYLOAD 1400
#define SITE_URL "dataex.goldprairie.com"

struct websockets_message_data
{
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + MAX_MESSAGE_PAYLOAD + LWS_SEND_BUFFER_POST_PADDING];
	unsigned int len;
	unsigned int index;
    std::string val;
};





static int websockets_callback(struct libwebsocket_context* context,
                               struct libwebsocket* wsi,
                               enum libwebsocket_callback_reasons reason,
                               void* user,
                               void* in,
                               size_t len)
{
	struct websockets_message_data* data = (struct websockets_message_data*)user;
    char* buf;
	int n;

	switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            data->index = 0;
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
        {
            size_t remaining = libwebsockets_remaining_packet_payload(wsi);
            data->val.append((char*)in, len);
            if (remaining == 0)
            {
                printf("%s\n", data->val.c_str());
                data->val = "";
            }
        }
        break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            // send packet
            buf = (char*)(data->buf + LWS_SEND_BUFFER_PRE_PADDING);
            strcpy(buf, "HELLO");
            data->len = 5;

            n = libwebsocket_write(wsi, (unsigned char*)buf, 5, LWS_WRITE_TEXT);
            break;

        default:
            break;
	}

	return 0;
}


bool WebSocketsClient::run()
{
    struct lws_context_creation_info info;
    struct libwebsocket_context* context;
    struct libwebsocket* wsi;

    static struct libwebsocket_protocols protocols[] =
    {
        {
            "",              // name
            websockets_callback,    // callback
            sizeof(struct websockets_message_data)   // per_session_data_size
        },

        {
            NULL, NULL, 0           // terminator
        }
    };



    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

	context = libwebsocket_create_context(&info);

	if (!context)
    {
		// libwebsocket init failed
		return false;
	}


    wsi = libwebsocket_client_connect(context, SITE_URL, 80, 0 /* use_ssl */, "/", SITE_URL, "origin", NULL, -1);

    if (!wsi)
    {
        // connect failed
        return false;
    }

    int res = 0;
    while (res >= 0)
    {
        res = libwebsocket_service(context, 50);
    }

    return true;
}


