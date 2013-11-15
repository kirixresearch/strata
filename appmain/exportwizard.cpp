/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-08-14
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "connectionpages.h"
#include "exportwizard.h"
#include "exportpages.h"
#include "structurevalidator.h"
#include "dlgcustomprompt.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/artprov.h>


#ifdef FindWindow
#undef FindWindow
#endif


// ExportWizard class implementation

BEGIN_EVENT_TABLE(ExportWizard, wxWindow)
    EVT_BUTTON(wxID_CANCEL, ExportWizard::onClose)
    EVT_SIZE(ExportWizard::onSize)
END_EVENT_TABLE()


ExportWizard::ExportWizard()
{
    m_wizard = NULL;
    m_connection_bar = NULL;

    m_path_selection_page = NULL;
    m_datasource_selection_page = NULL;
    m_server_properties_page = NULL;
    m_table_selection_page = NULL;
    m_delimitedtext_settings_page = NULL;

    // save the base path to the registry
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    prefs->remove(wxT("exportwizard.default_kpg_compressed"));  // old registry entry (2005.1 and older)
    prefs->flush();
    
    m_template.m_ei.kpg_compressed = true;  // compressed/uncompressed is no longer exposed
                                            // on the interface, so always use compression
}

ExportWizard::~ExportWizard()
{
}

bool ExportWizard::initDoc(IFramePtr frame,
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

    doc_site->setCaption(_("Export"));
    wxSize min_site_size = doc_site->getContainerWindow()->GetSize();
    doc_site->setMinSize(min_site_size.x, min_site_size.y);
    
    m_frame = frame;
    m_doc_site = doc_site;

    // create connection bar
    m_connection_bar = new ConnectionBar(this, -1, conntypeExport);
    m_connection_bar->sigConnectionTypeChanged.connect(this, &ExportWizard::onExportTypeChanged);

    // create the wizard
    m_wizard = new kcl::Wizard(this, -1);
    m_wizard->sigCancelPressed.connect(this, &ExportWizard::onWizardCancelled);
    m_wizard->sigFinishPressed.connect(this, &ExportWizard::onWizardFinished);
    
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

    m_path_selection_page = new PathSelectionPage(m_wizard, &m_template.m_ei);
    m_datasource_selection_page = new DataSourceSelectionPage(m_wizard, &m_template.m_ei);
    m_server_properties_page = new ServerPropertiesPage(m_wizard, &m_template.m_ei);
    m_table_selection_page = new ExportTableSelectionPage(m_wizard, &m_template.m_ei);
    m_delimitedtext_settings_page = new ExportDelimitedTextSettingsPage(m_wizard, &m_template.m_ei);

    m_wizard->addPage(m_path_selection_page, _("Path Selection"));
    m_wizard->addPage(m_datasource_selection_page, _("ODBC Data Source Selection"));
    m_wizard->addPage(m_server_properties_page, _("Server Properties"));
    m_wizard->addPage(m_table_selection_page, _("Table Selection"));
    m_wizard->addPage(m_delimitedtext_settings_page, _("Text-Delimited Settings"));

    m_path_selection_page->setMessage(_("Please enter the location of the database or directory to which you would like to export."));
    m_path_selection_page->setFileTypeLabel(_("Export as:"));
    m_path_selection_page->showElements(PathSelectionPage::showPath | PathSelectionPage::showFileType);
    m_path_selection_page->setBrowse(true, false, false);
    
    // connect page signals
    m_path_selection_page->sigFileTypeChanged.connect(this, &ExportWizard::onFileTypeChanged);
    m_path_selection_page->sigPageChanging.connect(this, &ExportWizard::onPathSelectionPageChanging);
    m_datasource_selection_page->sigPageChanging.connect(this, &ExportWizard::onDataSourceSelectionPageChanging);

    // start the wizard off with the default type
    int default_type = m_connection_bar->getDefaultConnectionType();
    m_connection_bar->selectConnectionType(default_type);
    return true;
}

wxWindow* ExportWizard::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ExportWizard::setDocumentFocus()
{
}

ExportTemplate& ExportWizard::getTemplate()
{
    return m_template;
}

