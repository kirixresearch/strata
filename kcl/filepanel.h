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

#include <vector>
class wxTreeCtrl;

namespace kcl
{




class FileInfo
{
public:

    wxString name;
    bool folder;
    wxULongLong size;
    wxDateTime datetime;
};

class FileCtrl : public wxListCtrl
{
friend class FilePanel;

public:

    FileCtrl(wxWindow* parent,
             wxWindowID id = wxID_ANY,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxLC_LIST);

    bool goToDir(const wxString& dir);

    void getSelection(std::vector<FileInfo>& files);

private:

    bool populate();

    wxString m_curdir;
    wxString m_filespec;
    std::vector<FileInfo> m_files;
};


class FilePanel : public wxPanel
{
public:
   
    FilePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~FilePanel();

    void setFilterString(const wxString& value);
    void setFilterIndex(int value);

    wxString getFilename();
    void getFilenames(std::vector<wxString>& result);

    wxString getPath();
    void getPaths(std::vector<wxString>& result);

private:

    void onTreeSelectionChanging(wxTreeEvent& evt);
    void onTreeSelectionChanged(wxTreeEvent& evt);
    void onFileCtrlItemSelected(wxListEvent& evt);
    void onPathCtrlEnterPressed(wxCommandEvent& evt);
    void onChildFocus(wxChildFocusEvent& evt);
    void onIdle(wxIdleEvent& evt);

private:

    wxTreeCtrl* m_location_tree;
    FileCtrl* m_file_ctrl;
    wxTextCtrl* m_path_ctrl;
    wxChoice* m_filter_ctrl;

    wxString m_filter_string;
    int m_filter_index;
    std::vector<wxString> m_wildcards;
    bool m_path_ctrl_focus_received;

    DECLARE_EVENT_TABLE()
};


};


#endif


