/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2013-12-12
 *
 */



#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/filectrl.h>
#include "filepanel.h"


namespace kcl
{


enum
{
    ID_First = wxID_HIGHEST+1
};


BEGIN_EVENT_TABLE(FilePanel, wxNavigationEnabled<wxPanel>)
END_EVENT_TABLE()


FilePanel::FilePanel(wxWindow* parent, wxWindowID id) : wxPanel(parent,
                                                                id,
                                                                wxDefaultPosition,
                                                                wxDefaultSize,
                                                                wxTAB_TRAVERSAL | wxCLIP_CHILDREN)
{
    m_file_ctrl = new wxFileListCtrl(this, -1, "", false);
    m_file_ctrl->GoToHomeDir();

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_file_ctrl, 1, wxEXPAND);

    SetSizer(main_sizer);
}

FilePanel::~FilePanel()
{
}



}; // namespace kcl




