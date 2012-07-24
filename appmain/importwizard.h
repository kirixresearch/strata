/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-02-19
 *
 */


#ifndef __APP_IMPORTWIZARD_H
#define __APP_IMPORTWIZARD_H


#include "connectionwizard.h"
#include "importtemplate.h"


// forward declarations
class PathSelectionPage;
class DataSourceSelectionPage;
class ServerPropertiesPage;
class ImportTableSelectionPage;
class ImportDelimitedTextSettingsPage;
class SavePage;


class ImportWizard : public wxWindow,
                     public cfw::IDocument,
                     public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ImportWizard")
    XCM_BEGIN_INTERFACE_MAP(ImportWizard)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
    XCM_END_INTERFACE_MAP()

public:
    
    enum ImportWizardMode
    {
        ModeImport = 0,     // default
        ModeOpen = 1        // the import wizard is being shown in the
                            // context of a File->Open action
    };
    
public:

    ImportWizard();
    ~ImportWizard();

public:

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    ImportTemplate& getTemplate();

    bool loadTemplate(const wxString& path);
    bool saveTemplate(const wxString& path);
    
    void setTitle(const wxString& title) { m_title = title; }
    void setMode(int mode) { m_mode = mode; }

private:
    
    void updatePageRouting();
    
    // wizard page's connected signals
    void onPathSelectionPageChanging(bool forward, bool* allow);
    void onDataSourceSelectionPageChanging(bool forward, bool* allow);
    void onFileTypeChanged(int file_type);

    // wizard's connected signals
    void onImportTypeChanged(int type);
    void onWizardCancelled(kcl::Wizard* wizard);
    void onWizardFinished(kcl::Wizard* wizard);

    // event handlers
    void onClose(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);

    void onSave(wxCommandEvent& evt);
    
public: // signals

    xcm::signal1<ImportWizard*> sigImportWizardFinished;

private:

    cfw::IFramePtr m_frame;
    cfw::IDocumentSitePtr m_doc_site;
    
    wxBoxSizer* m_sizer;
    kcl::Wizard* m_wizard;
    ConnectionBar* m_connection_bar;

    PathSelectionPage* m_path_selection_page;
    DataSourceSelectionPage* m_datasource_selection_page;
    ServerPropertiesPage* m_server_properties_page;
    ImportTableSelectionPage* m_table_selection_page;
    ImportDelimitedTextSettingsPage* m_delimitedtext_settings_page;
    
    ImportTemplate m_template;
    std::vector<int> m_problem_rows;
    wxString m_title;
    int m_mode;
    
    DECLARE_EVENT_TABLE()
};


#endif

