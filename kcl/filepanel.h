/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2013-12-12
 *
 */


#ifndef __KCL_FILEPANEL_H
#define __KCL_FILEPANEL_H

class wxFileListCtrl;

namespace kcl
{


class FilePanel :  public wxPanel
{
public:
   
    FilePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~FilePanel();

private:

    wxFileListCtrl* m_file_ctrl;

    DECLARE_EVENT_TABLE()
};


};


#endif


