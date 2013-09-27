/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-06-02
 *
 */


#include "appmain.h"
#include "connectionpages.h"
#include "connectionwizard.h"


// this utility function returns the connection bar type
// based on the input connection type

int getConnectionBarType(int connection_type)
{
    if (connection_type == dbtypeFilesystem)
        return ConnectionBar::typeFolder;

    if (connection_type == dbtypeMySql)
        return ConnectionBar::typeMySql;
    
    if (connection_type == dbtypeSqlServer)
        return ConnectionBar::typeSqlServer;

    if (connection_type == dbtypeOracle)
        return ConnectionBar::typeOracle;
    
    if (connection_type == dbtypeDb2)
        return ConnectionBar::typeDb2;

    if (connection_type == dbtypeOdbc)
        return ConnectionBar::typeOdbc;

    if (connection_type == dbtypeClient)
        return ConnectionBar::typeClient;

    return ConnectionBar::typeFolder;
}

// -- ConnectionBar class implementation --

BEGIN_EVENT_TABLE(ConnectionBar, kcl::ButtonBar)
    EVT_BUTTON(-1, ConnectionBar::onItemClicked)
END_EVENT_TABLE()

ConnectionBar::ConnectionBar(wxWindow* parent,
                             wxWindowID id,
                             int conn_types)
              : kcl::ButtonBar(parent, id)
{
    setSpaceEvenly(true);
    setItemMinSize(56, -1);
    
    m_types = (ConnectionTypes)conn_types;
    m_default_item = NULL;
    populate();
}

ConnectionBar::~ConnectionBar()
{

}

int ConnectionBar::getDefaultConnectionType()
{
    if (m_default_item == NULL)
        return -1;
        
    return m_default_item->id;
}

int ConnectionBar::getSelectedConnectionType()
{
    if (m_selected_item == NULL)
        return -1;
        
    return m_selected_item->id;
}

void ConnectionBar::selectConnectionType(int conn_type)
{
    std::vector<kcl::ButtonBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        if ((*it)->id == conn_type)
        {
            selectItem((*it)->id);
            return;
        }
    }
}

void ConnectionBar::populate()
{
    kcl::ButtonBarItem* item;
    kcl::ButtonBarItem* default_item = NULL;
    
    if (m_types & dbtypePackage ||
        m_types & dbtypeAccess ||
        m_types & dbtypeExcel ||
        m_types & dbtypeDelimitedText ||
        m_types & dbtypeFixedLengthText ||
        m_types & dbtypeXbase)
    {
        item = addItem(ConnectionBar::typeFile,
                       GETBMP(gf_blank_document_32),
                       _("File"));
        if (default_item == NULL)
            default_item = item;
    }

    if (m_types & dbtypeFilesystem)
    {
        item = addItem(ConnectionBar::typeFolder,
                       GETBMP(gf_folder_open_32),
                       _("Folder"));
        if (default_item == NULL)
            default_item = item;
    }

    if (m_types & dbtypeMySql)
    {
        item = addItem(ConnectionBar::typeMySql,
                       GETBMP(gf_db_my_32),
                       _("MySQL"));
        if (default_item == NULL)
            default_item = item;
    }
    
    if (m_types & dbtypeSqlServer)
    {
        item = addItem(ConnectionBar::typeSqlServer,
                       GETBMP(gf_db_sq_32),
                       _("SQL Server"));
        if (default_item == NULL)
            default_item = item;
    }

    if (m_types & dbtypeOracle)
    {
        item = addItem(ConnectionBar::typeOracle,
                       GETBMP(gf_db_or_32),
                       _("Oracle"));
        if (default_item == NULL)
            default_item = item;
    }
    
    if (m_types & dbtypeDb2)
    {
        item = addItem(ConnectionBar::typeDb2,
                       GETBMP(gf_db_db_32),
                       _("DB2"));
        if (default_item == NULL)
            default_item = item;
    }

    if (m_types & dbtypeOdbc)
    {
        item = addItem(ConnectionBar::typeOdbc,
                       GETBMP(gf_db_od_32),
                       _("ODBC"));
        if (default_item == NULL)
            default_item = item;
    }

/*
// disable temporarily
    if (m_types & dbtypeClient)
    {
        item = addItem(ConnectionBar::typeClient,
                       GETBMP(gf_globe_32),
                       _("Http"));
        if (default_item == NULL)
            default_item = item;
    }
*/

    m_default_item = default_item;
}

void ConnectionBar::onItemClicked(wxCommandEvent& evt)
{
    sigConnectionTypeChanged(evt.GetId());
}


    
    
// -- ConnectionWizard class implementation --

BEGIN_EVENT_TABLE(ConnectionWizard, wxWindow)
    EVT_BUTTON(wxID_CANCEL, ConnectionWizard::onClose)
    EVT_SIZE(ConnectionWizard::onSize)