void ExportWizard::onExportTypeChanged(int type)
{
    int export_type = dbtypeUndefined;
    
    // figure out the export type, if we can
    switch (type)
    {
        case ConnectionBar::typeMySql:     export_type = dbtypeMySql;        break;
        case ConnectionBar::typePostgres:  export_type = dbtypePostgres;     break;
        case ConnectionBar::typeSqlServer: export_type = dbtypeSqlServer;    break;
        case ConnectionBar::typeOracle:    export_type = dbtypeOracle;       break;
        case ConnectionBar::typeDb2:       export_type = dbtypeDb2;          break;
        case ConnectionBar::typeOdbc:      export_type = dbtypeOdbc;         break;
    }
    
    // we didn't change anything, we're done
    if (m_template.m_ei.type == export_type &&
        type != ConnectionBar::typeFile)
        return;

    // handle page routing
    switch (type)
    {
        case ConnectionBar::typeFile:
        {
            // calling this function will actually elicit a sigFileTypeChanged
            // signal being fired by the PathSelectionPage, which will be
            // handled in the ExportWizard::onFileTypeChanged() function
            export_type = m_path_selection_page->getFileType();
            break;
        }
        
        case ConnectionBar::typeSqlServer:
        case ConnectionBar::typeMySql:
        case ConnectionBar::typePostgres:
        case ConnectionBar::typeOracle:
        case ConnectionBar::typeDb2:
        {
            if (export_type == dbtypeMySql)     { m_template.m_ei.port = 3306;  }
            if (export_type == dbtypePostgres)  { m_template.m_ei.port = 5432;  }
            if (export_type == dbtypeSqlServer) { m_template.m_ei.port = 1433;  }
            if (export_type == dbtypeOracle)    { m_template.m_ei.port = 1521;  }
            if (export_type == dbtypeDb2)       { m_template.m_ei.port = 50000; }

            m_wizard->setPageOrder(m_server_properties_page,
                                   m_table_selection_page);

            // update the page message based on the import type
            wxString message = _("Please enter the connection settings for the database to which you would like to export.");
            m_server_properties_page->setMessage(message);

            m_server_properties_page->showElements(
                                ServerPropertiesPage::showAll &
                                ~ServerPropertiesPage::showSavePassword);
            break;
        }
        
        case ConnectionBar::typeOdbc:
        {
            m_wizard->setPageOrder(m_datasource_selection_page,
                                   m_server_properties_page,
                                   m_table_selection_page);
           
            // update the page message based on the export type
            wxString message = _("Please select the ODBC data source to which you would like to export.");
            m_datasource_selection_page->setMessage(message);
            
            message = _("Please enter the username and password for the selected database.");
            m_server_properties_page->setMessage(message);

            m_server_properties_page->showElements(
                                ServerPropertiesPage::showUsername |
                                ServerPropertiesPage::showPassword);
            break;
        }
    }
    
    // export_type will be "dbtypeUndefined" for all
    // file exports except for package files
    m_template.m_ei.last_type = m_template.m_ei.type;
    m_template.m_ei.type = export_type;
    
    // we're changing export types, clear out this info
    m_template.m_ei.tables.clear();
    //m_template.m_ei.path = wxEmptyString;

    // set the control values based on the connection info
    m_path_selection_page->loadPageData();
    m_server_properties_page->loadPageData();

    m_wizard->startWizard(0);
    Layout();
}

static inline bool isFileExport(int type)
{
    if (type == dbtypePackage ||
        type == dbtypeAccess ||
        type == dbtypeExcel)
    {
        return true;
    }
    
    return false;
}

static inline bool isFolderExport(int type)
{
    if (type == dbtypeFilesystem ||
        type == dbtypeXbase ||
        type == dbtypeDelimitedText ||
        type == dbtypeFixedLengthText)
    {
        return true;
    }
    
    return false;
}

