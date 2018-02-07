/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-05
 *
 */


#include "scripthost.h"
#include "http.h"
#include "memory.h"
#include <curl/curl.h>
#include <kl/file.h>
#include <kl/utf8.h>
#include <kl/string.h>
#include <kl/url.h>



namespace scripthost
{



class HttpRequestThread : public kl::thread
{
public:

    HttpRequest* m_request;
    int m_method;

    HttpRequestThread(HttpRequest* request) : kl::thread()
    {
        m_request = request;
        m_request->baseRef();
    }

    ~HttpRequestThread()
    {
        m_request->baseUnref();
    }

    unsigned int entry()
    {
        m_request->doSend();
        
        // set busy to false
        m_request->m_state_mutex.lock();
        m_request->m_busy = false;
        m_request->m_state_mutex.unlock();

        // fire 'finished' signal
        m_request->fireFinishedEvent();
        
        return 0;
    }
};


static std::wstring multipartEncode(const std::wstring& input)
{
    std::wstring result;
    result.reserve(input.length() + 10);
    
    const wchar_t* ch = input.c_str();
    unsigned int c;
    
    wchar_t buf[80];

    while ((c = (unsigned int)*ch))
    {
        if (c > 255)
        {
            swprintf(buf, 80, L"&#%d;", c);
            result += buf;
        }
         else
        {
            result += *ch;
        }

        ++ch;
    }

    return result;
}



// writer function for the http header

// static
size_t HttpRequest::http_header_writer(void* ptr, size_t size, size_t nmemb, void *stream)
{
    std::string* str = (std::string*)stream;
    size_t total_size = 0;
    
    // limit string length to 5 MB
    if (str->length() < 5000000)
    {
        total_size = size*nmemb;
        
        // only keep the last http header
        if (0 == strncmp((const char*)ptr, "HTTP/", 5))
            *str = "";
        
        str->append((const char*)ptr, total_size);
    }

    // return the number of bytes actually taken care of; if it differs from
    // the amount passed to the function, it will signal an error to the library,
    // abort the transfer and return CURLE_WRITE_ERROR
    return total_size;
}

// static
size_t HttpRequest::http_response_writer(void* ptr, size_t size, size_t nmemb, void *stream)
{
    HttpRequest* pThis = (HttpRequest*)stream;
    size_t total_size = 0;
    
    HttpResponsePiece piece;
    piece.len = size*nmemb;
    piece.curpos = 0;
    piece.buf = new unsigned char[piece.len];
    memcpy(piece.buf, ptr, piece.len);
    
    pThis->m_response_mutex.lock();
    pThis->m_response_pieces.push_back(piece);
    pThis->m_response_bytes += piece.len;
    pThis->m_response_mutex.unlock();
    
    return piece.len;
}



// (CLASS) HttpRequest
// Category: Network
// Description: A class for issuing HTTP requests and processing the results.
// Remarks: HttpRequest allows the user to communicate with HTTP Servers.  Both the normal
//     and the secure HTTPS protocols are supported.
//
// Property(HttpRequest.methodGet) : An integer representing an Http GET request.
// Property(HttpRequest.methodHead) : An integer representing an Http HEAD request.
// Property(HttpRequest.methodPost) : An integer representing an Http POST request.
// Property(HttpRequest.methodPut) : An integer representing an Http PUT request.

// (EVENT) HttpRequest.finished
// Description: Fired when asynchronous web events are finished

HttpRequest::HttpRequest()
{
    CURLcode curl_result;
    
    m_curl = (CURL*)0;
    curl_result = CURLE_OK;
    
    m_curl = curl_easy_init();
    
    // initialize header and form field pointers
    m_headers = NULL;
    m_formfields = NULL;
    m_formfieldslast = NULL;
    m_post_multipart = false;
    m_auto_encode = true;
    m_async = false;
    m_busy = false;
    m_response_bytes = 0;
    m_method = methodGet;
    m_proxy = "";
    m_proxy_port = 0;
    
    // see http://curl.haxx.se/lxr/source/docs/examples/https.c
    // these are necessary unless we want to include a certificate bundle
    curl_result = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_result = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
    
    // follow redirects
    curl_result = curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);
    
    // when redirecting, set the referer
    curl_result = curl_easy_setopt(m_curl, CURLOPT_AUTOREFERER, 1);
    
    // set the maximimum number of redirects to infinity
    curl_result = curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, -1);
    
    // set the default proxy type
    //curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
    
    // set the default authentication methods
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    
    // allow cookies
    curl_result = curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");
    
    // include headers in body output
    //curl_result = curl_easy_setopt(m_curl, CURLOPT_HEADER, 1);
    
    curl_version_info_data* a = curl_version_info(CURLVERSION_NOW);
}

HttpRequest::~HttpRequest()
{
    // clean up headers and form fields
    clearHeaders();
    clearFormFields();

    curl_easy_cleanup(m_curl);
    
    freeResponsePieces();
}

// (CONSTRUCTOR) HttpRequest.constructor
// Description: Creates a new HttpRequest object.
//
// Syntax: HttpRequest()
//
// Remarks: Creates a new HttpRequest object.

