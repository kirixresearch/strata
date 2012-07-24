/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-19
 *
 */


#ifndef __KXINTER_PANELDATAFORMAT_H
#define __KXINTER_PANELDATAFORMAT_H


xcm_interface IDataFormatPanel : public xcm::IObject
{
    XCM_INTERFACE_NAME("kxinter.IDataFormatPanel")

public:

    virtual void refresh() = 0;
    virtual void setActivePage(int page) = 0;
    virtual void editFieldName(int field) = 0;
};


XCM_DECLARE_SMARTPTR(IDataFormatPanel)



class DataFormatDefinition;

class DataFormatPanel : public wxWindow,
                        public cfw::IDocument,
                        public IDataFormatPanel,
                        public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("kxinter.DataFormatPanel")
    XCM_BEGIN_INTERFACE_MAP(DataFormatPanel)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
        XCM_INTERFACE_ENTRY(IDataFormatPanel)
    XCM_END_INTERFACE_MAP()


public:
    
    DataFormatPanel();
    virtual ~DataFormatPanel();

    void setDataFormat(DataFormatDefinition* dfd);
    void refresh();
    void setActivePage(int page);
    void editFieldName(int field);

private:

    void onDataFormatEditor(wxCommandEvent& event);
    void onSize(wxSizeEvent& event);
    void onExtraPanelCommand(int command_type, int command_id);

private:

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    bool onSiteClosing(bool force);

private:

    kcl::NotebookControl* m_notebook;
    DataFormatDefinition* m_dfd;

    DECLARE_EVENT_TABLE()
};




#endif

