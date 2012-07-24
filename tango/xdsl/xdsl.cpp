/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-01-10
 *
 */


#include <kl/klib.h>
#include "tango.h"
#include "database.h"


class DatabaseMgr : public tango::IDatabaseMgr
{
    XCM_CLASS_NAME("xdsl.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
	    XCM_INTERFACE_ENTRY(tango::IDatabaseMgr)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseMgr()
    {
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

        if (!db->openDatabase(tango::dbtypeAccess,
                              L"",
                              0,
                              L"",
                              uid,
                              password,
                              location))
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

};




// -- these are the publicly creatable classes --

XCM_BEGIN_DYNAMIC_MODULE(xdsl)
    XCM_BEGIN_CLASS_MAP()
	    XCM_CLASS_ENTRY(DatabaseMgr)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdsl)