void HttpRequest::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    initComponent(env);
    
    /*

    // TODO: reimplement
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs)
    {
        if (prefs->getLong(wxT("internet.proxy.type"), prefProxyDirect) == prefProxyManual)
        {
            // get http proxy info from the registry
            m_proxy = tostr(prefs->getString(wxT("internet.proxy.http"), wxT("")));
            m_proxy_port = prefs->getLong(wxT("internet.proxy.http_port"), 8080);
        }
    }
    
    // set the curl proxy info
    CURLcode curl_result;
    curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXY, m_proxy.c_str());
    curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYPORT, m_proxy_port);
    */


    //getMember(L"finished")->setObject(scripthost::Event::createObject(env));
}

// (METHOD) HttpRequest.send
// Description: Issues the HTTP request.
//
// Syntax: function HttpRequest.send()
//
// Remarks: Issues an HTTP request to the URL location specified on the
//     request object with the specified HTTP method.  See the setUrl() and
//     setMethod() calls for more information.  The resulting data can
//     be retrieved by subsequently calling getResponseText() and/or binaryRead()
//     methods.

void HttpRequest::send(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (isLoadingInternal())
    {
        retval->setNull();
        return;
    }

    if (m_async)
    {
        m_state_mutex.lock();
        m_busy = true;
        m_state_mutex.unlock();
    
    
        retval->setNull();
        
        if (env->getParamCount() > 0)
            m_location = kl::tostring(env->getParam(0)->getString());
        
        HttpRequestThread* thread = new HttpRequestThread(this);
        if (thread->create() != 0)
            return;
    }
     else
    {
        doSend();
    }
}

// (METHOD) HttpRequest.setUrl
// Description: Sets the URL to be requested
//
// Syntax: function HttpRequest.setUrl(url : String)
//
// Remarks: Sets the URL on the request object.
//
// Param(url): The url to which to send the request.

void HttpRequest::setUrl(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
        m_location = kl::tostring(env->getParam(0)->getString());
}

// (METHOD) HttpRequest.getUrl
// Description: Returns the URL string of the request object
//
// Syntax: function HttpRequest.getUrl() : String
//
// Remarks: Returns the URL which is being retrieved or which was retrieved.
//
// Returns: The url which is being retrieved or which was retrieved.

void HttpRequest::getUrl(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(kl::towstring(m_location));
}

// (METHOD) HttpRequest.setMethod
// Description: Sets the HttpRequest method type to use when issuing the request
//
// Syntax: function HttpRequest.setMethod(type : String) : Boolean
// Syntax: function HttpRequest.setMethod(type : Integer) : Boolean
//
// Remarks: This function sets the HttpRequest method |type| to use when 
//     issuing the Http request.  If the string form of the function is used, 
//     the request method |type| may be either "GET", "POST", "HEAD", or "PUT";
//     if the integer form of the function is used, the request method type 
//     may be either HttpRequest.methodGet, HttpRequest.methodPost, 
//     HttpRequest.methodHead, or HttpRequest.methodPut.
//
// Param(type): The request type to which to set the request object.
//
// Returns: Returns true if the request type is set, and false otherwise.

void HttpRequest::setMethod(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() > 0)
    {
        if (env->getParam(0)->isString())
        {
            std::wstring m = env->getParam(0)->getString();
            kl::makeUpper(m);
            if (m == L"GET")
                m_method = HttpRequest::methodGet;
            else if (m == L"POST")
                m_method = HttpRequest::methodPost;
            else if (m == L"HEAD")
                m_method = HttpRequest::methodHead;
            else if (m == L"PUT")
                m_method = HttpRequest::methodPut;
            else
                return;
            
            retval->setBoolean(true);
            return;
        }
         else
        {
            int m = env->getParam(0)->getInteger();
            if (m == HttpRequest::methodGet ||
                m == HttpRequest::methodPut ||
                m == HttpRequest::methodPost ||
                m == HttpRequest::methodHead)
            {
                m_method = m;
                retval->setBoolean(true);
                return;
            }
        }
    } 
}

// (METHOD) HttpRequest.getMethod
// Description: Returns the HttpRequest method to use when issuing the request
//
// Syntax: function HttpRequest.getMethod() : Integer
//
// Remarks: This function returns the Http request method to use when issuing
//     the Http request.  One of HttpRequest.methodGet, HttpRequest.methodPost, 
//     HttpRequest.methodHead, or HttpRequest.methodPut.
//
// Returns: Returns the Http request method to use when issuing the Http
//     request.  One of HttpRequest.methodGet, HttpRequest.methodPost, 
//     HttpRequest.methodHead, or HttpRequest.methodPut.

void HttpRequest::getMethod(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(m_method);
}

// (METHOD) HttpRequest.isLoading
// Description: Returns a value indicating whether the request object is busy
//     or not.
//
// Syntax: function HttpRequest.isLoading() : Boolean
//
// Remarks: Returns a boolean value indicating whether the HttpRequest object is
//     busy.  If an asynchronous web request is currently running in the
//     background, a call to this method will return |true|, indicating that
//     the object is not ready for another request, and that the complete result
//     value is not yet ready for retrieval.
//
// Returns: True if the object is busy, false otherwise.

