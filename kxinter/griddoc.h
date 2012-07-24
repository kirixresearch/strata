/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-12-14
 *
 */


#ifndef __KXINTER_GRIDDOC_H
#define __KXINTER_GRIDDOC_H


xcm_interface IGridDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("kxinter.IGridDoc")

public:

    virtual tango::ISetPtr getSet() = 0;
};


XCM_DECLARE_SMARTPTR(IGridDoc)




class GridDoc : public wxWindow,
                public cfw::IDocument,
                public IGridDoc
{

XCM_CLASS_NAME_NOREFCOUNT("kxinter.GridDoc")
XCM_BEGIN_INTERFACE_MAP(GridDoc)
    XCM_INTERFACE_ENTRY(cfw::IDocument)
    XCM_INTERFACE_ENTRY(IGridDoc)
XCM_END_INTERFACE_MAP()



public:
    GridDoc();
    ~GridDoc();

    void onSize(wxSizeEvent& event);

    void setName(const wxString& name);
    bool setIterator(tango::IIteratorPtr);

    // -- IDocument interface implementation --

    virtual bool initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr site, wxWindow* docsite_wnd, wxWindow* panesite_wnd);
    virtual wxWindow* getDocumentWindow();
    virtual void setDocumentFocus();
    bool onSiteClosing(bool force);

    // -- IGridDoc interface implementation --
    tango::ISetPtr getSet();

    // -- menu event handlers --
    void onSetOrderAscending(wxCommandEvent& event);
    void onSetOrderDescending(wxCommandEvent& event);

    // -- grid event handlers --
    void onGridColumnRightClick(kcl::GridEvent& event);


private:
    kcl::Grid* m_grid;
    cfw::IDocumentSitePtr m_doc_site;
    tango::ISetPtr m_set;
    tango::IIteratorPtr m_iter;


    DECLARE_EVENT_TABLE()
};




#endif



