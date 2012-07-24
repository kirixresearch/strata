/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-05-22
 *
 */


#include "appmain.h"
#include "mongoose.h"
#include "webserver.h"
#include "jsonconfig.h"
#include "reportdoc.h"
#include "appcontroller.h"
#include "scriptmemory.h"
#include "zlib.h"

// script related
#include "scripthostapp.h"
#include <kl/utf8.h>
#include <kl/url.h>
#include <kl/md5.h>

// report related
#include "reportlayout.h"

// json related
#include "../../kscript/json.h"


// member and function helpers for extracting 
// GET, and POST parameters
struct request_member
{
    std::wstring type;
    std::wstring key;
    std::wstring value;
};


static std::string compress_str(const std::string& str)
{
    unsigned long input_size = str.size();
    const char* input = str.c_str();

    unsigned long output_size = input_size + input_size*0.1 + 12;   // per zlib comment, must be at least 0.1% input_size plus 12 bytes
    char* output = new char[output_size];

    // compress the string
    int result = compress((unsigned char*)output, &output_size, (const unsigned char*)input, input_size);
    if (result != Z_OK)
        return "";

    return std::string(output, output_size);
}

static std::string uncompress_str(const std::string& str, unsigned long len)
{
    unsigned long input_size = str.size();
    const char* input = str.c_str();

    unsigned long output_size = len + len*0.1 + 100;
    const char* output = new char[output_size];

    // compress the string
    int result = uncompress((unsigned char*)output, &output_size, (const unsigned char*)input, input_size);
    if (result != Z_OK)
        return "";

    return std::string(output, output_size);
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

// helper classes for converting tango structures into JSON; adapted 
// from xdhttp\jsonutil.h to avoid including that file; note: some 
// changes (e.g. namespace, and removing JsonStruct::fromJsonNode()) 
// were necessary for compilation, so not an exact copy
class JsonFileType
{
public:

    JsonFileType()
    {
    }

    ~JsonFileType()
    {
    }
        
    static std::wstring toString(int type)
    {
        const wchar_t* result = L"";
        switch (type)
        {
            case tango::filetypeFolder:
                result = L"folder";
                break;
                
            case tango::filetypeNode:
                result = L"node";
                break;
                
            case tango::filetypeSet:
                result = L"set";
                break;
                
            case tango::filetypeStream:
                result = L"stream";
                break;
        }
        
        return kl::towstring(result);
    }

    static int fromString(const std::wstring& type)
    {
        if (type == L"folder")
            return tango::filetypeFolder;
            
        if (type == L"set")
            return tango::filetypeSet;        
            
        if (type == L"node")
            return tango::filetypeNode;
            
        if (type == L"stream")
            return tango::filetypeStream;

        // TODO: what do we do for an invalid file type?        
        return -1;
    }

private:

};


class JsonColumnType
{
public:

    JsonColumnType()
    {
    }

    ~JsonColumnType()
    {
    }

    static std::wstring toString(int type)
    {
        const wchar_t* result = L"";
        switch (type)
        {
            default:
            case tango::typeUndefined:
                result = L"undefined";
                break;
        
            case tango::typeInvalid:
                result = L"invalid";
                break;
        
            case tango::typeCharacter:
                result = L"character";
                break;
                
            case tango::typeWideCharacter:
                result = L"widecharacter";
                break;
                
            case tango::typeNumeric:
                result = L"numeric";
                break;

            case tango::typeDouble:
                result = L"double";
                break;
                
            case tango::typeInteger:
                result = L"integer";
                break;

            case tango::typeDate:
                result = L"date";
                break;
                
            case tango::typeDateTime:
                result = L"datetime";
                break;

            case tango::typeBoolean:
                result = L"boolean";
                break;
                
            case tango::typeBinary:
                result = L"binary";
                break;
        }
        
        return kl::towstring(result);
    }

    static int fromString(const std::wstring& type)
    {
        if (type == L"undefined")
            return tango::typeUndefined;
            
        if (type == L"invalid")
            return tango::typeInvalid;
            
        if (type == L"character")
            return tango::typeCharacter;
            
        if (type == L"widecharacter")
            return tango::typeWideCharacter;

        if (type == L"numeric")
            return tango::typeNumeric;
            
        if (type == L"double")
            return tango::typeDouble;
            
        if (type == L"integer")
            return tango::typeInteger;
            
        if (type == L"date")
            return tango::typeDate;
            
        if (type == L"datetime")
            return tango::typeDateTime;
            
        if (type == L"boolean")
            return tango::typeBoolean;
            
        if (type == L"binary")
            return tango::typeBinary;
            
        return tango::typeInvalid;
    }

private:

};


class JsonStructure
{
public:

    JsonStructure()
    {
    }

    ~JsonStructure()
    {
    }

    static kscript::JsonNode toJsonNode(tango::ISetPtr set)
    {
        kscript::JsonNode root;

        if (set.isNull())
            return root;

        tango::IStructurePtr structure = set->getStructure();
        if (structure.isNull())
            return root;

        // set the total number of items
        int idx;
        int count = structure->getColumnCount();
        root["total_count"].setInteger(count);

        // set the number of rows
        root["row_count"].setInteger((int)set->getRowCount());

        // set the fast row count flag
        bool fast_row_count = false;
        if (set->getSetFlags() & tango::sfFastRowCount)
            fast_row_count = true;

        root["fast_row_count"].setBoolean(fast_row_count);

        // set the items
        kscript::JsonNode items = root["items"];
        for (idx = 0; idx < count; ++idx)
        {
            kscript::JsonNode item = items.appendElement();
            
            tango::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);
            item["name"] = info->getName();
            item["type"] = JsonColumnType::toString(info->getType());
            item["width"].setInteger(info->getWidth());
            item["scale"].setInteger(info->getScale());        
        }

        return root;
    }
    
private:

};



