/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#include <kl/klib.h>
#include <xd/xd.h>
#include "database.h"
#include "../xdcommon/errorinfo.h"


class DatabaseMgr : public xd::IDatabaseMgr
{
    XCM_CLASS_NAME("xdduckdb.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(xd::IDatabaseMgr)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseMgr()
    {
    }

    virtual ~DatabaseMgr()
    {
    }

    xd::IDatabasePtr open(const std::wstring& connection_str)
    {
        xd::ConnectionString c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return xcm::null;
        
        // check if the provider refers to us, or a different dll/shared lib
        if (provider != L"xdduckdb")
        {
            return xcm::null;
        }
        
        // parse the connection string
        
        std::wstring database = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        

        DuckdbDatabase* db = new DuckdbDatabase;
        db->ref();

        if (!db->openDatabase(database, uid, password))
        {
            m_error.setError(db->getErrorCode(), db->getErrorString());

            db->unref();
            return xcm::null;
        }

        return xd::IDatabasePtr(db, false);
    }
    
    bool createDatabase(const std::wstring& connection_str)
    {
        xd::ConnectionString c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return false;
        
        // check if the provider refers to us, or a different dll/shared lib
        if (provider != L"xdduckdb")
            return false;


        std::wstring location = c.getValue(L"database");

        DuckdbDatabase* db = new DuckdbDatabase;
        db->ref();

        if (!db->createDatabase(location))
        {
            db->unref();
            return false;
        }

        db->unref();
        return true;
    }
    
    xd::DatabaseEntryEnum getDatabaseList(const std::wstring& host,
                                          int port,
                                          const std::wstring& uid,
                                          const std::wstring& password)
    {
        return xd::DatabaseEntryEnum();
    }

    std::wstring getErrorString()
    {
        return L"";
    }

    int getErrorCode()
    {
        return 0;
    }


private:

    ThreadErrorInfo m_error;

};




// these are the publicly creatable classes

XCM_BEGIN_DYNAMIC_MODULE(xdduckdb)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdduckdb)


