/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-04-26
 *
 */


#ifndef __KXINTER_PANELCONSOLE_H
#define __KXINTER_PANELCONSOLE_H


#include "kxinter.h"


xcm_interface IConsolePanel;
XCM_DECLARE_SMARTPTR(IConsolePanel)


xcm_interface IConsolePanel : public xcm::IObject
{
    XCM_INTERFACE_NAME("kxinter.IConsolePanel")


public:

    virtual void refresh() = 0;
};




class CommandTextCtrl;

class ConsolePanel : public wxWindow,
                     public IConsolePanel,
                     public cfw::IDocument,
                     public xcm::signal_sink
{

XCM_CLASS_NAME_NOREFCOUNT("kxinter.ConsolePanel")
XCM_BEGIN_INTERFACE_MAP(ConsolePanel)
    XCM_INTERFACE_ENTRY(cfw::IDocument)
    XCM_INTERFACE_ENTRY(IConsolePanel)
XCM_END_INTERFACE_MAP()


private:
    cfw::IFramePtr m_frame;
    CommandTextCtrl* m_text;

public:
    ConsolePanel();
    ~ConsolePanel();

    void doSizing();

    // -- event handlers --
    void onSize(wxSizeEvent& event);

    // -- IDocument interface implementation --
    bool initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr site, wxWindow* docsite_wnd, wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // -- IConsolePanel interface implementation --
    void refresh();


private:
    DECLARE_EVENT_TABLE()
};




#endif



