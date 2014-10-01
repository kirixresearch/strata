/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-04-16
 *
 */


#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <kl/string.h>
#include <kl/file.h>
#include "database.h"
#include "../xdcommon/connectionstr.h"



#include <odbcinst.h>

void getOdbcDriverNames(std::vector<std::wstring>& drivers);

class DatabaseMgr : public xd::IDatabaseMgr
{
    XCM_CLASS_NAME("xdodbc.DatabaseMgr")
    XCM_BEGIN_INTERFACE_MAP(DatabaseMgr)
        XCM_INTERFACE_ENTRY(xd::IDatabaseMgr)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseMgr()
    {
        m_env = 0;
        m_conn = 0;
    }

    bool createDatabase(const std::wstring& connection_str)
    {
        xd::ConnectionStringParser c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return false;

        std::wstring location = c.getValue(L"database");

        // create an MDB database
        std::wstring ext = kl::afterLast(location, L'.');
        kl::makeUpper(ext);

        if (ext == L"MDB")
        {
            std::wstring cmd = L"CREATE_DB=\"";
            cmd += location;
            cmd += L"\"\0";


#ifdef WIN32
            if (!::SQLConfigDataSource(NULL,
                                       ODBC_CONFIG_DSN,
                                       _t("Microsoft Access Driver (*.mdb)"),
                                       sqlt(cmd)))
            {
                return false;
            }
#else
            // the unicode api on linux isn't quite right,
            // so we have to do this
            if (!::SQLConfigDataSource(NULL,
                                       ODBC_CONFIG_DSN,
                                       "Microsoft Access Driver (*.mdb)",
                                       kl::tostring(cmd).c_str()))
            {
                return false;
            }
#endif

            // open file on local computer or network share
            OdbcDatabase* db = new OdbcDatabase;
            db->ref();
            
            if (!db->open(xd::dbtypeAccess, L"", 0, L"", L"admin", L"", location))
            {
                db->unref();
                return false;
            }
      
            db->unref();
            return true;
        }
         else if (ext == L"XLS" || ext == L"XLSX")
        {
            // create an XLS file

            std::vector<std::wstring> drivers;
            getOdbcDriverNames(drivers);
            const wchar_t* driver = L"Microsoft Excel Driver (*.xls)";
            std::vector<std::wstring>::iterator driver_it;
            for (driver_it = drivers.begin(); driver_it != drivers.end(); ++driver_it)
            {
                std::wstring d = *driver_it;
                kl::makeUpper(d);
                if (d.find(L"EXCEL DRIVER") != d.npos)
                    driver = driver_it->c_str();
            }



            std::wstring cmd;
            
            cmd = L"DRIVER={";
            cmd += driver;
            cmd += L"};";
            cmd += L"DSN='';READONLY=FALSE;";
            cmd += L"DBQ=";
            cmd += location;

            HENV env;
            HDBC conn;
            SQLRETURN retval;
            SQLTCHAR out_str[2048];
            short out_length;

            // allocate odbc environment
            SQLAllocEnv(&env);
            SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            SQLAllocConnect(env, &conn);

            retval = SQLDriverConnect(conn,
                                      NULL,
                                      sqlt(cmd),
                                      cmd.length(),
                                      (SQLTCHAR*)out_str,
                                      2048,
                                      &out_length,
                                      SQL_DRIVER_NOPROMPT);
            #ifdef _DEBUG
            testSqlConn(conn);
            #endif
            
            SQLDisconnect(conn);
            SQLFreeConnect(conn);
            SQLFreeEnv(env);
                    

            // open file on local computer or network share
            OdbcDatabase* db = new OdbcDatabase;
            db->ref();
            
            if (!db->open(xd::dbtypeExcel, L"", 0, L"", L"admin", L"", location))
            {
                db->unref();
                return false;
            }
      
            db->unref();
            return true;
        }
        
        return false;
    }
    
    
    xd::IDatabasePtr open(const std::wstring& connection_str)
    {
        xd::ConnectionStringParser c(connection_str);
        std::wstring provider = c.getLowerValue(L"xdprovider");
        if (provider.empty())
            return xcm::null;
        
        // check if the provider refers to us, or a different dll/shared lib
        if (provider != L"xdodbc")
        {
            return xcm::null;
        }
        
        
        
        // parse the connection string
        
        std::wstring dbtype = c.getLowerValue(L"xddbtype");
        std::wstring host = c.getValue(L"host");
        std::wstring port = c.getValue(L"port");
        std::wstring database = c.getValue(L"database");
        std::wstring uid = c.getValue(L"user id");
        std::wstring password = c.getValue(L"password");
        


        if (host.empty() && dbtype != L"dsn")
        {
            // open file on local computer or network share
            OdbcDatabase* db = new OdbcDatabase;
            db->ref();
            
            std::wstring ext = kl::afterLast(database, L'.');
            kl::makeUpper(ext);

            if (c.getValue(L"create_if_not_exists") == L"true" && !xf_get_file_exist(database))
                return this->createDatabase(database);

            if (ext == L"MDB" || ext == L"ACCDB")
            {
                if (!db->open(xd::dbtypeAccess, L"", 0, L"", uid, password, database))
                {
                    db->unref();
                    return xcm::null;
                }
            }
             else if (ext == L"XLS" || ext == L"XLSX")
            {

                if (!db->open(xd::dbtypeExcel, L"", 0, L"", uid, password, database))
                {
                    db->unref();
                    return xcm::null;
                }
            }
             else
            {
                db->unref();
                return xcm::null;
            }
            

            return xd::IDatabasePtr(db, false);
        }
         else
        {
            OdbcDatabase* db = new OdbcDatabase;
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
            
            
            int xd_dbtype = xd::dbtypeOdbc;
            
            if (dbtype == L"mssql")
                xd_dbtype = xd::dbtypeSqlServer;
            else if (dbtype == L"mysql")
                xd_dbtype = xd::dbtypeMySql;
            else if (dbtype == L"db2")
                xd_dbtype = xd::dbtypeDb2;
            else if (dbtype == L"oracle")
                xd_dbtype = xd::dbtypeOracle;
            else if (dbtype == L"access")
                xd_dbtype = xd::dbtypeAccess;
            else if (dbtype == L"dsn")
                xd_dbtype = xd::dbtypeOdbc;
            
            
            // check for pass-through connection string    
            if (dbtype == L"passthrough")
            {
                // not supported yet;
                return xcm::null;
            }


            // attempt to open the database
            if (!db->open(xd_dbtype, host_fixed, kl::wtoi(port), database, uid, password, L""))
            {
                m_error.setError(db->getErrorCode(), db->getErrorString());
                
                db->unref();
                return xcm::null;
            }

            return xd::IDatabasePtr(db, false); 
        }
        
        return xcm::null;
    }

