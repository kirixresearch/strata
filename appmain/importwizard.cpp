/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-07-24
 *
 */


#include "appmain.h"
#include "connectionpages.h"
#include "importwizard.h"
#include "importpages.h"
#include "appcontroller.h"
#include "dlgdatabasefile.h"


#ifdef FindWindow
#undef FindWindow
#endif



// -- ImportWizard class implementation --

const int ID_SaveTemplateButton = wxID_HIGHEST + 1;


BEGIN_EVENT_TABLE(ImportWizard, wxWindow)
    EVT_BUTTON(wxID_CANCEL, ImportWizard::onClose)
    EVT_BUTTON(ID_SaveTemplateButton, ImportWizard::onSave)
    EVT_SIZE(ImportWizard::onSize)
END_EVENT_TABLE()


ImportWizard::ImportWizard()
{
    m_wizard = NULL;
    m_connection_bar = NULL;
    m_title = _("Import");
    m_mode = ImportWizard::ModeImport;
    
    m_path_selection_page = NULL;
    m_datasource_selection_page = NULL;
    m_server_properties_page = NULL;
    m_table_selection_page = NULL;
    m_delimitedtext_settings_page = NULL;
}

ImportWizard::~ImportWizard()
{
}

bool ImportWizard::initDoc(IFramePtr frame,
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
    m_connection_bar = new ConnectionBar(this, -1, conntypeImport);
    
    // create the wizard
    m_wizard = new kcl::Wizard(this, -1);
    m_wizard->sigCancelPressed.connect(this, &ImportWizard::onWizardCancelled);
    m_wizard->sigFinishPressed.connect(this, &ImportWizard::onWizardFinished);
    
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


    wxButton* save_button = new wxButton(m_wizard,
                                         ID_SaveTemplateButton,
                                         _("Save"));

    m_wizard->getExtraSizer()->Prepend(save_button, 0, wxEXPAND);
    m_wizard->getExtraSizer()->PrependSpacer(8);    


    m_path_selection_page = new PathSelectionPage(m_wizard, &m_template.m_ii);
    m_datasource_selection_page = new DataSourceSelectionPage(m_wizard, &m_template.m_ii);
    m_server_properties_page = new ServerPropertiesPage(m_wizard, &m_template.m_ii);
    m_table_selection_page = new ImportTableSelectionPage(m_wizard, &m_template.m_ii);
    m_delimitedtext_settings_page = new ImportDelimitedTextSettingsPage(m_wizard, &m_template.m_ii);
    
    m_path_selection_page->setMessage(_("Please enter the location of the files you would like to import."));
    m_path_selection_page->setPathLabel(_("Files:"));
    
    m_wizard->addPage(m_path_selection_page, _("Path Selection"));
    m_wizard->addPage(m_datasource_selection_page, _("ODBC Data Source Selection"));
    m_wizard->addPage(m_server_properties_page, _("Server Properties"));
    m_wizard->addPage(m_table_selection_page, _("Table Selection"));
    m_wizard->addPage(m_delimitedtext_settings_page, _("Text-Delimited Settings"));

    // populate the Path Selection Page's file dialog filter
    wxString filter;
    filter += _("All Files");
    filter += wxT(" (*.*)|*.*|");
    filter += _("All Text Files");
    filter += wxT(" (*.csv, *.tsv, *.prn, *.txt)|*.csv;*.tsv;*.prn;*.txt|");
    filter += _("Comma-Delimited Files");
    filter += wxT(" (*.csv)|*.csv|");
    filter += _("Tab-Delimited Files");
    filter += wxT(" (*.tsv)|*.tsv|");
    filter += _("Microsoft FoxPro/Xbase Files");
    filter += wxT(" (*.dbf)|*.dbf|");
    filter += _("Microsoft Access Files");
    filter += wxT(" (*.accdb, *.mdb)|*.accdb;*.mdb|");
    filter += _("Microsoft Excel Files");
    filter += wxT(" (*.xls, *.xlsx)|*.xls;*.xlsx|");
    filter += _("Package Files");
    filter += wxT(" (*.kpg)|*.kpg|");
    filter.RemoveLast(); // get rid of the last pipe sign

    m_path_selection_page->setFileDialogFilter(filter);

    // connect page signals
    m_path_selection_page->sigPageChanging.connect(this, &ImportWizard::onPathSelectionPageChanging);
    m_datasource_selection_page->sigPageChanging.connect(this, &ImportWizard::onDataSourceSelectionPageChanging);


    // make sure the import wizard starts on the table selection page
    if (m_mode == ImportWizard::ModeOpen)
    {
        int type;
        
        switch (m_template.m_ii.type)
        {
            default:              type = ConnectionBar::typeFile;      break;
            case dbtypeMySql:     type = ConnectionBar::typeMySql;     break;
            case dbtypePostgres:  type = ConnectionBar::typePostgres;  break;
            case dbtypeSqlServer: type = ConnectionBar::typeSqlServer; break;
            case dbtypeDb2:       type = ConnectionBar::typeDb2;       break;
            case dbtypeOracle:    type = ConnectionBar::typeOracle;    break;
            case dbtypeOdbc:      type = ConnectionBar::typeOdbc;      break;
        }
        
        
        m_connection_bar->selectConnectionType(type);
        updatePageRouting();
        
        m_wizard->startWizard(m_table_selection_page);
    }
     else
    {
        int default_type = m_connection_bar->getDefaultConnectionType();
        m_connection_bar->selectConnectionType(default_type);
        updatePageRouting();
        m_wizard->startWizard(0);
    }
    
    
    m_connection_bar->sigConnectionTypeChanged.connect(this, &ImportWizard::onImportTypeChanged);

    Layout();
    return true;
}

