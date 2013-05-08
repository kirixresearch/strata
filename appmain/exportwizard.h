/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-08-14
 *
 */


#ifndef __APP_EXPORTWIZARD_H
#define __APP_EXPORTWIZARD_H


#include "connectionwizard.h"
#include "exporttemplate.h"




    
    
// forward declarations
class PathSelectionPage;
class DataSourceSelectionPage;
class ServerPropertiesPage;
class ExportTableSelectionPage;
class ExportDelimitedTextSettingsPage;
class SavePage;


class ExportWizard : public wxWindow,
                     public IDocument,
                     public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ExportWizard")
    XCM_BEGIN_INTERFACE_MAP(ExportWizard)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    ExportWizard();
    ~ExportWizard();

public:

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
                 
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    ExportTemplate& getTemplate();

    bool loadTemplate(const wxString& path);
    bool saveTemplate(const wxString& path);

private:

    // wizard page's connected signals
    void onPathSelectionPageChanging(bool forward, bool* allow);
    void onDataSourceSelectionPageChanging(bool forward, bool* allow);
    void onFileTypeChanged(int file_type);

    // wizard's connected signals
    void onExportTypeChanged(int type);
    void onWizardCancelled(kcl::Wizard* wizard);
    void onWizardFinished(kcl::Wizard* wizard);

    // event handlers
    void onClose(wxCommandEvent& evt);
    void onSize(wxSizeEvent& event);

private:

    void setDefaultExportPath(const wxString& path);
    wxString getDefaultExportPath();

public: // signals

    xcm::signal1<ExportWizard*> sigExportWizardFinished;

private:

    IFramePtr m_frame;
    IDocumentSitePtr m_doc_site;
    
    ExportTemplate m_template;
    
    wxBoxSizer* m_sizer;
    kcl::Wizard* m_wizard;
    ConnectionBar* m_connection_bar;

    PathSelectionPage* m_path_selection_page;
    DataSourceSelectionPage* m_datasource_selection_page;
    ServerPropertiesPage* m_server_properties_page;
    ExportTableSelectionPage* m_table_selection_page;
    ExportDelimitedTextSettingsPage* m_delimitedtext_settings_page;

    DECLARE_EVENT_TABLE()
};


#endif