void HttpRequest::isLoading(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(isLoadingInternal());
}

// (METHOD) HttpRequest.isDone
// Description: Returns a value indicating whether the request is done
//
// Syntax: function HttpRequest.isDone() : Boolean
//
// Remarks: Returns a boolean value indicating whether the last http request is
//     finished.  This value is equivalent to !isLoading().
//
// Returns: True if the request is finished, false if it is still running.

void HttpRequest::isDone(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(isLoadingInternal() ? false : true);
}

// (METHOD) HttpRequest.getTotalBytes()
// Description: Returns the number of bytes in the input stream
//
// Syntax: function HttpRequest.getTotalBytes() : Number
//
// Remarks: Returns the number of remaining bytes in the input stream.  This
//     value represents the total number of bytes available to binaryRead()
//     method invocation.
//
// Returns: Returns the number of bytes remaining in the input stream.

void HttpRequest::getTotalBytes(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_response_mutex.lock();
    retval->setInteger((int)m_response_bytes);
    m_response_mutex.unlock();
}

// (METHOD) HttpRequest.getResponseText()
// Description: Returns the server response as a string object
//
// Syntax: function HttpRequest.getResponseText() : String
//
// Remarks: Returns a string object with the response text from the web request.
//
// Returns: Returns the server response as a string object.

void HttpRequest::getResponseText(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(getResponseString());
}

// (METHOD) HttpRequest.binaryRead()
// Description: Returns a MemoryBuffer object with the requested bytes
//
// Syntax: function HttpRequest.getTotalBytes(bytes : Number) : MemoryBuffer
//
// Remarks: Reads a specified number of bytes from the input stream.  The bytes
//     are returned in a MemoryBuffer object.  If the specified number of bytes
//     is not entirely available, the function returns those bytes which are
//     currently available.  If no more bytes are available, an empty MemoryBuffer
//     object is returned.
//
// Returns: Returns a MemoryBuffer object with the requested bytes

void HttpRequest::binaryRead(kscript::ExprEnv* env, kscript::Value* retval)
{
    KL_AUTO_LOCK(m_response_mutex);
    
    size_t requested_size = 0;
    
    if (env->getParamCount() > 0)
    {
        if (env->getParam(0)->getInteger() > 0)
            requested_size = (size_t)env->getParam(0)->getInteger();
    }
    
    // no bytes requested, return empty buffer
    MemoryBuffer* m = MemoryBuffer::createObject(env);
    if (requested_size > 0)
    {
        m->alloc(requested_size);
        
        unsigned char* result = m->getBuffer();
        size_t bytes_remaining = requested_size;
        size_t result_curpos = 0;
        
        std::list<HttpResponsePiece>::iterator it, it_end;
        it_end = m_response_pieces.end();
        for (it = m_response_pieces.begin(); it != it_end; ++it)
        {
            if (bytes_remaining == 0)
                break;
                
            size_t bytes_left_this_piece = it->len - it->curpos;
            size_t bytes_to_copy = bytes_left_this_piece;
            
            if (bytes_to_copy > bytes_remaining)
                bytes_to_copy = bytes_remaining;
            
            memcpy(result + result_curpos, it->buf + it->curpos, bytes_to_copy);
            it->curpos += bytes_to_copy;
            result_curpos += bytes_to_copy;
            bytes_remaining -= bytes_to_copy;
            m_response_bytes -= bytes_to_copy;
        }
        
        // delete the pieces we're finished with
        while (m_response_pieces.size() > 0)
        {
            it = m_response_pieces.begin();
            if (it->curpos >= it->len)
            {
                delete[] it->buf;
                m_response_pieces.erase(it);
            }
             else
            {
                break;
            }
        }
        
        // if the request for the specified number of bytes could not entirely
        // be fulfilled, truncate the result
        
        m->setSizeInternal(requested_size - bytes_remaining);
    }
    
    retval->setObject(m);
}

// (METHOD) HttpRequest.setBasicAuth
// Description: Sets the login and password for the HTTP request.
//
// Syntax: function HttpRequest.setBasicAuth(login : String,
//                                           password : String)
//
// Remarks: Sets the |login| and |password| for the HTTP request.
//
// Param(login): The |login| to use for the HTTP Request.
// Param(password): The |password| to use for the HTTP Request.

void HttpRequest::setBasicAuth(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    std::string username, password;
    
    if (env->getParamCount() < 1)
        return;

    if (env->getParamCount() >= 1)
        username = kl::tostring(env->getParam(0)->getString());

    if (env->getParamCount() >= 2)
        password = kl::tostring(env->getParam(1)->getString());

    // construct the login string and set the curl login and password
    m_basic_auth = username;
    m_basic_auth += ":";
    m_basic_auth += password;
    
    curl_result = curl_easy_setopt(m_curl, CURLOPT_USERPWD, m_basic_auth.c_str());
}