wxWindow* ImportWizard::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ImportWizard::setDocumentFocus()
{

}

ImportTemplate& ImportWizard::getTemplate()
{
    return m_template;
}


void ImportWizard::updatePageRouting()
{
    // handle page routing
    switch (m_template.m_ii.type)
    {
        default:
        {
            m_path_selection_page->setBrowse(true, true, true);
            m_wizard->setPageOrder(m_path_selection_page,
                                   m_table_selection_page);
            break;
        }
        
        case dbtypeDelimitedText:
        {
            m_path_selection_page->setBrowse(true, true, true);
            m_wizard->setPageOrder(m_path_selection_page,
                                   m_delimitedtext_settings_page,
                                   m_table_selection_page);
            break;
        }
        
        case dbtypeSqlServer:
        case dbtypeMySql:
        case dbtypePostgres:
        case dbtypeOracle:
        case dbtypeDb2:
        {
            m_wizard->setPageOrder(m_server_properties_page,
                                   m_table_selection_page);

            // update the page message based on the import type
            wxString message = _("Please enter the connection settings for the database from which you would like to import.");
            m_server_properties_page->setMessage(message);

            m_server_properties_page->showElements(
                                ServerPropertiesPage::showAll &
                                ~ServerPropertiesPage::showSavePassword);
            break;
        }

        case dbtypeOdbc:
        {
            m_wizard->setPageOrder(m_datasource_selection_page,
                                   m_server_properties_page,
                                   m_table_selection_page);
           
            // update the page message based on the import type
            wxString message = _("Please select the ODBC data source from which you would like to import.");
            m_datasource_selection_page->setMessage(message);
            
            message = _("Please enter the username and password for the selected database.");
            m_server_properties_page->setMessage(message);

            m_server_properties_page->showElements(
                                ServerPropertiesPage::showUsername |
                                ServerPropertiesPage::showPassword);
            break;
        }
    }

}