// server session object for server-side iterators
class ServerSessionQuery : public ServerSessionObject
{
public:

    ServerSessionQuery(const std::wstring& query)
    {
        setType(L"server.query");
        m_query = query;
        m_total_count = 0;
        m_current_row = 0;
    }
    
    virtual ~ServerSessionQuery()
    {
        reset();
    }

public:

    bool execute()
    {
        tango::IDatabasePtr db = g_app->getDatabase();
        if (db.isNull())
            return false;

        // if we've previously execute the query, clear the
        // iterator handles and reset the iterator
        reset();

        xcm::IObjectPtr result;
        if (!db->execute(towstr(m_query), tango::sqlPassThrough, result, NULL))
            return false;

        tango::IIteratorPtr iter = result;
        if (iter.isNull())
            return false;

        // if the iterator is forward-only, try to turn on
        // tango's backward scroll row cache
        iter->setIteratorFlags(tango::ifReverseRowCache, tango::ifReverseRowCache);

        // get the set and the row count
        tango::ISetPtr set = iter->getSet();
        m_total_count = set->getRowCount();
        m_iter = iter;

        // populate the column handles
        populateHandles();

        return true;
    }

    bool getResult(kscript::JsonNode& node, int start_row = 0, int offset = 0)
    {
        if (m_iter.isNull())
            return false;

        node["total_count"].setInteger(m_total_count);
        
        if (start_row > 0)
            node["start"].setInteger(start_row);
        if (offset > 0)
            node["limit"].setInteger(offset);

        kscript::JsonNode items = node["items"];

        // if we haven't returned any rows yet or we're past
        // the eof marker, set the cursor to the first row
        if (m_current_row <= 0 || m_iter->eof())
        {
            m_iter->goFirst();
            m_current_row = 1;
        }

        // if the start row isn't specified or is set to an
        // invalid value, set the cursor to the first row;
        // if the start row is specified, skip to that row
        if (start_row <= 1)
        {
            m_iter->goFirst();
            m_current_row = 1;
        }
        else
        {
            int offset = start_row - m_current_row;
            m_iter->skip(offset);
            m_current_row += offset;
        }

        int count = 0;
        while (!m_iter->eof())
        {
            kscript::JsonNode item = items.appendElement();
            
            std::vector<tango::objhandle_t>::iterator it, it_end;
            it_end = m_handles.end();
            
            for (it = m_handles.begin(); it != it_end; ++it)
            {
                writeItemNode(*it, item);
            }

            m_iter->skip(1);
            m_current_row++;
            
            // if the offset is specified and we've output the
            // number of specified rows, we're done
            count++;            
            if (offset > 0 && count >= offset)
                break;
        }
        
        // set the limit to the actual number of rows returned
        if (offset > 0)
        {
            kscript::JsonNode limit = node["limit"];
            limit.setInteger(count);
        }
        
        return true;
    }

private:

    void reset()
    {
        m_total_count = 0;    
    
        if (m_iter.isNull())
            return;

        if (m_handles.empty())
            return;
            
        std::vector<tango::objhandle_t>::iterator it, it_end;
        it_end = m_handles.end();
        
        for (it = m_handles.begin(); it != it_end; ++it)
        {
            m_iter->releaseHandle(*it);
        }

        m_handles.clear();
        m_iter = xcm::null;
    }

    void populateHandles()
    {
        if (m_iter.isNull())
            return;
    
        tango::IStructurePtr structure = m_iter->getStructure();
        if (structure.isNull())
            return;

        int idx, count = structure->getColumnCount();
        for (idx = 0; idx < count; ++idx)
        {
            tango::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);
            std::wstring name = info->getName();
            m_handles.push_back(m_iter->getHandle(name));
        }
    }

    bool writeItemNode(tango::objhandle_t handle, kscript::JsonNode& item)
    {
        tango::IColumnInfoPtr info = m_iter->getInfo(handle);
        int type = info->getType();
        std::wstring name = info->getName();

        switch (type)
        {
            case tango::typeUndefined:
            case tango::typeInvalid:
            case tango::typeBinary:
                return false;
        
            case tango::typeCharacter:
                item[name] = kl::towstring(m_iter->getString(handle));
                return true;

            case tango::typeWideCharacter:
                item[name] = m_iter->getWideString(handle);
                return true;

            case tango::typeNumeric:
            case tango::typeDouble:
                {
                    double d = m_iter->getDouble(handle);
                    char buf[40];
                    sprintf(buf, "%.*f", info->getScale(), d);
                    item[name] = kl::towstring(buf);
                }
                return true;

            case tango::typeInteger:
                {
                    int i = m_iter->getInteger(handle);
                    char buf[40];
                    sprintf(buf, "%d", i);
                    item[name] = kl::towstring(buf);
                }
                return true;

            case tango::typeDate:
                {
                    tango::datetime_t d;
                    d = m_iter->getDateTime(handle);

                    if (d == 0)
                    {
                        item[name] = kl::towstring(L"0000-00-00");
                    }
                    else
                    {
                        char buf[40];
                        tango::DateTime dt = d;

                        sprintf(buf, "%04d-%02d-%02d", dt.getYear(),
                                                       dt.getMonth(),
                                                       dt.getDay());
                        item[name] = kl::towstring(buf);
                    }
                }
                return true;

            case tango::typeDateTime:                    
                {
                    tango::datetime_t d;
                    d = m_iter->getDateTime(handle);

                    if (d == 0)
                    {
                        item[name] = kl::towstring(L"0000-00-00 00:00:00");
                    }
                    else
                    {
                        char buf[40];                    
                        tango::DateTime dt = d;

                        sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", dt.getYear(),
                                                                      dt.getMonth(),
                                                                      dt.getDay(),
                                                                      dt.getHour(),
                                                                      dt.getMinute(),
                                                                      dt.getSecond());
                        item[name] = kl::towstring(buf);
                    }
                }
                return true;

            case tango::typeBoolean:
                item[name].setBoolean(m_iter->getBoolean(handle));
                return true;
        }

        return false;
    }