END_EVENT_TABLE()


ConnectionWizard::ConnectionWizard()
{
    m_wizard = NULL;
    m_connection_bar = NULL;
    m_title = _("Create Connection");
    m_mode = ConnectionWizard::ModeNormal;
}

ConnectionWizard::~ConnectionWizard()
{

}

bool ConnectionWizard::initDoc(IFramePtr frame,
                               IDocumentSitePtr doc_site,
                               wxWindow* docsite_wnd,
                               wxWindow* panesite_wnd)
{
    // create document's window
    bool result = Create(docsite_wnd,
                         -1,
                         wxDefaultPosition,
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE);
    if (!result)
    {
        return false;
    }
    
    doc_site->setCaption(m_title);
    wxSize min_site_size = doc_site->getContainerWindow()->GetSize();
    doc_site->setMinSize(min_site_size.x, min_site_size.y);

    m_frame = frame;
    m_doc_site = doc_site;

    // create connection bar
    m_connection_bar = new ConnectionBar(this, -1, conntypeConnection);
    m_connection_bar->sigConnectionTypeChanged.connect(this, &ConnectionWizard::onConnectionTypeChanged);

    // create the wizard
    m_wizard = new kcl::Wizard(this, -1);
    m_wizard->sigCancelPressed.connect(this, &ConnectionWizard::onWizardCancelled);
    m_wizard->sigFinishPressed.connect(this, &ConnectionWizard::onWizardFinished);
    
    if (m_mode == ConnectionWizard::ModeNormal)
        m_wizard->setFinishButtonLabel(_("Connect"));
     else if (m_mode == ConnectionWizard::ModeProperties)
        m_wizard->setFinishButtonLabel(_("Save"));
    
    // this button is created so that the ESC key can be used to
    // close the panel -- it is completely hidden to the user
    wxButton* close_button;
    close_button = new wxButton(this, wxID_CANCEL, wxEmptyString, wxDefaultPosition, wxSize(0,0));
    
    // create main sizer
    m_sizer = new wxBoxSizer(wxVERTICAL);
    m_sizer->Add(m_connection_bar, 0, wxEXPAND);
    m_sizer->Add(m_wizard, 1, wxEXPAND);
    m_sizer->Add(close_button, 0, wxEXPAND);
    SetSizer(m_sizer);
    Layout();

    m_path_selection_page = new PathSelectionPage(m_wizard, &m_ci);
    m_datasource_selection_page = new DataSourceSelectionPage(m_wizard, &m_ci);
    m_server_properties_page = new ServerPropertiesPage(m_wizard, &m_ci);

    m_wizard->addPage(m_path_selection_page, _("Path Selection"));
    m_wizard->addPage(m_datasource_selection_page, _("ODBC Data Source"));
    m_wizard->addPage(m_server_properties_page, _("Server Properties"));

    // only show the path on the path selection page
    m_path_selection_page->showElements(PathSelectionPage::showPath);

    // connect page signals
    m_datasource_selection_page->sigPageChanged.connect(this,
                    &ConnectionWizard::onDataSourceSelectionPageChanged);
    m_datasource_selection_page->sigPageChanging.connect(this,
                    &ConnectionWizard::onDataSourceSelectionPageChanging);

    if (m_mode == ConnectionWizard::ModeNormal)
    {
        // start the wizard off with the default type
        int default_type = m_connection_bar->getDefaultConnectionType();
        m_connection_bar->selectConnectionType(default_type);
    }
     else
    {
        // start the wizard off with the connection type
        // specified in the connection info
        int default_type = getConnectionBarType(m_ci.type);
        m_connection_bar->selectConnectionType(default_type);
    }
    
    return true;
}

wxWindow* ConnectionWizard::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ConnectionWizard::setDocumentFocus()
{

}

// Provider    - database provider (xdnative, xdfs, xdodbc, etc.)
// Server      - name or IP of the database server
// Port        - port number
// Database    - either a database name on the remote server or
//               a file name of a local database (e.g. file.mdb) or
// User Id     - user name used for authentication
// Password    - password used for authentication

