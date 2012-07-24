/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-08-27
 *
 */


#ifndef __KXINTER_STRUCTWINDOW_H
#define __KXINTER_STRUCTWINDOW_H


#include <list>


struct StructField
{
    wxString name;
    int type;
    int width;
    int scale;
    int pos;
    int dynamic;
    bool dyn_state; // for the grid

    StructField()
    {
        name = wxT("");
        type = -1;
        width = -1;
        scale = -1;
        pos = -1;
        dynamic = -1;
        dyn_state = false;
    }

    StructField(const StructField& c)
    {
        name = c.name;
        type = c.type;
        width = c.width;
        scale = c.scale;
        pos = c.pos;
        dynamic = c.dynamic;
        dyn_state = c.dyn_state;
    }

    StructField& operator=(const StructField& c)
    {
        name = c.name;
        type = c.type;
        width = c.width;
        scale = c.scale;
        pos = c.pos;
        dynamic = c.dynamic;
        dyn_state = c.dyn_state;
        return *this;
    }
};


struct StructAction
{
    enum
    {
        undefined = -1,
        addField = 0,
        deleteField = 1,
        moveField = 2,
        modifyField = 3
    };

    int action;

    StructField old_field;
    StructField new_field;

    StructAction()
    {
        action = -1;
    }
};



// -- StructWindow class declaration --

class StructWindow : public wxWindow,
                     public cfw::IDocument
{
    XCM_CLASS_NAME_NOREFCOUNT("kxinter.StructWindow")
    XCM_BEGIN_INTERFACE_MAP(StructWindow)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
    XCM_END_INTERFACE_MAP()

private:
    bool addField(const wxString& name, int type, int width, int scale, int pos);
    bool modifyField(const wxString& oldname, const wxString& name = wxEmptyString, int type = -1, int width = -1, int scale = -1, int pos = -1, int dynamic = -1);
    bool insertField(int pos);
    bool deleteField(int pos);
    bool moveField(int pos);

    bool modifyTable(cfw::IDocumentSitePtr target_site);
    bool createTable();
    void populate();
    bool markDuplicateFields();
    bool validateStructure();
    void setRowCellProps(int row);

    void onGridCursorMove(kcl::GridEvent& event);
    void onGridEndEdit(kcl::GridEvent& event);
    void onGridEditChange(kcl::GridEvent& event);

    void onExecute(wxCommandEvent& event);
    void onInsertField(wxCommandEvent& event);
    void onDeleteField(wxCommandEvent& event);
    
    void updateStatusBar();

    // -- utility functions --
    void outputFieldList();
    void outputActionList();

public:
    
    StructWindow();
    virtual ~StructWindow();

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow() { return static_cast<wxWindow*>(this); }
    void setDocumentFocus();
    bool onSiteClosing(bool force);

    // -- StructWindow methods --
    void setModifySet(tango::ISetPtr modify_set);

    // -- event handlers --
    void onSize(wxSizeEvent& event);

    DECLARE_EVENT_TABLE()


private:
    kcl::Grid* m_grid;
    std::list<StructField> m_fields;
    std::list<StructAction> m_actions;
    wxString m_set_name;
    int m_new_field_counter;
    bool m_modify;
    cfw::IDocumentSitePtr m_doc_site;

    tango::ISetPtr m_modify_set;
    wxStatusBar* m_status_bar;
    kcl::Button* m_execute_button;

};






#endif


