/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-10-08
 *
 */


#ifndef __APP_PANELFIND_H
#define __APP_PANELFIND_H

// interface

xcm_interface IFindPanel;
XCM_DECLARE_SMARTPTR(IFindPanel)


xcm_interface IFindPanel : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IFindPanel")

public:

    virtual void setMode(int mode, bool layout = false) = 0;
};


class FindPanel : public wxPanel,
                  public cfw::IDocument,
                  public IFindPanel,
                  public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.FindPanel")
    XCM_BEGIN_INTERFACE_MAP(FindPanel)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
        XCM_INTERFACE_ENTRY(IFindPanel)
    XCM_END_INTERFACE_MAP()

public:

    // indexes here correspond to the contents
    // of the mode choice control
    enum
    {
        ModeFindInFiles = 0,
        ModeFind = 1,
        ModeFindReplace = 2
    };
    
public:

    FindPanel();
    void setMode(int mode, bool layout = false);
    void setFindValue(const wxString& val);
    
private:

    ~FindPanel();

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    
    void addReplaceComboItem();
    void savePrefs();

    void onKeyDown(wxKeyEvent& evt);
    void onModeChanged(wxCommandEvent& evt);
    void onFindPrevNext(wxCommandEvent& evt);
    void onFindAll(wxCommandEvent& evt);
    void onFindPrefsChanged(wxCommandEvent& evt);
    void onFindInBrowse(wxCommandEvent& evt);
    void onReplace(wxCommandEvent& evt);
    void onReplaceAll(wxCommandEvent& evt);
    void onModeCombo(wxCommandEvent& evt);
    void onComboChanged(wxCommandEvent& evt);
    void onComboEnter(wxCommandEvent& evt);
    void onComboTabPressed(wxKeyEvent& evt);
    void onFrameEvent(cfw::Event& evt);

private:

    cfw::IDocumentSitePtr m_doc_site;
    
    wxBoxSizer* m_find_mode_sizer;
    wxBoxSizer* m_find_sizer;
    wxBoxSizer* m_find_in_sizer;
    wxBoxSizer* m_replace_sizer;
    wxBoxSizer* m_button_sizer;
    wxBoxSizer* m_main_sizer;
    
    wxStaticText* m_find_mode_label;
    wxStaticText* m_find_label;
    wxStaticText* m_find_in_label;
    wxStaticText* m_replace_label;
    
    wxChoice* m_find_mode_combo;
    wxComboBox* m_find_combo;
    wxComboBox* m_find_in_combo;
    wxComboBox* m_replace_combo;
    wxCheckBox* m_matchcase_checkbox;
    wxCheckBox* m_wholecell_checkbox;
    wxCheckBox* m_allopen_checkbox;
    
    wxButton* m_findprev_button;
    wxButton* m_findnext_button;
    wxButton* m_findall_button;
    wxButton* m_replace_button;
    wxButton* m_replaceall_button;
    
    wxString m_caption;
    int m_mode;
    
    DECLARE_EVENT_TABLE()
};



// this typedef is a workaround for the C2352 problem in VC++ 6.0
typedef kcl::BitmapComboPopup FindComboPopupBase;

class FindComboPopup : public kcl::BitmapComboPopup
{
    void Init()
    {
        FindComboPopupBase::Init();
        
        // get rid of the bitmaps in the popup list
        setOptions(0);
    }

    bool Create(wxWindow* parent)
    {
        return FindComboPopupBase::Create(parent);
    }
};


class FindComboControl : public kcl::BitmapComboControl
{
public:

    FindComboControl(wxWindow* parent,
                     wxWindowID id,
                     kcl::BitmapComboPopup* popup = NULL);
    
    ~FindComboControl();

protected:

    void onFind(wxCommandEvent& evt);
   
private:

    bool m_is_empty;
    
    DECLARE_EVENT_TABLE()
};




#endif


