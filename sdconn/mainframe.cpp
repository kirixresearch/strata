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


    // create bitmap element
    kcl::ScrollListElement* bitmap;
    bitmap = item->addElement(GETBMP(gf_checkmark_32));
    bitmap->setPadding(0,0,15,15);
    bitmap->setName(wxT("bitmap"));

    // create name text element
    kcl::ScrollListElement* name;
    name = item->addElement("Ap_hist");
    name->setPadding(0,0,10,8);
    name->setRelativePosition(bitmap,
                              kcl::ScrollListElement::positionOnRight);
    name->setTextBold(true);
    name->setTextWrap(false);
    name->setName(wxT("name"));


    m_list->addItem(item);
}

