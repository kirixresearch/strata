/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-07-09
 *
 */


#include "appmain.h"
#include "dlgprojectmgr.h"
#include "appcontroller.h"
#include "paneloptions.h"
#include <wx/dir.h>


// ProjectMgr class implementation

ProjectMgr::ProjectMgr()
{
    refresh();
}

ProjectMgr::~ProjectMgr()
{
}

void ProjectMgr::refresh()
{
    m_projects.clear();

    // setting for displaying project size
    bool display_project_size = g_app->getAppPreferences()->getBoolean(wxT("app.project_mgr.display_project_size"), true);

    // read all data from the sysconfig
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");

    std::vector<std::wstring> project_keys = config->getGroups();
    std::vector<std::wstring>::iterator it;


    std::wstring dbname, dblocation, dbuser, dbpasswd;
    bool local;
    ProjectInfo c;

    for (it = project_keys.begin(); it != project_keys.end(); ++it)
    {
        config->setPath(L"/RecentDatabases/" + *it);

        config->read(L"Local",    &local,     true);
        config->read(L"Name",     dbname,     L"");
        config->read(L"Location", dblocation, L"");
        config->read(L"User",     dbuser,     L"");
        config->read(L"Password", dbpasswd,   L"");

        c.entry_name = *it;
        c.local = local;
        c.name = dbname;
        c.location = dblocation;
        c.user_id = dbuser;
        c.passwd = dbpasswd;

        if (c.name.length() == 0)
        {
            std::wstring lowercase_loc = c.location;
            kl::makeLower(lowercase_loc);
            
            if (lowercase_loc.find(L"xdprovider") == lowercase_loc.npos)
            {
                c.name = c.location;
                size_t idx = c.name.find_last_of(PATH_SEPARATOR_CHAR);
                if (idx != c.name.npos)
                    c.name = c.name.substr(idx+1);
            }
        }
        
        
        // fix stupid bug we had
        if (c.user_id == "admin" && c.passwd == "admin")
            c.passwd = wxT("");

        m_projects.push_back(c);
    }
}

bool ProjectMgr::addProjectEntry(const wxString& name,
                                 const wxString& location,
                                 const wxString& user_id,
                                 const wxString& password,
                                 bool local)
{
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");

    wxString new_connection;
    int counter = 0;
    while (1)
    {
        new_connection = wxString::Format(wxT("connection%03d"), counter++);
        if (!config->exists(towstr(new_connection)))
            break;
    }

    config->setPath(towstr(new_connection));
    config->write(wxT("Local"), local);
    config->write(wxT("Name"), towstr(name));
    config->write(wxT("Location"), towstr(location));
    config->write(wxT("User"), towstr(user_id));
    config->write(wxT("Password"), towstr(password));

    config->flush();

    refresh();

    return true;
}

bool ProjectMgr::modifyProjectEntry(int idx,
                                    const wxString& name,
                                    const wxString& location,
                                    const wxString& user_id,
                                    const wxString& password)
{
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");
    config->setPath(m_projects[idx].entry_name);

    if (name.Length() > 0)
        config->write(wxT("Name"), towstr(name));

    if (location.Length() > 0)
        config->write(wxT("Location"), towstr(location));

    if (user_id.Length() > 0)
        config->write(wxT("User"), towstr(user_id));

    if (password.Length() > 0)
        config->write(wxT("Password"), towstr(password));
    
    refresh();

    return true;
}

bool ProjectMgr::deleteProjectEntry(int idx)
{
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/RecentDatabases");
    config->deleteGroup(m_projects[idx].entry_name);

    refresh();

    return true;
}

std::vector<ProjectInfo>& ProjectMgr::getProjectEntries()
{
    return m_projects;
}

int ProjectMgr::getIdxFromEntryName(const wxString& entry_name)
{
    int idx = 0;
    
    std::vector<ProjectInfo>::iterator it;
    for (it = m_projects.begin(); it != m_projects.end(); ++it)
    {
        if (it->entry_name == entry_name)
            return idx;
        idx++;
    }

    return -1;
}