void ImportWizard::onImportTypeChanged(int type)
{
    int import_type = dbtypeUndefined;
    
    // figure out the import type, if we can
    switch (type)
    {
        case ConnectionBar::typeMySql:     import_type = dbtypeMySql;        break;
        case ConnectionBar::typePostgres:  import_type = dbtypePostgres;     break;
        case ConnectionBar::typeSqlServer: import_type = dbtypeSqlServer;    break;
        case ConnectionBar::typeOracle:    import_type = dbtypeOracle;       break;
        case ConnectionBar::typeDb2:       import_type = dbtypeDb2;          break;
        case ConnectionBar::typeOdbc:      import_type = dbtypeOdbc;         break;
    }
    
    // we didn't change anything, we're done
    if (m_template.m_ii.type == import_type && type != ConnectionBar::typeFile)
    {
        return;
    }
    
    
    if (import_type == dbtypeMySql)     { m_template.m_ii.port = 3306;  }
    if (import_type == dbtypePostgres)  { m_template.m_ii.port = 5432;  }
    if (import_type == dbtypeSqlServer) { m_template.m_ii.port = 1433;  }
    if (import_type == dbtypeOracle)    { m_template.m_ii.port = 1521;  }
    if (import_type == dbtypeDb2)       { m_template.m_ii.port = 50000; }



    m_template.m_ii.last_type = m_template.m_ii.type;
    m_template.m_ii.type = import_type;
    
    
    updatePageRouting();
    
    
    // set the control values based on the connection info
    m_path_selection_page->loadPageData();
    m_server_properties_page->loadPageData();
    
    m_wizard->startWizard(0);
    Layout();
}

