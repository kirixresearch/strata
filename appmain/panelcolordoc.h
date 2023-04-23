/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-26
 *
 */


#ifndef H_APP_PANELCOLORDOC_H
#define H_APP_PANELCOLORDOC_H


class ColorDocPanel : public wxPanel,
                      public IDocument,
                      public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ColorDocPanel")
    XCM_BEGIN_INTERFACE_MAP(ColorDocPanel)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    ColorDocPanel(const wxString& default_color_button_text = wxEmptyString);
    ~ColorDocPanel();

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    void onColorSelected(wxColor color, int button);

public: // signals

    xcm::signal2<wxColour, int> sigColorSelected;

private:

    kcl::ColorPanel* m_panel;
    wxString m_default_color_text;
};




#endif

