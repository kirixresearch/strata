/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2005-11-29
 *
 */


//#define USE_PROPERTY_GRID
#ifdef USE_PROPERTY_GRID


#ifndef __CFW_PROPERTIESPANEL_H
#define __CFW_PROPERTIESPANEL_H


#include <xcm/xcm.h>
#include "framework.h"


class wxPropertyGrid;
class wxPropertyGridEvent;


namespace cfw
{

class PropertiesPanel : public wxPanel,
                        public cfw::IDocument,
                        public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("cfw.PropertiesPanel")
    XCM_BEGIN_INTERFACE_MAP(PropertiesPanel)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
    XCM_END_INTERFACE_MAP()

public:

    PropertiesPanel();
    ~PropertiesPanel();

private:

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);

    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // -- frame events --
    void onActiveChildChanged(cfw::IDocumentSitePtr doc_site);
    void onFrameEvent(cfw::Event& evt);
    
private:

    void populate();
    void onPropertyChange(wxPropertyGridEvent& evt);
    
private:

    wxPropertyGrid* m_pg;
    cfw::IFramePtr m_frame;
    wxString m_last_string;
  
    DECLARE_EVENT_TABLE()
};

#endif


}; // namespace cfw

#endif