std::wstring ConnectionWizard::getConnectionString()
{
    std::wstring result;
    std::wstring database;
    
    database = m_ci.database;
    
    switch (m_ci.type)
    {
        case dbtypeXdnative:
            result += L"Xdprovider=xdnative;";
            database = m_ci.path;
            break;
        case dbtypeMySql:
            result += L"Xdprovider=xdmysql;";
            break;
        case dbtypeOracle:
            result += L"Xdprovider=xdoracle;";
            break;
        case dbtypePostgres:
            result += L"Xdprovider=xdpgsql;";
            break;
        case dbtypeOdbc:
            result += L"Xdprovider=xdodbc;Xddbtype=dsn;";
            break;
        case dbtypeDb2:
            result += L"Xdprovider=xdodbc;Xddbtype=db2;";
            break;
        case dbtypeSqlServer:
            result += L"Xdprovider=xdodbc;Xddbtype=mssql;";
            break;
        case dbtypeAccess:
            result += L"Xdprovider=xdodbc;Xddbtype=access";
            database = m_ci.path;
            break;
        case dbtypeExcel:
            // probably doesn't work yet
            result += L"Xdprovider=xdfs;";
            database = m_ci.path;
            break;
        case dbtypeFilesystem:
            result += L"Xdprovider=xdfs;";
            database = m_ci.path;
            break;
        case dbtypeClient:
            result += L"Xdprovider=xdclient;";
            break;
        case dbtypeXbase:
            result += L"Xdprovider=xdfs;Xdfiletype=xbase;";
            database = m_ci.path;
            break;
        case dbtypeDelimitedText:
            result += L"Xdprovider=xdfs;Xdfiletype=delimitedtext;";
            database = m_ci.path;
            break;
        case dbtypeFixedLengthText:
            result += L"Xdprovider=xdfs;Xdfiletype=fixedtext;";
            database = m_ci.path;
            break;
    }
    
    if (m_ci.server.length() > 0)
    {
        result += L"Host=";
        result += m_ci.server;
        result += L";";
    }
    
    if (m_ci.port != 0)
    {
        result += L"Port=" + kl::itowstring(m_ci.port) + L";";
    }
    
    result += L"Database=" + database + L";";
    
    if (m_ci.username.length() > 0)
    {
        result += L"User ID=" + m_ci.username + L";";
    }
    
    if (m_ci.password.length() > 0)
    {
        result += L"Password=" + m_ci.password + L";";
    }
    
    return result;
}


void ConnectionWizard::setConnectionString(const std::wstring& str)
{
    xd::ConnectionStringParser cstr;
    cstr.parse(str);
    
    bool xdfs = false;
    
    ConnectionInfo ci;
    
    // database type
    if (cstr.getLowerValue(L"Xdprovider") == L"xdnative")
        ci.type = dbtypeXdnative;
    else if (cstr.getLowerValue(L"Xdprovider") == L"xdmysql")
        ci.type = dbtypeMySql;
    else if (cstr.getLowerValue(L"Xdprovider") == L"xdoracle")
        ci.type = dbtypeOracle;
    else if (cstr.getLowerValue(L"Xdprovider") == L"xdpgsql")
        ci.type = dbtypePostgres;
    else if (cstr.getLowerValue(L"Xdprovider") == L"xdfs")
    {
        xdfs = true;
        ci.type = dbtypeFilesystem;

        if (cstr.getLowerValue(L"Xdfiletype") == L"xbase")
            ci.type = dbtypeXbase;
        else if (cstr.getLowerValue(L"Xdfiletype") == L"delimitedtext")
            ci.type = dbtypeDelimitedText;
        else if (cstr.getLowerValue(L"Xdfiletype") == L"fixedlengthtext")
            ci.type = dbtypeFixedLengthText;
    }
    else if (cstr.getLowerValue(L"Xdprovider") == L"xdclient")
        ci.type = dbtypeClient;
    else if (cstr.getLowerValue(L"Xdprovider") == L"xdodbc")
    {
        if (cstr.getLowerValue(L"Xddbtype") == L"dsn")
            ci.type = dbtypeOdbc;
        else if (cstr.getLowerValue(L"Xddbtype") == L"db2")
            ci.type = dbtypeDb2;
        else if (cstr.getLowerValue(L"Xddbtype") == L"mssql")
            ci.type = dbtypeSqlServer;
        else if (cstr.getLowerValue(L"Xddbtype") == L"access")
            ci.type = dbtypeAccess;
        else if (cstr.getLowerValue(L"Xddbtype") == L"mysql")
            ci.type = dbtypeMySql;
    }
    
    ci.server = cstr.getLowerValue(L"Host");
    ci.port = 0;
    
    wxString port_str = cstr.getLowerValue(L"Port");
    if (port_str.Length() > 0)
        ci.port = wxAtoi(port_str);
    
    if (xdfs)
        ci.path = cstr.getValue(L"Database");
         else
        ci.database = cstr.getValue(L"Database");
        
    ci.username = cstr.getValue(L"User ID");
    ci.password = cstr.getValue(L"Password");
    
    m_ci = ci;
}



ConnectionInfo ConnectionWizard::getConnectionInfo()
{
    return m_ci;
}

void ConnectionWizard::setConnectionInfo(const ConnectionInfo& cinfo)
{
    m_ci = cinfo;
}

void ConnectionWizard::onDataSourceSelectionPageChanged()
{
    // make sure this page's button is the 'Finish" button
    m_wizard->setPageOrder(m_datasource_selection_page);
}

