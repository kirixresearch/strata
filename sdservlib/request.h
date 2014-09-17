/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2009-05-22
 *
 */


#ifndef __SDSERVLIB_REQUEST_H
#define __SDSERVLIB_REQUEST_H


class PostValueBase
{
public:

    virtual ~PostValueBase() { }

    virtual void setName(const std::wstring& value) { m_name = value; }
    virtual const std::wstring& getName() { return m_name; }

    virtual void setFilename(const std::wstring& value) { m_filename = value; }
    virtual const std::wstring& getFilename() { return m_filename; }

    virtual void setTempFilename(const std::wstring& value) { m_temp_filename = value; }
    virtual const std::wstring& getTempFilename() { return m_temp_filename; }

    virtual unsigned char* getData() { return NULL; } // not implemented by PostValueFile
    virtual size_t getDataSize() { return 0; }

    virtual void start() = 0;
    virtual bool append(const unsigned char* buf, size_t len) = 0;
    virtual void finish() = 0;

protected:

    std::wstring m_name;
    std::wstring m_filename;
    std::wstring m_temp_filename;
};


class RequestFileInfo
{
public:

    bool isOk() const { return post_filename.empty() ? false : true; }

    std::wstring post_filename;
    std::wstring temp_filename;
};



class PostHookBase
{
public:

    virtual ~PostHookBase() { }

    virtual PostValueBase* onPostValue(const std::wstring& key, const std::wstring& filename) = 0;
};


class RequestInfo
{
public:

    virtual std::wstring getURI() = 0;
    virtual std::wstring getGetValue(const std::wstring& key) = 0;
    virtual std::wstring getValue(const std::wstring& key, const std::wstring& def = L"") = 0;
    virtual bool getValueExists(const std::wstring& key) = 0;
    virtual RequestFileInfo getPostFileInfo(const std::wstring& key) = 0;
    virtual int getContentLength() = 0;
    virtual void setPostHook(PostHookBase* hook) = 0;
    virtual void readPost() = 0;
    virtual bool isMultipart() = 0;

    virtual void setValue(const std::wstring& key, const std::wstring& value) = 0;
    virtual void setContentType(const char* content_type) = 0;
    virtual void setContentLength(int length) = 0;
    virtual size_t writePiece(const void* ptr, size_t length) = 0;
    virtual size_t write(const void* ptr, size_t length) = 0;
    virtual size_t write(const std::string& str) = 0;
    virtual size_t write(const std::wstring& str) = 0;

    virtual void sendNotFoundError() = 0;
};




#endif