private:

    std::vector<tango::objhandle_t> m_handles;

    std::wstring m_query;
    tango::IIteratorPtr m_iter;
    int m_total_count;
    int m_current_row;
};


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


    
    if (*m_req->request_method == 'G')
    {
        if (m_req->query_string)
        {
            // get method
            std::vector<request_member> parts;
            std::vector<request_member>::iterator it;
            extractPairs(towstr(m_req->query_string), parts);
            for (it = parts.begin(); it != parts.end(); ++it)
                m_get[it->key] = it->value;
        }
    }
     else
    {
        if (m_req->post_data)
        {
            if (boundary)
            {
                const char* p = m_req->post_data;
                while (parsePart(p, boundary, boundary_length, m_req->post_data + m_req->post_data_len, &p));
            }
             else
            {
                // post method -- regular
                std::vector<request_member> parts;
                std::vector<request_member>::iterator it;
                std::string post_data(m_req->post_data, m_req->post_data_len);
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
    return towstr(m_req->uri);
}

std::wstring RequestInfo::getQuery()
{
    std::wstring result;
    if (m_req->query_string)
        result = towstr(m_req->query_string);
        
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
    
    mg_write(m_conn, reply.c_str(), reply.length());
}



size_t RequestInfo::write(const void* ptr, size_t length)
{
    checkHeaderSent();
    
    return (size_t)mg_write(m_conn, ptr, (int)length);
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









class WebQueryString : public ScriptHostBase
{
    typedef kscript::ValueObject BaseClass;
    
    BEGIN_KSCRIPT_CLASS("WebQueryString", WebQueryString)
    END_KSCRIPT_CLASS()

public:

    WebQueryString()
    {
        m_req = NULL;
    }
    
    void initGet(RequestInfo* req)
    {
        m_req = req;
        
        std::map<std::wstring,std::wstring>& get_values = req->getGetValues();
        std::map<std::wstring,std::wstring>::iterator it;
        for (it = get_values.begin(); it != get_values.end(); ++it)
            this->createMember(it->first, 0)->setString(it->second);
    }
    
    void initPost(RequestInfo* req)
    {
        m_req = req;
        
        std::map<std::wstring,RequestPostInfo>& post_values = req->getPostValues();
        std::map<std::wstring,RequestPostInfo>::iterator it;
        for (it = post_values.begin(); it != post_values.end(); ++it)
        {
            if (it->second.data)
            {
                std::string val(it->second.data, it->second.length);
                this->createMember(it->first, 0)->setString(kl::towstring(val));
            }
             else
            {
                this->createMember(it->first, 0)->setString(it->second.str);
            }
        }
    }
       
private:

    RequestInfo* m_req;
    std::map<std::wstring, kscript::Value*> m_vars;
};

class WebPostFile : public ScriptHostBase
{
    typedef kscript::ValueObject BaseClass;
    
    BEGIN_KSCRIPT_CLASS("WebPostFile", WebPostFile)
        KSCRIPT_METHOD("saveFile", WebPostFile::saveFile)
    END_KSCRIPT_CLASS()

public:

    WebPostFile()
    {
    }
    
    void init(const RequestPostInfo& info)
    {
        m_info = info;
    }
    
    void saveFile(kscript::ExprEnv* env, kscript::Value* retval)
    {
        retval->setBoolean(false);
        if (env->getParamCount() < 1)
            return;
            
        std::wstring fname = env->getParam(0)->getString();
        xf_file_t f = xf_open(fname, xfCreate, xfWrite, xfShareNone);
        if (!f)
            return;

        size_t written = (size_t)xf_write(f, m_info.data, 1, m_info.length);
        xf_close(f);
        
        if (written != m_info.length)
        {
            xf_remove(fname);
            return;
        }

        retval->setBoolean(true);
    }
    
private:

    RequestPostInfo m_info;
};



class HttpCookie : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("HttpCookie", HttpCookie)
        KSCRIPT_METHOD("constructor", HttpCookie::constructor)
    END_KSCRIPT_CLASS()

public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval)
    {
    }
    
};

class WebRequest : public ScriptHostBase
{    
    BEGIN_KSCRIPT_CLASS("WebRequest", WebRequest)
    END_KSCRIPT_CLASS()

public:

    WebRequest()
    {
        m_req = NULL;
    }
    
    ~WebRequest() { }
    
    void init(RequestInfo* req)
    {
        m_req = req;
        
        WebQueryString* query = WebQueryString::createObject();
        query->initGet(req);
        kscript::Value query_value;
        query_value.setObject(query);
        this->setMember(L"query", &query_value);
        
        WebQueryString* post = WebQueryString::createObject();
        post->initPost(req);
        kscript::Value post_value;
        post_value.setObject(post);
        this->setMember(L"post", &post_value);
        
        
        std::map<std::wstring, RequestPostInfo>& files = m_req->getFileValues();
        kscript::Value files_array;
        files_array.setObject();
        std::map<std::wstring, RequestPostInfo>::iterator it;
        for (it = files.begin(); it != files.end(); ++it)
        {
            kscript::Value* v = files_array.createMember(it->first, kscript::Value::attrNone);
            WebPostFile* f = WebPostFile::createObject();
            f->init(it->second);
            v->setObject(f);
        }
        this->setMember(L"files", &files_array);
        
        std::map<std::wstring, std::wstring>& cookies = m_req->getCookies();
        kscript::Value cookies_array;
        cookies_array.setObject();
        std::map<std::wstring, std::wstring>::iterator c_it;
        for (c_it = cookies.begin(); c_it != cookies.end(); ++c_it)
        {
            kscript::Value* v = cookies_array.createMember(c_it->first, kscript::Value::attrNone);
            v->setString(c_it->second);
        }
        this->setMember(L"cookies", &cookies_array);
    }
    
private:

    RequestInfo* m_req;
};



class WebResponse : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("WebResponse", WebResponse)
        KSCRIPT_METHOD("constructor", WebResponse::constructor)
        KSCRIPT_METHOD("setStatusCode", WebResponse::setStatusCode)
        KSCRIPT_METHOD("setContentType", WebResponse::setContentType)
        KSCRIPT_METHOD("addHeader", WebResponse::addHeader)
        KSCRIPT_METHOD("addCookie", WebResponse::addCookie)
        KSCRIPT_METHOD("write", WebResponse::write)
    END_KSCRIPT_CLASS()

public:

    WebResponse()
    {
        m_req = NULL;
    }
    
    
    ~WebResponse() { }
    
    void init(RequestInfo* req)
    {
        m_req = req;
    }
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval)
    {
    }
    
    void setStatusCode(kscript::ExprEnv* env, kscript::Value* retval)
    {
        if (env->getParamCount() == 0)
        {
            retval->setBoolean(false);
            return;
        }
        
        m_req->setStatusCode(env->getParam(0)->getInteger());
        retval->setBoolean(true);
    }
    
    void setContentType(kscript::ExprEnv* env, kscript::Value* retval)
    {
        if (env->getParamCount() == 0)
        {
            retval->setBoolean(false);
            return;
        }
        
        std::string str = kl::tostring(env->getParam(0)->getString());
        m_req->setContentType(str.c_str());
        retval->setBoolean(true);
    }
    
    void addHeader(kscript::ExprEnv* env, kscript::Value* retval)
    {
        if (env->getParamCount() == 0)
        {
            retval->setBoolean(false);
            return;
        }
        
        std::string str = kl::tostring(env->getParam(0)->getString());
        
        m_req->addHeader(str.c_str());
        retval->setBoolean(true);
    }
    
    void addCookie(kscript::ExprEnv* env, kscript::Value* retval)
    {
        retval->setBoolean(false);
        
        if (env->getParamCount() == 0)
            return;
            
        if (env->getParam(0)->isObject() &&
            env->getParam(0)->getObject()->isKindOf(HttpCookie::staticGetClassId()))
        {
        }
         else
        {
            if (env->getParamCount() < 2)
                return;
            
            ResponseCookie c;
            c.name = kl::tostring(env->getParam(0)->getString());
            c.value = kl::tostring(env->getParam(1)->getString());
            m_req->addCookie(c);
        }
    }
     
    void write(kscript::ExprEnv* env, kscript::Value* retval)
    {
        retval->setBoolean(false);
        if (env->getParamCount() == 0)
            return;
        
        if (env->getParam(0)->isObject() &&
            env->getParam(0)->getObject()->isKindOf(MemoryBuffer::staticGetClassId()))
        {
            MemoryBuffer* buf = static_cast<MemoryBuffer*>(env->getParam(0)->getObject());
            
            int offset = 0;
            int bytes_to_write = -1;
            
            if (env->getParamCount() >= 2)
                offset = env->getParam(1)->getInteger();
            if (env->getParamCount() >= 3)
            {
                bytes_to_write = env->getParam(1)->getInteger();
                if (bytes_to_write < 0)
                    return;
            }
            
            if (offset < 0)
                return;
            if ((size_t)offset >= buf->getBufferSize())
                return;
            
            if (bytes_to_write == -1)
                bytes_to_write = (int)buf->getBufferSize() - offset;
                
            m_req->write(buf->getBuffer() + offset, (size_t)bytes_to_write);
            
            retval->setBoolean(true);
        }
         else
        {
            kscript::Value v;
            env->getParam(0)->toString(&v);
            
            std::string str = kl::tostring(v.getString());
            
            m_req->write(str.c_str(), str.length());
            retval->setBoolean(true);
        }
    }
    
