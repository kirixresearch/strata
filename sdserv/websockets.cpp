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


extern Controller g_controller;










WebSocketsRequestInfo::WebSocketsRequestInfo(WebSocketsClient* client)
{
    m_content_length = 0;
    m_client = client;
}


std::wstring WebSocketsRequestInfo::getURI()
{
    return m_uri;
}

std::wstring WebSocketsRequestInfo::getValue(const std::wstring& key, const std::wstring& def)
{
    std::map<std::wstring, std::wstring>::const_iterator p_it;
    p_it = m_params.find(key);
    if (p_it == m_params.end())
        return def;
    return p_it->second;
}

bool WebSocketsRequestInfo::getValueExists(const std::wstring& key)
{
    std::map<std::wstring, std::wstring>::const_iterator p_it;
    p_it = m_params.find(key);
    if (p_it != m_params.end())
        return true;
    return false;
}

void WebSocketsRequestInfo::setValue(const std::wstring& key, const std::wstring& value)
{
    m_params[key] = value;
}


int WebSocketsRequestInfo::getContentLength()
{
    return m_content_length;
}

RequestFileInfo WebSocketsRequestInfo::getPostFileInfo(const std::wstring& key)
{
    RequestFileInfo rfi;
    return rfi;
}


void WebSocketsRequestInfo::setContentType(const char* content_type)
{
    m_content_type = content_type;
}

size_t WebSocketsRequestInfo::write(const void* ptr, size_t length)
{
    std::string msg((char*)ptr, length);
    m_client->send(msg);
    return msg.length();
}

size_t WebSocketsRequestInfo::write(const std::wstring& str)
{
    return write(kl::tostring(str));
}

size_t WebSocketsRequestInfo::write(const std::string& str)
{
    std::string header = "Token: " + m_token + "\n\n";
    m_client->send(header + str);
    return str.length();
}



void WebSocketsRequestInfo::sendNotFoundError()
{
    std::string reply;
    reply = "Token: " + m_token + "\n" +
            "Error: NOTFOUND\n\n";

    m_client->send(reply);
}


















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
                }
                 else
                {
                    buf = new char[LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING];
                    memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, front.c_str(), len);
                    n = libwebsocket_write(wsi, (unsigned char*)(buf+LWS_SEND_BUFFER_PRE_PADDING), len, LWS_WRITE_TEXT);
                    delete[] buf;

                    // schedule another write
                    libwebsocket_callback_on_writable(context, wsi);
                }

                wsclient->m_write_bufs.pop();

                // schedule another write if there's more to write
                if (!wsclient->m_write_bufs.empty())
                    libwebsocket_callback_on_writable(context, wsi);

                break;
            }

        default:
            break;
	}

	return 0;
}




void WebSocketsClient::onMessage(const std::string& msg)
{
    printf("RECEIVED: %s\n\n\n", msg.c_str());

    std::wstring wmsg = kl::towstring(msg);
    
    // parse headers
    std::wstring token, method, path, params;

    size_t end = wmsg.find(L"\n\n");
    size_t pos = 0;
    size_t lf, colon, eq, amp;
    std::wstring chunk, key, value;
    if (end == wmsg.npos)
        return; // invalid format
    while (pos < end)
    {
        lf = wmsg.find('\n', pos);

        chunk = wmsg.substr(pos, lf-pos);

        colon = chunk.find(':');
        if (colon == chunk.npos)
            return; // invalid format

        key = chunk.substr(0, colon);
        value = chunk.substr(colon+1);
        kl::trim(value);

        if (key == L"Token") token = value;
        else if (key == L"Parameters") params = value;
        else if (key == L"Path") path = value;
        else if (key == L"Method") method = value;

        pos = lf+1;
    }


    WebSocketsRequestInfo req(this);
    req.m_uri = path;
    req.m_token = kl::tostring(token);


    // parse parameters
    pos = 0;
    end = params.length();
    while (pos < end)
    {
        amp = params.find('&', pos);
        if (amp == params.npos)
            amp = end;

        chunk = params.substr(pos, amp-pos);

        eq = chunk.find('=');
        if (eq == chunk.npos)
            break; // invalid format (missing equals)

        key = chunk.substr(0, eq);
        value = chunk.substr(eq+1);
        kl::trim(key);
        kl::trim(value);

        req.setValue(key, value);

        pos = amp+1;
    }


    g_controller.invokeApi(path, method, req);
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
            "",                                       // name
            websockets_callback,                      // callback
            sizeof(struct websockets_message_data),   // per_session_data_size
            81920,                                    // rx_buffer_size
            0                                         // no_buffer_all_partial_tx
        },

        {
            NULL, NULL, 0           // terminator
        }
    };



    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.user = (void*)this;
    info.ka_time = 300;
    info.ka_probes = 6;
    info.ka_interval = 10;

	m_context = libwebsocket_create_context(&info);

	if (!m_context)
    {
		// libwebsocket init failed
		return false;
	}


    int try_count;

    while (true)
    {


        try_count = 0;
        while (true)
        {
            m_wsi = libwebsocket_client_connect(m_context, server.c_str(), port, ssl ? 2 : 0, "/", server.c_str(), "origin", NULL, -1);
            if (m_wsi)
                break;

            // connect failed, try again
            try_count++;
            printf("Could not connect, trying again. Try %d\n", try_count);
            ::Sleep(10000);
        }



        time_t t, last_ping = 0;
        int counter = 0;

        int res = 0;
        while (res >= 0)
        {
            res = libwebsocket_service(m_context, 10);

            ++counter;
            if (counter % 2000 == 0)
            {
                t = time(NULL);
                if (t - last_ping > 300) // 5 minutes)
                {
                    send("PING");
                    last_ping = t;
                }
            }
        }

        m_wsi = NULL;
    }


    return true;
}