/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2009-05-22
 *
 */


#include "sdservlib.h"
#include "http.h"
#include "mongoose.h"
#include "controller.h"
#include "webserv.h"
#include <kl/portable.h>
#include <kl/url.h>
#include <kl/md5.h>
#include <kl/memory.h>
#include <kl/file.h>
#include <kl/system.h>




// member and function helpers for extracting 
// GET, and POST parameters
struct request_member
{
    std::wstring type;
    std::wstring key;
    std::wstring value;
};



inline void* quickMemmem(const void* haystack, size_t haystack_len, 
                         const void* needle, size_t needle_len) 
{ 
    const char* last = ((const char*)haystack + haystack_len - needle_len);
    const char* p;
 
    if (haystack_len < needle_len)
        return NULL;
 
    for (p = (const char*)haystack; p <= last; ++p)
    {
        if (0 == memcmp(p, needle, needle_len))
            return (void*)p;
    }

    return NULL;
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



// -- RequestInfo class implementation --

HttpRequestInfo::HttpRequestInfo(struct mg_connection* conn, const struct mg_request_info* ri)
{
    m_conn = conn;
    m_req = ri;
    m_content_type = "";    
    m_status_code = 200;
    m_header_sent = false;
    m_accept_compressed = false;
    m_content_length = -1;
    m_boundary = NULL;
    m_boundary_length = 0;
    m_request_content_length = 0;
    m_request_bytes_read = 0;
    m_post_hook = NULL;
    m_request_post_read_invoked = false;
    m_error = false;
    m_strip_path = 0;
}

HttpRequestInfo::~HttpRequestInfo()
{
    // remove temporary post files
    std::map<std::wstring, RequestFileInfo>::iterator f_it;
    for (f_it = m_files.begin(); f_it != m_files.end(); ++f_it)
        xf_remove(f_it->second.temp_filename);

    checkHeaderSent();

    delete[] m_boundary;
    delete m_post_hook;
}


void HttpRequestInfo::read()
{    
    size_t content_length = -1;
        
    int h;
    for (h = 0; h < m_req->num_headers; ++h)
    {
        if (0 == strncasecmp("Content-Type", m_req->http_headers[h].name, 12))
        {
            const char* bptr = strstr(m_req->http_headers[h].value, "boundary=");
            if (!bptr)
                break;
            bptr += 9;
            if (*bptr == '"') bptr++;
            size_t boundary_length = strlen(bptr);
            if (boundary_length > 0 && *(bptr+boundary_length-1) == '"')
                boundary_length--;
            if (boundary_length > 120)
                return;
            m_boundary = new char[128];
            m_boundary[0] = '-';
            m_boundary[1] = '-';
            memcpy(m_boundary+2, bptr, boundary_length);
            boundary_length+=2;
            m_boundary[boundary_length] = 0;
            m_boundary_length = boundary_length;
        }
         else if (0 == strncasecmp("Content-Length", m_req->http_headers[h].name, 14))
        {
            m_request_content_length = atoi(m_req->http_headers[h].value);
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


    if (m_req->query_string && *m_req->query_string)
    {
        // get method
        std::vector<request_member> parts;
        std::vector<request_member>::iterator it;
        extractPairs(kl::towstring(m_req->query_string), parts);
        for (it = parts.begin(); it != parts.end(); ++it)
            m_get[it->key] = it->value;
    }
}

void HttpRequestInfo::readPost()
{
    m_request_post_read_invoked = true;

    if (*m_req->request_method != 'P')
        return;

    if (m_boundary_length > 0)
    {
        readMultipart();
        return;
    }
     else
    {
        kl::membuf post_data_buf;

        #define BUFFERSIZE 4096

        char buf[BUFFERSIZE];
        int buf_len;
        int wanted_bytes;
        int bytes_left = m_request_content_length;

        while (true)
        {
            if (bytes_left != -1)
                wanted_bytes = std::min(BUFFERSIZE, bytes_left);
                    else
                wanted_bytes = BUFFERSIZE;

            if (wanted_bytes == 0)
                break;

            buf_len = mg_read(m_conn, buf, wanted_bytes);
            if (buf_len == -1)
                break;
            m_request_bytes_read += buf_len;
            
            if (bytes_left != -1)
                bytes_left -= buf_len;

            post_data_buf.append((unsigned char*)buf, buf_len);
            
            if (bytes_left == 0)
                break;
            if (buf_len != wanted_bytes)
                break;
        }

        char* post_data = (char*)post_data_buf.getData();
        size_t post_data_len = post_data_buf.getDataSize();
        

        // post method -- regular
        std::vector<request_member> parts;
        std::vector<request_member>::iterator it;
        std::string s_post_data(post_data, post_data_len);
        extractPairs(kl::towstring(s_post_data), parts);
        for (it = parts.begin(); it != parts.end(); ++it)
        {
            RequestPostInfo& info = m_post[it->key];

            info.value = it->value;
        }
    }
}

bool HttpRequestInfo::isHTTP_1_1() const
{
    return *(m_req->http_version + 2) == '1' ? true : false;
}



class PostValueMemory : public PostValueBase
{
public:

    virtual unsigned char* getData() { return m_membuf.getData(); }
    virtual size_t getDataSize() { return m_membuf.getDataSize(); }

    virtual void start() { }
    bool append(const unsigned char* buf, size_t len) { m_membuf.append(buf, len); return true; }
    virtual void cancelled() { m_membuf.setDataSize(0); }
    virtual void finish() { }

private:

    kl::membuf m_membuf;
};

class PostValueFile : public PostValueBase
{
public:

    PostValueFile()
    {
        m_f = (xf_file_t)0;
    }

    virtual void start()
    {
        m_temp_filename = xf_get_temp_filename(L"sdupl", L"tmp");
        m_f = xf_open(m_temp_filename, xfCreate, xfReadWrite, xfShareNone);
    }

    bool append(const unsigned char* buf, size_t len)
    {
        xf_write(m_f, buf, 1, len);
        return true;
    }

    virtual void cancelled()
    {
        if (m_f)
            xf_close(m_f);
        m_f = (xf_file_t)0;
        xf_remove(m_temp_filename);
        m_temp_filename = L"";
    }

    virtual void finish()
    {
        if (m_f)
            xf_close(m_f);
        m_f = (xf_file_t)0;
    }

public:

    xf_file_t m_f;
};



#define MULTIPART_BUFFER_SIZE 10000
#define MULTIPART_OVERLAP_SIZE 200


class MultipartHeaderInfo
{
public:
    std::wstring name;
    std::wstring filename;

    void parseHeaders(const char* headers, const char* endp)
    {
        const char* p = headers;
        name = L"";
        filename = L"";

        while (1)
        {
            if (p >= endp)
                break;
        
            if ((*p == '\r' && *(p+1) == '\n') || p == headers)
            {
                if (*p == '\r' && *(p+1) == '\n')
                    p += 2;
            
                if (0 == memcmp("Content-Disposition:", p, 20))
                {
                    extractValue(p + 20, endp, "name", name);
                    extractValue(p + 20, endp, "filename", filename);
                }
            }
        
            ++p;
        }
    }

private:

    void extractValue(const char* haystack, const char* endp, const char* needle, std::wstring& value)
    {
        char prev_char = 0;  // previous significant character
        int needle_len = strlen(needle);

        value = L"";

        const char* p = haystack;
        while (p < endp - needle_len - 1 && *p != '\r' && *p != '\n')
        {
            if ((prev_char == ':' || prev_char == ';') &&
                0 == memcmp(p, needle, needle_len) &&
                (isspace(*(p+needle_len)) || *(p+needle_len) == '='))
            {
                // found the key, now extract everything between the quotes
                p += needle_len;

                bool quote = false;

                while (p < endp && *p != '\r' && *p != '\n' && *p != ';')
                {
                    if (*p == '"')
                    {
                        if (quote)
                            break;
                        quote = true;
                    }
                     else
                    {
                        if (quote)
                            value += (wchar_t)*p;
                    }

                    p++;
                }

                return;
            }

            if (!isspace(*p))
                prev_char = *p;
            ++p;
        }
    }
};

void dump(char* buf, int len, const char* comment)
{
    buf[len] = 0;
    printf("READ (%s): %s\n\n\n\n", comment, buf);
}


void HttpRequestInfo::readMultipart()
{
    PostValueBase* curpart = NULL;
    MultipartHeaderInfo hdrinfo;

    char buf[MULTIPART_BUFFER_SIZE+1];
    int buf_len = 0;
    int r;
    const char* boundary_pos;
    const char* curpos;
    const char* data_begin;

    // inital read
    r = mg_read(m_conn, buf, MULTIPART_BUFFER_SIZE);
    if (r == -1)
    {
        this->setError(true);
        return;
    }

    //dump(buf, r, "initial");
    m_request_bytes_read += r;
    buf_len = r;
    curpos = buf;



    bool cancelled = false;
    bool custom = false;

    while (true)
    {
        // look for boundary
        boundary_pos = (const char*)quickMemmem(curpos, buf + buf_len - curpos, m_boundary, m_boundary_length);
        if (boundary_pos)
        {
            // a boundary was found; add remaining data to the post part
            if (curpart)
            {
                // -2: we don't want the \r\n before the boundary
                int data_len = boundary_pos - curpos - 2;
                if (!curpart->append((unsigned char*)curpos, data_len))
                {
                    cancelled = true;
                    break;
                }


                // finish out the part and commit it
                curpart->finish();

                if (!custom)
                {
                    if (curpart->getFilename().length() > 0)
                    {
                        // a multipart file post

                        RequestFileInfo& info = m_files[curpart->getName()];
                        info.temp_filename = curpart->getTempFilename();
                        info.post_filename = curpart->getFilename();
                    }
                     else
                    {
                        // normal multipart post value

                        std::string data((const char*)curpart->getData(), curpart->getDataSize());
                        RequestPostInfo& info = m_post[curpart->getName()];
                        info.value = kl::towstring(data);
                    }
                }

                delete curpart;
                curpart = NULL;
            }

            curpos = boundary_pos + m_boundary_length;
            if (*curpos == '-' && *(curpos+1) == '-')
            {
                // found a terminator -- we are done
                break;
            }

            if (*curpos != 0x0d || *(curpos+1) != 0x0a)
            {
                // malformed
                cancelled = true;
                return;
            }

            curpos += 2;

            data_begin = (const char*)quickMemmem(curpos, buf_len - (curpos - buf), "\r\n\r\n", 4);
            if (!data_begin)
            {
                // couldn't find data begin -- shift the buffer up and read in more data
                buf_len = buf_len - (curpos-buf); // new buffer size
                memmove(buf, curpos, buf_len);
                curpos = buf;

                r = mg_read(m_conn, buf + buf_len, MULTIPART_BUFFER_SIZE - buf_len);
                if (r == -1)
                {
                    // read error
                    cancelled = true;
                    break;
                }

                m_request_bytes_read += r;
                //dump(buf+buf_len, r, "needed more");
                buf_len += r;

                data_begin = (const char*)quickMemmem(curpos, buf_len - (curpos - buf), "\r\n\r\n", 4);
                if (!data_begin)
                {
                    // still couldn't find data begin -- malformed post
                    cancelled = true;
                    break;
                }
            }
            data_begin += 4;

            hdrinfo.parseHeaders(curpos, data_begin);

            curpos = data_begin;

            curpart = NULL;
            custom = false;
            if (m_post_hook)
            {
                curpart = m_post_hook->onPostValue(hdrinfo.name, hdrinfo.filename);
                if (curpart)
                    custom = true;
            }
            
            if (!custom)
            {
                if (hdrinfo.filename.length() > 0)
                    curpart = new PostValueFile;
                     else
                    curpart = new PostValueMemory;
            }

            curpart->setName(hdrinfo.name);
            curpart->setFilename(hdrinfo.filename);
            curpart->start();
        }
         else
        {
            // no boundary was found, add all data from the buffer, minus overlap
            int commit_len = buf_len - (curpos - buf);
            commit_len -= MULTIPART_OVERLAP_SIZE;
            if (commit_len > 0)
            {
                if (!curpart->append((unsigned char*)curpos, commit_len))
                {
                    cancelled = true;
                    break;
                }
                curpos += commit_len;
            }

            buf_len = buf_len - (curpos-buf); // new buffer size
            memmove(buf, curpos, buf_len);
            curpos = buf;


            // fill up buffer with more data

            r = mg_read(m_conn, buf + buf_len, MULTIPART_BUFFER_SIZE - buf_len);
            if (r == -1)
            {
                // read error
                cancelled = true;
                break;
            }

            m_request_bytes_read += r;

            //dump(buf+buf_len, r, "last");
            buf_len += r;

            if (buf_len == 0)
            {
                // unterminated part
                cancelled = true;
                break;
            }
        }
    }

    if (cancelled)
    {
        this->setError(true);
        if (curpart)
            curpart->cancelled();
    }

    delete curpart;
}


void HttpRequestInfo::checkReadRequestBody()
{
    if (*m_req->request_method != 'P')
        return;

    if (!m_request_post_read_invoked)      
        readPost();
}

std::wstring HttpRequestInfo::getValue(const std::wstring& key, const std::wstring& def)
{
    checkReadRequestBody();

    std::map<std::wstring, RequestPostInfo>::iterator p_it;
    p_it = m_post.find(key);
    if (p_it != m_post.end())
    {
        return p_it->second.value;
    }
    
    std::map<std::wstring, std::wstring>::iterator g_it;
    g_it = m_get.find(key);
    if (g_it != m_get.end())
        return g_it->second;
        
    return def;
}

bool HttpRequestInfo::getValueExists(const std::wstring& key)
{
    checkReadRequestBody();

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

std::wstring HttpRequestInfo::getGetValue(const std::wstring& key)
{
    std::map<std::wstring, std::wstring>::iterator g_it;
    g_it = m_get.find(key);
    if (g_it != m_get.end())
        return g_it->second;
    return L"";
}

std::wstring HttpRequestInfo::getPostValue(const std::wstring& key)
{
    checkReadRequestBody();

    std::map<std::wstring, RequestPostInfo>::iterator p_it;
    p_it = m_post.find(key);
    if (p_it != m_post.end())
    {
        return p_it->second.value;
    }
    
    return L"";
}


RequestFileInfo HttpRequestInfo::getPostFileInfo(const std::wstring& key)
{
    checkReadRequestBody();

    std::map<std::wstring, RequestFileInfo>::iterator it;

    it = m_files.find(key);

    if (it == m_files.end())
        return RequestFileInfo();

    return it->second;
}

bool HttpRequestInfo::acceptCompressed()
{
    return m_accept_compressed;
}

std::wstring HttpRequestInfo::getHost()
{
    const char* host = mg_get_header(this->m_conn, "Host");
    if (!host) return L"";
    return kl::towstring(host);
}

std::wstring HttpRequestInfo::getURI()
{
    if (m_uri.length() == 0)
    {
         m_uri = kl::towstring(m_req->uri);
         if (m_strip_path > 0)
         {
            const char* p = m_req->uri;
            const char* q = strchr(p, '?');
            for (int i = 0; i < m_strip_path; ++i)
            {
                p = strchr(p+1, '/');
                if (!p || (q && p > q))
                {
                    m_uri = L"/";
                    if (q)
                        m_uri += kl::towstring(q);
                    return m_uri;
                }
            }

            m_uri = kl::towstring(p);
         }
    }

    return m_uri;
}

std::wstring HttpRequestInfo::getQuery()
{
    std::wstring result;
    if (m_req->query_string)
        result = kl::towstring(m_req->query_string);
        
    return result;
}

void HttpRequestInfo::sendNotFoundError()
{
    setStatusCode(404);
    setContentType("text/html");
    write("{ \"success\": false, \"msg\": \"Not found\" }");
}


void HttpRequestInfo::setStatusCode(int code, const char* msg)
{
    m_status_code = code;
}

void HttpRequestInfo::setContentType(const char* content_type)
{
    m_content_type = content_type;
}

void HttpRequestInfo::setContentLength(int length)
{
    m_content_length = length;
}

void HttpRequestInfo::redirect(const char* location, int http_code)
{
    m_status_code = http_code;
    std::string loc = "Location: ";
    loc += location;
    addHeader(loc.c_str());
}

void HttpRequestInfo::addHeader(const char* header)
{
    m_headers.push_back(header);
}

void HttpRequestInfo::addCookie(ResponseCookie& cookie)
{
    m_response_cookies.push_back(cookie);
}

void HttpRequestInfo::addCookie(const std::string& name, const std::string& value, time_t expire_time)
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


void HttpRequestInfo::checkHeaderSent()
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
    
    
    if (isHTTP_1_1())
    {
        if (m_content_length == -1)
        {
            mg_force_close(m_conn);
            reply += "Connection: close\r\n\r\n";
        }
         else
        {
            reply += "Connection: Keep-Alive\r\n\r\n";
        }
    }
     else
    {
        mg_force_close(m_conn);
        reply += "\r\n";
    }

    mg_write(m_conn, reply.c_str(), reply.length());
}



size_t HttpRequestInfo::write(const void* ptr, size_t length)
{
    setContentLength((int)length);

    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, ptr, (int)length);
}

size_t HttpRequestInfo::write(const std::string& str)
{
    setContentLength((int)str.length());

    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, str.c_str(), (int)str.length());
}

size_t HttpRequestInfo::write(const std::wstring& str)
{
    setContentLength((int)str.length());

    return write(kl::tostring(str));
}


size_t HttpRequestInfo::writePiece(const void* ptr, size_t length)
{
    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, ptr, (int)length);
}

