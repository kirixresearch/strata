/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-11-27
 *
 */


#include "appmain.h"
#include "dlgdatabasefile.h"
#include "dbdoc.h"
#include "dbcombo.h"


enum
{
    ID_Name_TextCtrl = 10000
};


BEGIN_EVENT_TABLE(DlgDatabaseFile, wxDialog)
    EVT_BUTTON(wxID_OK, DlgDatabaseFile::onOK)
    EVT_BUTTON(wxID_CANCEL, DlgDatabaseFile::onCancel)
    EVT_TEXT(ID_Name_TextCtrl, DlgDatabaseFile::onPathChanged)
    EVT_TEXT_ENTER(ID_Name_TextCtrl, DlgDatabaseFile::onEnterPressed)
END_EVENT_TABLE()



DlgDatabaseFile::DlgDatabaseFile(wxWindow* parent, int mode) :
        wxDialog(parent, 
                 -1,
                 wxEmptyString,
                 wxDefaultPosition,
                 wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE |
                 wxRESIZE_BORDER |
                 wxCLIP_CHILDREN |
                 wxNO_FULL_REPAINT_ON_RESIZE)
{
    m_dbdoc = NULL;
    m_dbcombo = NULL;
    m_name_ctrl = NULL;
    m_ok_button = NULL;
    
    m_mode = mode;
    m_overwrite_prompt = true;
    m_root_folder = wxT("/");
    m_folder = wxT("/");
    m_filename = wxEmptyString;
    
    // for now, make modeSave and modeSaveSmall one in the same thing
    if (m_mode == modeSave)
        m_mode = modeSaveSmall;
    
    wxSize small_size = FromDIP(wxSize(360, 145));
    wxSize default_size = FromDIP(wxSize(400, 360));

    SetSize(m_mode == modeSaveSmall ? small_size : default_size);
    SetMinSize(m_mode == modeSaveSmall ? small_size : default_size);
    
    wxString caption = wxEmptyString;
    wxString ok_button_text = wxEmptyString;
    
    switch (m_mode)
    {
        default:
        case modeSelectFolder:
            caption = _("Select Folder");
            ok_button_text = _("OK");
            break;

        case modeOpenTable:
        case modeOpen:
            caption = _("Open");
            ok_button_text = _("Open");
            break;

        case modeSave:
        case modeSaveSmall:
            caption = _("Save As");
            ok_button_text = _("Save");
            break;
    }
    
    if (m_caption.IsEmpty())
        m_caption = caption;

    SetTitle(m_caption);

    
    // populating the dbdoc and then the dbcombo is an expensive operation,
    // so only create the one we need here -- this will mean we'll need to
    // be a little careful in the code to make sure the controls exist
    
    wxBoxSizer* create_sizer = NULL;
    wxBoxSizer* tree_sizer = NULL;
    
    wxStaticText* label_create = NULL;
    
    if (m_mode == modeSaveSmall)
    {
        // create the db combo sizer
        
        label_create = new wxStaticText(this, -1, _("Save in:"));
        m_dbcombo = new DbComboCtrl(this);
        m_dbcombo->setShowFullPath(true, false);
        m_dbcombo->sigItemSelected.connect(this, &DlgDatabaseFile::onComboItemSelected);
        
        create_sizer = new wxBoxSizer(wxHORIZONTAL);
        create_sizer->Add(label_create, 0, wxALIGN_CENTER);
        create_sizer->AddSpacer(FromDIP(5));
        create_sizer->Add(m_dbcombo, 1, wxALIGN_CENTER);
    }
     else
    {
        // create database tree control sizer
        
        wxBusyCursor bc;
        
        m_dbdoc = new DbDoc;
        m_dbdoc->initAsWindow(this,
                              -1,
                              wxPoint(0,0),
                              wxDefaultSize,
                              0);
        m_dbdoc->setDatabase(g_app->getDatabase(), wxT("/"));
        
        m_dbdoc->getFsPanel()->setMultiSelect(false);
        m_dbdoc->getFsPanel()->setView(fsviewTree);
        m_dbdoc->getFsPanel()->sigItemActivated().disconnect();
        m_dbdoc->getFsPanel()->sigItemActivated().connect(this, &DlgDatabaseFile::onItemActivated);
        m_dbdoc->getFsPanel()->sigItemBeginLabelEdit().disconnect();
        m_dbdoc->getFsPanel()->sigItemBeginLabelEdit().connect(this, &DlgDatabaseFile::onItemBeginLabelEdit);
        m_dbdoc->getFsPanel()->sigItemSelected().disconnect();
        m_dbdoc->getFsPanel()->sigItemSelected().connect(this, &DlgDatabaseFile::onItemSelected);
        
        m_dbdoc->refresh();
        
        // make sure the dbdoc has a border
        wxWindow* wnd = m_dbdoc->getDocWindow();
        long flags = wnd->GetWindowStyle();
        wnd->SetWindowStyle(flags | kcl::DEFAULT_BORDER);
        
        tree_sizer = new wxBoxSizer(wxHORIZONTAL);
        tree_sizer->Add(wnd, 1, wxEXPAND);
        
        // turn off drag&drop for the dialog panel
        m_dbdoc->getFsPanel()->setDragDrop(false);
    }
    
    
    setRootFolder(wxT("/"));
    
    
    // create name sizer
    
    wxStaticText* label_name = new wxStaticText(this, -1, _("Name:"));
    
    // we have to do this because of the way wx handles events...
    // we don't want the text changed event to work yet
    m_name_ctrl = NULL;
    m_name_ctrl = new wxTextCtrl(this,
                                 ID_Name_TextCtrl,
                                 m_filename,
                                 wxDefaultPosition,
                                 wxDefaultSize,
                                 wxTE_PROCESS_ENTER);

    wxBoxSizer* name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(label_name, 0, wxALIGN_CENTER);
    name_sizer->AddSpacer(5);
    name_sizer->Add(m_name_ctrl, 1, wxALIGN_CENTER);

    
    // create a platform standards-compliant OK/Cancel sizer
    
    m_ok_button = new wxButton(this, wxID_OK, ok_button_text);
    m_ok_button->SetDefault();
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(m_ok_button);
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight() + FromDIP(16));
    ok_cancel_sizer->SetMinSize(min_size);
    
    
    // create spacer sizer
    wxBoxSizer* spacer_sizer = new wxBoxSizer(wxHORIZONTAL);
    spacer_sizer->AddStretchSpacer();
    
    
    // create main sizer
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    (tree_sizer != NULL) ? (main_sizer->Add(tree_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8))) : 0;
    (create_sizer != NULL) ? (main_sizer->Add(create_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8))) : 0;
    main_sizer->Add(name_sizer, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, FromDIP(8));
    (create_sizer != NULL) ? (main_sizer->Add(spacer_sizer, 1, wxEXPAND)) : 0;
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    
    
    // handle sizer layout for our different modes
    if (m_mode == modeSaveSmall)
    {
        // measure the label widths
        if (label_create)
        {
            wxSize label_size = getMaxTextSize(label_create, label_name);
            label_size.x += FromDIP(5);
            
            create_sizer->SetItemMinSize(label_create, label_size);
            name_sizer->SetItemMinSize(label_name, label_size);
        }
    } 
     else if (m_mode == modeSelectFolder || m_mode == modeOpen || m_mode == modeOpenTable)
    {
        main_sizer->Show(name_sizer, false, true);
    }
    
    
    SetSizer(main_sizer);
    Layout();
    Centre();
    
    setFocusDeferred(m_name_ctrl);
}

