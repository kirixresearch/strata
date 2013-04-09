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
        XCM_AUTO_LOCK(m_obj_mutex);

        tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();
        if (dbmgr.isNull())
            return false;
        

        wxString provider;
        wxString dbtype;
        
        switch (m_type)
        {
            case dbtypeXdnative:        provider = wxT("xdnative"); break;

            #ifdef WIN32
            case dbtypeAccess:       provider = wxT("xdodbc"); dbtype = wxT("access"); break;
            case dbtypeSqlServer:    provider = wxT("xdodbc"); dbtype = wxT("mssql"); break;
            case dbtypeMySql:        provider = wxT("xdodbc"); dbtype = wxT("mysql"); break;
            #else
            case dbtypeAccess:       provider = wxT("xdaccess"); break;
            case dbtypeSqlServer:    provider = wxT("xdsqlserver"); break;
            case dbtypeMySql:        provider = wxT("xddrizzle"); break;
            //case dbtypeMySql:        provider = wxT("xdmysql"); break;
            #endif

            case dbtypeExcel:        provider = wxT("xdodbc"); dbtype = wxT("excel"); break;
            case dbtypeSqlite:       provider = wxT("xdsqlite"); break;

            case dbtypePostgres:     provider = wxT("xdpgsql"); break;
            case dbtypeOracle:       provider = wxT("xdoracle"); break;
            case dbtypeOdbc:         provider = wxT("xdodbc"); dbtype = wxT("dsn"); break;
            case dbtypeDb2:          provider = wxT("xdodbc"); dbtype = wxT("db2"); break;
            case dbtypeClient:       provider = wxT("xdclient"); break;
            case dbtypeFilesystem:
            case dbtypeXbase:
            case dbtypeDelimitedText:
            case dbtypeFixedLengthText:
                                     provider = wxT("xdfs"); break;
            default:
                return false;
        }


        // -- build connection string --
        
        wxString cstr;
        
        cstr += wxT("xdprovider=");
        cstr += provider;
        cstr += wxT(";");
        
        if (dbtype.Length() > 0)
        {
            cstr += wxT("xddbtype=");
            cstr += dbtype;
            cstr += wxT(";");
        }
        
        if (m_host.Length() > 0)
        {
            cstr += wxT("host=");
            cstr += m_host;
            cstr += wxT(";");
        }
        
        if (m_port != 0)
        {
            cstr += wxString::Format(wxT("port=%d;"), m_port);
        }
        
        cstr += wxT("user id=");
        cstr += m_username;
        cstr += wxT(";");
        
        cstr += wxT("password=");
        cstr += m_password;
        cstr += wxT(";");
        
        if (m_database.Length() > 0)
        {
            cstr += wxT("database=");
            cstr += m_database;
            cstr += wxT(";");
        }
         else
        {
            cstr += wxT("database=");
            cstr += m_path;
            cstr += wxT(";");
        }
        
        m_db = dbmgr->open(towstr(cstr));
        
        
        // set the temp and definition directory from our main db
        if (m_db.isOk())
        {
            tango::IDatabasePtr main_db = g_app->getDatabase();
            if (main_db.isOk())
            {
                tango::IAttributesPtr attr = main_db->getAttributes();
                if (attr.isOk())
                {
                    std::wstring def_dir = attr->getStringAttribute(tango::dbattrDefinitionDirectory);
                    std::wstring temp_dir = attr->getStringAttribute(tango::dbattrTempDirectory);
                    attr = m_db->getAttributes();
                    if (attr.isOk())
                    {
                        attr->setStringAttribute(tango::dbattrDefinitionDirectory, def_dir);
                        attr->setStringAttribute(tango::dbattrTempDirectory, temp_dir);
                    }
                }
            }
        }
        
        // if an error occurred, get an error string
        m_error_string.Clear();
        if (m_db.isNull())
            m_error_string = dbmgr->getErrorString();
            
        return m_db.isOk();
    }

    wxString getErrorString()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_error_string;
    }
    
    void close()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_db.clear();
    }

    bool isOpen()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_db.isOk();
    }

    void setType(int new_val)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_type = new_val;
    }

    int getType()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_type;
    }

    wxString getDescription()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_description;
    }

    void setDescription(const wxString& new_val)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_description = new_val;
    }

    wxString getHost()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_host;
    }

    void setHost(const wxString& new_val)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_host = new_val;
    }

    int getPort()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_port;
    }

    void setPort(int new_val)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_port = new_val;
    }

    wxString getDatabase()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_database;
    }

    void setDatabase(const wxString& new_val)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_database = new_val;
    }

    wxString getUsername()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_username;
    }

    void setUsername(const wxString& new_val)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_username = new_val;
    }

    wxString getPassword()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_password;
    }

    void setPassword(const wxString& new_val)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_password = new_val;
    }

    wxString getPath()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_path;
    }

    void setPath(const wxString& path)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_path = path;
    }

    wxString getFilter()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_filter;
    }

    void setFilter(const wxString& filter)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_filter = filter;
    }

    tango::IDatabasePtr getDatabasePtr()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        return m_db;
    }

    void setDatabasePtr(tango::IDatabasePtr new_val)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_db = new_val;
    }

private:

    xcm::mutex m_obj_mutex;

    int m_type;
    wxString m_description;

    // -- used for directories --
    wxString m_path;
    wxString m_filter;

    // -- used for databases --
    wxString m_host;
    int m_port;
    wxString m_database;
    wxString m_username;
    wxString m_password;

    tango::IDatabasePtr m_db;
    
    wxString m_error_string;
};





IConnectionPtr createUnmanagedConnection()
{
    Connection* connection = new Connection;
    IConnectionPtr c = static_cast<IConnection*>(connection);
    return c;
}
