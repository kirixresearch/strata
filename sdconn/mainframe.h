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

namespace kcl { class ScrollListControl; }

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrame() { }

public:

    kcl::ScrollListControl* m_list;
};
