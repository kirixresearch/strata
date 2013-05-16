/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-02-27
 *
 */


#include "appmain.h"
#include "connectionpages.h"
#include "connectionwizard.h"
#include "dlgdatabasefile.h"
#include <wx/stdpaths.h>
#include <wx/filepicker.h>


// control ids

// flag for disabling the "save password" check box in the connection page;
// turn off for now

// TODO: this is a temporary measure since the UI is sometimes showing the
// checkbox and sometimes isn't; since there's various code in various
// places (such as save/load) that relies on the "save password" checkbox, it's 
// safest to set a flag that turns on/off all the features at the same time
// that can be reviewed later
static bool enable_save_password = false;

enum
{
    // data source selection page
    ID_DataSourceGrid = wxID_HIGHEST + 1,

    // path selection page
    ID_PathTextCtrl,
    ID_FileTypeChoice,
    ID_FilterChoice,
    ID_VersionChoice,
    ID_CompressCheckBox,
    ID_BrowseButton,
    
    // server properties page
    ID_ServerTextCtrl,
    ID_DatabaseTextCtrl,
    ID_PortTextCtrl,
    ID_UsernameTextCtrl,
    ID_PasswordTextCtrl,
    ID_SavePasswordCheckBox,
    
    // save page
    ID_SaveTemplate
};


enum FilterIndex
{
    FilterIndex_AllDatabaseFiles = 0,
    FilterIndex_Dbf = 1,
    FilterIndex_Csv = 2,
    FilterIndex_Tsv = 3,
    FilterIndex_Txt = 4,
    FilterIndex_All = 5
};


// utility functions

wxString choice2FilterString(int idx)
{
    switch (idx)
    {
        case FilterIndex_AllDatabaseFiles:
            return wxT("dbf,csv,icsv,tsv,txt");
        case FilterIndex_Dbf:
            return wxT("dbf");
        case FilterIndex_Csv:
            return wxT("csv");
        case FilterIndex_Tsv:
            return wxT("tsv");
        case FilterIndex_Txt:
            return wxT("txt");
        case FilterIndex_All:
            return wxT("*");
    }

    return wxT("*");
}

int filter2ChoiceSelection(wxString filter)
{
    if (!filter.CmpNoCase(wxT("dbf,csv,icsv,tsv,txt")))
        return FilterIndex_AllDatabaseFiles;
    if (!filter.CmpNoCase(wxT("dbf")))
        return FilterIndex_Dbf;
    if (!filter.CmpNoCase(wxT("csv")))
        return FilterIndex_Csv;
    if (!filter.CmpNoCase(wxT("tsv")))
        return FilterIndex_Tsv;
    if (!filter.CmpNoCase(wxT("txt")))
        return FilterIndex_Txt;
    if (!filter.CmpNoCase(wxT("*")))
        return FilterIndex_All;

    return FilterIndex_All;
}




// -- PathSelectionPage class implementation --

BEGIN_EVENT_TABLE(PathSelectionPage, kcl::WizardPage)
    EVT_BUTTON(ID_BrowseButton, PathSelectionPage::onBrowse)
    EVT_TEXT_ENTER(ID_PathTextCtrl, PathSelectionPage::onPathTextEnter)
    EVT_TEXT(ID_PathTextCtrl, PathSelectionPage::onPathTextChanged)
    EVT_CHOICE(ID_FileTypeChoice, PathSelectionPage::onFileTypeSelectionChanged)
    EVT_CHOICE(ID_FilterChoice, PathSelectionPage::onFilterSelectionChanged)
    EVT_CHOICE(ID_VersionChoice, PathSelectionPage::onVersionSelectionChanged)
    EVT_CHECKBOX(ID_CompressCheckBox, PathSelectionPage::onCompressionChecked)
END_EVENT_TABLE()