void DlgDatabaseFile::setRootFolder(const wxString& path, const wxString& root_label)
{
    m_root_folder = path;
    
    if (m_mode == modeSaveSmall)
    {
        m_dbcombo->setPopupRootFolder(path, root_label);
        return;
    }
     else
    {
        DbFolderFsItem* root = new DbFolderFsItem;
        root->setPath(m_root_folder);
        root->setDatabase(g_app->getDatabase());
        
        if (m_mode == modeSelectFolder)
            root->setFoldersOnly(true);
            
        if (m_mode == modeOpenTable)
            root->setTablesOnly(true);
        
        wxString label = root_label;
        if (label.IsEmpty())
        {
            wxString project_name = g_app->getProjectName();
            if (project_name.Length() > 0)
                label = wxString::Format(_("Project '%s'"), project_name.c_str());
                 else
                label = _("Project");
        }
        
        root->setLabel(label);
        root->setBitmap(GETBMP(gf_project_16));
        m_dbdoc->getFsPanel()->setRootItem(root);
        m_dbdoc->getFsPanel()->setMultiSelect(false);
        m_dbdoc->getFsPanel()->setView(fsviewTree);
        m_dbdoc->getFsPanel()->refresh();
    }
}

void DlgDatabaseFile::setAffirmativeButtonLabel(const wxString& label)
{
    if (label.IsEmpty())
    {
        if (m_mode == modeOpen || m_mode == modeOpenTable)
            m_affirmative_button_label = _("Open");
             else if (m_mode == modeSave || m_mode == modeSaveSmall)
            m_affirmative_button_label = _("Save");
             else
            m_affirmative_button_label = _("OK");
    }
     else
    {
        m_affirmative_button_label = label;
    }
    
    m_ok_button->SetLabel(m_affirmative_button_label);
}

