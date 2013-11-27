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

    void addItem();

public:

    wxAuiToolBar* m_toolbar;
    kcl::ScrollListControl* m_list;
};