PathSelectionPage::PathSelectionPage(kcl::Wizard* parent,
                                     ConnectionInfo* ci)
                  : kcl::WizardPage(parent), m_ci(ci)
{
    SetSize(parent->GetClientSize());

    // create message
    m_default_message = _("Please enter the location of the folder to which you would like to connect.");
    m_message = new wxStaticText(this, -1, m_default_message);
    resizeStaticText(m_message);

    // create filetype sizer
    m_default_filetype_label = _("File Type:");
    m_filetype_label = new wxStaticText(this, -1, m_default_filetype_label);
    m_filetype_choice = new wxChoice(this,
                                     ID_FileTypeChoice,
                                     wxDefaultPosition,
                                     wxSize(200,21),
                                     0,
                                     NULL);

    m_filetype_choice->Append(_("Text-Delimited Files"));
    m_filetype_choice->Append(_("Fixed-Length Files"));
    m_filetype_choice->Append(_("Microsoft FoxPro/Xbase Files"));
    m_filetype_choice->Append(_("Microsoft Access File"));
    m_filetype_choice->Append(_("Microsoft Excel File"));
    m_filetype_choice->Append(_("Package File"));
    m_filetype_choice->SetSelection(filetypeTextDelimited);
    
    m_filetype_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_filetype_sizer->Add(50,23);
    m_filetype_sizer->Add(m_filetype_label, 0, wxALIGN_CENTER);
    m_filetype_sizer->Add(m_filetype_choice, 1, wxALIGN_CENTER);
    m_filetype_sizer->Add(50,23);

    // create path sizer
    m_default_path_label = _("Folder:");
    m_path_label = new wxStaticText(this, -1, m_default_path_label);
    
    m_path_textctrl = NULL;
    m_path_textctrl = new wxTextCtrl(this,
                                     ID_PathTextCtrl,
                                     m_ci->path,
                                     wxDefaultPosition,
                                     wxSize(200,21),
                                     wxTE_PROCESS_ENTER);

    wxButton* browse_button = new wxButton(this,
                                           ID_BrowseButton,
                                           _("Browse..."));

    m_path_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_path_sizer->Add(50,23);
    m_path_sizer->Add(m_path_label, 0, wxALIGN_CENTER);
    m_path_sizer->Add(m_path_textctrl, 1, wxALIGN_CENTER);
    m_path_sizer->AddSpacer(5);
    m_path_sizer->Add(browse_button);
    m_path_sizer->Add(50,23);

    // create filter sizer
    m_filter_label = new wxStaticText(this, -1, _("Filter:"));
    m_filter_choice = new wxChoice(this,
                                   ID_FilterChoice,
                                   wxDefaultPosition,
                                   wxSize(200,21),
                                   0,
                                   NULL);

    m_filter_choice->Append(_("All Database Files (*.dbf; *.csv; *.tsv; *.txt)"));
    m_filter_choice->Append(_("Microsoft FoxPro/Xbase Files (*.dbf)"));
    m_filter_choice->Append(_("Comma-Delimited Files (*.csv)"));
    m_filter_choice->Append(_("Tab-Delimited Files (*.tsv)"));
    m_filter_choice->Append(_("Text Files (*.txt)"));
    m_filter_choice->Append(_("All Files (*.*)"));
    m_filter_choice->SetSelection(filter2ChoiceSelection(m_ci->filter));

    m_filter_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_filter_sizer->Add(50,23);
    m_filter_sizer->Add(m_filter_label, 0, wxALIGN_CENTER);
    m_filter_sizer->Add(m_filter_choice, 1, wxALIGN_CENTER);
    m_filter_sizer->Add(50,23);

    // create version sizer
    m_version_label = new wxStaticText(this, -1, _("Version:"));
    m_version_choice = new wxChoice(this,
                                    ID_VersionChoice,
                                    wxDefaultPosition,
                                    wxSize(200,21),
                                    0,
                                    NULL);

    m_version_choice->Append(_("Version 1.0"));
    m_version_choice->Append(_("Version 2.0 (Recommended)"));
    m_version_choice->SetSelection(m_ci->kpg_version - 1);

    m_version_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_version_sizer->Add(50,23);
    m_version_sizer->Add(m_version_label, 0, wxALIGN_CENTER);
    m_version_sizer->Add(m_version_choice, 1, wxALIGN_CENTER);
    m_version_sizer->Add(50,23);

    m_compress_checkbox = new wxCheckBox(this,
                                         ID_CompressCheckBox,
                                         _("Compress File"));
    m_compress_checkbox->SetValue(m_ci->kpg_compressed);
    
    // create compression sizer
    m_compression_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_compression_sizer->Add(50,23);
    m_compression_sizer->Add(1,1);      // placeholder sizer item (we remove and insert
                                        // this spacer item in showElements()
    m_compression_sizer->Add(m_compress_checkbox, 0, wxEXPAND);
    m_compression_sizer->Add(50,23);

    // create main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(20);
    m_main_sizer->Add(m_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_main_sizer->AddSpacer(4);
    m_main_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                      0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_main_sizer->AddSpacer(2);
    m_main_sizer->Add(m_filetype_sizer, 0, wxEXPAND | wxTOP, 10);
    m_main_sizer->Add(m_path_sizer, 0, wxEXPAND | wxTOP, 10);
    m_main_sizer->Add(m_filter_sizer, 0, wxEXPAND | wxTOP, 10);
    m_main_sizer->Add(m_version_sizer, 0, wxEXPAND | wxTOP, 10);
    m_main_sizer->Add(m_compression_sizer, 0, wxEXPAND | wxTOP, 10);
    SetSizer(m_main_sizer);

    // this functions calls Layout()
    showElements(showPath);
    
    m_browse_for_files = false;
    m_filedialog_select_multiple = false;
    m_filedialog_open = true;
}

void PathSelectionPage::onPageChanged()
{
    // populate path from the template
    m_path_textctrl->SetValue(m_ci->path);

    m_path_textctrl->SetFocus();
    m_path_textctrl->SetInsertionPointEnd();
    sigPageChanged();
}

bool PathSelectionPage::onPageChanging(bool forward)
{
    // the user can always go back a page
    if (!forward)
        return true;

    bool allow = true;
    sigPageChanging(forward, &allow);
    if (sigPageChanging.isActive())
    {
        if (allow)
        {
            // there's a possibility that the page data may have changed,
            // update the page's path text control to reflect any changes
            if (m_path_textctrl->GetValue() != m_ci->path)
                m_path_textctrl->SetValue(m_ci->path);
        }
        
        m_path_textctrl->SetFocus();
        m_path_textctrl->SetInsertionPointEnd();
        
        // signal receiver has already handled error-checking
        return allow;
    }

    if (m_ci->path.empty())
    {
        // the path was empty
        appMessageBox(_("A valid location needs to be specified to continue."),
                           _("Invalid Location"),
                           wxOK | wxICON_EXCLAMATION | wxCENTER,
                           g_app->getMainWindow());
        
        m_path_textctrl->SetFocus();
        m_path_textctrl->SetInsertionPointEnd();
        return false;
    }

    // see if the directory exists
    if (m_ci->type == dbtypeFilesystem)
    {
        xf_dirhandle_t dir_handle = xf_opendir(m_ci->path);
        if (dir_handle != NULL)
        {
            // we successfully opened the directory
            xf_closedir(dir_handle);
            return true;
        }
         else
        {
            // we could not open the directory
            appMessageBox(_("The specified folder does not exist or is invalid.  Please choose a valid folder to continue."),
                               _("Invalid Path"),
                               wxOK | wxICON_EXCLAMATION | wxCENTER,
                               g_app->getMainWindow());
            
            m_path_textctrl->SetFocus();
            m_path_textctrl->SetInsertionPointEnd();
            return false;
        }
    }
    
    return true;
}

void PathSelectionPage::onBrowse(wxCommandEvent& evt)
{
    if (!m_browse_for_files)
    {
        wxDirDialog dlg(getWizard()->GetParent(),
                        _("Choose Folder"));

        wxString start_dir = m_path_textctrl->GetValue();
        
        if (xf_get_directory_exist(towstr(start_dir)))
        {
            if (start_dir.Last() != PATH_SEPARATOR_CHAR)
                start_dir += PATH_SEPARATOR_CHAR;

            dlg.SetPath(start_dir);
        }

        if (dlg.ShowModal() == wxID_OK)
        {
            start_dir = dlg.GetPath();
            if (start_dir.Last() != PATH_SEPARATOR_CHAR)
                start_dir += PATH_SEPARATOR_CHAR;
                
            if (start_dir.Length() > 3 && (start_dir.Last() == '/' || start_dir.Last() == '\\'))
                start_dir.RemoveLast();
        
            m_path_textctrl->SetValue(start_dir);
            m_path_textctrl->SetInsertionPointEnd();
            m_path_textctrl->SetFocus();
        }
        
        return;
    }

    wxString caption;
    long flags = 0;
    
    // set file dialog flags
    if (m_filedialog_open)
        flags |= wxFD_OPEN;
         else
        flags |= wxFD_SAVE;

    if (m_filedialog_select_multiple)
    {
        flags |= wxFD_MULTIPLE;
        caption = _("Choose Files");
    }
     else
    {
        caption = _("Choose File");
    }
    
    // set file dialog filter
    if (m_filedialog_filter.IsEmpty())
    {
        wxString filter;
        filter  = _("All Files");
        filter += wxT(" (*.*)|*.*|");
        filter.RemoveLast(); // get rid of the last pipe sign

        m_filedialog_filter = filter;
    }

    wxFileDialog dlg(getWizard()->GetParent(),
                     caption,
                     wxEmptyString,
                     wxEmptyString,
                     m_filedialog_filter,
                     flags);
    wxString start_path = m_path_textctrl->GetValue();
    
    // if a path isn't specified, use the default directory
    // location; otherwise, set the directory based on the
    // path
    if (!start_path.IsEmpty())
    {
        if (!xf_get_directory_exist(towstr(start_path)))
        {
            start_path = start_path.BeforeLast(PATH_SEPARATOR_CHAR);
            start_path += PATH_SEPARATOR_CHAR;
        }

        dlg.SetPath(start_path);
    }

    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString paths;
        dlg.GetPaths(paths);
        size_t count = paths.Count();
        
        if (count == 1)
        {
            m_path_textctrl->SetValue(paths.Item(0));
        }
         else
        {
            // create space-delimited, quotation-mark qualified string
            wxString ext;
            wxString path_str;
            for (size_t i = 0; i < count; ++i)
            {
                ext = paths.Item(i).AfterLast(wxT('.'));
                
                path_str += wxT("\"");
                path_str += paths.Item(i);
                path_str += wxT("\"");
                
                if (i != count-1)
                    path_str += wxT(" ");
            }
            
            m_path_textctrl->SetValue(path_str);
        }
            
        m_path_textctrl->SetInsertionPointEnd();
        m_path_textctrl->SetFocus();
    }
}

