/*!
 *
 * Copyright (c) 2024, fopen GmbH.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2024-03-14
 *
 */


#ifndef H_APP_BOOKMARKFSCOMBO_H
#define H_APP_BOOKMARKFSCOMBO_H



class BookmarkFsComboCtrlPopup;

class BookmarkFsComboCtrl : public wxComboCtrl
{
public:
    BookmarkFsComboCtrl() { m_path = "/"; }

    BookmarkFsComboCtrl(wxWindow* parent,
                        wxWindowID id = wxID_ANY,
                        const wxString& value = wxEmptyString,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = 0,
                        const wxValidator& validator = wxDefaultValidator,
                        const wxString& name = wxComboBoxNameStr);

    bool Create(wxWindow* parent,
                wxWindowID id = wxID_ANY,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr);

    void setBookmarkFs(IBookmarkFsPtr bookmark_fs);

    wxString getPath() const { return m_path; }
    void setPath(const wxString& path) { m_path = path; }

protected:
    virtual void OnButtonClick() override;

private:
    friend class BookmarkFsComboCtrlPopup;

    IBookmarkFsPtr m_bookmark_fs;
    wxString m_path;
};

class BookmarkFsComboCtrlPopup : public wxComboPopup
{
public:

    BookmarkFsComboCtrlPopup();
    virtual ~BookmarkFsComboCtrlPopup();

    virtual void Init() override;
    virtual bool Create(wxWindow* parent) override;
    virtual wxWindow* GetControl() override;
    virtual void SetStringValue(const wxString& value) override;
    virtual wxString GetStringValue() const override;

    void populateTree(IBookmarkFsPtr bookmark_fs);

    void onTreeSelectionChanged(wxTreeEvent& event);

protected:
    wxTreeCtrl* m_treectrl;
    wxString m_selected_value;

    void OnTreeItemActivated(wxTreeEvent& event);
};



#endif  // H_APP_BOOKMARKFSCOMBO_H



