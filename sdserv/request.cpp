/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2009-05-22
 *
 */


#include "sdserv.h"
#include "request.h"
#include "mongoose.h"
#include <kl/portable.h>
#include <kl/url.h>
#include <kl/md5.h>
#include <kl/memory.h>



// member and function helpers for extracting 
// GET, and POST parameters
struct request_member
{
    std::wstring type;
    std::wstring key;
    std::wstring value;
};


static std::string compress_str(const std::string& str)
{return "";/*
    unsigned long input_size = str.size();
    const char* input = str.c_str();

    unsigned long output_size = (unsigned long)(input_size + input_size*0.1 + 12);   // per zlib comment, must be at least 0.1% input_size plus 12 bytes
    char* output = new char[output_size];

    // compress the string
    int result = compress((unsigned char*)output, &output_size, (const unsigned char*)input, input_size);
    if (result != Z_OK)
        return "";

    return std::string(output, output_size);*/
}

static std::string uncompress_str(const std::string& str, unsigned long len)
{return "";/*
    unsigned long input_size = str.size();
    const char* input = str.c_str();

    unsigned long output_size = len + len*0.1 + 100;
    const char* output = new char[output_size];

    // compress the string
    int result = uncompress((unsigned char*)output, &output_size, (const unsigned char*)input, input_size);
    if (result != Z_OK)
        return "";

    return std::string(output, output_size);*/
}


static void extractPairs(const std::wstring& str,
                         std::vector<request_member>& info)
{
    std::wstring::const_iterator it, it_end;
    it_end = str.end();

    bool key = true;
    request_member param;
    for (it = str.begin(); it != it_end; ++it)
    {
        if (*it == L'=')
        {
            key = false;
            continue;
        }
        
        if (*it == L'&')
        {
            key = true;
            info.push_back(param);

            param.key = L"";
            param.value = L"";

            continue;
        }
    
        if (key)
            param.key += (*it);
             else
            param.value += (*it);    
    }
    
    // if there's anything left over, save it
    if (param.key.length() + param.value.length() > 0)
        info.push_back(param);
        
    // url decode the parameters
    std::vector<request_member>::iterator itm, itm_end;
    itm_end = info.end();
    
    for (itm = info.begin(); itm != itm_end; ++itm)
    {
        itm->key = kl::url_decodeURI(itm->key);
        itm->value = kl::url_decodeURI(itm->value);
    }
}

static std::wstring getQueryId(RequestInfo& ri)
{
    std::wstring result;
    
    // add the uri
    std::wstring uri = ri.getURI();
    result.append(uri);
    
    // add on the get parameters without the limit parameters
    std::map< std::wstring, std::wstring > get_values = ri.getGetValues();
    std::map< std::wstring, std::wstring >::iterator it, it_end;
    it_end = get_values.end();
    
    bool first = true;
    for (it = get_values.begin(); it != it_end; ++it)
    {
        if (first)
        {
            result.append(L"?");
            first = false;
        }
        else
        {
            result.append(L"&");
        }
        
        if (it->first == L"start")
            continue;
            
        if (it->first == L"limit")
            continue;
        
        result.append(it->first);
        result.append(L"=");
        result.append(it->second);
    }
    
    return kl::md5str(result);
}



RequestPostInfo::RequestPostInfo()
{
}

bool RequestPostInfo::saveToFile(const std::wstring& path)
{
    return false;
}


// -- RequestInfo class implementation --

RequestInfo::RequestInfo(struct mg_connection* conn, const struct mg_request_info* ri)
{
    m_conn = conn;
    m_req = ri;
    m_content_type = "";    
    m_status_code = 200;
    m_header_sent = false;
    m_accept_compressed = false;
    m_content_length = -1;
    parse();
}

RequestInfo::~RequestInfo()
{
    checkHeaderSent();
}