size_t HttpRequestInfo::writePiece(const std::string& str)
{
    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, str.c_str(), (int)str.length());
}

size_t HttpRequestInfo::writePiece(const std::wstring& str)
{
    return write(kl::tostring(str));
}


std::wstring HttpRequestInfo::getMethod() const
{
    return kl::towstring(m_req->request_method);
}

bool HttpRequestInfo::isMethodGet() const
{
    return (m_req->request_method && *m_req->request_method == 'G') ? true : false;
}

bool HttpRequestInfo::isMethodPost() const
{
    if (!m_req->request_method)
        return false;
    
    if (*m_req->request_method == 'P' && *(m_req->request_method+1) == 'O')
        return true;
    
    return false;
}







HttpServer::HttpServer(Sdserv* sdserv)
{
    m_sdserv = sdserv;
    m_options_arr_size = 0;
    m_strip_path = 0;
}

//static
int HttpServer::request_callback(struct mg_connection* conn)
{
    const struct mg_request_info* request_info = mg_get_request_info(conn);
    HttpServer* server = (HttpServer*)request_info->user_data;

    /*
    std::string reply;
    reply += "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nHI";
    mg_write(conn, reply.c_str(), reply.length());
    return 1;
    */

    server->m_sdserv->updateLastAccessTimestamp();

    HttpRequestInfo req(conn, request_info);
    req.m_strip_path = server->m_strip_path;
    req.read();

    if (!server->m_sdserv->m_controller->onRequest(req))
    {
        req.setStatusCode(404);
        req.setContentType("text/html");
        req.write("{ \"success\": false, \"msg\": \"Not found\" }");
    }

    return 1;
}