void PathSelectionPage::onPathTextEnter(wxCommandEvent& evt)
{
    //onPageChanging(true);
    getWizard()->goNextPage(true);
}

void PathSelectionPage::onPathTextChanged(wxCommandEvent& evt)
{
    if (m_path_textctrl)
        m_ci->path = evt.GetString();
}

void PathSelectionPage::onFileTypeSelectionChanged(wxCommandEvent& evt)
{
    // switch the connection (or import/export) type in the respective info
    switch (m_filetype_choice->GetSelection())
    {
        case filetypePackage:
            m_ci->last_type = m_ci->type;
            m_ci->type = dbtypePackage;
            break;
        case filetypeAccess:
            m_ci->last_type = m_ci->type;
            m_ci->type = dbtypeAccess;
            break;
        case filetypeExcel:
            m_ci->last_type = m_ci->type;
            m_ci->type = dbtypeExcel;
            break;
        case filetypeXbase:
            m_ci->last_type = m_ci->type;
            m_ci->type = dbtypeXbase;
            break;
        case filetypeTextDelimited:
            m_ci->last_type = m_ci->type;
            m_ci->type = dbtypeDelimitedText;
            break;
        case filetypeFixedLength:
            m_ci->last_type = m_ci->type;
            m_ci->type = dbtypeFixedLengthText;
            break;
    }
    
    // populate the file dialog filter
    wxString filter;
    
    if (m_ci->type == dbtypePackage)
    {
        filter += _("Package Files");
        filter += wxT(" (*.kpg)|*.kpg|");
    }
     else if (m_ci->type == dbtypeAccess)
    {
        filter += _("Microsoft Access Files");
        filter += wxT(" (*.mdb, *.accdb)|*.mdb;*.accdb|");
    }
     else if (m_ci->type == dbtypeExcel)
    {
        filter += _("Microsoft Excel Files");
        filter += wxT(" (*.xls, *.xlsx)|*.xls;*.xlsx|");
    }
     else if (m_ci->type == dbtypeXbase)
    {
        filter += _("Microsoft FoxPro/Xbase Files");
        filter += wxT(" (*.dbf)|*.dbf|");
    }
     else if (m_ci->type == dbtypeDelimitedText)
    {
        filter += _("Comma-Delimited Files");
        filter += wxT(" (*.csv)|*.csv|");
        filter += _("Tab-Delimited Files");
        filter += wxT(" (*.tsv)|*.tsv|");
        filter += _("Text Files");
        filter += wxT(" (*.txt)|*.txt|");
        filter += _("All Files");
        filter += wxT(" (*.*)|*.*|");
    }
     else if (m_ci->type == dbtypeFixedLengthText)
    {
        filter += _("Text Files");
        filter += wxT(" (*.txt)|*.txt|");
        filter += _("All Files");
        filter += wxT(" (*.*)|*.*|");
    }
    filter.RemoveLast(); // get rid of the last pipe sign

    setFileDialogFilter(filter);
    sigFileTypeChanged(m_ci->type);
}