// (METHOD) HttpRequest.setProxy
// Description: Sets the proxy information for the HTTP request.
//
// Syntax: function HttpRequest.setProxy(location : String,
//                                       port : Integer,
//                                       login : String,
//                                       password : String)
//
// Remarks: Sets the proxy |location| and, optionally, the
//     proxy |port|, |login|, and |password|.
//
// Param(location): The |location| of the proxy server.
// Param(port): The |port| on which to connect to the proxy.
// Param(login): The |login| to use when connecting to the proxy.
// Param(password): The |password| to use when connecting to the proxy.

void HttpRequest::setProxy(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    std::string username;
    std::string password;
    int port = 8080;
    
    if (env->getParamCount() < 1)
        return;

    if (env->getParamCount() >= 1)
        m_proxy = kl::tostring(env->getParam(0)->getString());

    if (env->getParamCount() >= 2)
        m_proxy_port = env->getParam(1)->getInteger();

    if (env->getParamCount() >= 3)
        username = kl::tostring(env->getParam(2)->getString());

    if (env->getParamCount() >= 4)
        password = kl::tostring(env->getParam(3)->getString());

    // set curl proxy info
    curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXY, m_proxy.c_str());
    curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYPORT, m_proxy_port);

    // set proxy username and password
    m_proxy_auth = username;
    m_proxy_auth += ":";
    m_proxy_auth += password;
    
    curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYUSERPWD, m_proxy_auth.c_str());
}

// (METHOD) HttpRequest.setAutoEncode
// Description: Enables or disables url encoding
//
// Syntax: function HttpRequest.setAutoEncode(value : Boolean)
//
// Remarks: Sets whether the HttpRequest object should automatically
//     encode GET and POST parameters with url encoding.
//
// Param(value): True if url encoding it is to be enabled, false otherwise

void HttpRequest::setAutoEncode(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
        m_auto_encode = env->getParam(0)->getBoolean();
         else
        m_auto_encode = true;
}

// (METHOD) HttpRequest.setAsync
// Description: Turns asynchronous mode on or off
//
// Syntax: function HttpRequest.setAsync(value : Boolean)
//
// Remarks: Turns asynchronous mode on or off.  If asynchronous mode is on,
//     requests will return immediately and the processing will be done in
//     the background.  Upon completion of the request, the |finished| event
//     is fired.
//
// Param(value): Specifying |true| turns on asynchronous mode and |false|
//     turns it off

void HttpRequest::setAsync(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
        m_async = env->getParam(0)->getBoolean();
         else
        m_async = true;
}

// (METHOD) HttpRequest.setRequestHeader
// Description: Adds a header item to the list of header items to send
//     when issuing an HTTP request.
//
// Syntax: function HttpRequest.setRequestHeader(header : String)
//
// Remarks: Adds a |header| item to the list of header items to send
//     when issuing an HTTP request.
//
// Param(header): The |header| to add to the list of headers sent
//     in the HTTP request.

void HttpRequest::setRequestHeader(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    if (env->getParamCount() < 1)
        return;

    // get the input string

    std::string header = kl::tostring(env->getParam(0)->getString());

    // set the header; note: curl_slist_append creates a copy 
    // of the contents of header, and will clean up the copy in 
    // curl_slist_free_all(), which is called in clearHeaders()

    m_headers = curl_slist_append(m_headers, header.c_str());
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);
}

// (METHOD) HttpRequest.clearRequestHeaders
// Description: Clears all the header items from the list of header items to
//     send when issuing an HTTP request.
//
// Syntax: function HttpRequest.clearRequestHeaders()
//
// Remarks: Clears all the header items from the list of header items to
//     send when issuing an HTTP request.

void HttpRequest::resetRequestHeaders(kscript::ExprEnv* env, kscript::Value* retval)
{
    clearHeaders();
}

// (METHOD) HttpRequest.setPostValue
// Description: Sets a form field to a specified value.
//
// Syntax: function HttpRequest.setPostValue(field : String,
//                                           value : String) : Boolean
//
// Remarks: Sets a form |field| to a specified |value|, which 
//     will be used when issuing an HTTP POST request.
//
// Param(field): The form |field| for which to set the |value|.
// Param(value): The |value| to which to set the form |field|.
// Returns: Returns true if the form |field| was set to the specified
//     |value|, and false otherwise.

// TODO: would be great to include the memory object as one of the
//     items that can be sent, allowing raw binary information to
//     be uploaded (for bitmaps, etc).

