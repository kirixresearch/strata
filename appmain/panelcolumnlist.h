/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-21
 *
 */


#ifndef __APP_PANELCOLUMNLIST_H
#define __APP_PANELCOLUMNLIST_H


class ColumnListItem
{
public:
    wxString text;
    wxBitmap bitmap;
    bool active;
};


xcm_interface IColumnListTarget : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IColumnListTarget")
public:

    virtual void getColumnListItems(std::vector<ColumnListItem>& items) = 0;
    virtual void onColumnListDblClicked(const std::vector<wxString>& items) { }
};
XCM_DECLARE_SMARTPTR(IColumnListTarget);



xcm_interface IColumnListPanel : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IColumnListPanel")
public:

    virtual void populate() = 0;
};
XCM_DECLARE_SMARTPTR(IColumnListPanel);



class ColumnListPanel : public wxPanel,
                        public cfw::IDocument,
                        public IColumnListPanel,
                        public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ColumnListPanel")
    XCM_BEGIN_INTERFACE_MAP(ColumnListPanel)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
        XCM_INTERFACE_ENTRY(IColumnListPanel)
    XCM_END_INTERFACE_MAP()

    enum
    {
        sortNone = 0,
        sortAscending = 1,
        sortDescending = 2
    };

public:

    ColumnListPanel();
    ~ColumnListPanel();

private:

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // -- panel methods --
    void populate();
    
    // event handlers
    void onGridCellDblClick(kcl::GridEvent& evt);
    void onGridColumnClick(kcl::GridEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onSetFocus(wxFocusEvent& evt);
    void onKillFocus(wxFocusEvent& evt);

private:

    // -- frame events --
    void onActiveChildChanged(cfw::IDocumentSitePtr doc_site);
    void onFrameEvent(cfw::Event& evt);
    bool onSiteClosing(bool force);

private:

    kcl::RowSelectionGrid* m_grid;
    int m_sort_state;
    int m_site_id;
    int m_last_target_site_id;

    DECLARE_EVENT_TABLE()
};

#endif
