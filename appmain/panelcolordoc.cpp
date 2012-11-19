/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-26
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "panelcolordoc.h"
#include "tabledoc.h"


ColorDocPanel::ColorDocPanel(const wxString& default_color_button_text)
{
    m_default_color_text = default_color_button_text;
}

ColorDocPanel::~ColorDocPanel()
{
}

// -- IDocument --
bool ColorDocPanel::initDoc(IFramePtr frame,
                            IDocumentSitePtr site,
                            wxWindow* doc_site,
                            wxWindow* pane_site)
{
    if (!Create(doc_site,
                -1,
                wxPoint(0,0),
                doc_site->GetClientSize(),
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
    {
        return false;
    }

    m_panel = new kcl::ColorPanel(
                      this,
                      kcl::ColorPanelLayout(kcl::ColorPanelLayout::ColorGrid),
                      wxNO_BORDER);
    m_panel->setDefaultColor(*wxBLACK, m_default_color_text);
    m_panel->sigColorSelected.connect(this, &ColorDocPanel::onColorSelected);
    m_panel->sigCustomColorsRequested.connect(g_app->getAppController(),
                                  &AppController::onCustomColorsRequested);
    m_panel->sigCustomColorsChanged.connect(g_app->getAppController(),
                                  &AppController::onCustomColorsChanged);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_panel, 1, wxEXPAND);

    wxSize site_size = site->getContainerWindow()->GetSize();
    site->setMinSize(site_size.x, site_size.y);

    SetSizer(sizer);
    Layout();

    return true;
}

wxWindow* ColorDocPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void ColorDocPanel::setDocumentFocus()
{
}

void ColorDocPanel::onColorSelected(wxColor color, int button)
{
    sigColorSelected(color, button);
}