//static
bool HttpServer::run()
{



    // handle 'ports' and 'ssl_ports'
    std::vector<std::wstring> arr;
    std::vector<std::wstring>::iterator it;
    std::wstring tmps;

    kl::parseDelimitedList(m_sdserv->getOption(L"http.port"), arr, ',');
    for (it = arr.begin(); it != arr.end(); ++it)
    {
        if (it->empty())
            continue;
        if (tmps.length() > 0)
            tmps += L",";
        tmps += *it;
    }

    kl::parseDelimitedList(m_sdserv->getOption(L"http.ssl_port"), arr, ',');
    for (it = arr.begin(); it != arr.end(); ++it)
    {
        if (it->empty())
            continue;
        if (tmps.length() > 0)
            tmps += L",";
        tmps += *it;
        if (it->length() > 0)
            tmps += L"s";
    }

    if (tmps.length() == 0 || tmps.length() >= sizeof(m_ports)-1)
    {
        printf("Please specify at least one port or ssl_port.\n");
        return false;
    }
    strcpy(m_ports, kl::tostring(tmps).c_str());
    
    std::wstring strip_path = m_sdserv->getOption(L"http.strip_path");
    if (strip_path.length() == 0)
        m_strip_path = 0;
         else
        m_strip_path = kl::wtoi(strip_path);

    // set listening ports
    m_options[m_options_arr_size++] = "listening_ports";
    m_options[m_options_arr_size++] = m_ports;
    
    // enable keep alive by default
    m_options[m_options_arr_size++] = "enable_keep_alive";
    m_options[m_options_arr_size++] = "yes";

    // enable keep alive by default
    m_options[m_options_arr_size++] = "num_threads";
    m_options[m_options_arr_size++] = "30";
    
    
    
    tmps = m_sdserv->getOption(L"http.ssl_cert");
    if (tmps.length() > 0 && tmps.length() < sizeof(m_ssl_cert_file)-1)
    {
        if (!xf_get_file_exist(tmps))
        {
            printf("Certificate %ls does not exist.\n", tmps.c_str());
            return false;
        }

        strcpy(m_ssl_cert_file, kl::tostring(tmps).c_str());

        m_options[m_options_arr_size++] = "ssl_certificate";
        m_options[m_options_arr_size++] = m_ssl_cert_file;
    }


    // terminator
    m_options[m_options_arr_size++] = NULL;
    








    struct mg_context* ctx;
    struct mg_callbacks callbacks;

    if (m_options[0] == 0)
    {
        m_sdserv->signalServerNotReady();
        return false;
    }
    
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.begin_request = request_callback;

    ctx = mg_start(&callbacks, (void*)this, m_options);
    if (!ctx)
    {
        m_sdserv->signalServerNotReady();
        return false;
    }
    

    // get list of ports we are listening on
    int idx = 0;
    int port = 0;
    int ssl = 0;
        
    while (true)
    {
        if (!mg_get_listener_info(ctx, idx++, &port, &ssl))
            break;

        HttpListeningPort lp;
        lp.port = port;
        lp.ssl = ssl ? true : false;

        m_listening_ports.push_back(lp);
    }



    m_sdserv->signalServerReady();


    //wsrv_start();


    int counter = 0;

    while (1)
    {
        kl::millisleep(1000);

        if (m_sdserv->m_idle_quit > 0)
        {
            counter++;
            if ((counter % 10) == 0)
            {
                time_t t = time(NULL);
                bool quit = false;

                m_sdserv->m_last_access_mutex.lock();
                if (t - m_sdserv->m_last_access > m_sdserv->m_idle_quit)
                    quit = true;
                m_sdserv->m_last_access_mutex.unlock();

                if (quit)
                    break;
            }
        }
    }


    // causing a hang right now.  Maybe has something to do with keep alive?
    //mg_stop(ctx);

    return true;
}
