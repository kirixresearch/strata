/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-11-27
 *
 */


#ifndef __APP_DLGDATABASEFILE_H
#define __APP_DLGDATABASEFILE_H


class DbComboCtrl;
class DlgDatabaseFile : public wxDialog,
                        public xcm::signal_sink
{
public:

    enum
    {
        modeOpen = 0,
        modeSave = 1,
        modeSaveSmall = 2,
        modeSelectFolder = 3,
        modeOpenTable = 4
    };

public:

    DlgDatabaseFile(wxWindow* parent, int mode = DlgDatabaseFile::modeOpen);

    void setCaption(const wxString& new_value);
    void setRootFolder(const wxString& path, const wxString& root_label = wxEmptyString);
    void setPath(const wxString& new_value, bool select_text = false);
    void setAffirmativeButtonLabel(const wxString& label = wxEmptyString);
    void setOverwritePrompt(bool new_value);

    void onEnterPressed(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onPathChanged(wxCommandEvent& event);

    wxString getCaption();
    wxString getPath();

private:

    void onComboItemSelected();
    void onItemActivated(IFsItemPtr item);
    void onItemSelected(IFsItemPtr item);
    void onItemBeginLabelEdit(IFsItemPtr item, bool* allow);

private:

    DbDoc* m_dbdoc;
    DbComboCtrl* m_dbcombo;
    wxTextCtrl* m_name_ctrl;
    wxButton* m_ok_button;

    wxString m_caption;
    wxString m_affirmative_button_label;
    wxString m_folder;
    wxString m_filename;
    wxString m_root_folder;
    wxString m_root_label;
    int m_mode;
    bool m_overwrite_prompt;

    DECLARE_EVENT_TABLE()
};



#endif