void RequestInfo::parse()
{    
    const char* boundary = NULL;
    size_t boundary_length = 0;
    size_t content_length = 0;
        
    int h;
    for (h = 0; h < m_req->num_headers; ++h)
    {
        if (0 == strncasecmp("Content-Type", m_req->http_headers[h].name, 12))
        {
            boundary = strstr(m_req->http_headers[h].value, "boundary=");
            if (!boundary)
                break;
            boundary += 9;
            if (*boundary == '"') boundary++;
            boundary_length = strlen(boundary);
            if (boundary_length > 0 && *(boundary+boundary_length-1) == '"')
                boundary_length--;
        }
         else if (0 == strncasecmp("Content-Length", m_req->http_headers[h].name, 14))
        {
            content_length = atoi(m_req->http_headers[h].value);
        }
         else if (0 == strncasecmp("Cookie", m_req->http_headers[h].name, 6))
        {
            const char* p = m_req->http_headers[h].value;
            const char* eq = strchr(p, '=');
            if (eq)
            {
                std::wstring full = kl::towstring(p);
                std::wstring key = full.substr(0, eq-p);
                std::wstring value = full.substr(eq-p+1);
                m_cookies[key] = value;
            }
        }
         else if (0 == strncasecmp("Accept-Encoding", m_req->http_headers[h].name, 16))
        {
            const char* deflate = strstr(m_req->http_headers[h].value, "deflate");
            if (deflate)
                m_accept_compressed = true;
        }        
    }


    // for now, disable compression (remove this line later, if we want compression support)
    m_accept_compressed = false;


    
    if (*m_req->request_method == 'G')
    {
        if (m_req->query_string)
        {
            // get method
            std::vector<request_member> parts;
            std::vector<request_member>::iterator it;
            extractPairs(kl::towstring(m_req->query_string), parts);
            for (it = parts.begin(); it != parts.end(); ++it)
                m_get[it->key] = it->value;
        }
    }
     else
    {
        char buf[4096];
        int buf_len;

        while (true)
        {
            buf_len = mg_read(m_conn, buf, 4096);
            m_post_data_buf.append((unsigned char*)buf, buf_len);
            
            if (buf_len != 4096)
                break;
        }

        char* post_data = (char*)m_post_data_buf.getData();
        size_t post_data_len = m_post_data_buf.getDataSize();
        
        if (boundary)
        {
            const char* p = post_data;
            while (parsePart(p, boundary, boundary_length, post_data + post_data_len, &p));
        }
         else
        {
            // post method -- regular
            std::vector<request_member> parts;
            std::vector<request_member>::iterator it;
            std::string post_data(post_data, post_data_len);
            extractPairs(kl::towstring(post_data), parts);
            for (it = parts.begin(); it != parts.end(); ++it)
            {
                RequestPostInfo& info = m_post[it->key];
                info.data = NULL;
                info.str = it->value;
                info.length = info.str.length();
            }
        }

    }
}


static const char* findBoundary(const char* p,
                                const char* boundary,
                                size_t boundary_length,
                                const char* endp)
{
    const char* start = p;
    
    while (p < endp)
    {
        p = (const char*)memchr(p, '-', endp-p);
        if (!p)
            return NULL;
        if (p+1 >= endp)
            return NULL;
        if (*(p+1) != '-')
        {
            p++;
            continue;
        }
        if (p-3 > start && (*(p-1) != '\n' || *(p-2) != '\r'))
        {
            p++;
            continue;
        }
        p += 2;
        if (p+boundary_length >= endp)
        {
            p++;
            continue;
        }
        if (memcmp(p, boundary, boundary_length) == 0)
        {
            if (p-4 < start)
                return start;
                 else
                return p-4;
        }
            
        ++p;
    }
    
    return NULL;
}


static const char* findHeaderEnd(const char* p, const char* endp)
{
    const char* e = (const char*)memchr(p, '\r', p-endp);
    if (!e)
        e = (const char*)memchr(p, '\n', p-endp);
    return e;
}

static const char* findString(const char* str, const char* find, size_t find_len, const char* endp)
{
    while (str+find_len < endp)
    {
        if (memcmp(str, find, find_len) == 0)
            return str;
        str++;
    }
    return NULL;
}

inline const char* findChar(const char* str, const char find, const char* endp)
{
    return (const char*)memchr(str, find, endp-str);
}

