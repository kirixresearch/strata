/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  wxWebConnect Embedded Web Browser Control Library
 * Author:   Benjamin I. Williams
 * Created:  2007-04-23
 *
 */


#include <wx/wx.h>
#include "webframe.h"
#include "webcontrol.h"


///////////////////////////////////////////////////////////////////////////////
//  wxWebFrame class implementation
///////////////////////////////////////////////////////////////////////////////


BEGIN_EVENT_TABLE(wxWebFrame, wxFrame)
END_EVENT_TABLE()


wxWebFrame::wxWebFrame(wxWindow* parent,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style) : wxFrame(parent, id, title, pos, size, style)
{
    m_should_prevent_app_exit = true;
    
    m_ctrl = new wxWebControl(this, -1, wxPoint(0,0), wxSize(200,200));
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_ctrl, 1, wxEXPAND);
    
    SetSizer(sizer);
}

wxWebFrame::~wxWebFrame()
{
}

wxWebControl* wxWebFrame::GetWebControl()
{
    return m_ctrl;
}




///////////////////////////////////////////////////////////////////////////////
//  wxWebDialog class implementation
///////////////////////////////////////////////////////////////////////////////


BEGIN_EVENT_TABLE(wxWebDialog, wxDialog)
END_EVENT_TABLE()


wxWebDialog::wxWebDialog(wxWindow* parent,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style) : wxDialog(parent, id, title, pos, size, style)
{
    m_ctrl = new wxWebControl(this, -1, wxPoint(0,0), wxSize(200,200));
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_ctrl, 1, wxEXPAND);
    
    SetSizer(sizer);
}

wxWebDialog::~wxWebDialog()
{
}

wxWebControl* wxWebDialog::GetWebControl()
{
    return m_ctrl;
}