void HttpRequest::setPostValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    std::wstring field, value;
    
    // get the input
    if (env->getParamCount() >= 1)
        field = env->getParam(0)->getString();
    if (env->getParamCount() >= 2)
        value = env->getParam(1)->getString();

    if (field.empty())
    {
        // no key value passed, return error
        retval->setBoolean(false);
        return;
    }
    
    
    if (m_auto_encode)
    {
        std::string multipart_field = kl::tostring(multipartEncode(field));
        std::string multipart_value = kl::tostring(multipartEncode(value));

        std::string escape_field = kl::tostring(kl::url_escape(field));
        std::string escape_value = kl::tostring(kl::url_escape(value));

        // append the value to our post structure (note
        // this is only used in the case of multipart posts)
        curl_formadd(&m_formfields, &m_formfieldslast,
                     CURLFORM_COPYNAME, (const char*)multipart_field.c_str(),
                     CURLFORM_COPYCONTENTS, (const char*)multipart_value.c_str(),
                     CURLFORM_END);
        
        // append the value to our post string (regular, non-multipart post)
        if (m_post_string.length() > 0)
            m_post_string += "&";
        m_post_string += escape_field;
        m_post_string += "=";
        m_post_string += escape_value;
    }
     else
    {
        // append the value to our post structure (note
        // this is only used in the case of multipart posts)
        curl_formadd(&m_formfields, &m_formfieldslast,
                     CURLFORM_COPYNAME, kl::tostring(field).c_str(),
                     CURLFORM_COPYCONTENTS, kl::tostring(value).c_str(),
                     CURLFORM_END);
        
        // append the value to our post string (regular, non-multipart post)
        if (m_post_string.length() > 0)
            m_post_string += "&";
        m_post_string += kl::tostring(field);
        m_post_string += "=";
        m_post_string += kl::tostring(value);
    }

    retval->setBoolean(true);
}

// (METHOD) HttpRequest.setPostFile
// Description: Sets a form field to a specified filename.
//
// Syntax: function HttpRequest.setPostFile(field : String,
//                                          filename : String) : Boolean
//
// Remarks: Sets a form |field| to a specified filename.
//
// Param(field): The form |field| to set.
// Param(filename): The |filename| to which to set the form |field|.
// Returns: Returns true if the form |field| was set to the specified
//     |filename|, and false otherwise.

void HttpRequest::setPostFile(kscript::ExprEnv* env, kscript::Value* retval)
{
    std::string field, fname;
    
    // get the input
    if (env->getParamCount() >= 1)
        field = kl::tostring(env->getParam(0)->getString());
    if (env->getParamCount() >= 2)
        fname = kl::tostring(env->getParam(1)->getString());

    if (field.empty())
    {
        // no key value passed, return error
        retval->setBoolean(false);
        return;
    }

    if (!xf_get_file_exist(kl::towstring(fname)))
    {
        // file doesn't exist, return error
        retval->setBoolean(false);
        return;
    }
    
    // posting a file requires multi-part
    m_post_multipart = true;
    
    curl_formadd(&m_formfields, &m_formfieldslast,
                 CURLFORM_COPYNAME, field.c_str(),
                 CURLFORM_FILE, fname.c_str(), 
                 CURLFORM_END);
                 
    retval->setBoolean(true);
}

// (METHOD) HttpRequest.setPostData
// Description: Allows the post data payload to be set directly
//
// Syntax: function HttpRequest.setPostFile(post_string : String) : Boolean
//
// Remarks: Calling setPostData() allows the post data payload to be
//     set directly, in contrast with setPostValue(), which automatically
//     constructs the standard key=value format.
//
// Param(post_string): POST method data payload
// Returns: Returns true upon success, false otherwise

void HttpRequest::setPostData(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;

    // set the referer to a particular value
    std::wstring str = env->getParam(0)->getString();
    if (m_auto_encode)
    {
        m_post_string = kl::tostring(kl::url_escape(str));
    }
     else
    {
        m_post_string = kl::tostring(str);
    }
}

// (METHOD) HttpRequest.resetPostParameters
// Description: Clears the post parameters.
//
// Syntax: function HttpRequest.resetPostParameters()
//
// Remarks: Clears the post parameters.

void HttpRequest::resetPostParameters(kscript::ExprEnv* env, kscript::Value* retval)
{
    clearFormFields();
}

// (METHOD) HttpRequest.setReferrer
// Description: Sets the referrer to use when issuing the HTTP request.
//
// Syntax: function HttpRequest.setReferrer(referrer : String)
//
// Remarks: Sets the |referrer| to use when issuing the HTTP request.
//
// Param(referrer): The |referrer| to use when issuing the HTTP request.

// note: no need to document the mispelled setReferer version of the 
// function; if people use that version, the package will simply 
// accomodate it gracefully

void HttpRequest::setReferrer(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    if (env->getParamCount() < 1)
        return;

    // set the referer to a particular value
    m_referrer = kl::tostring(env->getParam(0)->getString());
    curl_result = curl_easy_setopt(m_curl, CURLOPT_REFERER, m_referrer.c_str());
}

// (METHOD) HttpRequest.setTimeout
// Description: Sets the timeout to use when issuing the HTTP request.
//
// Syntax: function HttpRequest.setTimeout(time : Integer)
//
// Remarks: Sets the timeout |time| in seconds to use when issuing 
//     the request.  The timeout is the maximimum amount of time
//     the request object waits for the server to respond to a request
//     after a connection is established.
//
// Param(time): The timeout |time|.

void HttpRequest::setTimeout(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    // set the referer to a particular value
    int timeout = 0;
    kscript::Value* value = env->getParam(0);

    // set the timeout value if the value is a finite, positive
    // number; if it's anything besides this, the timeout value
    // will be zero, which will disable the timeout
    if (!value->isAnyInfinity() && 
        !value->isNaN() && 
        !value->isUndefined())
    {
        timeout = env->getParam(0)->getInteger();
        if (timeout < 0)
            timeout = 0;
    }

    curl_result = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, timeout);
}


