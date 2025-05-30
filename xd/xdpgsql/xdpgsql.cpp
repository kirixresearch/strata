/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */



#include "libpq-fe.h"
#include <xd/xd.h>
#include "database.h"
#include "../xdcommon/errorinfo.h"
#include <kl/string.h>
#include <kl/utf8.h>



class DatabaseMgr : public xd::IDatabaseMgr
{
    XCM_CLASS_NAME("xdpgsql.DatabaseMgr")
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
        if (provider != L"xdpgsql")
        {
            return xcm::null;
        }
        
        // parse the connection string
        
        std::wstring host = c.getValue(L"host");
        std::wstring port = c.getValue(L"port");
        std::wstring database = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        

        PgsqlDatabase* db = new PgsqlDatabase;
        db->ref();

        if (!db->open(host, kl::wtoi(port), database, uid, password))
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
        if (provider != L"xdpgsql")
            return false;


        // parse the connection string
        
        std::wstring host = c.getValue(L"host");
        std::wstring port = c.getValue(L"port");
        std::wstring database = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        

        if (port.empty() || port == L"0")
            port = L"5432"; // default pgsql port;

        std::wstring pgcmd;
        pgcmd += L"host='" + host + L"'";
        pgcmd += L" port='" + port + L"'";
        pgcmd += L" dbname='template1'";
        pgcmd += L" user='" + uid + L"'";
        pgcmd += L" password='" + password + L"'";

        PGconn* conn = PQconnectdb(kl::toUtf8(pgcmd));
        if (!conn)
            return NULL;

        if (PQstatus(conn) != CONNECTION_OK)
        {
            m_error.setError(xd::errorGeneral, kl::towstring(PQerrorMessage(conn)));

            PQfinish(conn);
            return NULL;
        }


        pgcmd = L"CREATE DATABASE " + pgsqlQuoteIdentifierIfNecessary(database) + L" ENCODING 'UTF8'";
        PGresult* res = PQexec(conn, kl::toUtf8(pgcmd));
        if (!res)
            return false;

        int result_status = PQresultStatus(res);

        PQclear(res);
        PQfinish(conn);

        return (result_status == PGRES_COMMAND_OK ? true : false);
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




// -- these are the publicly creatable classes --

XCM_BEGIN_DYNAMIC_MODULE(xdpgsql)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdpgsql)


