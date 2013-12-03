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
#include <wx/filename.h>
#include <wx/aui/auibar.h>
#include <kl/string.h>
#include <kl/crypt.h>
#include "app.h"
#include "mainframe.h"
#include "dlgsettings.h"
#include "../kcl/scrolllistcontrol.h"



enum
{
    ID_AddTable = 20000,
    ID_Settings,
    ID_Delete
};


BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(ID_AddTable, MainFrame::onAddTable)
    EVT_MENU(ID_Settings, MainFrame::onSettings)
    EVT_MENU(ID_Delete, MainFrame::onDelete)
END_EVENT_TABLE()


MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
                                            : wxFrame((wxFrame*)NULL,-1,title,pos,size)
{
    m_toolbar = new wxAuiToolBar(this, -1, wxDefaultPosition, wxSize(40,80), wxAUI_TB_TEXT);
    m_toolbar->AddTool(ID_AddTable, _("Add Table"), GETBMP(gf_db_conn_blue_24));
    m_toolbar->AddTool(ID_Settings, _("Settings"), GETBMP(gf_gear_24));
    m_toolbar->AddSeparator();
    m_toolbar->AddTool(ID_Delete, _("Remove"), GETBMP(gf_x_24));
    m_toolbar->Realize();

    m_list = new kcl::ScrollListControl(this, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

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


void MainFrame::onAddTable(wxCommandEvent& evt)
{
    wxString filter;
    filter += _("All Files");
    filter += wxT(" (*.*)|*.*|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxFileDialog dlg(this,
                     _("Choose File"),
                     wxT(""),
                     wxT(""),
                     filter,
                     wxFD_OPEN|wxFD_MULTIPLE|wxFD_FILE_MUST_EXIST);

    if (dlg.ShowModal() != wxID_OK)
        return;

    xd::IDatabasePtr db = g_app->getDatabase();


    wxArrayString arr;
    dlg.GetPaths(arr);
    for (size_t i = 0; i < arr.GetCount(); ++i)
    {
        wxFileName fn(arr[i]);

        std::wstring key = kl::getUniqueString();
        std::wstring fullpath = arr[i];
        std::wstring filename = fn.GetName();

        db->setMountPoint(filename, L"", fullpath);
    }

    refreshList();


    g_app->getSdserv().updateAssetInformation();
}


void MainFrame::onSettings(wxCommandEvent& evt)
{
    DlgSettings dlg(this);

    kl::Config& config = g_app->getConfig();


    std::wstring user = config.read(L"Settings/User");
    std::wstring password = config.read(L"Settings/Password");
    std::wstring server = config.read(L"Settings/Server", kl::towstring(DEFAULT_SERVER));

    if (password.length() > 0)
        password = kl::decryptString(password, kl::towstring(PASSWORD_KEY));

    dlg.setUserName(user);
    dlg.setPassword(password);
    dlg.setService(server);

    if (dlg.ShowModal() != wxID_OK)
        return;

    config.write(L"Settings/User", towstr(dlg.getUserName()));
    config.write(L"Settings/Password", kl::encryptString(towstr(dlg.getPassword()), kl::towstring(PASSWORD_KEY)));
    config.write(L"Settings/Server", towstr(dlg.getService()));
}


void MainFrame::onDelete(wxCommandEvent& evt)
{
    std::vector<kcl::ScrollListItem*> to_delete;


    xd::IDatabasePtr db = g_app->getDatabase();

    size_t i, cnt = m_list->getItemCount();
    for (i = 0; i < cnt; ++i)
    {
        kcl::ScrollListItem* item = m_list->getItem(i);
        if (item->isSelected())
        {
            db->deleteFile(towstr(item->getExtraString()));
        }
    }


    refreshList();

}


void MainFrame::refreshList()
{
    m_list->clearItems();


    xd::IDatabasePtr db = g_app->getDatabase();
    xd::IFileInfoEnumPtr files = db->getFolderInfo(L"");
    xd::IFileInfoPtr finfo;

    size_t i, cnt = files->size();
    for (i = 0; i < cnt; ++i)
    {
        finfo = files->getItem(i);

        std::wstring cstr, fullpath;
        if (db->getMountPoint(finfo->getName(), cstr, fullpath))
        {   
            addItem(finfo->getName(), finfo->getName(), fullpath);
        }
    }

    m_list->refresh();
    m_list->SetFocus();
}



void MainFrame::addItem(const std::wstring& id, const std::wstring& name, const std::wstring& location)
{
    kcl::ScrollListItem* item = new kcl::ScrollListItem;
    item->setExtraString(id);

    // create bitmap element
    kcl::ScrollListElement* bitmap;
    bitmap = item->addElement(GETBMP(gf_checkmark_32), wxPoint(10,25));
    bitmap->setPadding(0,0,15,15);
    bitmap->setName(wxT("bitmap"));

    // create name text element
    kcl::ScrollListElement* element_name;
    element_name = item->addElement(name);
    element_name->setPadding(0,0,15,8);
    element_name->setRelativePosition(bitmap, kcl::ScrollListElement::positionOnRight);
    element_name->setTextBold(true);
    element_name->setTextWrap(false);
    element_name->setName(wxT("name"));


    // create name text element
    kcl::ScrollListElement* element_location;
    element_location = item->addElement(location);
    element_location->setPadding(0,0,10,8);
    element_location->setRelativePosition(element_name, kcl::ScrollListElement::positionBelow);
    element_location->setTextBold(false);
    element_location->setTextWrap(false);
    element_location->setName(wxT("name"));


     // create 'start now' button element
    wxButton* settings_button = new wxButton(m_list,
                                             -1,
                                             _("Settings"),
                                             wxDefaultPosition,
                                             wxSize(80,25),
                                             wxBU_EXACTFIT);

    kcl::ScrollListElement* element_settings;
    element_settings = item->addElement(settings_button);
    element_settings->setPadding(0,8,0,30);
    element_settings->setAbsolutePosition(wxPoint(-100, 25));
    element_settings->setName(wxT("settings_button"));

    m_list->addItem(item);
}


