/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-13
 *
 */


#ifndef __APP_DBCOMBO_H
#define __APP_DBCOMBO_H



#include "dbdoc.h"


class DbComboPopup : public DbDoc,
                     public wxComboPopup
{
public:

    // -- wxComboPopup overrides --
    
    virtual void Init();
    virtual bool Create(wxWindow* parent);
    
    virtual wxWindow* GetControl();
    
    virtual void SetStringValue(const wxString& s);
    virtual wxString GetStringValue() const;
    
    void PaintComboControl(wxDC& dc, const wxRect& rect);
    
    // -- DbComboPopup functions --
    
    void setRootFolder(const wxString& path,
                       const wxString& label);
    void setShowFullPath(bool show_full_path = true,
                         bool show_root_label_in_path = true);
    wxString getPath() const;
    
public: // signals
    
    xcm::signal0 sigItemSelected;
    
private:
    
    void onItemActivated(IFsItemPtr item);
    
private:

    DbDoc* m_dbdoc;
    IFsItemPtr m_selected_item;
    wxString m_root_path;
    wxString m_root_label;
    wxString m_selected_path;
    wxString m_init_value;
    bool m_show_full_path;
    bool m_show_root_label_in_path;
};




class DbComboCtrl : public wxComboCtrl,
                    public xcm::signal_sink
{
public:

    DbComboCtrl(wxWindow* parent, wxWindowID id = wxID_ANY);
    
    // -- wxComboCtrl overrides --
    
    void SetValue(const wxString& value);

    // -- DbComboCtrl functions --
    
    void setPopupRootFolder(const wxString& path,
                            const wxString& label);
    void setShowFullPath(bool show_full_path = true,
                         bool show_root_label_in_path = true);
    void setPath(const wxString& new_path);
    wxString getPath();

public: // signals
    
    xcm::signal0 sigItemSelected;
  
private:

    void onItemSelected();
    
private:

    DbComboPopup* m_popup;
    wxString m_root_path;
    wxString m_root_label;
};




#endif  // __APP_DBCOMBO_H



