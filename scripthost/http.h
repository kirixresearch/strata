/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-05
 *
 */


#ifndef H_SCRIPTHOST_SCRIPTHTTP_H
#define H_SCRIPTHOST_SCRIPTHTTP_H


#include <list>


struct curl_slist;
struct curl_httppost;

namespace scripthost
{


class HttpResponsePiece
{
public:
    unsigned char* buf;
    size_t curpos;
    size_t len;
};


class HttpRequest : public ScriptHostBase
{
    friend class HttpRequestThread;
    
    BEGIN_KSCRIPT_CLASS("HttpRequest", HttpRequest)     
        KSCRIPT_METHOD("constructor", HttpRequest::constructor)
        
        KSCRIPT_METHOD("send", HttpRequest::send)
        
        KSCRIPT_METHOD("setUrl", HttpRequest::setUrl)
        KSCRIPT_METHOD("getUrl", HttpRequest::getUrl)
        
        KSCRIPT_METHOD("setMethod", HttpRequest::setMethod)
        KSCRIPT_METHOD("getMethod", HttpRequest::getMethod)
        
        KSCRIPT_METHOD("isLoading", HttpRequest::isLoading)
        KSCRIPT_METHOD("isDone", HttpRequest::isDone)
        
        KSCRIPT_METHOD("getTotalBytes", HttpRequest::getTotalBytes)
        KSCRIPT_METHOD("binaryRead", HttpRequest::binaryRead)
        KSCRIPT_METHOD("getResponseText", HttpRequest::getResponseText)
        
        KSCRIPT_METHOD("setBasicAuth", HttpRequest::setBasicAuth)
        KSCRIPT_METHOD("setProxy", HttpRequest::setProxy)
        KSCRIPT_METHOD("setAutoEncode", HttpRequest::setAutoEncode)
        KSCRIPT_METHOD("setAsync", HttpRequest::setAsync)
        
        KSCRIPT_METHOD("setRequestHeader", HttpRequest::setRequestHeader)
        KSCRIPT_METHOD("resetRequestHeaders", HttpRequest::resetRequestHeaders)
        
        KSCRIPT_METHOD("setPostValue", HttpRequest::setPostValue)
        KSCRIPT_METHOD("setPostFile", HttpRequest::setPostFile)
        KSCRIPT_METHOD("setPostData", HttpRequest::setPostData)
        KSCRIPT_METHOD("resetPostParameters", HttpRequest::resetPostParameters)
        
        KSCRIPT_METHOD("setPutFile", HttpRequest::setPutFile)

        KSCRIPT_METHOD("setReferer", HttpRequest::setReferrer) // common misspelling that's in the HTTP spec
        KSCRIPT_METHOD("setReferrer", HttpRequest::setReferrer)
        KSCRIPT_METHOD("setTimeout", HttpRequest::setTimeout)
        KSCRIPT_METHOD("setUserAgent", HttpRequest::setUserAgent)
        
        // -- below are not 'officially' supported yet --

        KSCRIPT_METHOD("getResponseCode", HttpRequest::getResponseCode)
        KSCRIPT_METHOD("getConnectCode", HttpRequest::getConnectCode)
        KSCRIPT_METHOD("getFileTime", HttpRequest::getFileTime)
        KSCRIPT_METHOD("getTotalTime", HttpRequest::getTotalTime)
        KSCRIPT_METHOD("getNameLookupTime", HttpRequest::getNameLookupTime)
        KSCRIPT_METHOD("getConnectTime", HttpRequest::getConnectTime)
        KSCRIPT_METHOD("getPreTransferTime", HttpRequest::getPreTransferTime)
        KSCRIPT_METHOD("getStartTransferTime", HttpRequest::getStartTransferTime)
        KSCRIPT_METHOD("getRedirectTime", HttpRequest::getRedirectTime)
        KSCRIPT_METHOD("getRedirectCount", HttpRequest::getRedirectCount)
        KSCRIPT_METHOD("getSizeDownload", HttpRequest::getSizeDownload)
        KSCRIPT_METHOD("getSizeUpload", HttpRequest::getSizeUpload)
        KSCRIPT_METHOD("getSpeedDownload", HttpRequest::getSpeedDownload)
        KSCRIPT_METHOD("getSpeedUpload", HttpRequest::getSpeedUpload)
        KSCRIPT_METHOD("getHeaderSize", HttpRequest::getHeaderSize)
        KSCRIPT_METHOD("getRequestSize", HttpRequest::getRequestSize)
        KSCRIPT_METHOD("getSSLVerifyResult", HttpRequest::getSSLVerifyResult)
        KSCRIPT_METHOD("getContentLengthDownload", HttpRequest::getContentLengthDownload)
        KSCRIPT_METHOD("getContentLengthUpload", HttpRequest::getContentLengthUpload)
        KSCRIPT_METHOD("getContentType", HttpRequest::getContentType)
        
        // these are officially deprecated
        KSCRIPT_METHOD("get", HttpRequest::get)
        KSCRIPT_METHOD("put", HttpRequest::put)
        KSCRIPT_METHOD("head", HttpRequest::head)
        KSCRIPT_METHOD("post", HttpRequest::post)
    END_KSCRIPT_CLASS()

public:
  
    enum
    {
        methodGet = 1,
        methodPost = 2,
        methodPut = 3,
        methodHead = 4
    };
    
public:

    HttpRequest();
    ~HttpRequest();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

    void send(kscript::ExprEnv* env, kscript::Value* retval);
    void get(kscript::ExprEnv* env, kscript::Value* retval);
    void put(kscript::ExprEnv* env, kscript::Value* retval);
    void head(kscript::ExprEnv* env, kscript::Value* retval);
    void post(kscript::ExprEnv* env, kscript::Value* retval);

    void setUrl(kscript::ExprEnv* env, kscript::Value* retval);
    void getUrl(kscript::ExprEnv* env, kscript::Value* retval);

    void setMethod(kscript::ExprEnv* env, kscript::Value* retval);
    void getMethod(kscript::ExprEnv* env, kscript::Value* retval);

    void getTotalBytes(kscript::ExprEnv* env, kscript::Value* retval);
    void binaryRead(kscript::ExprEnv* env, kscript::Value* retval);
    void getResponseText(kscript::ExprEnv* env, kscript::Value* retval);

    void isLoading(kscript::ExprEnv* env, kscript::Value* retval);
    void isDone(kscript::ExprEnv* env, kscript::Value* retval);

    void setBasicAuth(kscript::ExprEnv* env, kscript::Value* retval);
    void setProxy(kscript::ExprEnv* env, kscript::Value* retval);
    void setAutoEncode(kscript::ExprEnv* env, kscript::Value* retval);
    void setAsync(kscript::ExprEnv* env, kscript::Value* retval);
    
    void setRequestHeader(kscript::ExprEnv* env, kscript::Value* retval);
    void resetRequestHeaders(kscript::ExprEnv* env, kscript::Value* retval);

    void setPostValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setPostFile(kscript::ExprEnv* env, kscript::Value* retval);
    void setPostData(kscript::ExprEnv* env, kscript::Value* retval);
    void resetPostParameters(kscript::ExprEnv* env, kscript::Value* retval);

    void setPutFile(kscript::ExprEnv* env, kscript::Value* retval);

    void setReferrer(kscript::ExprEnv* env, kscript::Value* retval);
    void setTimeout(kscript::ExprEnv* env, kscript::Value* retval);
    void setUserAgent(kscript::ExprEnv* env, kscript::Value* retval);

    void getResponseCode(kscript::ExprEnv* env, kscript::Value* retval);
    void getConnectCode(kscript::ExprEnv* env, kscript::Value* retval);
    void getFileTime(kscript::ExprEnv* env, kscript::Value* retval);
    void getTotalTime(kscript::ExprEnv* env, kscript::Value* retval);
    void getNameLookupTime(kscript::ExprEnv* env, kscript::Value* retval);
    void getConnectTime(kscript::ExprEnv* env, kscript::Value* retval);
    void getPreTransferTime(kscript::ExprEnv* env, kscript::Value* retval);
    void getStartTransferTime(kscript::ExprEnv* env, kscript::Value* retval);
    void getRedirectTime(kscript::ExprEnv* env, kscript::Value* retval);
    void getRedirectCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getSizeDownload(kscript::ExprEnv* env, kscript::Value* retval);
    void getSizeUpload(kscript::ExprEnv* env, kscript::Value* retval);
    void getSpeedDownload(kscript::ExprEnv* env, kscript::Value* retval);
    void getSpeedUpload(kscript::ExprEnv* env, kscript::Value* retval);
    void getHeaderSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getRequestSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getSSLVerifyResult(kscript::ExprEnv* env, kscript::Value* retval);
    void getContentLengthDownload(kscript::ExprEnv* env, kscript::Value* retval);
    void getContentLengthUpload(kscript::ExprEnv* env, kscript::Value* retval);
    void getContentType(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    void doSend();
    void doGet(kscript::ExprEnv* env, kscript::Value* retval);
    void doPut(kscript::ExprEnv* env, kscript::Value* retval);
    void doHead(kscript::ExprEnv* env, kscript::Value* retval);
    void doPost(kscript::ExprEnv* env, kscript::Value* retval);
    
    void fireFinishedEvent();
    bool isLoadingInternal();
    
    void clearHeaders();
    void clearFormFields();

    std::wstring getResponseString();
    void freeResponsePieces();
    
private:

    static size_t http_header_writer(void* ptr, size_t size, size_t nmemb, void *stream);
    static size_t http_response_writer(void* ptr, size_t size, size_t nmemb, void *stream);

private:

    void* m_curl;
    
    kscript::Value m_event_callback;
    
    // -- curl headers and form fields --
    curl_slist* m_headers;

    std::string m_location;
    std::string m_referrer;
    std::string m_user_agent;
    std::string m_basic_auth;
    std::string m_proxy;
    std::string m_proxy_auth;
    long m_proxy_port;
    int m_method;
    
    kl::mutex m_response_mutex;
    std::string m_response_header;
    std::list<HttpResponsePiece> m_response_pieces;
    size_t m_response_bytes;
    
    kl::mutex m_state_mutex;
    bool m_busy;
    
    curl_httppost* m_formfields;      // only used by multipart post
    curl_httppost* m_formfieldslast;  // only used by multipart post
    std::string m_post_string;        // only used by regular post
    std::wstring m_put_file;          // only used by put
    bool m_post_multipart;
    bool m_auto_encode;
    bool m_async;
};



}; // namespace scripthost


#endif