private:

    RequestInfo* m_req;
};







WebServer::WebServer()
{
    m_ctx = NULL;
}

WebServer::~WebServer()
{
    stop();
}

// static
void WebServer::request_handler(struct mg_connection* conn,
                                const struct mg_request_info* ri,
                                void* user_data)
{
    RequestInfo r(conn, ri);
    ((WebServer*)user_data)->onRequest(r);
}

void WebServer::start()
{
    m_ctx = mg_start();
    
    mg_set_option(m_ctx, "ports", "80");
    mg_bind_to_uri(m_ctx, "*", &request_handler, (void*)this);
}

void WebServer::stop()
{
    if (m_ctx)
    {
        mg_stop(m_ctx);
        m_ctx = NULL;
    }
    
    removeAllServerSessionObjects();
}

bool WebServer::isRunning() const
{
    return m_ctx ? true : false;
}

static void response_printer(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    RequestInfo* ri = (RequestInfo*)param;
    std::wstring wstr = env->getParam(0)->getString();
    std::string str = tostr(wstr);
    ri->write(str.c_str(), str.length());
}

void WebServer::onRequest(RequestInfo& ri)
{
    // get the intial time at the start of the request
    clock_t time_start = ::clock();

    // get the webserver root
    wxString wxuri = g_app->getAppPreferences()->getString(wxT("webserver.root"), wxT("/"));
    wxuri += wxT("/");
    wxuri += towx(ri.getURI());
    
    // remove extra slashes
    while (1)
    {
        int p = wxuri.Find(wxT("//"));
        if (p == -1)
            break;
        wxuri.erase(p, 1);
    }

    std::wstring uri = towstr(wxuri);
    std::wstring ext = kl::afterLast(uri, wxT('.'));

    tango::IDatabasePtr db = g_app->getDatabase();
    tango::IFileInfoPtr file;
    if (db)
        file = db->getFileInfo(uri);

    if (file.isNull())
    {
        ri.setStatusCode(404); // 404 Not Found
    }
    else if (file->getMimeType() == wxT("text/html") || ext == L"html" || ext == L"htm")
    {
        handleHtmlResponse(uri, ri);
    }
    else if (ext == L"sjs")
    {
        handleScriptResponse(uri, ri);
    }
    else if (ext == L"pdf")
    {
        // TODO: remove when file->getFileType() is fixed for
        // externally mounted PDFs

        // default handler is stream; but right now, pdfs in external
        // folders return filetype of tango::filetypeSet, and we don't
        // want to serve these as tables, so we need to preempt the
        // table handler
        handleStreamResponse(uri, ri);
    }
    else if (file->getType() == tango::filetypeFolder)
    {
        handleFolderResponse(uri, ri);
    }
    else if (file->getType() == tango::filetypeSet)
    {
        handleTableResponse(uri, ri);
    }
    else
    {
        handleStreamResponse(uri, ri);
    }

    // get the intial time at the end of the request
    clock_t time_end = ::clock();
    double total_time = (double)(time_end - time_start)/CLOCKS_PER_SEC;
    
    // if the preference is set, echo the command
    bool echo = g_app->getAppPreferences()->getBoolean(wxT("webserver.echo"), false);
    if (echo)
    {
        wchar_t buf[255];
	    swprintf(buf, 255, L"%2.4f", total_time);

        std::wstring output = L"";
        output += L"Time: ";
        output += buf;
        output += L"\t";
        output += L"Request: ";
        output += ri.getURI();

        std::wstring query = ri.getQuery();
        if (query.length() > 0)
            output = output + wxT("?") + query;

        output += wxT("\n");
        g_app->getAppController()->printConsoleText(output);
    }
}