void DlgDatabaseFile::setCaption(const wxString& new_value)
{
    m_caption = new_value;
    SetTitle(m_caption);
}

void DlgDatabaseFile::setPath(const wxString& new_value, bool select_text)
{
    wxString path = new_value;
    
    m_folder = path.BeforeLast(wxT('/'));
    m_filename = path.AfterLast(wxT('/'));
    
    if (m_folder.IsEmpty())
        m_folder = m_root_folder;
    
    if (m_dbcombo != NULL)
    {
        m_dbcombo->setPath(m_folder);
    }
     else
    {
        if (m_dbdoc)
        {
            IFsItemPtr item;
            
            if (m_name_ctrl->IsShown())
                item = m_dbdoc->getFsItemFromPath(m_folder);
                 else
                item = m_dbdoc->getFsItemFromPath(path);
            
            if (item)
                m_dbdoc->getFsPanel()->selectItem(item);
        }
    }
    
    if (m_name_ctrl != NULL)
    {
        m_name_ctrl->ChangeValue(m_filename);
        
        if (select_text)
            m_name_ctrl->SetSelection(-1, -1);
             else
            m_name_ctrl->SetInsertionPointEnd();
    }
}

void DlgDatabaseFile::setOverwritePrompt(bool new_value)
{
    m_overwrite_prompt = new_value;
}

void DlgDatabaseFile::onPathChanged(wxCommandEvent& evt)
{
    if (!m_name_ctrl)
        return;
        
    m_filename = m_name_ctrl->GetValue();

    if (m_filename.Length() == 0)
        m_ok_button->Enable(false);
         else
        m_ok_button->Enable(true);
}

void DlgDatabaseFile::onEnterPressed(wxCommandEvent& evt)
{
    wxCommandEvent empty;
    onOK(empty);
}

