/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/errorinfo.h"


class DatabaseMgr : public tango::IDatabaseMgr
{
    XCM_CLASS_NAME("xdhttp.DatabaseMgr")
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
        
        // check if the provider is xdnative, or in a different DLL
        if (provider != L"xdhttp")
        {
            return xcm::null;
        }
        
        // parse the connection string
        std::wstring host = c.getValue(L"host");
        std::wstring database = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        

        HttpDatabase* db = new HttpDatabase;
        db->ref();

        if (!db->open(host, database, uid, password))
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
        return xcm::null;
    }

    bool createDatabase(const std::wstring& location, int db_type)
    {
        return false;
    }
    
    tango::IDatabasePtr openLocalDatabase(const std::wstring& location,
                                          const std::wstring& uid,
                                          const std::wstring& password)
    {
        return xcm::null;
    }

    tango::IDatabasePtr openRemoteDatabase(int type, const std::wstring& host, 
                                           int port, const std::wstring& database,
                                           const std::wstring& uid,
                                           const std::wstring& password)
    {
        HttpDatabase* db = new HttpDatabase;
        db->ref();

        if (!db->open(host, database, uid, password))
        {
            db->unref();
            return xcm::null;
        }

        return tango::IDatabasePtr(db, false);
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




// these are the publicly creatable classes

XCM_BEGIN_DYNAMIC_MODULE(xdhttp)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdhttp)


