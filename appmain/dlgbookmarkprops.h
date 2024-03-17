/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-06
 *
 */


#ifndef H_APP_DLGLINKPROPS_H
#define H_APP_DLGLINKPROPS_H

class BookmarkFsComboCtrl;

class LinkPropsDialog : public wxDialog
{
public:

    enum
    {
        ModeCreate        = 0,
        ModeCreateFolder  = 1,
        ModeEdit          = 2,
        ModeEditNoDesc    = 3,
        ModeRename        = 4
    };
    
    LinkPropsDialog(wxWindow* parent,
                    const wxString& name = _("New Bookmark"),
                    const wxString& location  = wxEmptyString,
                    const wxString& tags = wxEmptyString,
                    const wxString& description = wxEmptyString);
    ~LinkPropsDialog();
    
    // wxDialog override
    int ShowModal();
    
    void setMode(int mode)                              { m_mode = mode;               }
    void setMessage(const wxString& message)            { m_message = message;         }
    
    void setFolder(const wxString& path)                { m_folder = path;             }
    void setName(const wxString& name)                  { m_name = name;               }
    void setLocation(const wxString& location)          { m_location = location;       }
    void setTags(const wxString& tags)                  { m_tags = tags;               }
    void setDescription(const wxString& description)    { m_description = description; }
    void setRunTarget(bool new_value)                   { m_run_target = new_value;    }

    wxString getPath();
    wxString getName();
    wxString getFolder() const       { return m_folder; }
    wxString getLocation() const     { return m_location;    }
    wxString getTags() const         { return m_tags;        }
    wxString getDescription() const  { return m_description; }
    bool     getRunTarget() const    { return m_run_target;  }
    
private:

    void updateRunTargetStatus();

    void onNameChanged(wxCommandEvent& evt);
    void onLocationChanged(wxCommandEvent& evt);
    void onTagsChanged(wxCommandEvent& evt);
    void onDescriptionChanged(wxCommandEvent& evt);
    void onRunTargetChanged(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);
    
private:

    wxTextCtrl*  m_name_textctrl;
    BookmarkFsComboCtrl* m_bookmarkfs_combo;
    wxTextCtrl*  m_location_textctrl;
    wxTextCtrl*  m_tags_textctrl;
    wxTextCtrl*  m_description_textctrl;
    wxCheckBox*  m_runtarget_checkbox;
    
    wxString m_folder;
    wxString m_name;
    wxString m_location;
    wxString m_tags;
    wxString m_description;
    bool     m_run_target;
    
    int m_mode;
    wxString m_message;
    
    DECLARE_EVENT_TABLE()
};


#endif  // __APP_DLGLINKPROPS_H

