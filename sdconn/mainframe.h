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

class wxAuiToolBar;
namespace kcl { class ScrollListControl; }

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrame() { }

    void addItem(const std::wstring& name, const std::wstring& location);

private:

    void refreshList();
    void onAddTable(wxCommandEvent& evt);
    void onSettings(wxCommandEvent& evt);

private:

    wxAuiToolBar* m_toolbar;
    kcl::ScrollListControl* m_list;

private:
    DECLARE_EVENT_TABLE()
};