// (METHOD) HttpRequest.setUserAgent
// Description: Sets the user agent to use when issuing the HTTP request.
//
// Syntax: function HttpRequest.setUserAgent(agent : String)
//
// Remarks: Sets the user |agent| to use when issuing the HTTP request.
//
// Param(agent): The |agent| to use when issuing the HTTP request.

void HttpRequest::setUserAgent(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    if (env->getParamCount() < 1)
        return;

    // set the user agent to a particular value
    m_user_agent = kl::tostring(env->getParam(0)->getString());
    curl_result = curl_easy_setopt(m_curl, CURLOPT_USERAGENT, m_user_agent.c_str());
}

// (UNDOCUMENTED.METHOD) HttpRequest.getResponseCode

void HttpRequest::getResponseCode(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    long result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &result);
    retval->setInteger((int)result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getConnectCode

void HttpRequest::getConnectCode(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    long result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_HTTP_CONNECTCODE, &result);
    retval->setInteger((int)result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getFileTime

void HttpRequest::getFileTime(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    long result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_FILETIME, &result);
    retval->setInteger((int)result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getTotalTime

void HttpRequest::getTotalTime(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_TOTAL_TIME, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getNameLookupTime

void HttpRequest::getNameLookupTime(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_NAMELOOKUP_TIME, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getConnectTime

void HttpRequest::getConnectTime(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_CONNECT_TIME, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getPreTransferTime

void HttpRequest::getPreTransferTime(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_PRETRANSFER_TIME, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getStartTransferTime

void HttpRequest::getStartTransferTime(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_STARTTRANSFER_TIME, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getRedirectTime

void HttpRequest::getRedirectTime(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_REDIRECT_TIME, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getRedirectCount

void HttpRequest::getRedirectCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    long result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_REDIRECT_COUNT, &result);
    retval->setInteger((int)result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getSizeDownload

void HttpRequest::getSizeDownload(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_SIZE_DOWNLOAD, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getSizeUpload

void HttpRequest::getSizeUpload(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_SIZE_UPLOAD, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getSpeedDownload

void HttpRequest::getSpeedDownload(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_SPEED_DOWNLOAD, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getSpeedUpload

void HttpRequest::getSpeedUpload(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_SPEED_UPLOAD, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getHeaderSize

void HttpRequest::getHeaderSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    long result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_HEADER_SIZE, &result);
    retval->setInteger((int)result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getRequestSize

void HttpRequest::getRequestSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    long result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_REQUEST_SIZE, &result);
    retval->setInteger((int)result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getSSLVerifyResult

void HttpRequest::getSSLVerifyResult(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    long result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_SSL_VERIFYRESULT, &result);
    retval->setInteger((int)result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getContentLengthDownload

void HttpRequest::getContentLengthDownload(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getContentLengthUpload

void HttpRequest::getContentLengthUpload(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    double result;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_CONTENT_LENGTH_UPLOAD, &result);
    retval->setDouble(result);
}

// (UNDOCUMENTED.METHOD) HttpRequest.getContentType

void HttpRequest::getContentType(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    char* r;
    curl_result = curl_easy_getinfo(m_curl, CURLINFO_CONTENT_TYPE, &r);
    std::string result = std::string(r);
    retval->setString(kl::towstring(result));
}

// (DEPRECATED.METHOD) HttpRequest.get
// Description: Issues an HTTP GET request.
//
// Syntax: function HttpRequest.get(location : String) : String
//
// Remarks: Issues an HTTP GET request to the specified |location|
//     and returns the results returned by the server as a string.
//
// Param(location): The URI |location| to which to issue the GET request.
// Returns: Returns a string containing the results returned by the server.
void HttpRequest::get(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isLoadingInternal())
    {
        retval->setNull();
        return;
    }

    m_method = methodGet;

    if (m_async)
    {
        m_state_mutex.lock();
        m_busy = true;
        m_state_mutex.unlock();
    
    
        retval->setNull();
        
        if (env->getParamCount() > 0)
            m_location = kl::tostring(env->getParam(0)->getString());
        
        HttpRequestThread* thread = new HttpRequestThread(this);
        if (thread->create() != 0)
            return;
    }
     else
    {
        doGet(env, retval);
    }
}

// (DEPRECATED.METHOD) HttpRequest.post
// Description: Issues an HTTP POST request.
//
// Syntax: function HttpRequest.post(location : String) : String
//
// Remarks: Issues an HTTP POST request to the specified |location|
//     and returns the results returned by the server as a string.
//     After the post operation is complete, the post parameter
//     array is automatically cleared out.
//
// Param(location): The URI |location| to which to issue the POST request.
// Returns: Returns a string containing the results returned by the server.
void HttpRequest::post(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isLoadingInternal())
    {
        retval->setNull();
        return;
    }

    m_method = methodPost;
    
    if (m_async)
    {
        m_state_mutex.lock();
        m_busy = true;
        m_state_mutex.unlock();

        retval->setNull();
        
        if (env->getParamCount() > 0)
            m_location = kl::tostring(env->getParam(0)->getString());

        HttpRequestThread* thread = new HttpRequestThread(this);
        if (thread->create() != 0)
            return;
    }
     else
    {
        doPost(env, retval);
    }
}

// (DEPRECATED.METHOD) HttpRequest.head
// Description: Issues an HTTP HEAD request.
//
// Syntax: function HttpRequest.head(location : String) : String
//
// Remarks: Issues an HTTP HEAD request to the specified |location|
//     and returns the results returned by the server as a string.
//
// Param(location): The URI |location| to which to issue the HEAD request.
// Returns: Returns a string containing the results returned by the server.
void HttpRequest::head(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_method = methodHead;

    // not yet implemented
    retval->setNull();
}

// (DEPRECATED.METHOD) HttpRequest.put
// Description: Issues an HTTP PUT request.
//
// Syntax: function HttpRequest.put(location : String) : String
//
// Remarks: Issues an HTTP PUT request to the specified |location|
//     and returns the results returned by the server as a string.
//
// Param(location): The URI |location| to which to issue the PUT request.
// Returns: Returns a string containing the results returned by the server.
void HttpRequest::put(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_method = methodPut;

    // not yet implemented
    retval->setNull();
}

void HttpRequest::doSend()
{
    CURLcode curl_result;
    
    // free any previous request
    freeResponsePieces();


    curl_result = curl_easy_setopt(m_curl, CURLOPT_URL, m_location.c_str());
    if (curl_result != CURLE_OK)
        return;


    // set the result functions
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)this);
    if (curl_result != CURLE_OK)
        return;
    
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, (void*)http_response_writer);
    if (curl_result != CURLE_OK)
        return;
        
    // result header
    std::string response_header;
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &response_header);
    if (curl_result != CURLE_OK)
        return;
        
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, (void*)http_header_writer);
    if (curl_result != CURLE_OK)
        return;

    //  get the full body 
    curl_result = curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0);
    if (curl_result != CURLE_OK)
        return;

    if (m_method == methodGet)
    { 
        // set the GET option
        curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1);
        if (curl_result != CURLE_OK)
            return;
    }
     else if (m_method == methodPost)
    {
        if (m_post_multipart)
        {
            // set the post multipart parameters
            curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, m_formfields);
            if (curl_result != CURLE_OK)
                return;
        }
         else
        {
            // set the POST option
            curl_result = curl_easy_setopt(m_curl, CURLOPT_POST, 1);
            if (curl_result != CURLE_OK)
                return;
                
            curl_result = curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, (const char*)m_post_string.c_str());
            if (curl_result != CURLE_OK)
                return;
        }
    }
     else if (m_method == methodHead)
    {
        // set the GET option
        curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1);
        if (curl_result != CURLE_OK)
            return;

        // only get the header
        curl_result = curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1);
        if (curl_result != CURLE_OK)
            return;
    }
     else if (m_method == methodPut)
    {
        // PUT not yet implemented
        return;
    }
     else
    {
        // unknown method
        return;
    }
    
    // retrieve the data from the URL
    curl_result = curl_easy_perform(m_curl);
    if (curl_result != CURLE_OK)
        return;

    m_response_mutex.lock();
    m_response_header = response_header;
    m_response_mutex.unlock();

    // reset post parameters, if any
    resetPostParameters(NULL,NULL);
}

void HttpRequest::doGet(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;
    
    // free any previous request
    freeResponsePieces();


    // see if we have an input string; if we do, set the location
    if (env->getParamCount() > 0)
        m_location = kl::tostring(env->getParam(0)->getString());
        
    curl_result = curl_easy_setopt(m_curl, CURLOPT_URL, m_location.c_str());
    if (curl_result != CURLE_OK)
        return;


    // set the result functions
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)this);
    if (curl_result != CURLE_OK)
        return;
    
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, (void*)http_response_writer);
    if (curl_result != CURLE_OK)
        return;
        
    // result header
    std::string response_header;
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &response_header);
    if (curl_result != CURLE_OK)
        return;
        
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, (void*)http_header_writer);
    if (curl_result != CURLE_OK)
        return;

    // set the GET option 
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1);
    if (curl_result != CURLE_OK)
        return;

    // get the full body
    curl_result = curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0);
    if (curl_result != CURLE_OK)
        return;
    
    // retrieve the data from the URL
    curl_result = curl_easy_perform(m_curl);
    if (curl_result != CURLE_OK)
        return;

    m_response_mutex.lock();
    m_response_header = response_header;
    m_response_mutex.unlock();

    std::wstring response = getResponseString();
    retval->setString(response);
}