bool WebServer::getServerSessionObject(const std::wstring& name, ServerSessionObject** obj)
{
    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return false;

    *obj = it->second;
    return true;
}

void WebServer::addServerSessionObject(const std::wstring& name, ServerSessionObject* obj)
{
    m_session_objects[name] = obj;
}

void WebServer::removeServerSessionObject(const std::wstring& name)
{
    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return;
        
    m_session_objects.erase(it);
}

void WebServer::removeAllServerSessionObjects()
{
    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();
    
    for (it = m_session_objects.begin(); it != it_end; ++it)
    {
        // free the session objects
        delete it->second;
    }
    
    m_session_objects.clear();
}

void WebServer::handleFolderResponse(const std::wstring& uri, RequestInfo& ri)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    tango::IFileInfoEnumPtr items = db->getFolderInfo(uri);

    if (items.isNull())
    {
        ri.setStatusCode(404);
        return;
    }

    JsonNode root_node;
    ri.setContentType("application/json");

    // report info
    root_node["total_count"] = (int)items->size();
    JsonNode items_node = root_node["items"];

    size_t i, count = items->size();
    int item_type;

    for (i = 0; i < count; ++i)
    {
        tango::IFileInfoPtr info = items->getItem(i);
        item_type = info->getType();

        JsonNode item_node = items_node.appendElement();
        item_node["name"] = info->getName();
        item_node["type"] = JsonFileType::toString(info->getType());
        
        switch (info->getFormat())
        {
            case tango::formatNative:
                item_node["format"] = kl::towstring("native");
                break;
                
            case tango::formatDelimitedText:
                item_node["format"] = kl::towstring("delimitedtext");
                break;
                
            case tango::formatFixedLengthText:
                item_node["format"] = kl::towstring("fixedlengthtext");
                break;
                
            case tango::formatText:
                item_node["format"] = kl::towstring("text");
                break;                                                                                    

            case tango::formatXbase:
                item_node["format"] = kl::towstring("xbase");
                break;
        }
    }

    wxString response;
    JsonConfig::saveToString(root_node, response);
    ri.write((const char*)response.mbc_str(), response.length());
}

void WebServer::handleTableResponse(const std::wstring& uri, RequestInfo& ri)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (!db->getFileExist(uri))
    {
        ri.setStatusCode(404);
        return;    
    }

    std::wstring method = ri.getGetValue(L"method");
    if (method.size() == 0 || method == L"describe")
    {
        // by default, return the structure of the table;
        // also, if the method is "describe", return the 
        // structure
    
        ri.setContentType("application/json");

        tango::ISetPtr set = db->openSet(uri);
        if (set.isNull())
        {
            ri.setStatusCode(404);
            return;
        }

        kscript::JsonNode root_node;
        root_node = JsonStructure::toJsonNode(set);

        wxString response = towx(root_node.toString());
        ri.write((const char*)response.mbc_str(), response.length());
    }
    else if (method == L"select")
    {
        // if the method is "select", query the table
        ri.setContentType("application/json");
        
        tango::ISetPtr set = db->openSet(uri);
        if (set.isNull())
        {
            ri.setStatusCode(404);
            return;
        }

        // see if we already have a server session iterator object 
        // corresponding to the URL query parameters; if we do, get it, 
        // otherwise create a new one
        std::wstring query_id = getQueryId(ri);

        ServerSessionObject* server_object;
        ServerSessionQuery* server_query;

        if (getServerSessionObject(query_id, &server_object))
        {
            server_query = (ServerSessionQuery*)server_object;
        }
        else
        {
            // build the query from the URL parameters;
            // TODO: properly escape parameters
            std::wstring query;
            query.append(L"SELECT * FROM ");
            query.append(uri);
            
            // add on any order clause
            std::wstring order = ri.getGetValue(L"order");
            if (order.length() > 0)
            {
                query.append(L" ORDER BY ");
                query.append(kl::url_decodeURIComponent(order));
            }

            // create a new server session query; if successful,
            // add the it to the list of server queries    
            server_query = new ServerSessionQuery(query);
            if (server_query->execute())
                addServerSessionObject(query_id, server_query);
        }

        int start = kl::wtoi(ri.getGetValue(L"start"));
        int limit = kl::wtoi(ri.getGetValue(L"limit"));

        kscript::JsonNode root_node;
        if (!server_query->getResult(root_node, start, limit))
        {
            ri.setStatusCode(404);
            return;
        }

        std::wstring node_wstr = root_node.toString();
        std::string node_str = tostr(node_wstr);
        
        if (!ri.acceptCompressed())
        {
            // no compression
            ri.write((const char*)node_str.c_str(), node_str.length());
        }
         else
        {
            // set the content encoding header
            std::string header = "Content-Encoding: deflate";
            ri.addHeader(header.c_str());            
        
            // compress the content
            unsigned long original_len = node_str.length();
            std::string r = compress_str(node_str);
            
            unsigned long size = r.length();
            char* data = new char[size];
            r.copy(data, size);          
            ri.write(data, size);

            std::string t = uncompress_str(std::string(data,size),original_len);

            delete [] data;
        }
    }
}

