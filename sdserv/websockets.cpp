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
#include "http.h"
#include "controller.h"
#include "websockets.h"
#include "libwebsockets.h"


#define MAX_MESSAGE_PAYLOAD 1400


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
    WebSocketsClient* wsclient = (WebSocketsClient*)libwebsocket_context_user(context);
	struct websockets_message_data* data = (struct websockets_message_data*)user;
    char* buf;
	int n;

	switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            data->index = 0;
            wsclient->send("HELLO");
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
        {
            size_t remaining = libwebsockets_remaining_packet_payload(wsi);
            data->val.append((char*)in, len);
            if (remaining == 0)
            {
                wsclient->onMessage(data->val);
                data->val = "";
            }
        }
        break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            if (!wsclient->m_write_bufs.empty())
            {
                buf = (char*)(data->buf + LWS_SEND_BUFFER_PRE_PADDING);

                std::string& front = wsclient->m_write_bufs.front();
                size_t len = front.length();

                if (len <= MAX_MESSAGE_PAYLOAD)
                {
                    memcpy(buf, front.c_str(), len);
                    n = libwebsocket_write(wsi, (unsigned char*)buf, len, LWS_WRITE_TEXT);
                    wsclient->m_write_bufs.pop();

                    // schedule another write if there's more to write
                    if (!wsclient->m_write_bufs.empty())
                        libwebsocket_callback_on_writable(context, wsi);
                }
                 else
                {
                    memcpy(buf, front.c_str(), MAX_MESSAGE_PAYLOAD);
                    n = libwebsocket_write(wsi, (unsigned char*)buf, len, LWS_WRITE_TEXT);
                    front = front.substr(MAX_MESSAGE_PAYLOAD);
                    
                    // schedule another write
                    libwebsocket_callback_on_writable(context, wsi);
                }


                break;
            }

        default:
            break;
	}

	return 0;
}


void WebSocketsClient::onMessage(const std::string& msg)
{
    printf("RECEIVED %s\n\n\n", msg.c_str());

    std::wstring wmsg = kl::towstring(msg);

    kl::JsonNode node;
    node.fromString(wmsg);

    if (node.childExists("token"))
    {
        std::wstring token = node["token"];
        
        kl::JsonNode result;
        result["token"] = token;

        std::string reply = kl::tostring(result.toString());
        send(reply);
    }
}


void WebSocketsClient::send(const std::string& msg)
{
    m_write_bufs.push(msg);
    libwebsocket_callback_on_writable(m_context, m_wsi);
}


bool WebSocketsClient::run(const std::string& server, int port, bool ssl)
{
    struct lws_context_creation_info info;

    static struct libwebsocket_protocols protocols[] =
    {
        {
            "",                                      // name
            websockets_callback,                     // callback
            sizeof(struct websockets_message_data)   // per_session_data_size
        },

        {
            NULL, NULL, 0           // terminator
        }
    };



    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.user = (void*)this;

	m_context = libwebsocket_create_context(&info);

	if (!m_context)
    {
		// libwebsocket init failed
		return false;
	}


    m_wsi = libwebsocket_client_connect(m_context, server.c_str(), port, ssl ? 1 : 0, "/", server.c_str(), "origin", NULL, -1);

    if (!m_wsi)
    {
        // connect failed
        return false;
    }

    int res = 0;
    while (res >= 0)
    {
        res = libwebsocket_service(m_context, 50);
    }

    return true;
}
