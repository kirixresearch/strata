/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-03-17
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <tds.h>
#include <ctpublic.h>
#include "tango.h"
#include "database.h"


class DatabaseMgr : public tango::IDatabaseMgr
{
    XCM_CLASS_NAME("xdsqlserver.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(tango::IDatabaseMgr)
    XCM_END_INTERFACE_MAP()

public:

    tango::IDatabasePtr createDatabase(const std::wstring& location,
                                       const std::wstring& dbname)
    {
        return xcm::null;
    }

    bool createDatabase(const std::wstring& location, int db_type)
    {
        return false;
    }
    
    tango::IDatabasePtr open(const std::wstring& connection_str)
    {
        return xcm::null;
    }
    
    tango::IDatabasePtr openLocalDatabase(const std::wstring&,
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
        SqlServerDatabase* db = new SqlServerDatabase;
        db->ref();

        // -- remove leading '\\', if necessary --

        std::wstring host_fixed;
        if (wcsncmp(host.c_str(), L"\\\\", 2) == 0)
        {
            host_fixed = host.c_str() + 2;
        }
         else
        {
            host_fixed = host;
        }

        // -- attempt to open the database --

        if (!db->open(host_fixed, port, database, uid, password))
        {
            db->unref();
            return xcm::null;
        }

        return tango::IDatabasePtr(db, false);
    }

    tango::IDatabaseEntryEnumPtr getDatabaseList(const std::wstring& host, int port,
                                                 const std::wstring& uid,
                                                 const std::wstring& password)
    {
        xcm::IVectorImpl<tango::IDatabaseEntryPtr>* vec = new xcm::IVectorImpl<tango::IDatabaseEntryPtr>;
        return vec;
    }

    std::wstring getErrorString()
    {
        return L"";
    }

    int getErrorCode()
    {
        return 0;
    }

};




// -- these are the publicly creatable classes --

XCM_BEGIN_DYNAMIC_MODULE(xdsqlserver)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
        XCM_CLASS_ENTRY(SqlServerDatabase)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdsqlserver)