void HttpRequest::doPut(kscript::ExprEnv* env, kscript::Value* retval)
{
    // free any previous request
    freeResponsePieces();
}

void HttpRequest::doHead(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    // free any previous request
    freeResponsePieces();

    // see if we have an input string; if we do, set the location
    if (env->getParamCount() > 0)
        m_location = kl::tostring(env->getParam(0)->getString());
        
    curl_result = curl_easy_setopt(m_curl, CURLOPT_URL, m_location.c_str());
    if (curl_result != CURLE_OK)
        return;

    // result string
    std::string result_string;

    // set the result functions
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, (void*)this);
    if (curl_result != CURLE_OK)
        return;
    
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, (void*)http_response_writer);
    if (curl_result != CURLE_OK)
        return;
        
    // set the GET option
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1);
    if (curl_result != CURLE_OK)
        return;

    // only get the header
    curl_result = curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1);
    if (curl_result != CURLE_OK)
        return;
    
    // retrieve the data from the URL
    curl_result = curl_easy_perform(m_curl);
    if (curl_result != CURLE_OK)
        return;

    // return the result string
    m_response_mutex.lock();
    std::wstring response = getResponseString();
    m_response_header = kl::tostring(response);
    retval->setString(response);
    m_response_mutex.unlock();
}

