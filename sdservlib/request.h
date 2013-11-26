/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2009-05-22
 *
 */


#ifndef __APP_REQUEST_H
#define __APP_REQUEST_H



class RequestFileInfo
{
public:

    bool isOk() const { return post_filename.empty() ? false : true; }

    std::wstring post_filename;
    std::wstring temp_filename;
};


class RequestInfo
{
public:

    virtual std::wstring getURI() = 0;
    virtual std::wstring getValue(const std::wstring& key, const std::wstring& def = L"") = 0;
    virtual bool getValueExists(const std::wstring& key) = 0;
    virtual void setValue(const std::wstring& key, const std::wstring& value) = 0;

    virtual int getContentLength() = 0;

    virtual RequestFileInfo getPostFileInfo(const std::wstring& key) = 0;
    
    virtual void setContentType(const char* content_type) = 0;
    virtual size_t write(const void* ptr, size_t length) = 0;
    virtual size_t write(const std::string& str) = 0;
    virtual size_t write(const std::wstring& str) = 0;

    virtual void sendNotFoundError() = 0;
};




#endif