void WebServer::handleHtmlResponse(const std::wstring& uri, RequestInfo& ri)
{
    tango::IDatabasePtr db = g_app->getDatabase();

    ri.setStatusCode(200);
    ri.setContentType("text/html");
    
    std::wstring text;
    if (!readStreamTextFile(db, uri, text))
        return;
        
    ri.write((const char*)tostr(text).c_str(), text.length());
}

void WebServer::handleScriptResponse(const std::wstring& uri, RequestInfo& ri)
{
    kscript::Value request, response;
    
    WebRequest* web_request = WebRequest::createObject();
    WebResponse* web_response = WebResponse::createObject();
    
    web_request->init(&ri);
    web_response->init(&ri);
    
    request.setObject(web_request);
    response.setObject(web_response);
    
    ScriptHostParams params;
    params.print_function.setFunction(response_printer, (void*)&ri);
    params.global_vars[L"Request"] = &request;
    params.global_vars[L"Response"] = &response;
    
    AppScriptError err;
    err.code = 0;
    
    ri.setStatusCode(200);
    ri.setContentType("text/html");
    
    
    // set random seed for this thread
    #ifdef _MSC_VER
    static int counter = 0;
    counter += 100;
    // ensures that the rand() is truly random in this thread
    int seed = (int)time(NULL);
    seed += (int)clock();
    seed += counter;
    srand(seed);
    #endif

    g_app->getAppController()->executeScript(
                         towx(uri), &params, &err,
                         false /* not async - wait until finished */);
    
    if (err.code != 0)
    {
        wxString s;
        s.Printf(wxT("%s (%d): %s"), (const wxChar*)err.file.c_str(),
                       err.line, (const wxChar*)err.message.c_str());
        ri.write((const char*)s.mbc_str(), s.length());
    }
}

void WebServer::handleStreamResponse(const std::wstring& uri, RequestInfo& ri)
{
    tango::IDatabasePtr db = g_app->getDatabase();

    tango::IStreamPtr stream;
    stream = db->openStream(uri);

    if (stream.isNull())
    {
        ri.setStatusCode(404); // 404 Not Found
        return;            
    }

    ri.setStatusCode(200);

    std::wstring ext = kl::afterLast(uri, wxT('.'));
    if (ext == L"css")
        ri.setContentType("text/css");
    else if (ext == L"js")
        ri.setContentType("text/javascript");
    else if (ext == L"gif")
        ri.setContentType("image/gif");
    else if (ext == L"jpg" || ext == L"jpeg")
        ri.setContentType("image/jpeg");
    else if (ext == L"png")
        ri.setContentType("image/png");
    else if (ext == L"svg")
        ri.setContentType("image/svg+xml");
    else
        ri.setContentType("text/plain");

    unsigned char buf[1024];
    unsigned long len = 0;
    while (1)
    {
        stream->read(buf, 1024, &len);
        if (len > 0)
            ri.write(buf, len);
        if (len != 1024)
            break;
    }
}


