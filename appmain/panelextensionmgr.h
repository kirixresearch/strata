/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-11
 *
 */


#ifndef H_APP_PANELEXTENSIONMGR_H
#define H_APP_PANELEXTENSIONMGR_H


class ExtensionInfo;
class ExtensionManagerPanel : public wxPanel,
                              public IDocument,
                              public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ExtensionManagerPanel")
    XCM_BEGIN_INTERFACE_MAP(ExtensionManagerPanel)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:
    ExtensionManagerPanel();
    ~ExtensionManagerPanel();

public:

    // IDocument
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    
    bool selectExtension(const wxString& guid);
    
private:

    void addItem(ExtensionInfo& info);
    void updateItem(kcl::ScrollListItem* item,
                    ExtensionInfo& info);
    void checkOverlayText();
    void populate();
    
    // events
    void onAddExtensionsButtonClicked(wxCommandEvent& evt);
    void onStartNowButtonClicked(wxCommandEvent& evt);
    void onCancelUninstallButtonClicked(wxCommandEvent& evt);
    void onUninstallButtonClicked(wxCommandEvent& evt);
    void onRunAtStartupChecked(wxCommandEvent& evt);
    
    // signal handlers
    void onSelectedItemChanged(kcl::ScrollListItem* old_item,
                               kcl::ScrollListItem* new_item);
    void onExtensionInstalled(ExtensionInfo& info);
    void onExtensionStarted(ExtensionInfo& info);
    
private:

    IDocumentSitePtr m_doc_site;
    
    kcl::ScrollListControl* m_extension_list;
    wxButton* m_addextensions_button;

    std::map<wxObject*, kcl::ScrollListItem*> m_control_map;
    
    DECLARE_EVENT_TABLE()
};


#endif


