/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-04-27
 *
 */


#ifndef __APP_CONNECTIONMGR_H
#define __APP_CONNECTIONMGR_H



xcm_interface IConnection : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IConnection")

public:

    virtual std::wstring getConnectionString() = 0;

    virtual bool open() = 0;
    virtual bool isOpen() = 0;

    virtual int getType() = 0;
    virtual void setType(int type) = 0;

    virtual std::wstring getDescription() = 0;
    virtual void setDescription(const std::wstring& new_val) = 0;

    virtual std::wstring getErrorString() = 0;
    
    // used for local connections

    virtual std::wstring getPath() = 0;
    virtual void setPath(const std::wstring& path) = 0;

    virtual std::wstring getFilter() = 0;
    virtual void setFilter(const std::wstring& filter) = 0;

    // used for remote connections

    virtual std::wstring getHost() = 0;
    virtual void setHost(const std::wstring& new_val) = 0;

    virtual int getPort() = 0;
    virtual void setPort(int new_val) = 0;

    virtual std::wstring getDatabase() = 0;
    virtual void setDatabase(const std::wstring& new_val) = 0;

    virtual std::wstring getUsername() = 0;
    virtual void setUsername(const std::wstring& new_val) = 0;

    virtual std::wstring getPassword() = 0;
    virtual void setPassword(const std::wstring& new_val) = 0;

    virtual xd::IDatabasePtr getDatabasePtr() = 0;
    virtual void setDatabasePtr(xd::IDatabasePtr new_val) = 0;
};

XCM_DECLARE_SMARTPTR(IConnection)



IConnectionPtr createUnmanagedConnection();



#endif



