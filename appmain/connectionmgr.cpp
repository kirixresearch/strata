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


// -- Connection class implementation --


class Connection : public IConnection
{
    XCM_CLASS_NAME("appmain.Connection")
        XCM_BEGIN_INTERFACE_MAP(Connection)
        XCM_INTERFACE_ENTRY(IConnection)
        XCM_END_INTERFACE_MAP()

public:


    Connection()
    {
        m_type = dbtypeUndefined;
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

    ~Connection()
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


        {
            KL_AUTO_LOCK(m_obj_mutex);

            // if an error occurred, get an error string
            m_error_string.clear();
            if (m_db.isNull())
                m_error_string = dbmgr->getErrorString();
        }


        return m_db.isOk();
    }

    std::wstring getConnectionString()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        std::wstring cstr;
        std::wstring provider;
        std::wstring dbtype;

        switch (m_type)
        {
        case dbtypeXdnative:        provider = L"xdnative"; break;

#ifdef WIN32
        case dbtypeAccess:       provider = L"xdodbc"; dbtype = L"access"; break;
        case dbtypeSqlServer:    provider = L"xdodbc"; dbtype = L"mssql"; break;
#else
        case dbtypeAccess:       provider = L"xdaccess"; break;
        case dbtypeSqlServer:    provider = L"xdsqlserver"; break;
#endif

        case dbtypeSqlite:       provider = L"xdsqlite"; break;
        case dbtypeMySql:        provider = L"xdmysql"; break;

        case dbtypePostgres:     provider = L"xdpgsql"; break;
        case dbtypeOracle:       provider = L"xdoracle"; break;
        case dbtypeOdbc:         provider = L"xdodbc"; dbtype = L"dsn"; break;
        case dbtypeDb2:          provider = L"xdodbc"; dbtype = L"db2"; break;
        case dbtypeClient:       provider = L"xdclient"; break;
        case dbtypeFilesystem:
        case dbtypeXbase:
        case dbtypeDelimitedText:
        case dbtypeFixedLengthText:
            provider = L"xdfs"; break;
        case dbtypePackage:      provider = L"xdkpg"; break;

        case dbtypeExcel:
            provider = L"xdodbc";
            dbtype = L"excel";

            if (kl::icontains(m_path, L".xlsx"))
            {
                provider = L"xdexcel";
                dbtype = L"";
                break;
            }

            break;

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
            if (m_type == dbtypePackage || m_type == dbtypeAccess || m_type == dbtypeSqlite || m_type == dbtypeExcel)
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
        KL_AUTO_LOCK(m_obj_mutex);

        return m_error_string;
    }

    void close()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_db.clear();
    }

    bool isOpen()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_db.isOk();
    }

    void setType(int new_val)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_type = new_val;
    }

    int getType()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_type;
    }

    std::wstring getDescription()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_description;
    }

    void setDescription(const std::wstring& new_val)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_description = new_val;
    }

    std::wstring getHost()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_host;
    }

    void setHost(const std::wstring& new_val)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_host = new_val;
    }

    int getPort()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_port;
    }

    void setPort(int new_val)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_port = new_val;
    }

    std::wstring getDatabase()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_database;
    }

    void setDatabase(const std::wstring& new_val)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_database = new_val;
    }

    std::wstring getUsername()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_username;
    }

    void setUsername(const std::wstring& new_val)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_username = new_val;
    }

    std::wstring getPassword()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_password;
    }

    void setPassword(const std::wstring& new_val)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_password = new_val;
    }

    std::wstring getPath()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_path;
    }

    void setPath(const std::wstring& path)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_path = path;
    }

    std::wstring getFilter()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_filter;
    }

    void setFilter(const std::wstring& filter)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_filter = filter;
    }

    xd::IDatabasePtr getDatabasePtr()
    {
        KL_AUTO_LOCK(m_obj_mutex);

        return m_db;
    }

    void setDatabasePtr(xd::IDatabasePtr new_val)
    {
        KL_AUTO_LOCK(m_obj_mutex);

        m_db = new_val;
    }

private:

    kl::mutex m_obj_mutex;

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
    Connection* connection = new Connection;
    IConnectionPtr c = static_cast<IConnection*>(connection);
    return c;
}
