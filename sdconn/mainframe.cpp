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



enum
{
    ID_AddTable = 20000
};


BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    //EVT_BUTTON(ID_AddTable, MainFrame::onAddTable)
END_EVENT_TABLE()


MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
                                            : wxFrame((wxFrame*)NULL,-1,title,pos,size)
{
    m_toolbar = new wxAuiToolBar(this, ID_AddTable, wxDefaultPosition, wxDefaultSize, wxAUI_TB_TEXT);
    m_toolbar->AddTool(16000, _("Add Table"), GETBMP(gf_db_conn_blue_24));
    m_toolbar->Realize();

    m_list = new kcl::ScrollListControl(this, -1);

    refreshList();
    /*
    addItem();
    addItem();
    addItem();
    addItem();
    */

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_toolbar, 0, wxEXPAND);
    sizer->Add(m_list, 1, wxEXPAND);
    SetSizer(sizer);
}



void MainFrame::refreshList()
{
    // sync the list from the model
    kl::Config& model = g_app->getConfig();

    model.setPath(L"Resources");


}



void MainFrame::addItem()
{
    kcl::ScrollListItem* item = new kcl::ScrollListItem;


    // create bitmap element
    kcl::ScrollListElement* bitmap;
    bitmap = item->addElement(GETBMP(gf_checkmark_32), wxPoint(10,25));
    bitmap->setPadding(0,0,15,15);
    bitmap->setName(wxT("bitmap"));

    // create name text element
    kcl::ScrollListElement* name;
    name = item->addElement("Ap_hist");
    name->setPadding(0,0,15,8);
    name->setRelativePosition(bitmap, kcl::ScrollListElement::positionOnRight);
    name->setTextBold(true);
    name->setTextWrap(false);
    name->setName(wxT("name"));


    // create name text element
    kcl::ScrollListElement* location;
    location = item->addElement("c:\\users\\server\\data\\ap_hist.csv");
    location->setPadding(0,0,10,8);
    location->setRelativePosition(name, kcl::ScrollListElement::positionBelow);
    location->setTextBold(false);
    location->setTextWrap(false);
    location->setName(wxT("name"));


     // create 'start now' button element
    wxButton* settings_button = new wxButton(m_list,
                                             -1,
                                             _("Settings"),
                                             wxDefaultPosition,
                                             wxSize(80,25),
                                             wxBU_EXACTFIT);

    kcl::ScrollListElement* settings;
    settings = item->addElement(settings_button);
    settings->setPadding(0,8,0,30);
    settings->setAbsolutePosition(wxPoint(-100, 25));
    settings->setName(wxT("settings_button"));

    m_list->addItem(item);
}