void PathSelectionPage::onFilterSelectionChanged(wxCommandEvent& evt)
{
    m_ci->filter = choice2FilterString(evt.GetSelection());
}

void PathSelectionPage::onVersionSelectionChanged(wxCommandEvent& evt)
{
    m_ci->kpg_version = evt.GetSelection() + 1;
}

void PathSelectionPage::onCompressionChecked(wxCommandEvent& evt)
{
    m_ci->kpg_compressed = evt.IsChecked();
}

int PathSelectionPage::getFileType()
{
    wxCommandEvent unused;
    onFileTypeSelectionChanged(unused);
    return m_ci->type;
}

void PathSelectionPage::setMessage(const wxString& message)
{
    if (message.IsEmpty())
        m_message->SetLabel(m_default_message);
     else
        m_message->SetLabel(message);
    
    resizeStaticText(m_message);
    Layout();
}

void PathSelectionPage::setPathLabel(const wxString& label)
{
    if (label.IsEmpty())
        m_path_label->SetLabel(m_default_path_label);
     else
        m_path_label->SetLabel(label);
    
    resizeLabelWidths();
}

void PathSelectionPage::setFileTypeLabel(const wxString& label)
{
    if (label.IsEmpty())
        m_filetype_label->SetLabel(m_default_filetype_label);
     else
        m_filetype_label->SetLabel(label);
    
    resizeLabelWidths();
}

void PathSelectionPage::setBrowse(bool browse_for_files,
                                  bool filedialog_open,
                                  bool filedialog_select_multiple)
{
    m_browse_for_files = browse_for_files;
    m_filedialog_open = filedialog_open;
    m_filedialog_select_multiple = filedialog_select_multiple;
}

void PathSelectionPage::setFileDialogFilter(const wxString& filter)
{
    m_filedialog_filter = filter;
}

void PathSelectionPage::setPath(const wxString& path)
{
    m_path_textctrl->SetValue(path);
}

wxString PathSelectionPage::getPath()
{
    return m_path_textctrl->GetValue();
}

void PathSelectionPage::resizeLabelWidths()
{
    wxSize label_size = getMaxTextSize(
                m_path_label->IsShown()     ? m_path_label     : NULL,
                m_filetype_label->IsShown() ? m_filetype_label : NULL,
                m_filter_label->IsShown()   ? m_filter_label   : NULL,
                m_version_label->IsShown()  ? m_version_label  : NULL);
    label_size.x += 10;

    m_path_sizer->SetItemMinSize(m_path_label, label_size);
    m_filetype_sizer->SetItemMinSize(m_filetype_label, label_size);
    m_filter_sizer->SetItemMinSize(m_filter_label, label_size);
    m_version_sizer->SetItemMinSize(m_version_label, label_size);
    
    // make sure the compression dropdown lines up with the text control
    m_compression_sizer->Remove(1);
    m_compression_sizer->Insert(1,label_size.GetWidth(), 1);

    Layout();
}

void PathSelectionPage::showElements(unsigned int options)
{
    Freeze();

    m_show_options = (ShowOptions)options;

    m_main_sizer->Show(m_path_sizer, false, true);
    m_main_sizer->Show(m_filetype_sizer, false, true);
    m_main_sizer->Show(m_filter_sizer, false, true);
    m_main_sizer->Show(m_version_sizer, false, true);
    m_main_sizer->Show(m_compression_sizer, false, true);
    
    if (options & PathSelectionPage::showPath)
        m_main_sizer->Show(m_path_sizer, true, true);
    if (options & PathSelectionPage::showFileType)
        m_main_sizer->Show(m_filetype_sizer, true, true);
    if (options & PathSelectionPage::showFilter)
        m_main_sizer->Show(m_filter_sizer, true, true);
    if (options & PathSelectionPage::showVersion)
        m_main_sizer->Show(m_version_sizer, true, true);
    if (options & PathSelectionPage::showCompression)
        m_main_sizer->Show(m_compression_sizer, true, true);
    
    // this function calls Layout() for us
    resizeLabelWidths();

    // handle focus
    m_path_textctrl->SetFocus();
    m_path_textctrl->SetInsertionPointEnd();

    Thaw();
}