void HttpRequest::doPost(kscript::ExprEnv* env, kscript::Value* retval)
{
    CURLcode curl_result;

    // input and result string
    std::string result_string;

    // free any previous request
    freeResponsePieces();

    if (m_post_multipart)
    {
        // set the post multipart parameters
        curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, m_formfields);
        if (curl_result != CURLE_OK)
            return;
    }
     else
    {
        // set the POST option
        curl_result = curl_easy_setopt(m_curl, CURLOPT_POST, 1);
        if (curl_result != CURLE_OK)
            return;
            
        curl_result = curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, (const char*)m_post_string.c_str());
        if (curl_result != CURLE_OK)
            return;
    }

    // set the curl URL options to read the contents of the URL
    if (env->getParamCount() > 0)
        m_location = kl::tostring(env->getParam(0)->getString());
    curl_result = curl_easy_setopt(m_curl, CURLOPT_URL, m_location.c_str());

    // set the response header parameters
    std::string response_header;
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, (void*)&m_response_header);
    if (curl_result != CURLE_OK)
        return;
        
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, (void*)http_header_writer);
    if (curl_result != CURLE_OK)
        return;

    // set the response parameters
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)this);
    if (curl_result != CURLE_OK)
        return;
    
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, (void*)http_response_writer);
    if (curl_result != CURLE_OK)
        return;

    // retrieve the data from the URL
    curl_result = curl_easy_perform(m_curl);
    if (curl_result != CURLE_OK)
        return;

    // set the header
    m_response_mutex.lock();
    m_response_header = response_header;
    m_response_mutex.unlock();
    
    // return the result string
    std::wstring response = getResponseString();
    retval->setString(response);

    // reset post parameters
    resetPostParameters(NULL,NULL);
}

void HttpRequest::fireFinishedEvent()
{
    //TODO: reimplement
    //invokeJsEvent(L"finished");
}

bool HttpRequest::isLoadingInternal()
{
    bool result;
    m_state_mutex.lock();
    result = m_busy;
    m_state_mutex.unlock();
    return result;
}

void HttpRequest::clearHeaders()
{
    CURLcode curl_result;

    // free the headers
    if (m_headers != NULL)
    {
        curl_slist_free_all(m_headers);
        m_headers = NULL;
    }

    // reset the headers
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, NULL);
}

void HttpRequest::clearFormFields()
{
    CURLcode curl_result;

    // free the form fields
    if (m_formfields != NULL)
    {
        curl_formfree(m_formfields);
        m_formfields = NULL;
        m_formfieldslast = NULL;
    }

    // reset the form fields
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, NULL);
    curl_result = curl_easy_setopt(m_curl, CURLOPT_POST, 0);
    
    m_post_multipart = false;
    m_post_string = "";

}

static bool isUtf8(const std::string& header, const std::string& content)
{
    // poor man's detect utf-8 routine.  This really should be implemented
    // better by parsing for charset= etc.
    
    std::string h = header;
    kl::makeLower(h);
    std::string c = content.substr(0, 500);
    kl::makeLower(c);
    
    if (h.find("utf-8") != -1)
        return true;
    if (c.find("utf-8") != -1)
        return true;
    return false;
}

std::wstring HttpRequest::getResponseString()
{
    if (isLoadingInternal())
        return L"";
 
    KL_AUTO_LOCK(m_response_mutex);
    
    std::string response_a;
    
    std::list<HttpResponsePiece>::iterator it, it_end;
    it_end = m_response_pieces.end();
    for (it = m_response_pieces.begin(); it != it_end; ++it)
    {
        response_a.append((const char*)it->buf, it->len);
        if (response_a.length() > 5000000)
            break;
    }
    
    if (isUtf8(m_response_header, response_a))
        return kl::utf8_utf8towstr(response_a.c_str());
         else
        return kl::towstring(response_a);
}

void HttpRequest::freeResponsePieces()
{
    KL_AUTO_LOCK(m_response_mutex);
    
    std::list<HttpResponsePiece>::iterator it, it_end;
    it_end = m_response_pieces.end();
    for (it = m_response_pieces.begin(); it != it_end; ++it)
        delete[] it->buf;
    m_response_pieces.clear();
}




}; // namespace scripthost