int ProjectMgr::getIdxFromLocation(const wxString& location)
{
    int idx = 0;
    
    std::vector<ProjectInfo>::iterator it;
    for (it = m_projects.begin(); it != m_projects.end(); ++it)
    {
        std::wstring entry_location = it->location;

        if (kl::icontains(entry_location, L"xdprovider="))
        {
            xd::ConnectionString cstr(entry_location);
            std::wstring provider = cstr.getLowerValue(L"xdprovider");

            if (provider == L"xdnative" || provider == L"xdfs")
            {
                entry_location = cstr.getLowerValue(L"database");
            }
        }


#ifdef __WXMSW__
        // windows paths are case insensitive
        if (0 == wcscasecmp(entry_location.c_str(), location.c_str()))
            return idx;
#else
        if (entry_location == location)
            return idx;
#endif
        idx++;
    }

    return -1;
}




// Add Project Dialog class implementation

enum
{
    ID_Add_Radio = wxID_HIGHEST + 1,
    ID_Add_BrowseButton,
    ID_Create_Radio,
    ID_Create_BrowseButton
};


class DlgAddProject : public wxDialog
{
public:

    enum AddAction
    {
        AddExistingProject = 0,
        CreateNewProject
    };
    
public:

    DlgAddProject(wxWindow* parent, ProjectInfo* info)
                        : wxDialog(parent, -1, _("Add Project"),
                             wxDefaultPosition,
                             wxSize(500,320),
                             wxDEFAULT_DIALOG_STYLE |
                             wxNO_FULL_REPAINT_ON_RESIZE |
                             wxCLIP_CHILDREN |
                             wxCENTER |
                             wxRESIZE_BORDER), m_info(info)
    {
        SetMinSize(wxSize(500,320));


        // create 'create project' radio button
        m_create_radio = new wxRadioButton(this, ID_Create_Radio, _("Create a new project"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

        // create 'create project' name sizer
        wxStaticText* label_create_name = new wxStaticText(this, -1, _("Name:"));
        m_create_name_textctrl = new wxTextCtrl(this, -1, _("New Project"));

        wxBoxSizer* create_name_sizer = new wxBoxSizer(wxHORIZONTAL);
        create_name_sizer->Add(label_create_name, 0, wxALIGN_CENTER | wxLEFT, 30);
        create_name_sizer->Add(m_create_name_textctrl, 1, wxALIGN_CENTER);

        // create 'create project' loc sizer
        wxStaticText* label_create_loc = new wxStaticText(this, -1, _("Location:"));
        m_create_loc_textctrl = new wxTextCtrl(this, -1, getDefaultProjectsPath());
        m_create_browse_button = new wxButton(this,
                                              ID_Create_BrowseButton,
                                              _("Browse..."));

        wxBoxSizer* create_loc_sizer = new wxBoxSizer(wxHORIZONTAL);
        create_loc_sizer->Add(label_create_loc, 0, wxALIGN_CENTER | wxLEFT, 30);
        create_loc_sizer->Add(m_create_loc_textctrl, 1, wxALIGN_CENTER);
        create_loc_sizer->AddSpacer(5);
        create_loc_sizer->Add(m_create_browse_button, 0);

        // create 'add project' radio button
        m_add_radio = new wxRadioButton(this, ID_Add_Radio, _("Add an existing project to the project manager list"));
        
        // create 'add project' location sizer
        
        wxStaticText* label_add_loc = new wxStaticText(this, -1, _("Location:"));
        m_add_loc_textctrl = new wxTextCtrl(this, -1);
        m_add_browse_button = new wxButton(this,
                                           ID_Add_BrowseButton,
                                           _("Browse..."));

        wxBoxSizer* add_loc_sizer = new wxBoxSizer(wxHORIZONTAL);
        add_loc_sizer->Add(label_add_loc, 0, wxALIGN_CENTER | wxLEFT, 30);
        add_loc_sizer->Add(m_add_loc_textctrl, 1, wxALIGN_CENTER);
        add_loc_sizer->AddSpacer(5);
        add_loc_sizer->Add(m_add_browse_button, 0);


        // measure the label widths
        wxSize label_size = getMaxTextSize(label_add_loc,
                                                label_create_loc,
                                                label_create_name);
        label_size.x += 10;

        add_loc_sizer->SetItemMinSize(label_add_loc, label_size);
        create_loc_sizer->SetItemMinSize(label_create_loc, label_size);
        create_name_sizer->SetItemMinSize(label_create_name, label_size);
        

        // create vertical sizer
        
        wxStaticText* message = new wxStaticText(this, -1, _("Please select whether you would like to create a new project or add an existing project to the project manager list."));
        wxStaticBitmap* bmp = new wxStaticBitmap(this, -1, GETBMP(gf_project_32));

        wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
        vert_sizer->Add(message, 0, wxEXPAND);
        vert_sizer->AddSpacer(20);
        vert_sizer->Add(m_create_radio, 0, wxEXPAND);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(create_name_sizer, 0, wxEXPAND);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(create_loc_sizer, 0, wxEXPAND);
        vert_sizer->AddSpacer(20);
        vert_sizer->Add(m_add_radio, 0, wxEXPAND);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(add_loc_sizer, 0, wxEXPAND);


        // create top sizer
        
        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        top_sizer->AddSpacer(7);
        top_sizer->Add(bmp, 0, wxTOP, 7);
        top_sizer->AddSpacer(15);
        top_sizer->Add(vert_sizer, 1, wxEXPAND | wxTOP, 7);

        // set some initial values
        m_add_loc_textctrl->SetValue(m_info->location);
        m_add_loc_textctrl->SetFocus();

        // create a platform standards-compliant OK/Cancel sizer
        
        wxButton* ok_button = new wxButton(this, wxID_OK);
        wxButton* cancel_button = new wxButton(this, wxID_CANCEL);
        
        wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
        ok_cancel_sizer->AddButton(ok_button);
        ok_cancel_sizer->AddButton(cancel_button);
        ok_cancel_sizer->Realize();
        ok_cancel_sizer->AddSpacer(5);
        
        // this code is necessary to get the sizer's bottom margin to 8
        wxSize min_size = ok_cancel_sizer->GetMinSize();
        min_size.SetHeight(min_size.GetHeight()+16);
        ok_cancel_sizer->SetMinSize(min_size);


        // create main sizer
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->AddSpacer(8);
        main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        main_sizer->AddStretchSpacer();
        main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
        SetSizer(main_sizer);
        Layout();
        
        SetSize(500,320);
        resizeStaticText(message, message->GetClientSize().GetWidth());
        Center();
        
        // start the dialog out on the "Add Existing Project" radio button
        wxCommandEvent unused;
        unused.SetId(ID_Create_Radio);
        onRadio(unused);
    }

    ~DlgAddProject()
    {
    
    }
    
    int getAction()
    {
        if (m_add_radio->GetValue())
            return DlgAddProject::AddExistingProject;
        
        return DlgAddProject::CreateNewProject;
    }
    
    void onRadio(wxCommandEvent& evt)
    {
        int id = evt.GetId();
        bool add = (id == ID_Add_Radio) ? true : false;
        bool create = (id == ID_Create_Radio) ? true : false;
        
        m_add_radio->SetValue(add);
        m_add_browse_button->Enable(add);
        m_add_loc_textctrl->Enable(add);
        if (add)
        {
            m_add_loc_textctrl->SetFocus();
            m_add_loc_textctrl->SetInsertionPointEnd();
        }
        
        m_create_radio->SetValue(create);
        m_create_loc_textctrl->Enable(create);
        m_create_browse_button->Enable(create);
        m_create_name_textctrl->Enable(create);
        if (create)
        {
            m_create_name_textctrl->SetFocus();
            m_create_name_textctrl->SetInsertionPointEnd();
            m_create_name_textctrl->SetSelection(-1,-1);
        }
    }
    
    void onAddBrowse(wxCommandEvent& evt)
    {
        wxString start_path = m_add_loc_textctrl->GetValue();
        if (!wxDir::Exists(start_path))
            start_path = getDefaultProjectsPath();
        
        wxDirDialog dlg(this, _("Please select a project folder"));
        dlg.SetPath(start_path);
        
        if (dlg.ShowModal() == wxID_OK)
        {
            m_add_loc_textctrl->SetValue(dlg.GetPath());
            m_add_loc_textctrl->SetInsertionPointEnd();
            m_add_loc_textctrl->SetFocus();
        }
    }

    void onCreateBrowse(wxCommandEvent& evt)
    {
        wxString start_path = m_create_loc_textctrl->GetValue();
        if (!wxDir::Exists(start_path))
            start_path = getDefaultProjectsPath();
        
        wxDirDialog dlg(this, _("Please select a project folder"));
        dlg.SetPath(start_path);
        
        if (dlg.ShowModal() == wxID_OK)
        {
            m_create_loc_textctrl->SetValue(dlg.GetPath());
            m_create_loc_textctrl->SetInsertionPointEnd();
            m_create_loc_textctrl->SetFocus();
        }
    }
    
    void resetFocus()
    {
        if (m_add_radio->GetValue())
        {
            m_add_loc_textctrl->SetFocus();
            m_add_loc_textctrl->SetInsertionPointEnd();
        }
         else
        {
            m_create_name_textctrl->SetFocus();
            m_create_name_textctrl->SetInsertionPointEnd();
        }
    }

    bool isAddPathOk()
    {
        //  check when adding an existing project
        
        m_info->name = wxEmptyString;
        m_info->location = m_add_loc_textctrl->GetValue().Trim(true).Trim(false);


        // empty path, bail out
        if (m_info->location.length() == 0)
        {
            appMessageBox(_("The specified location is empty.  Please enter the location of the project you would like to add."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            resetFocus();
            return false;
        }

        // if we're not using a raw connection string or a remote location, 
        // do some checks on the location that's been provided
        if (m_info->location.find(L"xdprovider=") == m_info->location.npos && 
            m_info->location.substr(0,6) != L"http://" && 
            m_info->location.substr(0,7) != L"https://")
        {
            if (!xf_get_directory_exist(m_info->location))
            {
                appMessageBox(_("The specified folder does not exist or is invalid."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                resetFocus();
                return false;
            }
        
            std::wstring test_path = m_info->location;
            test_path += PATH_SEPARATOR_STR;
            test_path += L"ofs";

            if (!xf_get_directory_exist(test_path))
            {
                appMessageBox(_("The specified folder does not contain a valid project."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                resetFocus();
                return false;
            }
        }
        
        // now check to see if this project already
        // exists in the project manager list
        ProjectMgr project_mgr;
        std::vector<ProjectInfo>& connections = project_mgr.getProjectEntries();
        std::vector<ProjectInfo>::iterator it;
        for (it = connections.begin(); it != connections.end(); ++it)
        {
            if (kl::iequals(it->location, m_info->location))
            {
                appMessageBox(_("The specified project already exists in the project manager list."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                resetFocus();
                return false;
            }
        }
        
        return true;
    }
    
    bool isCreatePathOk()
    {
        // check when creating a new project
        
        wxString location = m_create_loc_textctrl->GetValue();
        if (location.Last() != PATH_SEPARATOR_CHAR)
            location += PATH_SEPARATOR_CHAR;
        location += m_create_name_textctrl->GetValue();
        
        m_info->name = m_create_name_textctrl->GetValue();
        m_info->location = location;
        
        // empty path, bail out

        if (m_info->location.length() == 0)
        {
            appMessageBox(_("The specified location is empty.  Please enter the location of the project you would like to create."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            resetFocus();
            return false;
        }
    
        if (!xf_get_directory_exist(m_info->location))
        {
            if (xf_is_valid_directory_path(m_info->location))
            {
                // try to create all of the folders needed to create this path
                // (this function deletes the created folders on success or failure)
                if (!tryCreateFolderStructure(m_info->location))
                {
                    appMessageBox(_("The specified folder could not be created.  Please check to make sure the specified path is valid."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION | wxCENTER);
                    resetFocus();
                    return false;
                }
            }
             else
            {
                appMessageBox(_("The specified folder could not be created.  Please check to make sure the specified path is valid."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                resetFocus();
                return false;
            }
        }
         else
        {
            // check to see if this is a database project folder
            
            wxString ofs_path = m_info->location;
            ofs_path += PATH_SEPARATOR_STR;
            ofs_path += wxT("ofs");

            if (xf_get_directory_exist(towstr(ofs_path)))
            {
                appMessageBox(_("The specified folder already contains a project."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                resetFocus();
                return false;
            }

            // check to see if the folder is empty
            
            bool has_children = false;
            
            xf_dirhandle_t handle = xf_opendir(m_info->location);
            xf_direntry_t dirinfo;

            while (xf_readdir(handle, &dirinfo))
            {
                if (dirinfo.m_name == L"." || dirinfo.m_name == L"..")
                    continue;

                has_children = true;
                break;
            }

            xf_closedir(handle);

            if (has_children)
            {
                int res = appMessageBox(_("The specified folder is not empty.  It is highly recommended that an empty folder is used when creating a new project.  Would you like to continue?"),
                                             APPLICATION_NAME,
                                             wxYES_NO | wxICON_QUESTION | wxCENTER);
                if (res != wxYES)
                {
                    resetFocus();
                    return false;
                }
            }
        }
        
        // try to create the new project folder
        
        if (!g_app->getAppController()->createProject(m_info->location, m_info->name))
        {
            appMessageBox(_("An error occurred while attempting to create the project.  The specified location is probably inaccessible or invalid."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            resetFocus();
            return false;
        }

        return true;
    }
    
    bool isProjectPathOk()
    {
        if (m_add_radio->GetValue())
            return isAddPathOk();
             else
            return isCreatePathOk();
    }
    
    void onOK(wxCommandEvent& evt)
    {
        if (isProjectPathOk())
            EndModal(wxID_OK);
    }
    
    void onCancel(wxCommandEvent& evt)
    {
        EndModal(wxID_CANCEL);
    }
    
private:
    
    ProjectInfo* m_info;
    
    wxRadioButton* m_add_radio;
    wxTextCtrl* m_add_loc_textctrl;
    wxButton* m_add_browse_button;

    wxRadioButton* m_create_radio;
    wxTextCtrl* m_create_loc_textctrl;
    wxTextCtrl* m_create_name_textctrl;
    wxButton* m_create_browse_button;
    
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(DlgAddProject, wxDialog)
    EVT_BUTTON(wxID_OK, DlgAddProject::onOK)
    EVT_BUTTON(wxID_CANCEL, DlgAddProject::onCancel)
    EVT_RADIOBUTTON(ID_Add_Radio, DlgAddProject::onRadio)
    EVT_RADIOBUTTON(ID_Create_Radio, DlgAddProject::onRadio)
    EVT_BUTTON(ID_Add_BrowseButton, DlgAddProject::onAddBrowse)
    EVT_BUTTON(ID_Create_BrowseButton, DlgAddProject::onCreateBrowse)
END_EVENT_TABLE()




enum
{
    ID_AddProject = wxID_HIGHEST + 1,
    ID_OpenProject,
    ID_RenameProject,
    ID_RemoveProject,
    ID_DeleteProject
};

enum
{
    colName = 0,
    colType = 1,
    colLocation = 2,
    colSize = 3,
    colUser = 4
};


BEGIN_EVENT_TABLE(DlgProjectMgr, wxDialog)
    EVT_MENU(ID_OpenProject, DlgProjectMgr::onOpenProject)
    EVT_MENU(ID_RemoveProject, DlgProjectMgr::onRemoveProject)
    EVT_MENU(ID_RenameProject, DlgProjectMgr::onRenameProject)
    EVT_MENU(ID_DeleteProject, DlgProjectMgr::onDeleteProject)
    EVT_BUTTON(ID_AddProject, DlgProjectMgr::onAddProject)
    EVT_BUTTON(wxID_OK, DlgProjectMgr::onOpenProject)
    EVT_BUTTON(wxID_CANCEL, DlgProjectMgr::onCancel)
    EVT_KCLGRID_CELL_RIGHT_CLICK(DlgProjectMgr::onGridCellRightClick)
    EVT_KCLGRID_CELL_LEFT_DCLICK(DlgProjectMgr::onGridCellLeftDClick)
    EVT_KCLGRID_END_EDIT(DlgProjectMgr::onGridEndEdit)
END_EVENT_TABLE()



DlgProjectMgr::DlgProjectMgr(wxWindow* parent, wxWindowID id) :
                  wxDialog(parent, -1, _("Projects"),
                    wxDefaultPosition, wxSize(520,340),
                    wxCAPTION | wxRESIZE_BORDER | wxCLIP_CHILDREN)
{
    SetMinSize(wxSize(520,340));
    
    // create grid
    m_grid = new kcl::RowSelectionGrid(this,
                                       wxID_ANY,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       kcl::DEFAULT_BORDER,
                                       false, false);
    m_grid->setOptionState(kcl::Grid::optGhostRow |
                           kcl::Grid::optInvalidAreaClickable, false);
    m_grid->setGreenBarInterval(0);
    
    m_grid->createModelColumn(colName, _("Project Name"), kcl::Grid::typeCharacter, 256, 0);
    m_grid->createModelColumn(colType, _("Type"), kcl::Grid::typeCharacter, 256, 0);
    m_grid->createModelColumn(colLocation, _("Location"), kcl::Grid::typeCharacter, 512, 0);
    m_grid->createModelColumn(colSize, _("Size"), kcl::Grid::typeCharacter, 28, 0);
    m_grid->createModelColumn(colUser, _("User"), kcl::Grid::typeCharacter, 28, 0);
    m_grid->createDefaultView();

    m_grid->setColumnSize(colName, 160);
    m_grid->setColumnSize(colType, 60);
    m_grid->setColumnSize(colLocation, 160);
    m_grid->setColumnSize(colSize, 60);
    m_grid->setColumnSize(colUser, 100);
    
    // set cell properties for the grid
    kcl::CellProperties props;
    props.mask = kcl::CellProperties::cpmaskEditable;
    props.editable = false;
    m_grid->setModelColumnProperties(colType, &props);
    m_grid->setModelColumnProperties(colLocation, &props);
    m_grid->setModelColumnProperties(colSize, &props);
    m_grid->setModelColumnProperties(colUser, &props);

    props.mask = kcl::CellProperties::cpmaskAlignment;
    props.alignment = kcl::Grid::alignRight;
    m_grid->setModelColumnProperties(colSize, &props);
    
    // create a platform standards-compliant OK/Cancel sizer
    
    wxButton* add_button = new wxButton(this,
                                        ID_AddProject,
                                        _("Add Project..."));

    wxButton* open_button = new wxButton(this,
                                         wxID_OK,
                                         _("Open Project"));
    
    wxButton* cancelexit_button;
    if (g_app->getDatabase().isOk())
        cancelexit_button = new wxButton(this, wxID_CANCEL);
         else
        cancelexit_button = new wxButton(this, wxID_CANCEL, _("Exit"));
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(open_button);
    ok_cancel_sizer->AddButton(cancelexit_button);
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->Prepend(add_button, 0, wxALIGN_CENTER | wxLEFT, 8);
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    
    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(main_sizer);
    Layout();

    // this code changes the two proportional columns to be
    // non-proportionally sized, which will make the grid fill
    // the client window size initially, but still allow the user
    // to resize each column)

    bool display_project_size = g_app->getAppPreferences()->getBoolean(wxT("project_mgr.display_project_size"), true);

    // set the size of the location column
    int w, h;
    m_grid->GetClientSize(&w, &h);
    w -= m_grid->getColumnSize(m_grid->getColumnViewIdx(colName));
    if (display_project_size)
    {
        w -= m_grid->getColumnSize(m_grid->getColumnViewIdx(colSize));
    }
    m_grid->setColumnSize(m_grid->getColumnViewIdx(colLocation), w);

    m_grid->hideColumn(m_grid->getColumnViewIdx(colUser));      // hide user column for version 1.x
    m_grid->hideColumn(m_grid->getColumnViewIdx(colType));      // hide location column for version 1.x

    // if we don't want to display the project size (because it takes
    // a long time to populate in some circumstances, e.g. network drives),
    // hide the column
    if (!display_project_size)
        m_grid->hideColumn(m_grid->getColumnViewIdx(colSize));


    // populate grid
    populate();
    m_grid->clearSelection();
    if (m_grid->getRowCount() > 0)
        m_grid->setRowSelected(0, true);
    m_grid->moveCursor(0, colName, false);
    m_grid->refresh(kcl::Grid::refreshAll);

    SetSize(520,340);
    Center();
}

ProjectInfo DlgProjectMgr::getSelectedProject()
{
    if (!m_grid)
        return ProjectInfo();
    
    int row = m_grid->getCursorRow();
    std::vector<ProjectInfo>& connections = m_projmgr.getProjectEntries();
    if (row >= 0 && (size_t)row < connections.size())
        return connections[row];

    return ProjectInfo();
}

class ConnectionInfoLess
{
public:

     bool operator()(const ProjectInfo& c1,
                     const ProjectInfo& c2) const                
     {
         return wcscasecmp(c1.name.c_str(),
                           c2.name.c_str()) < 0 ? true : false;
     }
};

void DlgProjectMgr::populate()
{
    m_grid->deleteAllRows();
    
    // retrieve setting for displaying project size
    bool display_project_size = g_app->getAppPreferences()->getBoolean(wxT("project_mgr.display_project_size"), true);

    // now populate the grid
    m_projmgr.refresh();
    std::vector<ProjectInfo>& connections = m_projmgr.getProjectEntries();
    std::vector<ProjectInfo>::iterator it;
    int row = 0;
    
    // timers for making sure that the populate routine doesn't take too much time
    time_t t1, t2;
    t1 = time(NULL);

    std::sort(connections.begin(), connections.end(), ConnectionInfoLess());
    for (it = connections.begin(); it != connections.end(); ++it)
    {
        wxString name = it->name;
        if (name.Length() == 0)
        {
            name = _("(Untitled)");
        }

        std::wstring local_location = it->location;
        if (kl::icontains(local_location, L"xdprovider="))
        {
            xd::ConnectionString cstr(it->location);
            std::wstring provider = cstr.getLowerValue(L"xdprovider");

            if (provider == L"xdnative" || provider == L"xdfs")
            {
                local_location = cstr.getLowerValue(L"database");
            }
        }

        // calculate the project size
        wxString size_str = wxT("");
        
        t2 = time(NULL);
        if (display_project_size && (t2-t1) < 8)
        {
            double size = getProjectSize(local_location);
            double mb_size = size/1048576.0;
            double gb_size = size/1073741824.0;
            if (gb_size >= 1.0)
                size_str = wxString::Format(wxT("%.2f GB"), gb_size);
                 else
                size_str = wxString::Format(wxT("%.2f MB"), mb_size);
        }

        m_grid->insertRow(-1);
        m_grid->setCellString(row, colName, name);
        m_grid->setCellBitmap(row, colName, GETBMP(gf_project_16), kcl::Grid::alignLeft);
        m_grid->setCellString(row, colType, it->local ? _("Local") : _("Network"));
        m_grid->setCellString(row, colLocation, local_location);
        m_grid->setCellString(row, colSize, size_str);
        m_grid->setCellString(row, colUser, it->user_id);
        ++row;
    }

    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void DlgProjectMgr::checkOverlayText()
{
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(_("Click on the 'Add Project...' button\nbelow to create a new project"));
         else
        m_grid->setOverlayText(wxEmptyString);
}

void DlgProjectMgr::onGridCellRightClick(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    if (row >= 0 && row < m_grid->getRowCount())
    {
        m_grid->clearSelection();
        m_grid->setRowSelected(row, true);
        m_grid->moveCursor(row, colName);
        m_grid->refresh(kcl::Grid::refreshAll);

        wxMenu menuPopup;
        menuPopup.Append(ID_OpenProject, _("&Open"));
        menuPopup.Append(ID_RenameProject, _("&Rename"));
        menuPopup.AppendSeparator();
        menuPopup.Append(ID_RemoveProject, _("Remove from &List"));
        menuPopup.AppendSeparator();
        menuPopup.Append(ID_DeleteProject, _("&Delete"));

        wxPoint pt_mouse = ::wxGetMousePosition();
        pt_mouse = ScreenToClient(pt_mouse);
        PopupMenu(&menuPopup, pt_mouse);
    }
}

void DlgProjectMgr::onGridCellLeftDClick(kcl::GridEvent& evt)
{
    wxCommandEvent unused;
    onOpenProject(unused);
}

void DlgProjectMgr::onGridEndEdit(kcl::GridEvent& evt)
{
    if (evt.GetEditCancelled())
        return;
    
    if (evt.GetColumn() == colName)
    {
        int row = evt.GetRow();
        std::vector<ProjectInfo>& connections = m_projmgr.getProjectEntries();
        if (row >= 0 && (size_t)row < connections.size())
        {
            wxString new_name = evt.GetString();
            m_projmgr.modifyProjectEntry(row, new_name,
                                              wxEmptyString,
                                              wxEmptyString,
                                              wxEmptyString);
        }
    }
    
    m_grid->SetFocus();
}

void DlgProjectMgr::onOpenProject(wxCommandEvent& evt)
{
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    EndModal(wxID_OK);
}

void DlgProjectMgr::onCancel(wxCommandEvent& evt)
{
    // if we're here, either a) there's already a project open in which case
    // we'll just close this dialog and the user can continue with the open
    // project or b) there's no project open in which case this button says
    // 'Exit' and we'll exit the application (this is done after EndModal)
    
    if (g_app->getDatabase().isNull())
    {
        wxString appname = APPLICATION_NAME;
        wxString msg = wxString::Format(_("A project must be selected in order to run %s.  Are you sure you would like to exit %s?"),
                                        appname.c_str(), appname.c_str());
        
        // it is important to make sure that this dialog is used as the parent,
        // otherwise, the main application window gets the focus and the user
        // can do things outside of this dialog -- even though it is modal
        int res = appMessageBox(msg,
                                     APPLICATION_NAME,
                                     wxYES_NO |
                                     wxNO_DEFAULT |
                                     wxICON_QUESTION |
                                     wxCENTER,
                                     this);
        if (res == wxNO)
            return;
    }

    EndModal(wxID_CANCEL);
}

void DlgProjectMgr::onAddProject(wxCommandEvent& evt)
{
    ProjectInfo info;
    info.local = true;
    info.location = wxEmptyString;
    info.name = wxEmptyString;
    info.passwd = wxEmptyString;
    info.user_id = wxT("admin");

    DlgAddProject* dlg = new DlgAddProject(g_app->getMainWindow(), &info);
    if (dlg->ShowModal() == wxID_OK)
    {
        // when we create a new project (in the "Add Project" dialog),
        // the project manager adds an entry there
        if (dlg->getAction() == DlgAddProject::AddExistingProject)
        {
            m_projmgr.addProjectEntry(info.name,
                                      info.location,
                                      info.user_id,
                                      info.passwd,
                                      info.local);
        }
        
        //m_grid->Freeze();
        populate();
        m_grid->clearSelection();
        
        int row, row_count = m_grid->getRowCount();
        for (row = 0; row < row_count; ++row)
        {
            wxString loc = m_grid->getCellString(row, colLocation);
            if (loc.CmpNoCase(info.location) == 0)
            {
                m_grid->setRowSelected(row, true);
                m_grid->moveCursor(row, colName);
                break;
            }
        }
        
        m_grid->scrollVertToCursor();
        m_grid->refresh(kcl::Grid::refreshAll);
        //m_grid->Thaw();
    }
}

void DlgProjectMgr::onRemoveProject(wxCommandEvent& evt)
{
    int row = m_grid->getCursorRow();
    
    m_projmgr.deleteProjectEntry(row);
    populate();

    if (m_grid->getRowCount() == 0)
    {
        m_grid->moveCursor(0, colName);
        return;
    }
    
    if (row >= m_grid->getRowCount())
        row = m_grid->getRowCount()-1;
            
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->moveCursor(row, colName);
    m_grid->refresh(kcl::Grid::refreshAll);
}

void DlgProjectMgr::onRenameProject(wxCommandEvent& evt)
{
    int row = m_grid->getCursorRow();
    m_grid->moveCursor(row, colName, false);
    m_grid->beginEdit();
}



static bool _deleteTree(const wxString& path)
{
    wxLogNull log;

    std::vector<wxString> to_remove;

    wxString full_path;

    {
        wxDir dir(path);

        if (!dir.IsOpened())
        {
            return false;
        }

        wxString filename;
        bool cont = dir.GetFirst(&filename);
        while (cont)
        {
            full_path = path;
            if (full_path.Length() == 0 ||
                full_path.Last() != PATH_SEPARATOR_CHAR)
            {
                full_path += PATH_SEPARATOR_CHAR;
            }

            full_path += filename;

            if (::wxDirExists(full_path))
            {
                to_remove.push_back(full_path);
            }
             else
            {
                ::wxRemoveFile(full_path);
            }

            cont = dir.GetNext(&filename);
        }
    }



    for (std::vector<wxString>::iterator it = to_remove.begin();
            it != to_remove.end(); ++it)
    {
        _deleteTree(*it);
    }

    
    ::wxRmdir(path);

    return true;
}




void DlgProjectMgr::onDeleteProject(wxCommandEvent& evt)
{
    int row = m_grid->getCursorRow();
    if (row < 0 || row >= m_grid->getRowCount())
        return;
    
    std::vector<ProjectInfo>& connections = m_projmgr.getProjectEntries();
    wxString location = connections[row].location;

#ifdef WIN32
    if (location.CmpNoCase(g_app->getDatabaseLocation()) == 0)
#else
    if (location == g_app->getDatabaseLocation())
#endif
    {
        appMessageBox(_("The selected project is currently open.  Please close the project before deleting it."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }


    int res = appMessageBox(_("WARNING:  This operation will delete the selected project and all of its contents.  This includes all tables, queries, scripts, reports and other objects contained in the project.  Press 'Yes' only if you are absolutely certain you want to delete this project."),
                                 APPLICATION_NAME,
                                 wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION | wxCENTER);
    if (res != wxYES)
        return;

    AppBusyCursor bc;
    _deleteTree(location);
    
    m_projmgr.deleteProjectEntry(row);
    populate();
    
    if (m_grid->getRowCount() == 0)
    {
        m_grid->moveCursor(0, colName);
        return;
    }
    
    if (row >= m_grid->getRowCount())
        row = m_grid->getRowCount()-1;
            
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->moveCursor(row, colName);
    m_grid->refresh(kcl::Grid::refreshAll);
}

