/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-04-16
 *
 */


#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif


#include "mysql.h"

#include "tango.h"
#include "xdutil.h"

#include "database.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/errorinfo.h"
#include <xcm/xcm.h>
#include <kl/string.h>


class DatabaseMgr : public xd::IDatabaseMgr
{
    XCM_CLASS_NAME("xdmysql.DatabaseMgr")
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
        xd::ConnectionStringParser c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return xcm::null;
        
        // check if the provider refers to us, or a different dll/shared lib
        if (provider != L"xdmysql")
        {
            return xcm::null;
        }
        
        // parse the connection string
        
        std::wstring host = c.getValue(L"host");
        std::wstring port = c.getValue(L"port");
        std::wstring database = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        

        MysqlDatabase* db = new MysqlDatabase;
        db->ref();

        if (!db->open(host, kl::wtoi(port), database, uid, password))
        {
            m_error.setError(db->getErrorCode(), db->getErrorString());

            db->unref();
            return xcm::null;
        }

        return xd::IDatabasePtr(db, false);
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
        return m_error.getErrorString();
    }

    int getErrorCode()
    {
        return m_error.getErrorCode();
    }
    
public:

    ThreadErrorInfo m_error;
};




// these are the publicly creatable classes

XCM_BEGIN_DYNAMIC_MODULE(xdmysql)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
        XCM_CLASS_ENTRY(MysqlDatabase)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdmysql)


