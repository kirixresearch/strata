/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2003-06-02
 *
 */


#ifndef __APP_CONNECTIONWIZARD_H
#define __APP_CONNECTIONWIZARD_H


#include "connectionmgr.h"


enum ConnectionWizardDatabaseTypes
{
    cwdbtypeUndefined = 0x01,
    cwdbtypeXdnative = 0x02,
    cwdbtypePackage = 0x04,
    cwdbtypeAccess = 0x08,
    cwdbtypeExcel = 0x10,
    cwdbtypeSqlServer = 0x20,
    cwdbtypeMySql = 0x40,
    cwdbtypeOracle = 0x80,
    cwdbtypeOdbc = 0x100,
    cwdbtypeFilesystem = 0x200,
    cwdbtypeXbase = 0x400,
    cwdbtypeDelimitedText = 0x800,
    cwdbtypeDb2 = 0x1000,
    cwdbtypeFixedLengthText = 0x2000,
    cwdbtypeSqlite = 0x4000,
    cwdbtypePostgres = 0x8000,
    cwdbtypeClient = 0x100000,
    cwdbtypeFirebird = 0x200000,
    cwdbtypeAll = 0x0fffffff
};


enum ConnectionTypes
{
    conntypeImport =
    
        // excel import is windows only
        #ifdef __WXMSW__
        cwdbtypeExcel |
        #endif

        cwdbtypeMySql |
        cwdbtypePostgres |
        cwdbtypeDb2 |
        cwdbtypePackage |
        cwdbtypeAccess |
        cwdbtypeSqlServer |
        cwdbtypeOracle |
        cwdbtypeOdbc |
        cwdbtypeXbase |
        cwdbtypeDelimitedText |
        cwdbtypeFixedLengthText,

    conntypeExport =
    
        // excel, access, and oracle exports are windows only
        #ifdef WIN32
        cwdbtypeAccess |
        cwdbtypeExcel |
        cwdbtypeOracle |
        #endif

        cwdbtypeMySql |
        cwdbtypePostgres |
        cwdbtypeDb2 |
        cwdbtypePackage |
        cwdbtypeSqlServer |
        cwdbtypeOracle |
        cwdbtypeOdbc |
        cwdbtypeXbase |
        cwdbtypeDelimitedText |
        cwdbtypeFixedLengthText,

    conntypeConnection =
    
        cwdbtypeSqlServer |
        cwdbtypeMySql |
        cwdbtypePostgres |
        cwdbtypeOracle |
        cwdbtypeDb2 |
        cwdbtypeOdbc |
        cwdbtypeFilesystem | 
        cwdbtypeClient
};


class ConnectionInfo
{
public:

    ConnectionInfo()
    {
        type = cwdbtypeUndefined;
        last_type = cwdbtypeUndefined;
        description = L"";

        server = L"";
        port = 0;
        database = L"";
        username = L"";
        password = L"";
        save_password = false;

        path = L"";
        filter = L"";

        kpg_compressed = true;
        kpg_version = 2;
    }

    ConnectionInfo(const ConnectionInfo& c)
    {
        type = c.type;
        last_type = c.last_type;
        description = c.description;

        server = c.server;
        port = c.port;
        database = c.database;
        username = c.username;
        password = c.password;
        save_password = c.save_password;

        path = c.path;
        filter = c.filter;

        kpg_compressed = c.kpg_compressed;
        kpg_version = c.kpg_version;

        m_conn_ptr = c.m_conn_ptr;
    }

    ConnectionInfo& operator=(const ConnectionInfo& c)
    {
        type = c.type;
        last_type = c.last_type;
        description = c.description;

        server = c.server;
        port = c.port;
        database = c.database;
        username = c.username;
        password = c.password;
        save_password = c.save_password;

        path = c.path;
        filter = c.filter;

        kpg_compressed = c.kpg_compressed;
        kpg_version = c.kpg_version;

        m_conn_ptr = c.m_conn_ptr;

        return *this;
    }
    
public:

    int type;                 // connection type
    int last_type;            // if the user is changing the connection type,
                              // this is the connection type before the change
    std::wstring description;

    // used for databases
    std::wstring server;
    int port;
    std::wstring database;
    std::wstring username;
    std::wstring password;
    bool save_password;

    // used for directories and data projects
    std::wstring path;
    std::wstring filter;

    // used for package files only
    bool kpg_compressed;
    int kpg_version;

    // live connection ptr
    IConnectionPtr m_conn_ptr;
};


    
    
// class which encapsulates connection type switching through a button bar

class ConnectionBar : public kcl::ButtonBar
{
public:

    enum
    {
        typeFile = 1,
        typeFolder,
        typeMySql,
        typeSqlServer,
        typeDb2,
        typeOracle,
        typePostgres,
        typeOdbc,
        typeClient
    };
    
    ConnectionBar(wxWindow* parent, wxWindowID id, int conn_types);
    ~ConnectionBar();
    
    int getDefaultConnectionType();
    int getSelectedConnectionType();
    
    void selectConnectionType(int conn_type);
    
public: // signals

    xcm::signal1<int> sigConnectionTypeChanged;

private:

    void populate();
    
    // event handlers
    void onItemClicked(wxCommandEvent& evt);

private:

    ConnectionTypes m_types;
    kcl::ButtonBarItem* m_default_item;
    
    DECLARE_EVENT_TABLE()
};
    
    
// forward declarations
class PathSelectionPage;
class DataSourceSelectionPage;
class ServerPropertiesPage;


class ConnectionWizard : public wxWindow,
                         public IDocument,
                         public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ConnectionWizard")
    XCM_BEGIN_INTERFACE_MAP(ConnectionWizard)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    enum ConnectionWizardMode
    {
        ModeNormal = 0,
        ModeProperties = 1  // the user is editing connection properties
                            // (connection type is not allowed)
    };
    
public:

    ConnectionWizard();
    ~ConnectionWizard();

public:

    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    void setConnectionString(const std::wstring& cstr);
    std::wstring getConnectionString();
    
    void setConnectionInfo(const ConnectionInfo& cinfo);
    ConnectionInfo getConnectionInfo();
    
    void setTitle(const wxString& title) { m_title = title; }
    void setMode(int mode)               { m_mode = mode;   }
    
private:
    
    // wizard page's connected signals
    void onDataSourceSelectionPageChanged();
    void onDataSourceSelectionPageChanging(bool forward, bool* allow);
    
    // wizard's connected signals
    void onConnectionTypeChanged(int type);
    void onWizardCancelled(kcl::Wizard* wizard);
    void onWizardFinished(kcl::Wizard* wizard);
    
    // event handlers
    void onClose(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);

public: // signals

    xcm::signal1<ConnectionWizard*> sigConnectionWizardFinished;

private:

    IFramePtr m_frame;
    IDocumentSitePtr m_doc_site;
    wxString m_title;
    int m_mode;
        
    ConnectionInfo m_ci;

    wxBoxSizer* m_sizer;
    kcl::Wizard* m_wizard;
    ConnectionBar* m_connection_bar;
    
    PathSelectionPage* m_path_selection_page;
    DataSourceSelectionPage* m_datasource_selection_page;
    ServerPropertiesPage* m_server_properties_page;
    
    DECLARE_EVENT_TABLE()
};


#endif  // __APP_CONNECTIONWIZARD_H