void ExportWizard::onPathSelectionPageChanging(bool forward, bool* allow)
{
    if (!forward)
    {
        *allow = true;
        return;
    }
    
    ExportInfo* ei = &(m_template.m_ei);
    std::wstring ext = kl::afterLast(ei->path, '.');
    
    if (ei->path.empty())
    {
        // an empty path is not a valid path
        appMessageBox(_("A valid location needs to be specified to continue."),
                      _("Invalid Location"),
                      wxOK | wxICON_EXCLAMATION | wxCENTER,
                      g_app->getMainWindow());
        *allow = false;
        return;
    }
    
    if (isFileExport(ei->type))
    {
        // a directory path is not a valid file path
        if (xf_get_directory_exist(ei->path))
        {
            appMessageBox(_("The specified location is a folder.  Please specify a valid file location to continue."),
                          _("Invalid Location"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER);
            *allow = false;
            return;
        }
        
        if (xf_get_file_exist(ei->path))
        {
            int ID_AppendToFile = 100;
            int ID_OverwriteFile = 101;
            
            CustomPromptDlg dlg(this, 
                                _("Overwrite File?"),
                                _("The file name specified already exists.  Would you like to append to or overwrite this file?\n\nNOTE: Selecting 'Overwrite' will not actually overwrite the file until the 'Finish' button is pressed on the next page."));
            dlg.setButton1(ID_AppendToFile, _("Append"));
            dlg.setButton2(ID_OverwriteFile, _("Overwrite"));
            
            int result = dlg.ShowModal();
            if (result == ID_AppendToFile)
            {
                ei->overwrite_file = false;
            }
             else if (result == ID_OverwriteFile)
            {
                ei->overwrite_file = true;
            }
             else
            {
                // user pressed the 'Cancel' button
                *allow = false;
                return;
            }
        }
         else
        {
            // fix wrong file extension for package files
            if (ei->type == dbtypePackage && !kl::iequals(ext, L"kpg"))
                ei->path += L".kpg";
            
            // fix wrong file extension for Microsoft Access files
            if (ei->type == dbtypeAccess && !kl::iequals(ext, L"mdb"))
                ei->path += L".mdb";
            
            // fix wrong file extension for Microsoft Excel files
            if (ei->type == dbtypeExcel && !kl::iequals(ext, L"xls"))
                ei->path += L".xls";
            
            // check to see if the path entered is valid
            /*
            bool valid = true;
            wxFileName filename(ei->path);
                
            #ifdef WIN32
                if (filename.GetVolume().IsEmpty())
                    valid = false;
            #endif
            
            if (!filename.IsOk())
                valid = false;
            */
            
            if (!xf_is_valid_file_path(ei->path))
            {
                appMessageBox(_("The specified file path is invalid.  Please specify a valid file path to continue."),
                              _("Invalid File"),
                              wxOK | wxICON_EXCLAMATION | wxCENTER);
                *allow = false;
                return;
            }
            
            // note: we used to include a notice asking the user if they wanted
            // to create the specified file, which proved confusing; no need to 
            // ask the user if they want to create the file since they are exporting 
            // which already indicates they want to create the file
        }
    }
    
    if (isFolderExport(ei->type))
    {
        // do we have a valid folder path?
        if (!xf_get_directory_exist(ei->path))
        {
            appMessageBox(_("The specified folder path is invalid.  Please specify a valid folder path to continue."),
                          _("Invalid Folder"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER);
            *allow = false;
            return;
        }
        
        // ei->path is what is changed on the PathSelectionPage,
        // but ei->base_path is what is used by ExportJob
        ei->base_path = ei->path;
    }
    
    // update the export path label on the table selection page
    wxString exportpath_label = _("Export to");
    exportpath_label += wxT(" '");
    exportpath_label += ei->path;
    exportpath_label += wxT("'");
    m_table_selection_page->setExportPathLabel(exportpath_label);
    
    *allow = true;
    return;
}

void ExportWizard::onDataSourceSelectionPageChanging(bool forward, bool* allow)
{
    if (!forward)
    {
        *allow = true;
        return;
    }

    m_template.m_ei.tables.clear();
    
    // if the ODBC data source doesn't need a username or password,
    // go directly to the table selection page
    if (m_template.m_ei.m_conn_ptr.isOk())
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

void ExportWizard::onFileTypeChanged(int file_type)
{
    // set the default path if it hasn't yet been set
    wxString path = m_path_selection_page->getPath();
    if (path.Length() <= 0)
        path = getDefaultExportPath();

    // get the directory
    if (xf_get_directory_exist(towstr(path)))
    {
        if (path.Last() != PATH_SEPARATOR_CHAR)
            path += PATH_SEPARATOR_CHAR;
    }
    else
    {
        path = path.BeforeLast(PATH_SEPARATOR_CHAR);
        path += PATH_SEPARATOR_CHAR;
    }

    // set the parameters for the path selection page's file/directory dialog
    if (file_type == dbtypeAccess)
    {
        path += wxT("untitled.mdb");
        
        m_path_selection_page->setMessage(_("Please enter the location of the Microsoft Access file to which you would like to export."));
        m_path_selection_page->setPathLabel(_("File:"));
        m_path_selection_page->setPath(path);
        m_path_selection_page->setBrowse(true, false, false);
    }
     else if (file_type == dbtypeExcel)
    {
        path += wxT("untitled.xls");
        
        m_path_selection_page->setMessage(_("Please enter the location of the Microsoft Excel file to which you would like to export."));
        m_path_selection_page->setPathLabel(_("File:"));
        m_path_selection_page->setPath(path);
        m_path_selection_page->setBrowse(true, false, false);
    }
     else if (file_type == dbtypePackage)
    {
        path += wxT("untitled.kpg");
        
        m_path_selection_page->setMessage(_("Please enter the location of the package file to which you would like to export."));
        m_path_selection_page->setPathLabel(_("File:"));
        m_path_selection_page->setPath(path);
        m_path_selection_page->setBrowse(true, false, false);
    }
     else if (file_type == dbtypeDelimitedText ||
              file_type == dbtypeFixedLengthText ||
              file_type == dbtypeXbase)
    {
        m_path_selection_page->setMessage(_("Please enter the location of the folder to which you would like to export."));
        m_path_selection_page->setPathLabel(_("Folder:"));
        m_path_selection_page->setPath(path);
        m_path_selection_page->setBrowse(false, false, false);
    }

    m_path_selection_page->showElements(PathSelectionPage::showPath |
                                        PathSelectionPage::showFileType);

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

    m_template.m_ei.last_type = m_template.m_ei.type;
    m_template.m_ei.type = file_type;
    
    // we're changing export types, clear out this info
    m_template.m_ei.tables.clear();
}

void ExportWizard::onWizardCancelled(kcl::Wizard* wizard)
{
    m_frame->closeSite(m_doc_site);
}

static void deleteDirs(std::vector<wxString> dirs)
{
    std::vector<wxString>::reverse_iterator it;
    for (it = dirs.rbegin(); it != dirs.rend(); ++it)
    {
        std::wstring dir = towstr(*it);
        xf_rmdir(dir);
    }
}

inline wxString createFullFilePath(const wxString& base_path, const wxString& filename)
{
    wxString retval = base_path;
    
    if (retval.Last() != PATH_SEPARATOR_CHAR &&
        !filename.StartsWith(PATH_SEPARATOR_STR))
    {
        retval += PATH_SEPARATOR_CHAR;
    }

    retval += filename;
    return retval;
}


void ExportWizard::onWizardFinished(kcl::Wizard* wizard)
{
    // NOTE: there is also some basic error checking in
    //       ExportTableSelectionPage::onPageChanging()
    
    // if we're on a file or folder type, save the export path preference
    int connection_type = m_connection_bar->getSelectedConnectionType();
    if (connection_type == ConnectionBar::typeFile || 
        connection_type == ConnectionBar::typeFolder)
    {
        setDefaultExportPath(m_template.m_ei.path);
    }

    // CHECK: check to make sure a database exists and is open
    if (!g_app->isDatabaseOpen())
    {
        appMessageBox(_("Please create or open a project to continue."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    // CHECK: check to make sure we're exporting at least one table
    if (m_template.m_ei.tables.size() == 0)
    {
        appMessageBox(_("You must select at least one table to export."),
                           _("Export Wizard"),
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    
    xd::IDatabasePtr local_db = g_app->getDatabase();
            

    // PACKAGE FILE: do some error checking and start the export
    
    if (m_template.m_ei.type == dbtypePackage)
    {
        if (!xf_get_file_exist(m_template.m_ei.path) ||
             m_template.m_ei.overwrite_file)
        {
            sigExportWizardFinished(this);
            m_frame->closeSite(m_doc_site);
            return;
        }


        /*

        TODO: reimplement

        // CHECK: is the package file already open or in use?
        PkgFile pkgfile;
        if (!pkgfile.open(towstr(m_template.m_ei.path), PkgFile::modeRead))
        {
            appMessageBox(_("There was an error connecting to the specified package file.\nPlease make sure that the file exists and that it is not currently in use."),
                               _("Export Wizard"),
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        // CHECK: is there something wrong with the package file?
        PkgStreamEnum* stream_enum = pkgfile.getStreamEnum();
        if (!stream_enum)
        {
            appMessageBox(_("The specified package file is either empty or corrupt."),
                               _("Export Wizard"),
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        bool tablenames_already_in_pkg_file = false;
        
        int stream_count = stream_enum->getStreamCount();
        for (int i = 0; i < stream_count; ++i)
        {
            wxString tablename = stream_enum->getStreamName(i);

            // don't show hidden stream names
            if (*(tablename.c_str()) == '.')
                continue;

            int row = 0;
            
            std::vector<ExportTableSelection>::iterator it;
            for (it = m_template.m_ei.tables.begin();
                 it != m_template.m_ei.tables.end(); ++it)
            {
                // CHECK: are there duplicate table names in the package file
                if (!it->output_tablename.CmpNoCase(tablename))
                {
                    m_table_selection_page->markProblemRow(row, false);
                    tablenames_already_in_pkg_file = true;
                    break;
                }

                row++;
            }
        }

        delete stream_enum;

        if (tablenames_already_in_pkg_file)
        {
            // show what rows contain errors
            m_table_selection_page->refreshGrid();
            
            int result = appMessageBox(_("One or more of the table names specified for export already exists.  Would you like to overwrite these tables?"),
                                            _("Export Wizard"),
                                            wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION | wxCENTER);
            if (result == wxNO)
                return;
        }

        sigExportWizardFinished(this);
        m_frame->closeSite(m_doc_site);
        return;
        */
    }
    
    
    // XBASE/TEXT-DELIMITED: do some error checking and start the export
    
    if (m_template.m_ei.type == dbtypeXbase ||
        m_template.m_ei.type == dbtypeDelimitedText)
    {
        xd::IDatabasePtr fs_db;
        fs_db.create_instance("xdfs.Database");

        xd::IAttributesPtr attr;
        attr = fs_db->getAttributes();
        
        std::vector<wxString> dirs_created; // in case we have to remove the dirs

        // CHECK: is the path of the destination folder empty?
        if (m_template.m_ei.base_path.empty())
        {
            appMessageBox(_("The specified destination folder is invalid.  Please specify a valid destination folder to continue."),
                               _("Export Wizard"),
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        // CHECK: does the destination folder exist?
        if (!xf_get_directory_exist(m_template.m_ei.base_path))
        {
            int result = appMessageBox(_("The specified destination folder does not exist.  Would you like to create it?"),
                                            _("Export Wizard"),
                                            wxYES_NO | wxICON_QUESTION | wxCENTER);
            if (result == wxYES)
            {
                // traverse the path given and create all directories that do not exist

                wxString dirpath = kl::beforeFirst(m_template.m_ei.base_path, PATH_SEPARATOR_CHAR);
                wxString remainder = kl::afterFirst(m_template.m_ei.base_path, PATH_SEPARATOR_CHAR);

                if (dirpath.IsEmpty())
                    dirpath = PATH_SEPARATOR_CHAR;

                while (1)
                {
                    if (!xf_get_directory_exist(towstr(dirpath)))
                    {
                        // CHECK: can we create the folder?
                        if (!xf_mkdir(towstr(dirpath)))
                        {
                            // we have encountered an error, delete all
                            // of the directories that we created
                            deleteDirs(dirs_created);
                            
                            appMessageBox(_("The specified destination folder could not be created.  Please make sure the specified destination folder is valid."),
                                               _("Export Wizard"),
                                               wxOK | wxICON_EXCLAMATION | wxCENTER);
                            return;
                        }

                        dirs_created.push_back(dirpath);
                    }

                    // there are no more path separators, we're done
                    if (remainder.IsEmpty())
                        break;
                    
                    dirpath += PATH_SEPARATOR_CHAR;
                    dirpath += remainder.BeforeFirst(PATH_SEPARATOR_CHAR);
                    remainder = remainder.AfterFirst(PATH_SEPARATOR_CHAR);
                }
            }

            // the user does not want to create the destination folder, bail out
            if (result == wxNO)
                return;
        }

        int row = 0;
        xf_file_t file;
        wxString filename;
        bool input_tables_missing = false;
        bool filenames_already_exist = false;
        bool invalid_filenames_exist = false;
        bool invalid_fieldnames_exist = false;
        
        std::vector<int>::iterator row_it;
        std::vector<int> already_existing_files_rows;
        std::vector<int> missing_input_table_rows;
        std::vector<int> invalid_fieldnames_rows;
        std::vector<int> invalid_filenames_rows;
        
        // CHECK: can we create all of the files?
        std::vector<ExportTableSelection>::iterator it;
        for (it = m_template.m_ei.tables.begin();
             it != m_template.m_ei.tables.end(); ++it)
        {
            filename = createFullFilePath(m_template.m_ei.base_path,
                                          it->output_tablename);

            // CHECK: does the specified file have a valid filename?
            if (!xf_get_file_exist(towstr(filename)))
            {
                // try to create the file
                file = xf_open(towstr(filename), xfCreate, xfReadWrite, xfShareNone);
                if (!file)
                {
                    invalid_filenames_rows.push_back(row);
                    invalid_filenames_exist = true;
                }

                // close and delete the file if we successfully opened it
                xf_close(file);
                xf_remove(towstr(filename));
            }
            
            row++;
        }

        if (invalid_filenames_exist)
        {
            m_table_selection_page->clearProblemRows();

            for (row_it = invalid_filenames_rows.begin();
                 row_it != invalid_filenames_rows.end(); ++row_it)
            {
                m_table_selection_page->markProblemRow(*row_it, false);
            }

            // show what rows contain errors
            m_table_selection_page->refreshGrid();
            
            // we have encountered an error, delete all
            // of the directories that we created
            deleteDirs(dirs_created);
            
            appMessageBox(_("One or more files specified for export is invalid.  Please rename these files to continue."),
                               _("Export Wizard"),
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        row = 0;
        for (it = m_template.m_ei.tables.begin();
             it != m_template.m_ei.tables.end(); ++it)
        {
            xd::IStructurePtr structure = local_db->describeTable(it->input_tablename);

            if (structure.isNull())
            {
                missing_input_table_rows.push_back(row);
                input_tables_missing = true;
                continue;
            }
            
            bool found = StructureValidator::findInvalidFieldNames(structure,
                                                                   fs_db);
            if (found)
            {
                invalid_fieldnames_rows.push_back(row);
                invalid_fieldnames_exist = true;
            }
                        
            row++;
        }

        if (input_tables_missing)
        {
            m_table_selection_page->clearProblemRows();

            for (row_it = missing_input_table_rows.begin();
                 row_it != missing_input_table_rows.end(); ++row_it)
            {
                m_table_selection_page->markProblemRow(*row_it, false);
            }
            
            // show what rows contain errors
            m_table_selection_page->refreshGrid();
        
            // we have encountered an error, delete all
            // of the directories that we created
            deleteDirs(dirs_created);

            int result = appMessageBox(_("One or more of the tables no longer exists.  Please remove these tables from the export to continue."),
                                            _("Export Wizard"),
                                            wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        if (invalid_fieldnames_exist)
        {
            m_table_selection_page->clearProblemRows();

            for (row_it = invalid_fieldnames_rows.begin();
                 row_it != invalid_fieldnames_rows.end(); ++row_it)
            {
                m_table_selection_page->markProblemRow(*row_it, false);
            }
            
            // show what rows contain errors
            m_table_selection_page->refreshGrid();
        
            int ID_FixAndContinue = 100;
            int ID_Cancel = 101;

            wxString appname = APPLICATION_NAME;
            wxString message = wxString::Format(_("One or more of the tables contains fieldnames which are invalid in the export destination.  Would you like to have %s fix these fieldnames when performing the export?\n\nNOTE: Only the exported tables will have their fieldnames changed."),
                                                appname.c_str());
            CustomPromptDlg dlg(m_frame->getFrameWindow(),
                                _("Fix Invalid Fieldnames?"),
                                message,
                                wxSize(420,220));
            dlg.setButton1(ID_FixAndContinue, _("Fix Fieldnames"));
            dlg.setButton2(ID_Cancel, _("Cancel"));
            dlg.showButtons(CustomPromptDlg::showButton1 |
                            CustomPromptDlg::showButton2);
            
            int result = dlg.ShowModal();
            if (result != ID_FixAndContinue)
            {
                // we're not going to continue, delete all
                // of the directories that we created
                deleteDirs(dirs_created);
                return;
            }
            
            m_template.m_ei.fix_invalid_fieldnames = true;
        }

        row = 0;
        for (it = m_template.m_ei.tables.begin();
             it != m_template.m_ei.tables.end(); ++it)
        {
            filename = createFullFilePath(m_template.m_ei.base_path,
                                          it->output_tablename);

            // CHECK: does the specified file already exist?
            if (xf_get_file_exist(towstr(filename)))
            {
                already_existing_files_rows.push_back(row);
                filenames_already_exist = true;
            }
            
            row++;
        }

        if (filenames_already_exist)
        {
            m_table_selection_page->clearProblemRows();

            for (row_it = already_existing_files_rows.begin();
                 row_it != already_existing_files_rows.end(); ++row_it)
            {
                m_table_selection_page->markProblemRow(*row_it, false);
            }

            // show what rows contain errors
            m_table_selection_page->refreshGrid();
            
            int result = appMessageBox(_("One or more files specified for export already exists.  Would you like to overwrite these files?"),
                                            _("Export Wizard"),
                                            wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION | wxCENTER);
            if (result == wxNO)
            {
                // we have encountered an error, delete all
                // of the directories that we created
                deleteDirs(dirs_created);
                return;
            }
        }

        // every filename has been validated, we can continue

        for (it = m_template.m_ei.tables.begin();
             it != m_template.m_ei.tables.end(); ++it)
        {
            filename = createFullFilePath(m_template.m_ei.base_path,
                                          it->output_tablename);

            it->output_tablename = filename;
        }

        sigExportWizardFinished(this);
        m_frame->closeSite(m_doc_site);
        return;
    }
    
    
    // ALL OTHER EXPORTS: do some error checking and start the export
    
    IConnectionPtr conn = createUnmanagedConnection();
    conn->setType(m_template.m_ei.type);
    conn->setDescription(m_template.m_ei.description);
    conn->setHost(m_template.m_ei.server);
    conn->setPort(m_template.m_ei.port);
    conn->setDatabase(m_template.m_ei.database);
    conn->setUsername(m_template.m_ei.username);
    conn->setPassword(m_template.m_ei.password);
    conn->setPath(m_template.m_ei.path);

    AppBusyCursor bc;

    int conn_type = conn->getType();
    wxString conn_path = conn->getPath();
    
    // the location to copy the existing file to while we do our error checking;
    // if anything fails, copy this file back to its original location
    std::wstring existing_file_temp_loc;

    // CHECK: overwrite access or excel ok?
    if ((conn_type == dbtypeAccess || conn_type == dbtypeExcel) &&
        m_template.m_ei.overwrite_file)
    {
        // since we are dealing with an Access or Excel file,
        // we need to create the file before adding tables to it

        xd::IDatabaseMgrPtr db_mgr;
        db_mgr.create_instance("xdodbc.DatabaseMgr");
        if (db_mgr.isNull())
        {
            wxString appname = APPLICATION_NAME;
            appMessageBox(wxString::Format(_("%s is missing a software component.  To correct this problem, please reinstall %s."),
                                                appname.c_str(), appname.c_str()),
                               APPLICATION_NAME,
                               wxOK | wxICON_INFORMATION | wxCENTER);
            return;
        }
        
        xd::IDatabasePtr db_ptr;
        xd::IFileInfoEnumPtr items;
        xd::IFileInfoPtr info;

        wxString filetype_name;
        if (conn_type == dbtypeAccess)
            filetype_name = _("Microsoft Access file");
         else if (conn_type == dbtypeExcel)
            filetype_name = _("Microsoft Excel file");
         else
            filetype_name = _("file");
        
        // save the existing file as a temp file 
        if (xf_get_file_exist(towstr(conn_path)))
        {
            if (!xf_remove(towstr(conn_path)))
            {
                appMessageBox(wxString::Format(_("There specified %s is currently in use or you may not have permission to overwrite it.\nPlease make sure that the file to be overwritten is not currently in use."),
                                               filetype_name.c_str()),
                              _("Export Wizard"),
                              wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }
        }


        std::wstring cstr = L"Xdprovider=xdodbc;database=" + towstr(conn_path);

        bool res = db_mgr->createDatabase(cstr);
        if (!res)
        {
            appMessageBox(wxString::Format(_("There was an error connecting to the specified %s.\nPlease make sure that the file exists and that it is not currently in use."), filetype_name.c_str()),
                          _("Export Wizard"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }


        // if we still cannot open a connection to the file after
        // creating the file, bail out
        if (!conn->open())
        {
            xf_remove(towstr(conn_path));
            appMessageBox(wxString::Format(_("There was an error connecting to the specified %s.\nPlease make sure that the file exists and that it is not currently in use."),
                                           filetype_name.c_str()),
                          _("Export Wizard"),
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        conn.clear();
        xf_remove(towstr(conn_path));
    }
     else
    {
        // CHECK: can we open the existing database?
        if (!conn->open())
        {
            appMessageBox(_("There was an error connecting to the specified database.\nPlease make sure that the connection information for the database is correct."),
                          _("Export Wizard"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
    }


    // get the destination database pointer and list of existing tables
    xd::IDatabasePtr db_ptr = conn->getDatabasePtr();
    xd::IFileInfoEnumPtr tables = db_ptr->getFolderInfo(L"/");


    // CHECK: are all of the specified tablenames valid?
    std::vector<wxString> invalid_tablenames;

    int row = 0;
    std::vector<ExportTableSelection>::iterator it;
    for (it = m_template.m_ei.tables.begin();
         it != m_template.m_ei.tables.end(); ++it)
    {
        if (!isValidObjectName(it->output_tablename, db_ptr))
        {
            m_table_selection_page->markProblemRow(row, true);
            invalid_tablenames.push_back(it->output_tablename);
        }
        
        row++;
    }

    if (invalid_tablenames.size() > 0)
    {
        // show what rows contain errors
        m_table_selection_page->refreshGrid();
        
        appMessageBox(_("One or more of the tables specified for export has an invalid name."),
                      _("Export Wizard"),
                      wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    // CHECK: do all of the tables still exist in the source project?
    std::vector<wxString> missing_tables;
    
    row = 0;
    for (it = m_template.m_ei.tables.begin();
         it != m_template.m_ei.tables.end(); ++it)
    {
        if (!isValidTable(it->input_tablename, g_app->getDatabase()))
        {
            m_table_selection_page->markProblemRow(row, true);
            missing_tables.push_back(it->input_tablename);
        }

        row++;
    }
    
    if (missing_tables.size() > 0)
    {
        // show what rows contain errors
        m_table_selection_page->refreshGrid();
        
        appMessageBox(_("One or more of the tables specified for export no longer exists.\nRemove these tables from the list to continue."),
                      _("Export Wizard"),
                      wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    

    xd::IDatabasePtr db = g_app->getDatabase();

    // CHECK: are all of the fieldnames in the tables valid?
    std::vector<wxString> invalid_fieldnames;

    row = 0;
    for (it = m_template.m_ei.tables.begin();
         it != m_template.m_ei.tables.end(); ++it)
    {
        xd::IStructurePtr s = db->describeTable(it->input_tablename);
        if (s.isNull())
            continue;

        int i, col_count = s->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            xd::IColumnInfoPtr colinfo = s->getColumnInfoByIdx(i);
            wxString colname = colinfo->getName();

            if (!isValidFieldName(colname, db_ptr))
            {
                m_table_selection_page->markProblemRow(row, true);
                invalid_fieldnames.push_back(colname.MakeUpper());
            }
        }

        row++;
    }

    if (invalid_fieldnames.size() > 0)
    {
        // show what rows contain errors
        m_table_selection_page->refreshGrid();
        
        appMessageBox(_("One or more of the tables specified for export contains invalid fieldnames."),
                      _("Export Wizard"),
                      wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    

    // determine if we are exporting to a database that is contained in a file
    bool database_file = false;
    bool tablenames_already_in_database = false;
    if (m_template.m_ei.type == dbtypePackage ||
        m_template.m_ei.type == dbtypeAccess ||
        m_template.m_ei.type == dbtypeExcel)
    {
        database_file = true;
    }
    
    if ((database_file && !m_template.m_ei.overwrite_file) || !database_file)
    {
        // CHECK: Are we overwriting tables in the destination database?
        row = 0;
        for (it = m_template.m_ei.tables.begin();
             it != m_template.m_ei.tables.end(); ++it)
        {
            if (it->append)
            {
                row++;
                continue;
            }

            size_t i, table_count = tables->size();
            for (i = 0; i < table_count; ++i)
            {
                xd::IFileInfoPtr info = tables->getItem(i);

                if (kl::iequals(it->output_tablename, info->getName()))
                {
                    m_table_selection_page->markProblemRow(row, true);
                    tablenames_already_in_database = true;
                    break;
                }
            }

            row++;
        }

        if (tablenames_already_in_database)
        {
            // show what rows contain errors
            m_table_selection_page->refreshGrid();
            
            int result = appMessageBox(_("One or more of the table names specified for export already exists.  Would you like to overwrite these tables?"),
                                       _("Export Wizard"),
                                       wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION | wxCENTER);
            if (result == wxNO)
                return;
        }
    }

    sigExportWizardFinished(this);
    m_frame->closeSite(m_doc_site);
}

bool ExportWizard::loadTemplate(const wxString& path)
{
    return true;
}

bool ExportWizard::saveTemplate(const wxString& path)
{
    // save the data on the page that is currently being edited
    m_path_selection_page->savePageData();
    m_datasource_selection_page->savePageData();
    m_server_properties_page->savePageData();

    // save the template
    m_template.save(path);

    // refresh tree
    g_app->getAppController()->refreshDbDoc();

    return true;
}

void ExportWizard::onClose(wxCommandEvent& evt)
{
    m_frame->closeSite(m_doc_site);
}

void ExportWizard::onSize(wxSizeEvent& event)
{
    Layout();
}

void ExportWizard::setDefaultExportPath(const wxString& _path)
{
    if (_path.IsEmpty())
        return;

    // get the directory
    wxString path = _path;
    if (xf_get_directory_exist(towstr(path)))
    {
        if (path.Last() != PATH_SEPARATOR_CHAR)
            path += PATH_SEPARATOR_CHAR;
    }
    else
    {
        path = path.BeforeLast(PATH_SEPARATOR_CHAR);
        path += PATH_SEPARATOR_CHAR;
    }

    // try to get the default export path path from the registry
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    prefs->setString(wxT("general.last_export_path"), path);
    prefs->flush();
}

wxString ExportWizard::getDefaultExportPath()
{
    wxString default_path = wxStandardPaths::Get().GetDocumentsDir();

    // try to get the default export path path from the registry
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs->exists(wxT("general.last_export_path")))
        return prefs->getString(wxT("general.last_export_path"), default_path);

    return default_path;
}

