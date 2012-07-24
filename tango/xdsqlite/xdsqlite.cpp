/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/errorinfo.h"


class DatabaseMgr : public tango::IDatabaseMgr
{
    XCM_CLASS_NAME("xdsqlite.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(tango::IDatabaseMgr)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseMgr()
    {
    }

    tango::IDatabasePtr open(const std::wstring& connection_str)
    {
        xdcommon::ConnectionStr c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return xcm::null;
        
        // -- check if the provider is xdnative, or in a different DLL --
        if (provider != L"xdsqlite")
        {
            return xcm::null;
        }
        
        // -- parse the connection string --
        
        std::wstring database = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        

        SlDatabase* db = new SlDatabase;
        db->ref();

        if (!db->openDatabase(database, uid, password))
        {
            m_error.setError(db->getErrorCode(), db->getErrorString());

            db->unref();
            return xcm::null;
        }

        return tango::IDatabasePtr(db, false);
    }
    
    tango::IDatabasePtr createDatabase(const std::wstring& location,
                                       const std::wstring& dbname)
    {
        SlDatabase* db = new SlDatabase;
        db->ref();

        if (!db->createDatabase(location, dbname))
        {
            db->unref();
            return xcm::null;
        }

        return tango::IDatabasePtr(db, false);
    }

    bool createDatabase(const std::wstring& location, int db_type)
    {
        return false;
    }
    
    tango::IDatabasePtr openLocalDatabase(const std::wstring& location,
                                          const std::wstring& uid,
                                          const std::wstring& password)
    {
        SlDatabase* db = new SlDatabase;
        db->ref();

        if (!db->openDatabase(location,
                              uid,
                              password))
        {
            db->unref();
            return xcm::null;
        }

        return tango::IDatabasePtr(db, false);
    }

    tango::IDatabasePtr openRemoteDatabase(int type, const std::wstring& host, 
                                           int port, const std::wstring& database,
                                           const std::wstring& uid,
                                           const std::wstring& password)
    {
        return xcm::null;
    }

    tango::IDatabaseEntryEnumPtr getDatabaseList(const std::wstring& host,
                                                 int port,
                                                 const std::wstring& uid,
                                                 const std::wstring& password)
    {
        return xcm::null;
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




// -- these are the publicly creatable classes --

XCM_BEGIN_DYNAMIC_MODULE(xdsqlite)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdsqlite)