void PathSelectionPage::loadPageData()
{
    m_path_textctrl->SetValue(m_ci->path);
    m_filter_choice->SetSelection(filter2ChoiceSelection(m_ci->filter));
    m_version_choice->SetSelection(m_ci->kpg_version - 1);
    m_compress_checkbox->SetValue(m_ci->kpg_compressed);
}

void PathSelectionPage::savePageData()
{
    m_ci->path = m_path_textctrl->GetValue();
}




// -- DataSourceSelectionPage class implementation --

BEGIN_EVENT_TABLE(DataSourceSelectionPage, kcl::WizardPage)
    EVT_KCLGRID_CELL_LEFT_DCLICK(DataSourceSelectionPage::onGridCellLeftDClick)
END_EVENT_TABLE()


DataSourceSelectionPage::DataSourceSelectionPage(kcl::Wizard* parent,
                                                 ConnectionInfo* ci)
                        : kcl::WizardPage(parent), m_ci(ci)
{
    SetSize(parent->GetClientSize());

    // create message
    m_default_message = _("Please select the ODBC data source to which you would like to connect.");
    m_message = new wxStaticText(this, -1, m_default_message);
    resizeStaticText(m_message);

    // create grid
    m_grid = new kcl::Grid;
    m_grid->setCursorType(kcl::Grid::cursorRowHighlight);
    m_grid->setBorderType(kcl::DEFAULT_BORDER);
    m_grid->setOptionState(kcl::Grid::optHorzGridLines |
                           kcl::Grid::optVertGridLines |
                           kcl::Grid::optColumnMove |
                           kcl::Grid::optColumnResize |
                           kcl::Grid::optSelect |
                           kcl::Grid::optActivateHyperlinks, false);
    m_grid->createModelColumn(-1, _("Data Source"), kcl::Grid::typeCharacter, 256, 0);
    m_grid->createModelColumn(-1, _("Description"), kcl::Grid::typeCharacter, 1024, 0);
    m_grid->setRowLabelSize(0);
    m_grid->createDefaultView();
    m_grid->setColumnProportionalSize(0, 1);
    m_grid->setColumnProportionalSize(1, 2);
    m_grid->Create(this, ID_DataSourceGrid);

    kcl::CellProperties cp;
    cp.mask = kcl::CellProperties::cpmaskEditable;
    cp.editable = false;
    m_grid->setModelColumnProperties(0, &cp);
    
    populate();
    
    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(20);
    main_sizer->Add(m_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(4);
    main_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                    0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(12);
    main_sizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    main_sizer->AddSpacer(20);

    SetSizer(main_sizer);
    Layout();
}

void DataSourceSelectionPage::onPageChanged()
{
    if (m_grid->getRowCount() == 0)
        populate();

    m_grid->SetFocus();
    sigPageChanged();
}

bool DataSourceSelectionPage::onPageChanging(bool forward)
{
    // allow user to go back a page
    if (!forward)
        return true;

    // determine whether there are any ODBC data sources installed
    // on this system; notify the user if there are not
    int row_count = m_grid->getRowCount();
    if (row_count == 0)
    {
        m_grid->SetFocus();
        return false;
    }

    if (forward)
    {
        // store server info
        int row = m_grid->getCursorRow();
        m_ci->server = m_grid->getCellString(row, 0);

        // clear out the old connection info
        m_ci->port = 0;
        m_ci->database = wxT("");
        m_ci->username = wxT("");
        m_ci->password = wxT("");
        m_ci->save_password = false;

        m_ci->path = wxT("");
        m_ci->filter = wxT("");
        m_ci->kpg_compressed = true;
        m_ci->kpg_version = 2;

        m_ci->m_conn_ptr.clear();


        IConnectionPtr conn = createUnmanagedConnection();
        conn->setType(m_ci->type);
        conn->setDescription(m_ci->description);
        conn->setHost(m_ci->server);
        conn->setPort(m_ci->port);
        conn->setDatabase(m_ci->database);
        conn->setUsername(m_ci->username);
        conn->setPassword(m_ci->password);

        {
            AppBusyCursor bc;

            if (conn->open())
            {
                m_ci->m_conn_ptr = conn;
            }
        }
    }

    bool allow = true;
    sigPageChanging(forward, &allow);
    if (sigPageChanging.isActive())
    {
        if (!allow)
            m_grid->SetFocus();

        // signal receiver has already handled error-checking
        return allow;
    }

    return true;
}

void DataSourceSelectionPage::onGridCellLeftDClick(kcl::GridEvent& evt)
{
    onPageChanging(true);
}

void DataSourceSelectionPage::populate()
{
    xcm::ptr<tango::IDatabaseMgr> dbmgr;
    dbmgr.create_instance("xdodbc.DatabaseMgr");

    if (dbmgr.isNull())
    {
        wxString appname = APPLICATION_NAME;
        wxString message = wxString::Format(_("%s is missing a required component (xdodbc) and must be reinstalled"),
                                            appname.c_str());
        m_grid->setOverlayText(message);
        m_grid->SetFocus();
        return;
    }

    tango::IDatabaseEntryEnumPtr odbc_databases;
    odbc_databases = dbmgr->getDatabaseList(L"", 0, L"", L"");

    m_grid->deleteAllRows();

    int i, count = odbc_databases->size();

    for (i = 0; i < count; ++i)
    {
        m_grid->insertRow(-1);

        tango::IDatabaseEntryPtr item = odbc_databases->getItem(i);
        m_grid->setCellString(i, 0, item->getName());
        m_grid->setCellString(i, 1, item->getDescription());
    }

    // if we already have a server name, move the cursor
    // to the corresponding grid row

    int row_count = m_grid->getRowCount();
    std::wstring cell_str;

    for (i = 0; i < row_count; ++i)
    {
        cell_str = towstr(m_grid->getCellString(i, 0));

        if (kl::iequals(m_ci->server, cell_str))
        {
            m_grid->moveCursor(i, 0);
            break;
        }
    }

    if (row_count == 0)
        m_grid->setOverlayText(_("Please contact your database administrator\nto set up an ODBC database connection"));
         else
        m_grid->setOverlayText(wxEmptyString);
    
    m_grid->refresh(kcl::Grid::refreshAll);
}

void DataSourceSelectionPage::setMessage(const wxString& message)
{
    if (message.IsEmpty())
        m_message->SetLabel(m_default_message);
     else
        m_message->SetLabel(message);
    
    resizeStaticText(m_message);
    Layout();
}

void DataSourceSelectionPage::savePageData()
{

}




// -- ServerPropertiesPage class implementation --

BEGIN_EVENT_TABLE(ServerPropertiesPage, kcl::WizardPage)
    EVT_TEXT(ID_ServerTextCtrl, ServerPropertiesPage::onServerTextChanged)
    EVT_TEXT(ID_DatabaseTextCtrl, ServerPropertiesPage::onDatabaseTextChanged)
    EVT_TEXT(ID_PortTextCtrl, ServerPropertiesPage::onPortTextChanged)
    EVT_TEXT(ID_UsernameTextCtrl, ServerPropertiesPage::onUsernameTextChanged)
    EVT_TEXT(ID_PasswordTextCtrl, ServerPropertiesPage::onPasswordTextChanged)
    EVT_CHECKBOX(ID_SavePasswordCheckBox, ServerPropertiesPage::onSavePasswordChecked)
END_EVENT_TABLE()


ServerPropertiesPage::ServerPropertiesPage(kcl::Wizard* parent,
                                           ConnectionInfo* ci)
                     : kcl::WizardPage(parent), m_ci(ci)
{
    SetSize(parent->GetClientSize());

    // create message
    m_default_message = _("Please enter the connection settings for the database to which you would like to connect.");
    m_message = new wxStaticText(this, -1, m_default_message);
    resizeStaticText(m_message);

    // create the server sizer
    wxStaticText* label_server = new wxStaticText(this,
                                                  -1,
                                                  _("Server:"),
                                                  wxDefaultPosition,
                                                  wxDefaultSize);
    m_server_textctrl = NULL;
    m_server_textctrl = new wxTextCtrl(this, 
                                       ID_ServerTextCtrl, 
                                       m_ci->server,
                                       wxDefaultPosition,
                                       wxSize(200,21));
    
    m_server_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_server_sizer->Add(50,23);
    m_server_sizer->Add(label_server, 0, wxALIGN_CENTER);
    m_server_sizer->Add(m_server_textctrl, 1, wxALIGN_CENTER);
    m_server_sizer->Add(50,23);
    
    // create the database sizer
    wxStaticText* label_database = new wxStaticText(this,
                                                    -1,
                                                    _("Database:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_database_textctrl = NULL;
    m_database_textctrl = new wxTextCtrl(this, 
                                         ID_DatabaseTextCtrl,
                                         m_ci->database,
                                         wxDefaultPosition,
                                         wxSize(200,21));
    
    m_database_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_database_sizer->Add(50,23);
    m_database_sizer->Add(label_database, 0, wxALIGN_CENTER);
    m_database_sizer->Add(m_database_textctrl, 1, wxALIGN_CENTER);
    m_database_sizer->Add(50,23);

    // create the port number sizer
    wxStaticText* label_port = new wxStaticText(this,
                                                -1,
                                                _("Port Number:"),
                                                wxDefaultPosition,
                                                wxDefaultSize);
    m_port_textctrl = NULL;
    m_port_textctrl = new wxTextCtrl(this, 
                                     ID_PortTextCtrl, 
                                     wxString::Format(wxT("%d"), m_ci->port),
                                     wxDefaultPosition,
                                     wxSize(200,21));
    
    m_port_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_port_sizer->Add(50,23);
    m_port_sizer->Add(label_port, 0, wxALIGN_CENTER);
    m_port_sizer->Add(m_port_textctrl, 1, wxALIGN_CENTER);
    m_port_sizer->Add(50,23);

    // create the username sizer
    wxStaticText* label_username = new wxStaticText(this,
                                                    -1,
                                                    _("User Name:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_username_textctrl = NULL;
    m_username_textctrl = new wxTextCtrl(this, 
                                         ID_UsernameTextCtrl,
                                         m_ci->username,
                                         wxDefaultPosition,
                                         wxSize(200,21));
    
    m_username_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_username_sizer->Add(50,23);
    m_username_sizer->Add(label_username, 0, wxALIGN_CENTER);
    m_username_sizer->Add(m_username_textctrl, 1, wxALIGN_CENTER);
    m_username_sizer->Add(50,23);

    // create the password sizer
    wxStaticText* label_password = new wxStaticText(this,
                                                    -1,
                                                    _("Password:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_password_textctrl = NULL;
    m_password_textctrl = new wxTextCtrl(this,
                                         ID_PasswordTextCtrl,
                                         m_ci->password,
                                         wxDefaultPosition,
                                         wxSize(200,21),
                                         wxTE_PASSWORD);
    
    m_password_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_password_sizer->Add(50,23);
    m_password_sizer->Add(label_password, 0, wxALIGN_CENTER);
    m_password_sizer->Add(m_password_textctrl, 1, wxALIGN_CENTER);
    m_password_sizer->Add(50,23);

    // measure the label widths
    wxSize label_size = getMaxTextSize(label_server,
                                            label_database,
                                            label_username,
                                            label_password,
                                            label_port);
    label_size.x += 10;
    
    // create the save password sizer
    
    if (enable_save_password)
    {
        m_savepassword_checkbox = new wxCheckBox(this, 
                                                 ID_SavePasswordCheckBox,
                                                 _("Save Password"));

        m_savepassword_sizer = new wxBoxSizer(wxHORIZONTAL);
        m_savepassword_sizer->Add(50,15);
        m_savepassword_sizer->Add(label_size.x, 0);
        m_savepassword_sizer->Add(m_savepassword_checkbox, 0, wxALIGN_CENTER);
        m_savepassword_sizer->Add(50,15);
    }        

    m_server_sizer->SetItemMinSize(label_server, label_size);
    m_database_sizer->SetItemMinSize(label_database, label_size);
    m_username_sizer->SetItemMinSize(label_username, label_size);
    m_password_sizer->SetItemMinSize(label_password, label_size);
    m_port_sizer->SetItemMinSize(label_port, label_size);

    // create main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(20);
    m_main_sizer->Add(m_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_main_sizer->AddSpacer(4);
    m_main_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                      0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_main_sizer->AddSpacer(2);
    m_main_sizer->Add(m_server_sizer, 0, wxEXPAND | wxTOP, 10);
    m_main_sizer->Add(m_database_sizer, 0, wxEXPAND | wxTOP, 10);
    m_main_sizer->Add(m_port_sizer, 0, wxEXPAND | wxTOP, 10);
    m_main_sizer->Add(m_username_sizer, 0, wxEXPAND | wxTOP, 10);
    m_main_sizer->Add(m_password_sizer, 0, wxEXPAND | wxTOP, 10);
    
    if (enable_save_password)
        m_main_sizer->Add(m_savepassword_sizer, 0, wxEXPAND | wxTOP, 10);
    
    SetSizer(m_main_sizer);

    // start with the focus on the server text control
    m_server_textctrl->SetFocus();
    
    // this functions calls Layout()
    showElements(showAll & ~showSavePassword);
}

void ServerPropertiesPage::showElements(unsigned int options)
{
    if (options == m_show_options)
    {
        handleFocus();
        return;
    }
    
    Freeze();
    
    m_show_options = (ShowOptions)options;

    m_main_sizer->Show(m_server_sizer, false, true);
    m_main_sizer->Show(m_database_sizer, false, true);
    m_main_sizer->Show(m_username_sizer, false, true);
    m_main_sizer->Show(m_password_sizer, false, true);
    m_main_sizer->Show(m_port_sizer, false, true);
    
    if (enable_save_password)
        m_main_sizer->Show(m_savepassword_sizer, false, true);
    
    if (options & ServerPropertiesPage::showServer)
        m_main_sizer->Show(m_server_sizer, true, true);
    if (options & ServerPropertiesPage::showDatabase)
        m_main_sizer->Show(m_database_sizer, true, true);
    if (options & ServerPropertiesPage::showUsername)
        m_main_sizer->Show(m_username_sizer, true, true);
    if (options & ServerPropertiesPage::showPassword)
        m_main_sizer->Show(m_password_sizer, true, true);
    if (options & ServerPropertiesPage::showPort)
        m_main_sizer->Show(m_port_sizer, true, true);
    if (options & ServerPropertiesPage::showSavePassword && enable_save_password)
        m_main_sizer->Show(m_savepassword_sizer, true, true);
    
    handleFocus();
    Layout();
    
    Thaw();
}

void ServerPropertiesPage::handleFocus()
{
    wxWindow* focus_wnd = FindFocus();
    
    if (m_server_textctrl->IsShown())
    {
        if (focus_wnd != m_server_textctrl)
        {
            m_server_textctrl->SetFocus();
            m_server_textctrl->SetInsertionPointEnd();
        }
    }
     else
    {
        if (m_username_textctrl->IsShown())
        {
            if (focus_wnd != m_username_textctrl)
            {
                m_username_textctrl->SetFocus();
                m_username_textctrl->SetInsertionPointEnd();
            }
        }
    }
}

void ServerPropertiesPage::onPageChanged()
{
    handleFocus();
    sigPageChanged();
}

bool ServerPropertiesPage::onPageChanging(bool forward)
{
    if (forward)
    {
        // clear out the old connection info
        m_ci->path = wxT("");
        m_ci->filter = wxT("");
        m_ci->kpg_compressed = true;
        m_ci->kpg_version = 2;

        m_ci->m_conn_ptr.clear();

        IConnectionPtr conn = createUnmanagedConnection();
        conn->setType(m_ci->type);
        conn->setDescription(m_ci->description);
        conn->setHost(m_ci->server);
        conn->setPort(m_ci->port);
        conn->setDatabase(m_ci->database);
        conn->setUsername(m_ci->username);
        conn->setPassword(m_ci->password);

        {
            AppBusyCursor bc;

            if (!conn->open())
            {
                // if we cannot open the connection, bail out
                wxString error_message = _("There was an error connecting to the specified database.");
                

                
                wxString server_error_message = conn->getErrorString();
                if (!server_error_message.IsEmpty())
                {
                    error_message += wxT("  ");
                    error_message += server_error_message;
                }
                
                
                if (server_error_message.Contains(wxT("xdoracle")))
                {
                    error_message += wxT("\n");
                    error_message += _("Your system may be lacking the necessary Oracle client software.");
                }
                
                          
                appMessageBox(error_message,
                                   _("Error"),
                                   wxOK | wxICON_EXCLAMATION | wxCENTER,
                                   g_app->getMainWindow());

                // set the focus back to one of the text controls
                handleFocus();
                
                return false;
            }
        }
    }

    bool allow = true;
    sigPageChanging(forward, &allow);
    if (sigPageChanging.isActive())
    {
        if (!allow)
            handleFocus();

        // signal receiver has already handled error-checking
        return allow;
    }

    return true;
}

void ServerPropertiesPage::setMessage(const wxString& message)
{
    if (message.IsEmpty())
        m_message->SetLabel(m_default_message);
     else
        m_message->SetLabel(message);
    
    resizeStaticText(m_message);
    Layout();
}

void ServerPropertiesPage::loadPageData()
{
    m_server_textctrl->SetValue(m_ci->server);
    m_database_textctrl->SetValue(m_ci->database);
    m_port_textctrl->SetValue(wxString::Format(wxT("%d"), m_ci->port));
    m_username_textctrl->SetValue(m_ci->username);
    m_password_textctrl->SetValue(m_ci->password);
    
    if (enable_save_password)
        m_savepassword_checkbox->SetValue(m_ci->save_password);
}

void ServerPropertiesPage::savePageData()
{

}

void ServerPropertiesPage::onServerTextChanged(wxCommandEvent& evt)
{
    if (m_server_textctrl)
        m_ci->server = evt.GetString();
}

void ServerPropertiesPage::onDatabaseTextChanged(wxCommandEvent& evt)
{
    if (m_database_textctrl)
        m_ci->database = evt.GetString();
}

void ServerPropertiesPage::onPortTextChanged(wxCommandEvent& evt)
{
    if (m_port_textctrl)
        m_ci->port = wxAtoi(evt.GetString());
}

void ServerPropertiesPage::onUsernameTextChanged(wxCommandEvent& evt)
{
    if (m_username_textctrl)
        m_ci->username = evt.GetString();
}

void ServerPropertiesPage::onPasswordTextChanged(wxCommandEvent& evt)
{
    if (m_password_textctrl)
        m_ci->password = evt.GetString();
}

void ServerPropertiesPage::onSavePasswordChecked(wxCommandEvent& evt)
{
    m_ci->save_password = evt.IsChecked();
}




// -- SavePage class implementation --

BEGIN_EVENT_TABLE(SavePage, kcl::WizardPage)
    EVT_BUTTON(ID_SaveTemplate, SavePage::onSave)
END_EVENT_TABLE()


SavePage::SavePage(kcl::Wizard* parent,
                   ConnectionInfo* ci)
                     : kcl::WizardPage(parent), m_ci(ci)
{
    SetSize(parent->GetClientSize());

    // create message (to be filled out later -- this page is just a template)
    m_message = new wxStaticText(this, -1,
                                 _("<Enter page message here>"));
    resizeStaticText(m_message);

    // create the save button sizer
    m_save_button = new wxButton(this,
                                 ID_SaveTemplate,
                                 _("Save"),
                                 wxDefaultPosition,
                                 wxDefaultSize);
    setSaveButtonLabel();
    
    m_savebutton_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_savebutton_sizer->AddStretchSpacer(1);
    m_savebutton_sizer->Add(m_save_button, 0, wxALIGN_CENTER);
    m_savebutton_sizer->AddStretchSpacer(1);

    // create main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(20);
    m_main_sizer->Add(m_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_main_sizer->AddSpacer(20);
    m_main_sizer->Add(m_savebutton_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    
    SetSizer(m_main_sizer);
}

void SavePage::setMessage(const wxString& message)
{
    m_message->SetLabel(message);
    resizeStaticText(m_message);
    Layout();
}

void SavePage::setSaveButtonLabel(const wxString& label)
{
    wxSize s;

    if (label.IsEmpty())
    {
        m_save_button->SetLabel(_("Save"));
        s = m_save_button->GetDefaultSize();
    }
     else
    {
        m_save_button->SetLabel(label);
        
        int w, h;
        wxClientDC dc(this);
        dc.GetTextExtent(label, &w, &h);
        s.x = w+14;
        s.y = h+14;
    }
    
    m_save_button->SetMinSize(s);
}

void SavePage::onPageChanged()
{

}

bool SavePage::onPageChanging(bool forward)
{
    return true;
}

void SavePage::onSave(wxCommandEvent& evt)
{
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSave);
    dlg.setOverwritePrompt(true);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString ofs_path = dlg.getPath();
        sigOnSave(ofs_path);
    }
}




