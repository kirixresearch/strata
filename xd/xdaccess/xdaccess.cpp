/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-01-10
 *
 */


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/errorinfo.h"


class DatabaseMgr : public tango::IDatabaseMgr
{
    XCM_CLASS_NAME("xdaccess.DatabaseMgr")
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
        
        // -- check if the provider is xdaccess, or in a different DLL --
        if (provider != L"xdaccess")
        {
            return xcm::null;
        }
        
        
        
        // -- parse the connection string --
        
        std::wstring location = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        

        AccessDatabase* db = new AccessDatabase;
        db->ref();

        if (!db->open(tango::dbtypeAccess, L"", 0, L"", uid, password, location))
        {
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

    tango::IDatabaseEntryEnumPtr getDatabaseList(const std::wstring& host,
                                                 int port,
                                                 const std::wstring& uid,
                                                 const std::wstring& password)
    {
        return xcm::null;
    }


    std::wstring getErrorString()
    {
        return m_error.getErrorString();
    }

    int getErrorCode()
    {
        return m_error.getErrorCode();
    }


private:

    ThreadErrorInfo m_error;
};




// -- these are the publicly creatable classes --

XCM_BEGIN_DYNAMIC_MODULE(xdaccess)
    XCM_BEGIN_CLASS_MAP()
	    XCM_CLASS_ENTRY(DatabaseMgr)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdaccess)


