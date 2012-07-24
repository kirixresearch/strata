/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-07-09
 *
 */


#ifndef __APP_DLGPROJECTMGR_H
#define __APP_DLGPROJECTMGR_H


struct ProjectInfo
{
    wxString entry_name;
    wxString name;
    wxString location;
    wxString user_id;
    wxString passwd;
    bool local;
};


class ProjectInfoLess
{
public:

     bool operator()(const ProjectInfo& x,
                     const ProjectInfo& y) const                
     {
        return x.name.CmpNoCase(y.name) < 0 ? true : false;
     }
};


class ProjectMgr
{

public:

    ProjectMgr();
    ~ProjectMgr();

    bool addProjectEntry(const wxString& name,
                         const wxString& location,
                         const wxString& user_id,
                         const wxString& password,
                         bool local);
    bool modifyProjectEntry(int idx, const wxString& name,
                                     const wxString& location,
                                     const wxString& user_id,
                                     const wxString& password);
    bool deleteProjectEntry(int idx);

    void refresh();
    std::vector<ProjectInfo>& getProjectEntries();
    int getIdxFromEntryName(const wxString& entry_name);
    int getIdxFromLocation(const wxString& entry_name);

private:

    std::vector<ProjectInfo> m_projects;
};




class DlgProjectMgr : public wxDialog
{
public:

    DlgProjectMgr(wxWindow* parent, wxWindowID = -1);

    ProjectInfo getSelectedProject();

private:

    void populate();
    void checkOverlayText();
    
    void onAddProject(wxCommandEvent& evt);
    void onOpenProject(wxCommandEvent& evt);
    void onRenameProject(wxCommandEvent& evt);
    void onRemoveProject(wxCommandEvent& evt);
    void onDeleteProject(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);

    void onGridCellRightClick(kcl::GridEvent& evt);
    void onGridCellLeftDClick(kcl::GridEvent& evt);
    void onGridEndEdit(kcl::GridEvent& evt);

private:

    ProjectMgr m_projmgr;
    kcl::RowSelectionGrid* m_grid;

    DECLARE_EVENT_TABLE()
};



#endif

