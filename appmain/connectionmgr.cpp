/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-04-27
 *
 */


#include "appmain.h"
#include "connectionmgr.h"




class ConnectionImpl : public IConnection
{
    XCM_CLASS_NAME("appmain.Connection")
    XCM_BEGIN_INTERFACE_MAP(ConnectionImpl)
        XCM_INTERFACE_ENTRY(IConnection)
    XCM_END_INTERFACE_MAP()

public:


    ConnectionImpl()
    {
        m_type = xd::dbtypeUndefined;
        m_description = wxT("");
        m_host = wxT("");
        m_port = 0;
        m_database = wxT("");
        m_username = wxT("");
        m_password = wxT("");
        m_path = wxT("");
        m_filter = wxT("");

        m_db = xcm::null;
    }

    ~ConnectionImpl()
    {
    }

    bool open()
    {
        xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
        if (dbmgr.isNull())
            return false;
        
        std::wstring cstr = getConnectionString();

        m_db = dbmgr->open(cstr);
        
        // set the temp and definition directory from our main db
        if (m_db.isOk())
        {
            xd::IDatabasePtr main_db = g_app->getDatabase();
            if (main_db.isOk())
            {
                xd::IAttributesPtr attr = main_db->getAttributes();
                if (attr.isOk())
                {
                    std::wstring def_dir = attr->getStringAttribute(xd::dbattrDefinitionDirectory);
                    std::wstring temp_dir = attr->getStringAttribute(xd::dbattrTempDirectory);
                    attr = m_db->getAttributes();
                    if (attr.isOk())
                    {
                        attr->setStringAttribute(xd::dbattrDefinitionDirectory, def_dir);
                        attr->setStringAttribute(xd::dbattrTempDirectory, temp_dir);
                    }
                }
            }
        }
        

        // if an error occurred, get an error string
        m_error_string.clear();
        if (m_db.isNull())
            m_error_string = dbmgr->getErrorString();


        return m_db.isOk();
    }

    std::wstring getConnectionString()
    {
        std::wstring cstr;
        std::wstring provider;
        std::wstring dbtype;
        
        switch (m_type)
        {
            case xd::dbtypeXdnative:        provider = L"xdnative"; break;

            #ifdef WIN32
            case xd::dbtypeAccess:       provider = L"xdodbc"; dbtype = L"access"; break;
            case xd::dbtypeSqlServer:    provider = L"xdodbc"; dbtype = L"mssql"; break;
            #else
            case xd::dbtypeAccess:       provider = L"xdaccess"; break;
            case xd::dbtypeSqlServer:    provider = L"xdsqlserver"; break;
            #endif

            case xd::dbtypeExcel:        provider = L"xdodbc"; dbtype = L"excel"; break;
            case xd::dbtypeSqlite:       provider = L"xdsqlite"; break;
            case xd::dbtypeMySql:        provider = L"xdmysql"; break;

            case xd::dbtypePostgres:     provider = L"xdpgsql"; break;
            case xd::dbtypeOracle:       provider = L"xdoracle"; break;
            case xd::dbtypeOdbc:         provider = L"xdodbc"; dbtype = L"dsn"; break;
            case xd::dbtypeDb2:          provider = L"xdodbc"; dbtype = L"db2"; break;
            case xd::dbtypeClient:       provider = L"xdclient"; break;
            case xd::dbtypeFilesystem:
          //  case xd::dbtypeXbase:
          //  case xd::dbtypeDelimitedText:
          //  case xd::dbtypeFixedLengthText:
                                         provider = L"xdfs"; break;
            case xd::dbtypeKpg:          provider = L"xdkpg"; break;
            default:
                return L"";
        }


        // build connection string
        

        cstr += L"xdprovider=";
        cstr += provider;
        cstr += L";";
        
        if (dbtype.length() > 0)
        {
            cstr += L"xddbtype=";
            cstr += dbtype;
            cstr += L";";
        }
        
        if (m_host.length() > 0)
        {
            cstr += L"host=";
            cstr += m_host;
            cstr += L";";
        }
        
        if (m_port != 0)
        {
            cstr += L"port=";
            cstr += kl::itowstring(m_port);
            cstr += L";";
        }
        
        cstr += L"user id=";
        cstr += m_username;
        cstr += L";";
        
        cstr += L"password=";
        cstr += m_password;
        cstr += L";";
        
        if (m_database.length() > 0)
        {
            cstr += L"database=";
            cstr += m_database;
            cstr += L";";
        }
         else
        {
            if (m_type == xd::dbtypeKpg || m_type == xd::dbtypeAccess || m_type == xd::dbtypeSqlite || m_type == xd::dbtypeExcel)
            {
                cstr += L"database=";
                cstr += m_path;
                cstr += L";";
            }
        }
        
        return cstr;
    }



    std::wstring getErrorString()
    {
        return m_error_string;
    }
    
    void close()
    {
        m_db.clear();
    }

    bool isOpen()
    {
        return m_db.isOk();
    }

    void setType(int new_val)
    {
        m_type = new_val;
    }

    int getType()
    {
        return m_type;
    }

    std::wstring getDescription()
    {
        return m_description;
    }

    void setDescription(const std::wstring& new_val)
    {
        m_description = new_val;
    }

    std::wstring getHost()
    {
        return m_host;
    }

    void setHost(const std::wstring& new_val)
    {
        m_host = new_val;
    }

    int getPort()
    {
        return m_port;
    }

    void setPort(int new_val)
    {
        m_port = new_val;
    }

    std::wstring getDatabase()
    {
        return m_database;
    }

    void setDatabase(const std::wstring& new_val)
    {
        m_database = new_val;
    }

    std::wstring getUsername()
    {
        return m_username;
    }

    void setUsername(const std::wstring& new_val)
    {
        m_username = new_val;
    }

    std::wstring getPassword()
    {
        return m_password;
    }

    void setPassword(const std::wstring& new_val)
    {
        m_password = new_val;
    }

    std::wstring getPath()
    {
        return m_path;
    }

    void setPath(const std::wstring& path)
    {
        m_path = path;
    }

    std::wstring getFilter()
    {
        return m_filter;
    }

    void setFilter(const std::wstring& filter)
    {
        m_filter = filter;
    }

    xd::IDatabasePtr getDatabasePtr()
    {
        return m_db;
    }

    void setDatabasePtr(xd::IDatabasePtr new_val)
    {
        m_db = new_val;
    }

private:

    int m_type;
    std::wstring m_description;

    // used for directories
    std::wstring m_path;
    std::wstring m_filter;

    // used for databases
    std::wstring m_host;
    int m_port;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;

    xd::IDatabasePtr m_db;
    
    std::wstring m_error_string;
};





IConnectionPtr createUnmanagedConnection()
{
    ConnectionImpl* connection = new ConnectionImpl;
    IConnectionPtr c = static_cast<IConnection*>(connection);
    return c;
}