void ConnectionWizard::onDataSourceSelectionPageChanging(bool forward,
                                                         bool* allow)
{
    if (!forward)
    {
        *allow = true;
        return;
    }

    // if the ODBC data source doesn't need a username or password,
    // go directly to the table selection page
    if (m_ci.m_conn_ptr.isOk())
    {
        m_wizard->setPageOrder(m_datasource_selection_page);
        *allow = true;
        return;
    }
     else
    {
        m_wizard->setPageOrder(m_datasource_selection_page,
                               m_server_properties_page);
        m_wizard->goPage(m_server_properties_page);
        
        // this will prevent the wizard from closing
        *allow = false;
        return;
    }
}

void ConnectionWizard::onConnectionTypeChanged(int type)
{
    int conn_type = dbtypeUndefined;
    
    switch (type)
    {
        case ConnectionBar::typeMySql:     conn_type = dbtypeMySql;      break;
        case ConnectionBar::typeSqlServer: conn_type = dbtypeSqlServer;  break;
        case ConnectionBar::typeOracle:    conn_type = dbtypeOracle;     break;
        case ConnectionBar::typeDb2:       conn_type = dbtypeDb2;        break;
        case ConnectionBar::typeOdbc:      conn_type = dbtypeOdbc;       break;
        case ConnectionBar::typeFolder:    conn_type = dbtypeFilesystem; break;
        case ConnectionBar::typeClient:    conn_type = dbtypeClient;     break;
    }
    
    // something went wrong, bail out
    if (conn_type == dbtypeUndefined)
        return;
        
    if (m_mode != ConnectionWizard::ModeProperties)
    {
        // we didn't switch connection types, we're done
        if (m_ci.type == conn_type)
            return;
        
        // update (and clear out old) connection info
        m_ci.last_type = m_ci.type;
        m_ci.type = conn_type;
        m_ci.path = wxEmptyString;
        m_ci.server = wxEmptyString;
        m_ci.database = wxEmptyString;
        m_ci.username = wxEmptyString;
        m_ci.password = wxEmptyString;
        if (conn_type == dbtypeMySql)     { m_ci.port = 3306;  }
        if (conn_type == dbtypeSqlServer) { m_ci.port = 1433;  }
        if (conn_type == dbtypeOracle)    { m_ci.port = 1521;  }
        if (conn_type == dbtypeDb2)       { m_ci.port = 50000; }
        if (conn_type == dbtypeClient)    { m_ci.port = 80;    }
    }

    // -- handle wizard layout --
        
    switch (conn_type)
    {
        case dbtypeMySql:
        case dbtypeSqlServer:
        case dbtypeOracle:
        case dbtypeDb2:
        case dbtypeClient:
        {
            m_wizard->setPageOrder(m_server_properties_page);

            // this will reset the message to the default message
            m_server_properties_page->setMessage();
            
            m_server_properties_page->showElements(
                                ServerPropertiesPage::showAll &
                                ~ServerPropertiesPage::showSavePassword);
            break;
        }
            
        case dbtypeOdbc:
        {
            m_wizard->setPageOrder(m_datasource_selection_page);

            // update the page message based on the connection type
            wxString message = _("Please enter the username and password for the selected database.");
            m_server_properties_page->setMessage(message);

            m_server_properties_page->showElements(
                                ServerPropertiesPage::showUsername |
                                ServerPropertiesPage::showPassword);
            break;
        }
        
        case dbtypeFilesystem:
        {
            m_wizard->setPageOrder(m_path_selection_page);
            break;
        }
        
        case dbtypeXdnative:
        case dbtypePackage:
        case dbtypeAccess:
        case dbtypeExcel:
        case dbtypeXbase:
        case dbtypeDelimitedText:
        case dbtypeFixedLengthText:
        {
            // we don't allow connections to these types
            return;
        }
    }
    
    // set the control values based on the connection info
    if (conn_type == dbtypeFilesystem)
    {
        m_path_selection_page->loadPageData();
    }
    
    if (conn_type == dbtypeMySql ||
        conn_type == dbtypeSqlServer ||
        conn_type == dbtypeOracle ||
        conn_type == dbtypeDb2 ||
        conn_type == dbtypeClient)
    {
        m_server_properties_page->loadPageData();
    }
    
    m_wizard->startWizard(0);
    Layout();
}

void ConnectionWizard::onWizardCancelled(kcl::Wizard* wizard)
{
    m_frame->closeSite(m_doc_site);
}

void ConnectionWizard::onWizardFinished(kcl::Wizard* wizard)
{
    sigConnectionWizardFinished(this);
    m_frame->closeSite(m_doc_site);
}

void ConnectionWizard::onClose(wxCommandEvent& evt)
{
    m_frame->closeSite(m_doc_site);
}

void ConnectionWizard::onSize(wxSizeEvent& evt)
{
    Layout();
}