bool RequestInfo::parsePart(const char* p,
                            const char* boundary,
                            size_t boundary_length,
                            const char* endp,
                            const char** next)
{
    p = findBoundary(p, boundary, boundary_length, endp);
    if (!p)
    {
        // no start boundary found
        return false;
    }
    
    const char* end = findBoundary(p+boundary_length, boundary, boundary_length, endp);
    if (!end)
    {
        // no end boundary found
        return false;
    }
    
    
    p += boundary_length;
    
    std::string key;
    bool is_file = false;
    
    while (1)
    {
        if (p >= endp)
            return false;
        
        if (*p == '\r' && *(p+1) == '\n')
        {
            p += 2;
            
            if (*p == '\r' && *(p+1) == '\n')
            {
                // terminator -- content starts hereafter
                p += 2;
                break;
            }
            
            if (0 == memcmp("Content-Disposition:", p, 20))
            {
                const char* end = findHeaderEnd(p, endp);
                if (!end)
                    return false;
                
                const char* name = findString(p, "name=", 5, end);
                if (name)
                {
                    name+=5;
                    if (*name == '"')
                        name++;
                    const char* close_quote = findChar(name, '"', end);
                    if (!close_quote)
                        close_quote = end;
                    key = std::string(name, close_quote-name);
                    
                    if (findString(p, "filename=", 9, end))
                        is_file = true;
                }
            }
        }
        
        ++p;
    }
    
    
    if (key.length() > 0)
    {
        RequestPostInfo pi;
        pi.data = p;
        pi.length = end - pi.data;
        
        if (is_file)
            m_files[kl::towstring(key)] = pi;
             else
            m_post[kl::towstring(key)] = pi;
    }
    
    
    *next = end;
    return true;
}

std::wstring RequestInfo::getValue(const std::wstring& key)
{
    std::map<std::wstring, RequestPostInfo>::iterator p_it;
    p_it = m_post.find(key);
    if (p_it != m_post.end())
    {
        if (p_it->second.data)
        {
            std::string str(p_it->second.data, p_it->second.data + p_it->second.length);
            return kl::towstring(str);
        }
         else
        {
            return p_it->second.str;
        }
    }
    
    std::map<std::wstring, std::wstring>::iterator g_it;
    g_it = m_get.find(key);
    if (g_it != m_get.end())
        return g_it->second;
        
    return L"";
}

bool RequestInfo::getValueExists(const std::wstring& key) const
{
    std::map<std::wstring, RequestPostInfo>::const_iterator p_it;
    p_it = m_post.find(key);
    if (p_it != m_post.end())
        return true;
    
    std::map<std::wstring, std::wstring>::const_iterator g_it;
    g_it = m_get.find(key);
    if (g_it != m_get.end())
        return true;

    return false;
}

std::wstring RequestInfo::getGetValue(const std::wstring& key)
{
    std::map<std::wstring, std::wstring>::iterator g_it;
    g_it = m_get.find(key);
    if (g_it != m_get.end())
        return g_it->second;
    return L"";
}

std::wstring RequestInfo::getPostValue(const std::wstring& key)
{
    std::map<std::wstring, RequestPostInfo>::iterator p_it;
    p_it = m_post.find(key);
    if (p_it != m_post.end())
    {
        if (p_it->second.data)
        {
            std::string str(p_it->second.data, p_it->second.data + p_it->second.length);
            return kl::towstring(str);
        }
         else
        {
            return p_it->second.str;
        }
    }
    
    return L"";
}

bool RequestInfo::acceptCompressed()
{
    return m_accept_compressed;
}

std::wstring RequestInfo::getURI()
{
    return kl::towstring(m_req->uri);
}

std::wstring RequestInfo::getQuery()
{
    std::wstring result;
    if (m_req->query_string)
        result = kl::towstring(m_req->query_string);
        
    return result;
}

void RequestInfo::setStatusCode(int code, const char* msg)
{
    m_status_code = code;
}

void RequestInfo::setContentType(const char* content_type)
{
    m_content_type = content_type;
}

void RequestInfo::setContentLength(int length)
{
    m_content_length = length;
}

void RequestInfo::redirect(const char* location, int http_code)
{
    m_status_code = http_code;
    std::string loc = "Location: ";
    loc += location;
    addHeader(loc.c_str());
}

void RequestInfo::addHeader(const char* header)
{
    m_headers.push_back(header);
}

void RequestInfo::addCookie(ResponseCookie& cookie)
{
    m_response_cookies.push_back(cookie);
}

void RequestInfo::addCookie(const std::string& name, const std::string& value, time_t expire_time)
{
    ResponseCookie cookie;
    cookie.name = name;
    cookie.value = value;
    cookie.expire_time = expire_time;

    addCookie(cookie);
}


