/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
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
#include <xd/xd.h>
#include "database.h"


class DatabaseMgr : public xd::IDatabaseMgr
{
    XCM_CLASS_NAME("xdsqlserver.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(xd::IDatabaseMgr)
    XCM_END_INTERFACE_MAP()

public:


    bool createDatabase(const std::wstring& connection_str)
    {
        return false;
    }
    
    xd::IDatabasePtr open(const std::wstring& connection_str)
    {
/*
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

        return xd::IDatabasePtr(db, false);
        */

        return xcm::null;
    }

    xd::IDatabaseEntryEnumPtr getDatabaseList(const std::wstring& host, int port,
                                                 const std::wstring& uid,
                                                 const std::wstring& password)
    {
        xcm::IVectorImpl<xd::IDatabaseEntryPtr>* vec = new xcm::IVectorImpl<xd::IDatabaseEntryPtr>;
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

