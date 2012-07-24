/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-10
 *
 */


#include "dbmgr.h"
#include "database.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/util.h"
#include <kl/regex.h>
#include <kl/portable.h>


tango::IDatabasePtr DatabaseMgr::createDatabase(const std::wstring& location,
                                                const std::wstring& dbname)
{
    // create the database class

    Database* db = new Database;
    db->ref();

    if (!db->createDatabase(dbname, location))
    {
        delete db;
        return xcm::null;
    }

    return tango::IDatabasePtr(db, false);
}


bool DatabaseMgr::createDatabase(const std::wstring& location, int db_type)
{
    std::string forward;
    
    switch (db_type)
    {
        case tango::dbtypeXdnative:
        {
            // get name of database
            std::wstring dbname,s2;
            dbname = kl::afterLast(location, L'\\');
            s2 = kl::afterLast(location, L'/');
            if (dbname.length() > s2.length())
                dbname = s2;
                
            Database* db = new Database;
            db->ref();

            if (!db->createDatabase(dbname, location))
            {
                db->unref();
                return false;
            }

            db->unref();
            return true;
        }
        
        case tango::dbtypeAccess:   forward = "xdodbc.DatabaseMgr"; break;
        case tango::dbtypeExcel:    forward = "xdodbc.DatabaseMgr"; break;
        case tango::dbtypeSqlite:   forward = "xdsqlite.DatabaseMgr"; break;
    }
    
    if (forward.length() > 0)
    {
        tango::IDatabaseMgrPtr dbmgr;
        dbmgr.create_instance(forward.c_str());
        if (dbmgr.isOk())
        {
            return dbmgr->createDatabase(location, db_type);
        }
    }
    

    return false;
}
    
tango::IDatabasePtr DatabaseMgr::open(const std::wstring& _connection_str)
{
    m_error.clearError();

    std::wstring connection_str = xdcommon::urlToConnectionStr(_connection_str);
    
    if (connection_str.length() == 0)
    {
        // connection string passed directly, not url
        connection_str = _connection_str;
    }


    xdcommon::ConnectionStr c(connection_str);
    std::wstring provider = c.getLowerValue(L"xdprovider");
    if (provider.empty())
        return xcm::null;
    
    // check if the provider is xdnative, or in a different DLL
    if (provider != L"xdnative")
    {
        // the provider is some other tango DLL; we need to
        // load it and pass the connection string to it
        std::string dbmgr_class = kl::tostring(provider);
        dbmgr_class += ".DatabaseMgr";
        
        tango::IDatabaseMgrPtr dbmgr;
        if (!dbmgr.create_instance(dbmgr_class.c_str()))
        {
            wchar_t buf[255];
            swprintf(buf, 254, L"The driver component '%ls' could either not be initialized or is missing. %hs", provider.c_str(), xcm::get_last_error());
            m_error.setError(tango::errorGeneral, buf);
            return xcm::null;
        }
        
        tango::IDatabasePtr res_db = dbmgr->open(connection_str);
        
        m_error.setError(dbmgr->getErrorCode(), dbmgr->getErrorString());

        return res_db;
    }
    
    // parse the connection string
    
    std::wstring location = c.getValue(L"database");
    std::wstring uid = c.getValue(L"user id");
    std::wstring password = c.getValue(L"password");
        
    Database* db = new Database;
    db->ref();

    if (!db->openDatabase(location, uid, password))
    {
        m_error.setError(db->getErrorCode(), db->getErrorString());
        delete db;
        return xcm::null;
    }

    return tango::IDatabasePtr(db, false);
}

tango::IDatabaseEntryEnumPtr DatabaseMgr::getDatabaseList(const std::wstring& host,
                                                          int port,
                                                          const std::wstring& uid,
                                                          const std::wstring& password)
{
    xcm::IVectorImpl<tango::IDatabaseEntryPtr>* vec = new xcm::IVectorImpl<tango::IDatabaseEntryPtr>;
    return vec;
}


std::wstring DatabaseMgr::getErrorString()
{
    return m_error.getErrorString();
}

int DatabaseMgr::getErrorCode()
{
    return m_error.getErrorCode();
}




void DatabaseMgr::bindExprParser(void* parser)
{
    ::bindExprParser((kscript::ExprParser*)parser);
}

