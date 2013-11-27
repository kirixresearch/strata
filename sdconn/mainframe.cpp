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
#include <wx/aui/auibar.h>
#include "../kcl/scrolllistcontrol.h"
#include "app.h"
#include "mainframe.h"

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame((wxFrame*)NULL,-1,title,pos,size)
{
    m_toolbar = new wxAuiToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxAUI_TB_TEXT);
    m_toolbar->AddTool(16000, _("Add Table"), GETBMP(gf_db_conn_blue_24));
    m_toolbar->Realize();

    m_list = new kcl::ScrollListControl(this, -1);


    addItem();
    addItem();
    addItem();
    addItem();

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_toolbar, 0, wxEXPAND);
    sizer->Add(m_list, 1, wxEXPAND);
    SetSizer(sizer);
}



void MainFrame::addItem()
{
    kcl::ScrollListItem* item = new kcl::ScrollListItem;

    // create 'uninstall' button element
    wxButton* uninstall_button = new wxButton(m_list,
                                              -1,
                                              _("Uninstall"));

    kcl::ScrollListElement* uninstall2;
    uninstall2 = item->addElement(uninstall_button);
    uninstall2->setPadding(8,8,15,15);
    uninstall2->setVisible(true);
    uninstall2->setName(wxT("uninstall_button"));


    m_list->addItem(item);
}

