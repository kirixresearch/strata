/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-04
 *
 */


#ifndef __APP_PANELFILEINFO_H
#define __APP_PANELFILEINFO_H


#include <set>


class ProjectFileInfo
{
public:

    enum
    {
        typeFolder = 0,
        typeTable,
        typeReport,
        typeQuery,
        typeScript,
        typeOther
    };
    
    ProjectFileInfo()
    {
        wxString name = wxEmptyString;
        int type = typeOther;
        size = -2;
        records = -2;
    }

public:

    wxString name;
    int type;
    tango::tango_int64_t size;
    tango::tango_int64_t records;
};


class MultiFileInfoPanel : public wxPanel,
                           public cfw::IDocument,
                           public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.MultiFileInfoPanel")
    XCM_BEGIN_INTERFACE_MAP(MultiFileInfoPanel)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
    XCM_END_INTERFACE_MAP()

public:

    MultiFileInfoPanel();
    ~MultiFileInfoPanel();

    void addFile(const wxString& path);

private:

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    void populate();
    
    void onSave(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onGridColumnLeftClick(kcl::GridEvent& event);

private:

    cfw::IDocumentSitePtr m_doc_site;
    
    wxStaticText* m_folder_text;
    wxStaticText* m_table_text;
    wxStaticText* m_report_text;
    wxStaticText* m_query_text;
    wxStaticText* m_script_text;
    kcl::Grid* m_grid;
    
    bool m_sort_ascending;
    int m_sort_column;

    std::vector<ProjectFileInfo> m_info;
    std::set<wxString> m_inserted_items;

    DECLARE_EVENT_TABLE()
};






#endif