    xd::DatabaseEntryEnum getDatabaseList(const std::wstring& host,
                                          int port,
                                          const std::wstring& uid,
                                          const std::wstring& password)
    {
        // start a connection
        SQLAllocEnv(&m_env);
        SQLAllocConnect(m_env, &m_conn);
        SQLSetConnectOption(m_conn, SQL_ODBC_CURSORS, SQL_CUR_USE_ODBC);


        xd::DatabaseEntryEnum db_list;


        SQLRETURN r;


        #ifdef _UNICODE
            wchar_t name_buf[SQL_MAX_DSN_LENGTH+1];
            wchar_t desc_buf[1024];
        #else
            char name_buf[SQL_MAX_DSN_LENGTH+1];
            char desc_buf[1024];
        #endif

        short name_length;
        short desc_length;

        if (SQLDataSources(m_env,
                           SQL_FETCH_FIRST,
                           (SQLTCHAR*)name_buf,
                           SQL_MAX_DSN_LENGTH+1,
                           &name_length,
                           (SQLTCHAR*)desc_buf,
                           1024,
                           &desc_length) != SQL_SUCCESS)
        {
            SQLDisconnect(m_conn);
            SQLFreeConnect(m_conn);
            SQLFreeEnv(m_env);
            m_env = 0;
            
            // return empty list
            return db_list;
        }


        while (true)
        {
            xd::DatabaseEntry db_entry;
            db_entry.name =  kl::towstring(name_buf);
            db_entry.description = kl::towstring(desc_buf);

            db_list.push_back(db_entry);
            
            r = SQLDataSources(m_env,
                               SQL_FETCH_NEXT,
                               (SQLTCHAR*)name_buf,
                               SQL_MAX_DSN_LENGTH+1,
                               &name_length,
                               (SQLTCHAR*)desc_buf,
                               1024,
                               &desc_length);

            if (r == SQL_NO_DATA || r == SQL_ERROR)
            {
                break;
            }
        }

        // close out the connection
        if (m_conn)
        {
            SQLDisconnect(m_conn);
            SQLFreeConnect(m_conn);
            m_conn = 0;
        }

        SQLFreeEnv(m_env);
        m_env = 0;


        return db_list;
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

    HENV m_env;
    HDBC m_conn;
    
    ThreadErrorInfo m_error;
};




// these are the publicly creatable classes

XCM_BEGIN_DYNAMIC_MODULE(xdodbc)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
        XCM_CLASS_ENTRY(OdbcDatabase)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdodbc)


