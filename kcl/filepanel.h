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
    void setWildcard(const wxString& value);
    void setFolderOnly(bool value);

    void getSelection(std::vector<FileInfo>& files);

private:

    bool populate();

    wxString m_curdir;
    wxString m_filespec;
    bool m_folder_only;
    std::vector<FileInfo> m_files;
};




// event declarations/classes

class WXDLLIMPEXP_AUI FilePanelEvent : public wxNotifyEvent
{
public:
    FilePanelEvent(wxEventType command_type = wxEVT_NULL, int win_id = 0) : wxNotifyEvent(command_type, win_id)
    {
    }

    FilePanelEvent(const FilePanelEvent& c) : wxNotifyEvent(c)
    {
    }

    wxEvent *Clone() const { return new FilePanelEvent(*this); }

private:

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(FilePanelEvent)
};


class FilePanel : public wxPanel
{
public:
   
    FilePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~FilePanel();

    void setFolderOnly(bool value);
    void setFilterString(const wxString& value);
    void setFilterIndex(int value);

    void setDirectory(const wxString& value);

    wxString getFilename();
    void getFilenames(std::vector<wxString>& result);

    wxString getPath();
    void getPaths(std::vector<wxString>& result);

private:

    void onTreeSelectionChanging(wxTreeEvent& evt);
    void onTreeSelectionChanged(wxTreeEvent& evt);
    void onFileCtrlItemSelected(wxListEvent& evt);
    void onFileCtrlItemActivated(wxListEvent& evt);
    void onPathCtrlEnterPressed(wxCommandEvent& evt);
    void onFilterChoice(wxCommandEvent& evt);
    void onGoParentClicked(wxCommandEvent& evt);
    void onChildFocus(wxChildFocusEvent& evt);
    void onIdle(wxIdleEvent& evt);

private:

    wxTreeCtrl* m_location_tree;
    wxTextCtrl* m_path_ctrl;
    FileCtrl* m_file_ctrl;
    wxTextCtrl* m_filename_ctrl;
    wxChoice* m_filter_ctrl;
    bool m_folder_only;

    wxString m_filter_string;
    int m_filter_index;
    std::vector<wxString> m_wildcards;
    bool m_filename_ctrl_focus_received;

    DECLARE_EVENT_TABLE()
};









};  // namespace kcl




wxDECLARE_EXPORTED_EVENT( WXDLLIMPEXP_AUI, wxEVT_FILEPANEL_ITEM_ACTIVATED, kcl::FilePanelEvent );

typedef void (wxEvtHandler::*KclFilePanelEventFunction)(kcl::FilePanelEvent&);

#define KclFilePanelEventHandler(func) \
    wxEVENT_HANDLER_CAST(KclFilePanelEventFunction, func)

#define EVT_FILEPANEL_ITEM_ACTIVATED(id, func) \
   wx__DECLARE_EVT1(wxEVT_FILEPANEL_ITEM_ACTIVATED, id, KclFilePanelEventHandler(func))




#endif


