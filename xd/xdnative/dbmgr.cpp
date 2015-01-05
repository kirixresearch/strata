/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
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


bool DatabaseMgr::createDatabase(const std::wstring& connection_str)
{
    xd::ConnectionString c(connection_str);
    std::wstring provider = c.getLowerValue(L"xdprovider");
    if (provider.empty())
        return false;
        
    // check if the provider refers to us, or a different dll/shared lib
    if (provider == L"xdnative")
    {
        std::wstring location = c.getValue(L"database");

        // get name of database
        std::wstring dbname,s2;
        dbname = kl::afterLast(location, L'\\');
        s2 = kl::afterLast(location, L'/');
        if (dbname.length() > s2.length())
            dbname = s2;
                
        XdnativeDatabase* db = new XdnativeDatabase;
        db->ref();

        if (!db->createDatabase(dbname, location))
        {
            db->unref();
            return false;
        }

        db->unref();
        return true;
    }
     else
    {
        std::string forward = kl::tostring(provider) + ".DatabaseMgr";

        xd::IDatabaseMgrPtr dbmgr;
        dbmgr.create_instance(forward.c_str());
        if (dbmgr.isOk())
        {
            return dbmgr->createDatabase(connection_str);
        }
    }
    

    return false;
}
    
xd::IDatabasePtr DatabaseMgr::open(const std::wstring& _connection_str)
{
    m_error.clearError();

    std::wstring connection_str = xdcommon::urlToConnectionStr(_connection_str);
    
    if (connection_str.length() == 0)
    {
        // connection string passed directly, not url
        connection_str = _connection_str;
    }


    xd::ConnectionString c(connection_str);
    std::wstring provider = c.getLowerValue(L"xdprovider");
    if (provider.empty())
        return xcm::null;
    
    // check if the provider refers to us, or a different dll/shared lib
    if (provider != L"xdnative")
    {
        // the provider is some other xd DLL; we need to
        // load it and pass the connection string to it
        std::string dbmgr_class = kl::tostring(provider);
        dbmgr_class += ".DatabaseMgr";
        
        xd::IDatabaseMgrPtr dbmgr;
        if (!dbmgr.create_instance(dbmgr_class.c_str()))
        {
            wchar_t buf[255];
            swprintf(buf, 254, L"The driver component '%ls' could either not be initialized or is missing. %hs", provider.c_str(), xcm::get_last_error());
            m_error.setError(xd::errorGeneral, buf);
            return xcm::null;
        }
        
        xd::IDatabasePtr res_db = dbmgr->open(connection_str);
        
        m_error.setError(dbmgr->getErrorCode(), dbmgr->getErrorString());

        return res_db;
    }
    
    // parse the connection string
    
    std::wstring location = c.getValue(L"database");
    std::wstring uid = c.getValue(L"user id");
    std::wstring password = c.getValue(L"password");
        
    XdnativeDatabase* db = new XdnativeDatabase;
    db->ref();

    if (!db->openDatabase(location, uid, password))
    {
        m_error.setError(db->getErrorCode(), db->getErrorString());
        delete db;
        return xcm::null;
    }

    return xd::IDatabasePtr(db, false);
}

xd::DatabaseEntryEnum DatabaseMgr::getDatabaseList(const std::wstring& host,
                                                          int port,
                                                          const std::wstring& uid,
                                                          const std::wstring& password)
{
    return xd::DatabaseEntryEnum();
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

