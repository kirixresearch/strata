/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-07-09
 *
 */


#ifndef H_APP_DLGPROJECTMGR_H
#define H_APP_DLGPROJECTMGR_H

#include "projectmgr.h"


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

