/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-04-18
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include <oci.h>
#include "database.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/errorinfo.h"
#include <kl/string.h>


class DatabaseMgr : public tango::IDatabaseMgr
{
    XCM_CLASS_NAME("xdoracle.DatabaseMgr")
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
        tango::ConnectionStringParser c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return xcm::null;
        
        // check if the provider is xdnative, or in a different DLL
        if (provider != L"xdoracle")
        {
            return xcm::null;
        }
        
        // parse the connection string
        
        std::wstring host = c.getValue(L"host");
        std::wstring port = c.getValue(L"port");
        std::wstring database = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        
        int port_number = 1521;
        
        if (!port.empty())
            port_number = kl::wtoi(port);

        
        OracleDatabase* db = new OracleDatabase;
        db->ref();

        if (!db->open(host, port_number, database, uid, password))
        {
            m_error.setError(db->getErrorCode(), db->getErrorString());

            db->unref();
            return xcm::null;
        }

        return tango::IDatabasePtr(db, false);
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
        OracleDatabase* db = new OracleDatabase;
        db->ref();

        // remove leading '\\', if necessary

        std::wstring host_fixed;
        if (wcsncmp(host.c_str(), L"\\\\", 2) == 0)
        {
            host_fixed = host.c_str() + 2;
        }
         else
        {
            host_fixed = host;
        }

        // attempt to open the database

        if (!db->open(host_fixed, port, database, uid, password))
        {
            m_error.setError(db->getErrorCode(), db->getErrorString());

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
        xcm::IVectorImpl<tango::IDatabaseEntryPtr>* vec = new xcm::IVectorImpl<tango::IDatabaseEntryPtr>;
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

XCM_BEGIN_DYNAMIC_MODULE(xdoracle)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
        XCM_CLASS_ENTRY(OracleDatabase)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdoracle)