static const char* getHttpCodeString(int code)
{
    switch (code)
    {
        case 100: return "100 Continue";
        case 101: return "101 Switching Protocols";
        
        case 200: return "200 OK";
        case 201: return "201 Created";
        case 202: return "202 Accepted";
        case 203: return "203 Non-Authoritative Information";
        case 204: return "204 No Content";
        case 205: return "205 Reset Content";
        case 206: return "206 Partial Content";
        
        case 300: return "300 Multiple Choices";
        case 301: return "301 Moved Permanently";
        case 302: return "302 Found";
        case 303: return "303 See Other";
        case 304: return "304 Not Modified";
        case 305: return "305 Use Proxy";
        case 306: return "306 (Unused)";
        case 307: return "307 Temporary Redirect";
        
        case 400: return "400 Bad Request";
        case 401: return "401 Unauthorized";
        case 402: return "402 Payment Required";
        case 403: return "403 Forbidden";
        case 404: return "404 Not Found";
        case 405: return "405 Method Not Allowed";
        case 406: return "406 Not Acceptable";
        case 407: return "407 Proxy Authentication Required";
        case 408: return "408 Request Timeout";
        case 409: return "409 Conflict";
        case 410: return "410 Gone";
        case 411: return "411 Length Required";
        case 412: return "412 Precondition Failed";
        case 413: return "413 Request Entity Too Large";
        case 414: return "414 Request-URI Too Long";
        case 415: return "415 Unsupported Media Type";
        case 416: return "416 Requested Range Not Satisfiable";
        case 417: return "417 Expectation Failed";
        
        case 500: return "500 Internal Server Error";
        case 501: return "501 Not Implemented";
        case 502: return "502 Bad Gateway";
        case 503: return "503 Service Unavailable";
        case 504: return "504 Gateway Timeout";
        case 505: return "505 HTTP Version Not Supported";
    }
    
    return "";
}


void RequestInfo::checkHeaderSent()
{
    if (m_header_sent)
        return;
    m_header_sent = true;
    

    std::string reply;
  
    reply += "HTTP/1.1 ";
    reply += getHttpCodeString(m_status_code);
    reply += "\r\n";
    
    if (m_content_type.length() > 0)
    {
        reply += "Content-Type: ";
        reply += m_content_type;
        reply += "\r\n";
    }
     else
    {
        reply += "Content-Type: text/html\r\n";
    }
    
    if (m_content_length != -1)
    {
        char buf[80];
        sprintf(buf, "Content-Length: %d\r\n", m_content_length);
        reply += buf;
    }
    
    std::vector<std::string>::iterator it;
    for (it = m_headers.begin(); it != m_headers.end(); ++it)
    {
        reply += *it;
        reply += "\r\n";
    }
    
    std::vector<ResponseCookie>::iterator c_it;
    for (c_it = m_response_cookies.begin(); c_it != m_response_cookies.end(); ++c_it)
    {
        std::string cookie = "Set-Cookie: ";
        cookie += c_it->name;
        cookie += "=";
        cookie += c_it->value;
        
        reply += cookie;
        reply += "\r\n";
    }    
    
    
    
    reply += "Connection: close\r\n\r\n";
    //reply += "\r\n\r\n";
    
    mg_write(m_conn, reply.c_str(), reply.length());
}



size_t RequestInfo::write(const void* ptr, size_t length)
{
    setContentLength((int)length);

    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, ptr, (int)length);
}

size_t RequestInfo::write(const std::string& str)
{
    setContentLength((int)str.length());

    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, str.c_str(), (int)str.length());
}

size_t RequestInfo::write(const std::wstring& str)
{
    setContentLength((int)str.length());

    return write(kl::tostring(str));
}


size_t RequestInfo::writePiece(const void* ptr, size_t length)
{
    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, ptr, (int)length);
}

size_t RequestInfo::writePiece(const std::string& str)
{
    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, str.c_str(), (int)str.length());
}

size_t RequestInfo::writePiece(const std::wstring& str)
{
    return write(kl::tostring(str));
}


std::wstring RequestInfo::getMethod() const
{
    return kl::towstring(m_req->request_method);
}

bool RequestInfo::isMethodGet() const
{
    return (m_req->request_method && *m_req->request_method == 'G') ? true : false;
}

bool RequestInfo::isMethodPost() const
{
    if (!m_req->request_method)
        return false;
    
    if (*m_req->request_method == 'P' && *(m_req->request_method+1) == 'O')
        return true;
    
    return false;
}