/*
    // TODO: following code are old experimental "command" handlers for 
    // web server responses; they were triggered with the following logic
    // in WebServer::onRequest(); if the command was empty, the current
    // logic on WebServer::onRequest() applied; these are here for
    // reference implementations now, and can be removed when no longer
    // necessary

    wxString response;
    wxString command = towx(ri.getValue(L"command"));

    if (!command.IsEmpty())
    {
        // TODO: remove old logic; deprecated
    
             if (command == wxT("query"))        { handleQueryRequest(ri, response);  } // fall through
        else if (command == wxT("script"))       { handleScriptRequest(ri, response); } // fall through
        else if (command == wxT("report"))       { handleReportRequest(ri, response); } // done
        else if (command == wxT("create_mount")) { handleMountRequest(ri, response);  } // fall through


        wxString callback = ri.getValue(L"callback");
        if (callback.Length() > 0)
        {
            response.Prepend(callback + wxT("("));
            response.Append(wxT(");"));
        }

        ri.write((const char*)response.mbc_str(), response.length());    
    }


void WebServer::handleQueryRequest(RequestInfo& ri,
                                   wxString& response)
{
    ri.setContentType("text/plain");
    
    wxString path = ri.getValue(L"path");

    JsonNode root;
    JsonNode metaData, fields, rows;
    metaData = root["metaData"];
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;
    
    tango::ISetPtr set = db->openSet(towstr(path));
    if (set.isNull())
        return;
    tango::IIteratorPtr iter = set->createIterator(L"", L"", NULL);
    if (iter.isNull())
        return;
        
        
    metaData["idProperty"] = wxT("0");
    metaData["root"] = wxT("rows");
    metaData["totalProperty"] = wxT("row_count");
    fields = metaData["fields"];
    fields.setArray();
    rows = root["rows"];
    rows.setArray();

    std::vector<tango::objhandle_t> handles;
    
    tango::IStructurePtr structure = iter->getStructure();
    int i, column_count = structure->getColumnCount();
    
    for (i = 0; i < column_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);
        JsonNode field = fields.appendElement();
        
        field["mapping"] = wxString::Format(wxT("%d"), i+1);
        field["name"] = towx(colinfo->getName());
        
        switch (colinfo->getType())
        {
            default:                        field["type"] = wxT("auto");    break;
            case tango::typeCharacter:      field["type"] = wxT("string");  break;
            case tango::typeWideCharacter:  field["type"] = wxT("string");  break;
            case tango::typeNumeric:        field["type"] = wxT("float");   break;
            case tango::typeDouble:         field["type"] = wxT("float");   break;
            case tango::typeInteger:        field["type"] = wxT("int");     break;
            case tango::typeDate:           field["type"] = wxT("date");    break;
            case tango::typeDateTime:       field["type"] = wxT("date");    break;
            case tango::typeBoolean:        field["type"] = wxT("boolean"); break;
        }
        
        handles.push_back(iter->getHandle(colinfo->getName()));
    }
    
    int row_count = 0;
    iter->goFirst();
    while (!iter->eof())
    {
        JsonNode row = rows.appendElement();
        row.setArray();
        
        JsonNode counter = row.appendElement();
        counter.setInteger(row_count+1);
        
        for (i = 0; i < column_count; ++i)
        {
            JsonNode cell = row.appendElement();
            tango::objhandle_t handle = handles[i];
            
            switch (iter->getType(handle))
            {
                case tango::typeCharacter:
                case tango::typeWideCharacter:
                    cell.setString(towx(iter->getWideString(handle)));
                    break;
                case tango::typeNumeric:
                case tango::typeDouble:
                    cell.setDouble(iter->getDouble(handle));
                    break;
                case tango::typeInteger:
                    cell.setInteger(iter->getInteger(handle));
                    break;
                case tango::typeDate:
                case tango::typeDateTime:
                {
                    wxString s;
                    tango::DateTime dt = iter->getDateTime(handle);
                    s.Printf(wxT("%04d-%02d-%02d"), dt.getYear(), dt.getMonth(), dt.getDay());
                    cell.setString(s);
                    break;
                }
                case tango::typeBoolean:
                    cell.setBoolean(iter->getBoolean(handle));
                    break;
            }
        }
        
        iter->skip(1);
        row_count++;
        if (row_count >= 100)
            break;
    }

    root["row_count"].setInteger(row_count);
    
    // write the JSON to the respose string
    JsonConfig::saveToString(root, response);
}

void WebServer::handleScriptRequest(RequestInfo& ri,
                                    wxString& response)
{
    // TODO: code adapted from AppController::executeScript(); may
    // want to factor the part that reads the script data from the
    // file and puts it into a string


    // create a script host object
    ScriptHost* script_host = new ScriptHost; 


    // get the script location from the parameters
    wxString path = ri.getValue(L"path");

    // add the method, query, and parameter members to the 
    // request object
    kscript::Value* method_value = new kscript::Value;
    method_value->setString(ri.getMethod());

    kscript::Value* query_value = new kscript::Value;
    query_value->setString(wxString::From8BitData(ri->query_string));
    
    kscript::Value* param_object = new kscript::Value;

    param_object->setObject();

    std::vector<request_member> params; // get parameters
    if (ri->query_string)
        extractPairs(towstr(ri->query_string), params);
    
    std::vector<request_member>::iterator it, it_end;
    it_end = params.end();
    
    for (it = params.begin(); it != it_end; ++it)
    {
        kscript::Value* param_value = new kscript::Value;
        param_value->setString(it->value);
        param_object->setMember(it->key, param_value);
    }

    kscript::ValueObject* server_request = new kscript::ValueObject;
    server_request->setMember(std::wstring(L"method"), method_value);
    server_request->setMember(std::wstring(L"query"), query_value);
    server_request->setMember(std::wstring(L"params"), param_object); // get parameter object

    // create a server request object
    kscript::Value request_value;
    request_value.setObject(server_request);
    script_host->addValue(std::wstring(L"ServerRequest"), request_value);


    // get the database
    tango::IDatabasePtr db = g_app->getDatabase();
    tango::IStreamPtr stream = db->openStream(towstr(path));
    if (!stream)
        return;
    
    std::wstring value;


    // load script data
    wxMemoryBuffer buf;
    
    char* tempbuf = new char[1025];
    unsigned long read = 0;
    
    while (1)
    {
        if (!stream->read(tempbuf, 1024, &read))
            break;
        
        buf.AppendData(tempbuf, read);
        
        if (read != 1024)
            break;
    }
    
    delete[] tempbuf;
    

    unsigned char* ptr = (unsigned char*)buf.GetData();
    size_t buf_len = buf.GetDataLen();
    if (buf_len >= 2 && ptr[0] == 0xff && ptr[1] == 0xfe)
    {
        kl::ucsle2wstring(value, ptr+2, (buf_len-2)/2);
    }
     else if (buf_len >= 3 && ptr[0] == 0xef && ptr[1] == 0xbb && ptr[2] == 0xbf)
    {
        // utf-8
        wchar_t* tempbuf = new wchar_t[buf_len+1];
        kl::utf8_utf8tow(tempbuf, buf_len+1, (char*)ptr+3, buf_len-3);
        value = towx(tempbuf);
        delete[] tempbuf;
    }
     else
    {
        buf.AppendByte(0);
        value = wxString::From8BitData((char*)buf.GetData());
    }


    bool compile_result;
    compile_result = script_host->compile(value.c_str());

    if (!compile_result)
    {
        delete script_host;
        return;
    }

    script_host->run();
    response = towx(script_host->getRetval()->getString());

    delete script_host;
}


class GenerateReportHelper : public wxEvtHandler
{
public:

    int m_ready;

    GenerateReportHelper()
    {
        m_ready = 0;
    }
    
    bool ProcessEvent(wxEvent& evt)
    {
        wxCommandEvent& cevt = (wxCommandEvent&)evt;
        
        wxString path = cevt.GetString();
        
        tango::IDatabasePtr db = g_app->getDatabase();
        
        // set the source
        tango::ISetPtr set = db->openSet(towstr(path));
        if (set.isNull())
        {
            m_ready = 1;
            return true;
        }
        
        ReportCreateInfo data;
   
        tango::IStructurePtr structure = set->getStructure();
        size_t i = 0, count = structure->getColumnCount();
        if (count > 5)
            count = 5;
        for (i = 0; i < count; ++i)
        {
            tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);
            
            int width = colinfo->getWidth();
            if (width > 20)
                width = 20;
                
            // set the field info
            ReportCreateField field;
            field.field_name = towx(colinfo->getName());
            field.caption = cfw::makeProper(field.field_name);
            field.alignment = kcanvas::ALIGNMENT_LEFT;
            field.column_width = width*12*(kcanvas::CANVAS_MODEL_DPI/kcanvas::CANVAS_SCREEN_DPI);
            data.content_fields.push_back(field);
        }

        data.iterator = set->createIterator(L"", L"", NULL);
        data.path = towx(set->getObjectPath());
        data.sort_expr = wxT("");
        data.filter_expr = wxT("");
        
        // create a report
        ReportDoc* doc = new ReportDoc;
        cfw::IDocumentSitePtr doc_site;
        doc_site = g_app->getMainFrame()->createSite(static_cast<cfw::IDocument*>(doc),
                                                 cfw::sitetypeNormal, -1, -1, -1, -1);

        if (doc_site.isNull() || !doc->create(data))
        {
            delete doc;
            m_ready = 1;
            return true;
        }
        
        doc->saveFile(wxT("/temp_report"));
        
        g_app->getMainFrame()->closeSite(g_app->getMainFrame()->getLastChild(), cfw::closeForce);
        
        m_ready = 1;
        return true;
    }
};

void WebServer::handleReportRequest(RequestInfo& ri,
                                    wxString& response)
{
    ri.setContentType("application/pdf");
    
    // get the report location from the parameters
    wxString path = towx(ri.getValue(L"path"));


    tango::IDatabasePtr db = g_app->getDatabase();
    tango::IFileInfoPtr file_info;
    if (db)
        file_info = db->getFileInfo(towstr(path));
    if (file_info.isOk() && file_info->getType() == tango::filetypeSet)
    {
        GenerateReportHelper* g = new GenerateReportHelper;
        
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, 10000);
        e.SetString(path);
        ::wxPostEvent(g, e);
        
        while (!g->m_ready)
            wxThread::Sleep(100);


        path = wxT("/temp_report");
    }



    // TODO: this was adapted from HostPrinting::saveAsPdf();
    // may want to factor

    kcanvas::ICanvasPtr canvas = kcanvas::Canvas::create();
    kcanvas::IComponentPtr report_design = CompReportDesign::create();
    kcanvas::IComponentPtr report_layout = CompReportLayout::create();

    CompReportDesign* report_design_ptr = static_cast<CompReportDesign*>(report_design.p);
    CompReportLayout* report_layout_ptr = static_cast<CompReportLayout*>(report_layout.p);

    // TODO: for now, set the canvas manually; we have
    // to do this because we dynamically generate the
    // layout on the pdf canvas, which isn't yet created;
    // should reorganize pdf mechanism so the pdf canvas
    // exists when we need it
    report_design_ptr->setCanvas(canvas);
    report_layout_ptr->setCanvas(canvas);

    // set the design table
    report_layout_ptr->setDesignTable(report_design);

    // load the input
    if (!report_layout_ptr->load(path))
        return;

    // refresh the data source manually
    if (!report_layout_ptr->refresh())
        return;

    // block until we have a valid data source; wait for
    // 2 minutes, then bail out
    int count = 0;
    while (!report_layout_ptr->isModelLoaded())
    {
        wxThread::Sleep(100);
        ++count;

        if (count > 1200)
            return;
    }

    // create a temporary filename and remove that
    // file if it exists
    wxString filename = towx(xf_get_temp_filename(L"tmp", L"pdf"));
    xf_remove(towstr(filename));

    // save the pdf to a temporary file; set the "block" 
    // parameter to true so the components stay valid until 
    // the job is done
    if (!report_layout_ptr->saveAsPdf(filename, true))
    {
        xf_remove(towstr(filename));
        return;
    }

    // open the temp file
    xf_file_t source_file = xf_open(towstr(filename),
                                  xfOpen,
                                  xfRead,
                                  xfShareNone);

    if (!source_file)
    {
        xf_remove(towstr(filename));
        return;
    }

    // read the file into a buffer
    wxMemoryBuffer buf;
    unsigned int size_to_read = 4096;
    unsigned char buffer[4096];

    while (1)
    {
        // read a chunk of the file
        unsigned int read = xf_read(source_file, buffer, 1, size_to_read);

        // if we didn't read anything, we're done
        if (read == 0)
            break;

        // add the data to the buffer
        buf.AppendData(&buffer, read);
    }

    // close the temp file and remove it
    xf_close(source_file);
    xf_remove(towstr(filename));

    // write the output directly
    ri.write((const char*)buf.GetData(), buf.GetDataLen());    
}

void WebServer::handleMountRequest(RequestInfo& ri,
                                   wxString& response)
{
    wxString target = ri.getValue(L"target");
    wxString connection_string = ri.getValue(L"connection_string");
    wxString remote_path = ri.getValue(L"remote_path");
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isOk())
    {
        db->setMountPoint(towstr(target), towstr(connection_string), towstr(remote_path));
    }
}

*/

