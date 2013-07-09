/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2012-08-02
 *
 */

 
#include "xdclient.h"
#include "stream.h"
#include "../../kscript/kscript.h"
#include "../../kscript/json.h"
#include <kl/base64.h>


ClientStream::ClientStream(ClientDatabase* database, const std::wstring& handle)
{
    m_database = database;
    m_database->ref();
    m_handle = handle;
}

ClientStream::~ClientStream()
{
    m_database->unref();
}


bool ClientStream::read(void* pv,
                        unsigned long read_size,
                        unsigned long* read_count)
{
    wchar_t s_read_size[80];
    swprintf(s_read_size, 80, L"%lu", read_size);

    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    params.setParam(L"read_size", s_read_size);
    std::wstring sres = m_database->serverCall(L"", L"readstream", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;
    
    // create our own larger buffer 'tmpbuf' because the base64 functions cause an
    // overrun if one decodes to a buffer of the exact expected size

    std::string content = kl::tostring(response["data"].getString());
    size_t content_len = content.length();
    if (content_len == 0)
    {
        if (read_count)
            *read_count = 0;
        return false;
    }

    char* tmpbuf = new char[content.length()];
    kl::base64_decodestate state;
    kl::base64_init_decodestate(&state);
    int len = kl::base64_decode_block(content.c_str(), content.length(), tmpbuf, &state);
    tmpbuf[len] = 0;

    memcpy(pv, tmpbuf, len);
    delete[] tmpbuf;

    if (read_count)
        *read_count = (unsigned long)len;
    
    return true;
}

                      
bool ClientStream::write(const void* pv,
                         unsigned long write_size,
                         unsigned long* written_count)
{
    wchar_t s_write_size[80];
    swprintf(s_write_size, 80, L"%lu", write_size);

    char* base64_buf = new char[(write_size+2)*4];
    
    kl::base64_encodestate state;
    kl::base64_init_encodestate(&state);
    int l1 = kl::base64_encode_block((const char*)pv, write_size, base64_buf, &state);
    int l2 = kl::base64_encode_blockend(base64_buf+l1, &state);
    int base64_len = l1+l2;
    if (base64_len > 0 && base64_buf[base64_len-1] == '\n')
        base64_len--;
    base64_buf[base64_len] = 0;



    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    params.setParam(L"write_size", s_write_size);
    params.setParam(L"data", kl::towstring(base64_buf));
    delete[] base64_buf;

    std::wstring sres = m_database->serverCall(L"", L"writestream", &params, true);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    unsigned long written = (unsigned long)response["written"].getInteger();



    if (written_count)
        *written_count = written;
    
    return true;
}