void ImportWizard::onPathSelectionPageChanging(bool forward, bool* allow)
{
    if (!forward)
    {
        *allow = true;
        return;
    }

    if (m_template.m_ii.path.empty())
    {
        // the path was empty
        appMessageBox(_("A valid location needs to be specified to continue."),
                           _("Invalid Location"),
                           wxOK | wxICON_EXCLAMATION | wxCENTER,
                           g_app->getMainWindow());
        *allow = false;
        return;
    }

    // clear out existing structures
    m_template.m_ii.tables.clear();


    // NOTE: "path" input is either a single, non-quoted filename 
    // or a series of multiple quoted, space-delimited filenames:
    // examples:
    // 1. C:\data.kpg
    // 2. C:\Documents and Settings\username\Desktop\data.kpg
    // 3. "C:\data.kpg" "C:\Documents and Settings\username\Desktop\data.kpg"
    // As a result, if we have a quote, we parse based on space and remove
    // the quotes from the parsed pieces, or else we take the input as is
    std::vector<std::wstring> paths;
    std::vector<std::wstring>::iterator it, it_end;

    // quick space cleanup; then see if we have a delimited list or not; 
    // delimited list is characterized by a quote in the path (see #3 above)
    kl::trim(m_template.m_ii.path);

    size_t pos = m_template.m_ii.path.find(L"\"");
    if (pos == m_template.m_ii.path.npos)
    {
        // no delimter; just save the string
        paths.push_back(m_template.m_ii.path);
    }
     else
    {
        // get an array of strings from the space-delimited string
        kl::parseDelimitedList(m_template.m_ii.path, paths, wxT(' '), true);

        // if no strings were added to the array, add the whole path
        if (paths.size() == 0)
            paths.push_back(m_template.m_ii.path);
    }


    // remove the quotes from each of the paths along with
    // any leading/trailing spaces
    it_end = paths.end();
    for (it = paths.begin(); it != it_end; ++it)
    {
        kl::replaceStr(*it, L"\"", L"", true);
        kl::trimLeft(*it);
        kl::trimRight(*it);
    }

    // if we only have one element in the array, replace
    // the template path with the cleaned up version
    if (paths.size() == 1)
        m_template.m_ii.path = paths[0];


    // check to make sure all of the files exist
    for (it = paths.begin(); it != paths.end(); ++it)
    {
        if (!xf_get_file_exist(*it))
        {
            if (paths.size() > 1)
            {
                appMessageBox(_("One of the specified files does not exist or is invalid.  Please choose a valid file to continue."),
                                   _("Invalid File"),
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
            }
             else
            {
                appMessageBox(_("The specified file does not exist or is invalid.  Please choose a valid file to continue."),
                                   _("Invalid File"),
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
            }
            
            *allow = false;
            return;
        }
    }

    bool multiple_file_extensions = false;
    wxString first_ext;
    wxString ext;
    
    // check to make sure all the types are the same
    for (it = paths.begin(); it != paths.end(); ++it)
    {
        wxString name = *it;
        
        if (first_ext.IsEmpty())
            first_ext = name.AfterLast('.');
        
        // specify what type of import we're doing based on the file extension
        if (first_ext.CmpNoCase("kpg") == 0)
            m_template.m_ii.type = dbtypePackage;
         else if (first_ext.CmpNoCase("mdb") == 0)
            m_template.m_ii.type = dbtypeAccess;
         else if (first_ext.CmpNoCase("accdb") == 0)
            m_template.m_ii.type = dbtypeAccess;
         else if (first_ext.CmpNoCase("xls") == 0)
            m_template.m_ii.type = dbtypeExcel;
         else if (first_ext.CmpNoCase("xlsx") == 0)
            m_template.m_ii.type = dbtypeExcel;
         else if (first_ext.CmpNoCase("dbf") == 0)
            m_template.m_ii.type = dbtypeXbase;
         else
            m_template.m_ii.type = dbtypeDelimitedText;
            
        ext = name.AfterLast('.');
        
        // Package, Microsoft Access and Microsoft Excel files will all
        // populate their table lists in different locations, but for single
        // Xbase and text-delimited imports, we need to add the file here
        if (m_template.m_ii.type == dbtypeXbase ||
            m_template.m_ii.type == dbtypeDelimitedText)
        {
            ImportTableSelection its;
            its.show = true;
            its.selected = true;
            its.input_tablename = name;
            name = name.AfterLast(PATH_SEPARATOR_CHAR);
            if (name.Find(wxT('.')) != wxNOT_FOUND)
                name = name.BeforeLast(wxT('.'));
            its.output_tablename = name;
            
            m_template.m_ii.tables.push_back(its);
        }

        // see if we're importing multiple types; make sure to
        // compare extensions without case so that DBF = dbf, etc.
        if (ext.Lower() != first_ext.Lower())
            multiple_file_extensions = true;
    }

    // we can only import one filetype at a time for all imports
    // with the exception of the text-delimited import
    
    if (multiple_file_extensions &&
        m_template.m_ii.type != dbtypeDelimitedText)
    {
        appMessageBox(_("More than one file type was specified.  Please choose only one file type to continue."),
                           _("Multiple file types"),
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
                           
        m_template.m_ii.tables.clear();
        *allow = false;
        return;
    }
    
    for (it = paths.begin(); it != paths.end(); ++it)
    {
        wxString name = *it;
        ext = name.AfterLast('.');

        // we can only import one of these types of files at a time
        
        if (ext.CmpNoCase(wxT("kpg")) == 0 && paths.size() > 1)
        {
            appMessageBox(_("Only one package file can be imported at a time.  Please choose only one package file to continue."),
                          _("Too many files"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER);

            m_template.m_ii.tables.clear();
            *allow = false;
            return;
        }
         else if ((ext.CmpNoCase("mdb") == 0 || ext.CmpNoCase("accdb") == 0) && paths.size() > 1)
        {
            appMessageBox(_("Only one Microsoft Access file can be imported at a time.  Please choose only one Microsoft Access file to continue."),
                          _("Too many files"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER);

            m_template.m_ii.tables.clear();
            *allow = false;
            return;
        }
         else if ((ext.CmpNoCase("xls") == 0 || ext.CmpNoCase("xlsx") == 0) && paths.size() > 1)
        {
            appMessageBox(_("Only one Microsoft Excel file can be imported at a time.  Please choose only one Microsoft Excel file to continue."),
                          _("Too many files"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER);

            m_template.m_ii.tables.clear();
            *allow = false;
            return;
        }
    }
    
    
    // try to create an unmanaged connection for access and excel files
    if (m_template.m_ii.type == dbtypeAccess ||
        m_template.m_ii.type == dbtypeExcel)
    {
        IConnectionPtr conn = createUnmanagedConnection();
        conn->setType(m_template.m_ii.type);
        conn->setDescription(m_template.m_ii.description);
        conn->setHost(m_template.m_ii.server);
        conn->setPort(m_template.m_ii.port);
        conn->setDatabase(m_template.m_ii.database);
        conn->setUsername(m_template.m_ii.username);
        conn->setPassword(m_template.m_ii.password);
        conn->setPath(m_template.m_ii.path);

        // if we cannot open the connection, bail out
        if (!conn->open())
        {
            wxString error_message;
            if (m_template.m_ii.type == dbtypeAccess)
                error_message = _("There was an error opening the specified Microsoft Access file.  Make sure the file is not open by another application.");
             else if (m_template.m_ii.type == dbtypeExcel)
                error_message = _("There was an error opening the specified Microsoft Excel file.  Make sure the file is not open by another application.");
             else if (m_template.m_ii.type == dbtypeAccess)
                error_message = _("There was an error connecting to the specified database.");
            error_message += wxT("  ");
            error_message += conn->getErrorString();
            
            appMessageBox(error_message,
                               _("Import Wizard"),
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            *allow = false;
            return;
        }
    }

    updatePageRouting();
    

    *allow = true;
}

void ImportWizard::onDataSourceSelectionPageChanging(bool forward, bool* allow)
{
    if (!forward)
    {
        *allow = true;
        return;
    }

    m_template.m_ii.tables.clear();
    m_template.m_ii.field_mappings.clear();
    
    // if the ODBC data source doesn't need a username or password,
    // go directly to the table selection page
    if (m_template.m_ii.m_conn_ptr.isOk())
    {
        m_wizard->setPageOrder(m_datasource_selection_page,
                               m_table_selection_page);
        m_wizard->goPage(m_table_selection_page);
        
        // this will prevent the wizard from doing it's own page forward
        *allow = false;
    }
     else
    {
        m_wizard->setPageOrder(m_datasource_selection_page,
                               m_server_properties_page,
                               m_table_selection_page);
        m_wizard->goPage(m_server_properties_page);
        
        // this will prevent the wizard from closing
        *allow = false;
    }
}

void ImportWizard::onFileTypeChanged(int file_type)
{
    // set the parameters for the path selection page's file dialog
    if (file_type == dbtypeAccess ||
        file_type == dbtypeExcel  ||
        file_type == dbtypePackage)
    {
        m_path_selection_page->setBrowse(true, true, false);
    }
     else
    {
        m_path_selection_page->setBrowse(true, true, true);
    }

    m_path_selection_page->showElements(PathSelectionPage::showPath);

    if (file_type == dbtypeDelimitedText)
    {
        m_wizard->setPageOrder(m_path_selection_page,
                               m_delimitedtext_settings_page,
                               m_table_selection_page);
    }
     else
    {
        m_wizard->setPageOrder(m_path_selection_page,
                               m_table_selection_page);
    }

    m_template.m_ii.last_type = m_template.m_ii.type;
    m_template.m_ii.type = file_type;
    
    // we're changing import types, clear out this info
    m_template.m_ii.tables.clear();
    m_template.m_ii.field_mappings.clear();
}

void ImportWizard::onWizardCancelled(kcl::Wizard* wizard)
{
    m_frame->closeSite(m_doc_site);
}

void ImportWizard::onWizardFinished(kcl::Wizard* wizard)
{
    // NOTE: most of the error checking is in
    //       ImportTableSelectionPage::onPageChanging()
    

    // CHECK: check to make sure a database exists and is open
    if (!g_app->isDatabaseOpen())
    {
        appMessageBox(_("Please create or open a project to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    // CHECK: check to make sure the import folder will not overwrite
    //        any current items (folder included) in the database
    wxString folder_path;
    wxStringTokenizer t(m_template.m_ii.base_path, wxT("/"));
    while (t.HasMoreTokens())
    {
        wxString s = t.GetNextToken();

        if (s.IsEmpty())
            continue;

        folder_path += wxT("/");
        folder_path += s;

        // if the path exists in the database, check to see if it's a folder
        if (g_app->getDatabase()->getFileExist(towstr(folder_path)))
        {
            // if the object is a folder, we're fine
            xd::IFileInfoPtr info;
            info = g_app->getDatabase()->getFileInfo(towstr(folder_path));
            if (info->getType() == xd::filetypeFolder)
                continue;
            
            // the object is not a folder, don't allow the user to create
            // a folder that will overwrite an object in the database
            appMessageBox(
                _("The specified import folder is invalid.  Check to make sure the path does not contain any invalid characters\nand that it will not create a folder which will overwrite an item in the project."),
                _("Import Wizard"),
                wxOK | wxICON_EXCLAMATION | wxCENTER);
            m_template.m_ii.tables.clear();
            return;
        }
    }
    
    // CHECK: check to make sure we're not overwriting any
    //        any items in the database
    bool overwrite = false;
    std::vector<ImportTableSelection>::iterator it;
    for (it = m_template.m_ii.tables.begin();
         it != m_template.m_ii.tables.end(); ++it)
    {
        // don't include unselected tables in the check
        if (!it->selected)
            continue;
        
        wxString tablename = it->output_tablename;
        wxString check_path = m_template.m_ii.base_path;

        // handle empty base path
        if (check_path.IsEmpty())
            check_path += wxT('/');

        // handle no slash between base path and tablename
        if (check_path.Last() != wxT('/') && !tablename.StartsWith(wxT("/")))
            check_path += wxT("/");

        // handle double slash between base path and tablename
        if (check_path.Last() == wxT('/') && tablename.StartsWith(wxT("/")))
            check_path.RemoveLast();

        check_path += tablename;

        if (g_app->getDatabase()->getFileExist(towstr(check_path)))
        {
            xd::IFileInfoPtr info;
            info = g_app->getDatabase()->getFileInfo(towstr(check_path));
            
            // we're appending to this table, we're fine
            if (it->append && info->getType() == xd::filetypeTable)
                continue;

            // don't allow imported items to overwrite folders
            if (info->getType() == xd::filetypeFolder)
            {
                appMessageBox(
                    _("One or more items has the same path as an existing folder in the project.  Please change the output paths of these items to continue."),
                    _("Import Wizard"),
                    wxOK | wxICON_EXCLAMATION | wxCENTER);
                m_template.m_ii.tables.clear();
                return;
            }

            if (!overwrite)
            {
                int result = appMessageBox(
                    _("One or more items has the same path as an existing item in the project.  Would you like to overwrite these items?"),
                    _("Import Wizard"),
                    wxYES_NO | wxCANCEL | wxICON_EXCLAMATION | wxCENTER,
                    g_app->getMainWindow());
                
                if (result == wxYES)
                {
                    // don't show this error message again
                    overwrite = true;
                }
                 else if (result == wxNO)
                {
                    m_template.m_ii.tables.clear();
                    return;
                }
                 else
                {
                    return;
                }
            }
        }
    }

    sigImportWizardFinished(this);
    m_frame->closeSite(m_doc_site);
}

bool ImportWizard::loadTemplate(const wxString& path)
{
    return m_template.load(towstr(path));
}

bool ImportWizard::saveTemplate(const wxString& path)
{
    // update the import template
    m_path_selection_page->savePageData();
    m_datasource_selection_page->savePageData();
    m_server_properties_page->savePageData();
    m_table_selection_page->savePageData();
    m_delimitedtext_settings_page->savePageData();

    return m_template.save(towstr(path));
}


void ImportWizard::onSave(wxCommandEvent& evt)
{
    if (!g_app->isDatabaseOpen())
    {
        appMessageBox(_("To save the import template, please create or open a project"),
                      _("Import Wizard"),
                      wxOK | wxICON_EXCLAMATION | wxCENTER);

        return;
    }


    DlgDatabaseFile dlg(g_app->getMainWindow(),
                        DlgDatabaseFile::modeSave);
    dlg.setCaption(_("Save Import Template"));
    dlg.setOverwritePrompt(true);
    if (dlg.ShowModal() != wxID_OK)
    {
        return;
    }

    saveTemplate(dlg.getPath());
    
    g_app->getAppController()->refreshDbDoc();
}




void ImportWizard::onClose(wxCommandEvent& evt)
{
    m_frame->closeSite(m_doc_site);
}

void ImportWizard::onSize(wxSizeEvent& evt)
{
    Layout();
}

