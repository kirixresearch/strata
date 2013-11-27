/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Connector
 * Author:   Benjamin I. Williams
 * Created:  2013-11-27
 *
 */

#include <wx/wx.h>
#include "../kcl/scrolllistcontrol.h"
#include "mainframe.h"

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame((wxFrame*)NULL,-1,title,pos,size)
{

    m_list = new kcl::ScrollListControl(this, -1);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_list, 1, wxEXPAND);
    SetSizer(sizer);
}