void DlgDatabaseFile::onOK(wxCommandEvent& evt)
{
    xd::IDatabasePtr db = g_app->getDatabase();

    wxString path = getPath();

    // check to see if the path entered is valid
    if (m_mode != modeSelectFolder)
    {
        // make sure a file is actually specified
        if (!isValidObjectName(path.AfterLast(wxT('/'))))
        {
            appInvalidObjectMessageBox(path);
            return;
        }
    
        if (!isValidObjectPath(path))
        {
            appInvalidObjectMessageBox(path);
            return;
        }
    }


    if (m_mode == modeSave || m_mode == modeSaveSmall)
    {
        // make sure user is saving the file to a valid folder

        wxStringTokenizer tkz(path, wxT("/"));
        wxString stub = wxT("/");
        while (tkz.HasMoreTokens())
        {
            wxString token = tkz.GetNextToken();

            if (token.IsEmpty())
                continue;

            if (!tkz.HasMoreTokens())
                break;

            stub += token;

            xd::IFileInfoPtr file_info = g_app->getDatabase()->getFileInfo(towstr(stub));
            if (file_info.isNull())
                break;

            if (file_info->getType() != xd::filetypeFolder)
            {
                appMessageBox(_("The specified output path is invalid because it does not specify a valid folder."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);

                return;
            }
        }
    }


    bool exists = false;

    if (m_mode == modeOpen || m_mode == modeOpenTable || m_mode == modeSave || m_mode == modeSaveSmall)
    {
        exists = db->getFileExist(towstr(path));

        if (exists && (m_mode == modeOpen ||
                       m_mode == modeOpenTable ||
                       m_mode == modeSave ||
                       m_mode == modeSaveSmall))
        {
            // make sure it's a file
            xd::IFileInfoPtr file_info = db->getFileInfo(towstr(path));

            if (file_info)
            {
                if (file_info->getType() == xd::filetypeFolder)
                {
                    appMessageBox(_("The file name you have entered is a folder. A folder is not valid for this operation."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION | wxCENTER);
                    return;
                }
            }
        }

        if ((m_mode == modeOpen || m_mode == modeOpenTable) && !exists)
        {
            appMessageBox(_("The file name you have entered does not exist."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
        
        if ((m_mode == modeSave || m_mode == modeSaveSmall) && exists)
        {
            xd::IFileInfoPtr file_info = db->getFileInfo(towstr(path));
            if (file_info.isNull())
                return;

            if (file_info->getType() == xd::filetypeFolder)
            {
                appMessageBox(_("The file name you have entered is a folder, and is not a valid target location."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }

            if (m_overwrite_prompt)
            {
                int result;
                result = appMessageBox(_("The file name you have entered already exists.  Would you like to overwrite it?"),
                                            APPLICATION_NAME,
                                            wxYES_NO | wxCANCEL | wxICON_EXCLAMATION | wxCENTER);

                if (result == wxCANCEL)
                {
                    EndModal(wxID_CANCEL);
                    return;
                }

                if (result == wxNO)
                    return;
            }
        }
    }


    EndModal(wxID_OK);
}

void DlgDatabaseFile::onCancel(wxCommandEvent& evt)
{
    m_folder = wxEmptyString;
    m_filename = wxEmptyString;
    EndModal(wxID_CANCEL);
}

wxString DlgDatabaseFile::getPath()
{
    // if we're using the dbcombo control, get the path
    // from it at this time
    if (m_mode == modeSaveSmall)
        m_folder = m_dbcombo->getPath();
    
    if (m_folder.IsEmpty())
        return wxEmptyString;
    
    wxString path = m_folder;

    // make sure the folder and filename are separated by one
    // slash only; allow the "filename" to also contain partial
    // folder info, so that we might have m_folder = "/abc/" and
    // m_filename = "/def/ghi"
    if (path.Last() != wxT('/') && !m_filename.StartsWith(wxT("/")))
        path += wxT('/');
    if (path.Last() == wxT('/') && m_filename.StartsWith(wxT("/")))
        path.RemoveLast();

    path += m_filename;
    
    path.Trim();
    path.Trim(TRUE);

    if (path.GetChar(0) != wxT('/'))
        path.Prepend(wxT("/"));

    if (m_mode == modeSelectFolder)
    {
        // if we just want the folder part, make sure
        // there is no trailing slash
        if (path.Length() > 0 && path.Last() == wxT('/'))
            path.RemoveLast();
    }
    
    return path;
}

wxString DlgDatabaseFile::getCaption()
{
    return m_caption;
}

void DlgDatabaseFile::onItemSelected(IFsItemPtr item)
{
    IDbFolderFsItemPtr folder = item;
    if (folder)
    {
        m_folder = DbDoc::getFsItemPath(item);
        if (m_folder.IsEmpty() || m_folder.Last() != wxT('/'))
            m_folder += wxT('/');
    }
     else
    {
        wxString path = DbDoc::getFsItemPath(item);

        if (path.Length() > 0 && path.Last() == wxT('/'))
            path.RemoveLast();

        m_folder = path.BeforeLast(wxT('/'));
        m_filename = path.AfterLast(wxT('/'));
        if (m_folder.IsEmpty())
            m_folder = wxT("/");
        m_name_ctrl->SetValue(m_filename);
    }
}

void DlgDatabaseFile::onItemActivated(IFsItemPtr item)
{
    onItemSelected(item);
    
    if (m_mode != modeSelectFolder)
    {
        wxString path = getPath();
        
        xd::IFileInfoPtr file_info;
        file_info = g_app->getDatabase()->getFileInfo(towstr(path));

        if (!file_info)
            return;

        if (file_info->getType() == xd::filetypeFolder)
            return;
    }

    wxCommandEvent evt;
    onOK(evt);
}

void DlgDatabaseFile::onComboItemSelected()
{
    // item in dropdown combo box was changed

    if (m_name_ctrl)
    {
        m_name_ctrl->SetFocus();
        m_name_ctrl->SetInsertionPointEnd();
        m_name_ctrl->SetSelection(-1, -1);
    }
}
    

void DlgDatabaseFile::onItemBeginLabelEdit(IFsItemPtr item,
                                           bool* allow)
{
    *allow = false;
    return;
}
