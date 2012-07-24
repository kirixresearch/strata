/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-07-23
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/colordlg.h>
#include <xcm/xcm.h>
#include "button.h"
#include "colorpanel.h"
#include "popupcontainer.h"
#include "util.h"


namespace kcl
{


enum
{
    ID_FirstColor = wxID_HIGHEST+1,
    ID_MoreColorsButton = wxID_HIGHEST+500,
    ID_DefaultColorButton,
    ID_NoColorButton,
};


BEGIN_EVENT_TABLE(ColorPanel, wxPanel)
    EVT_SIZE(ColorPanel::onSize)
END_EVENT_TABLE()


const int BUTTON_SIZE = 18;
const int BORDER_PADDING = 3;
const int COLOR_SQUARE_SIZE = 14;


ColorPanel::ColorPanel(wxWindow* parent,
                       ColorPanelLayout layout,
                       long style) :
                           wxPanel(parent,
                                   wxID_ANY,
                                   wxPoint(0,0),
                                   wxSize(154,129),
                                   wxCLIP_CHILDREN | style)
{
    SetBackgroundColour(*wxWHITE);

    m_highlight_color = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    
    // initialize variabled
    m_main_sizer = NULL;
    m_grid_sizer = NULL;
    m_staticline = NULL;
    m_default_color_button = NULL;
    m_more_colors_button = NULL;
    m_no_color_button = NULL;
    
    // set our default number of columns
    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);
    
    // resize panel based on visible buttons
    if (layout.getDefaultColorButtonExists())
        cli_height += 23;
    if (layout.getNoColorButtonExists())
        cli_height += 23;
    SetClientSize(cli_width, cli_height);
    

    // -- create color grid --
    
    int cols = (cli_width-(BORDER_PADDING*2))/BUTTON_SIZE;
    m_grid_sizer = new wxGridSizer(cols);

    populateColorVector();

    int i = 0;
    std::vector<wxColour>::iterator it;
    for (it = m_colors.begin(); it != m_colors.end(); ++it)
    {
        kcl::Button* b = new kcl::Button(this,
                                         ID_FirstColor + (i++),
                                         wxDefaultPosition,
                                         wxSize(BUTTON_SIZE, BUTTON_SIZE),
                                         wxEmptyString);
                                          
        b->setMode(kcl::Button::modeFlat);
        b->setRightClickAllowed(true);
        b->sigCustomRender.connect(this, &ColorPanel::onButtonRender);
        b->sigButtonClicked.connect(this, &ColorPanel::onButtonClicked);
        m_grid_sizer->Add(b);
    }

    // create default color button
    if (layout.getDefaultColorButtonExists())
    {
        m_default_color_button = new kcl::Button(this,
                                                 ID_DefaultColorButton,
                                                 wxDefaultPosition,
                                                 wxSize(80,22),
                                                 _("Default Color"));
                                               
        m_default_color_button->setMode(kcl::Button::modeFlat);
        m_default_color_button->setRightClickAllowed(true);
        m_default_color_button->sigCustomRender.connect(this,
                                &ColorPanel::onButtonRender);
        m_default_color_button->sigButtonClicked.connect(this,
                                &ColorPanel::onDefaultColorClicked);
    }

    // create no fill color button
    if (layout.getNoColorButtonExists())
    {
        m_no_color_button = new kcl::Button(this,
                                            ID_NoColorButton,
                                            wxDefaultPosition,
                                            wxSize(80,22),
                                            _("No Color"));
                                          
        m_no_color_button->setMode(kcl::Button::modeFlat);
        m_no_color_button->setRightClickAllowed(true);
        m_no_color_button->sigCustomRender.connect(this,
                           &ColorPanel::onButtonRender);
        m_no_color_button->sigButtonClicked.connect(this,
                           &ColorPanel::onNoColorClicked);
    }

    // create staticline
    m_staticline = new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1));
    
    // create more colors button
    m_more_colors_button = new kcl::Button(this,
                                           ID_MoreColorsButton,
                                           wxDefaultPosition,
                                           wxSize(80,22),
                                           _("More..."));
                                          
    m_more_colors_button->setMode(kcl::Button::modeFlat);
    m_more_colors_button->setRightClickAllowed(true);
    m_more_colors_button->sigCustomRender.connect(this,
                          &ColorPanel::onButtonRender);
    m_more_colors_button->sigButtonClicked.connect(this,
                          &ColorPanel::onMoreColorsClicked);

    // create main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(2);
    
    std::vector<int>::iterator it2;
    for (it2 = layout.elements.begin(); it2 != layout.elements.end(); ++it2)
    {
        if (*it2 == ColorPanelLayout::DefaultColorButton)
        {
            m_main_sizer->AddSpacer(1);
            m_main_sizer->Add(m_default_color_button, 0,
                              wxEXPAND | wxLEFT | wxRIGHT, BORDER_PADDING);
            continue;
        }

        if (*it2 == ColorPanelLayout::ColorGrid)
        {
            m_main_sizer->AddSpacer(1);
            m_main_sizer->Add(m_grid_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, BORDER_PADDING);
            continue;
        }
        
        if (*it2 == ColorPanelLayout::NoColorButton)
        {
            m_main_sizer->AddSpacer(1);
            m_main_sizer->Add(m_no_color_button, 0,
                              wxEXPAND | wxLEFT | wxRIGHT, BORDER_PADDING);
            continue;
        }
    }
    
    m_main_sizer->AddSpacer(3);
    m_main_sizer->Add(m_staticline, 0, wxEXPAND);
    m_main_sizer->Add(m_more_colors_button, 0, wxEXPAND | wxALL, BORDER_PADDING);
    SetSizer(m_main_sizer);
    Layout();

    m_last_mode = ColorPanel::ModeFill;
    setMode(ColorPanel::ModeFill);
}

ColorPanel::~ColorPanel()
{
    sigDestructing(this);
}

void ColorPanel::onSize(wxSizeEvent& event)
{
    int cli_height, cli_width;
    GetClientSize(&cli_width, &cli_height);

    if (m_grid_sizer)
        m_grid_sizer->SetCols((cli_width-(BORDER_PADDING*2))/BUTTON_SIZE);
        
    event.Skip();
}

void ColorPanel::onButtonRender(kcl::Button* button,
                                wxDC* dc,
                                int flags)
{
    int xoff, yoff;
    int cli_width, cli_height;
    button->GetClientSize(&cli_width, &cli_height);

    if (flags & kcl::Button::buttonDepressed)
    {
        // draw selected background (darker blue)
        dc->SetPen(m_highlight_color);
        dc->SetBrush(kcl::stepColor(m_highlight_color, 140));
        dc->DrawRectangle(0, 0, cli_width, cli_height);
    }
     else
    {
        wxPoint pt_mouse = ::wxGetMousePosition();
        pt_mouse = ScreenToClient(pt_mouse);

        wxRect r(button->GetPosition(), button->GetClientSize());
        if (r.Contains(pt_mouse))
        {
            // draw hover background (lighter blue)
            dc->SetPen(m_highlight_color);
            dc->SetBrush(kcl::stepColor(m_highlight_color, 170));
            dc->DrawRectangle(0, 0, cli_width, cli_height);
        }
         else
        {
            // draw no background
            dc->SetPen(*wxWHITE);
            dc->SetBrush(*wxWHITE);
            dc->DrawRectangle(0, 0, cli_width, cli_height);
        }
    }
    
    wxFont text_font = wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false);
    
    int id = button->GetId();
    if (id == ID_DefaultColorButton)
    {
        int text_w, text_h;
        wxString text = m_default_color_button->getText();
            
        dc->SetTextForeground(*wxBLACK);
        dc->SetFont(text_font);
        dc->GetTextExtent(text, &text_w, &text_h);
        
        int padding = 6;
        xoff = (cli_width-COLOR_SQUARE_SIZE-padding-text_w)/2;
        yoff = (cli_height-COLOR_SQUARE_SIZE)/2;
        
        // draw the color square button
        dc->SetPen(*wxGREY_PEN);
        dc->SetBrush(wxBrush(m_default_color, wxSOLID));
        dc->DrawRectangle(xoff, yoff, COLOR_SQUARE_SIZE, COLOR_SQUARE_SIZE);
        
        // draw the button label
        xoff += (COLOR_SQUARE_SIZE+padding);
        yoff  = (cli_height-text_h)/2;
        dc->SetTextForeground(*wxBLACK);
        dc->DrawText(text, xoff, yoff);
    }
     else if (id == ID_NoColorButton)
    {
        int text_w, text_h;
        wxString text = m_no_color_button->getText();

        dc->SetTextForeground(*wxBLACK);
        dc->SetFont(text_font);
        dc->GetTextExtent(text, &text_w, &text_h);
        
        // draw the button label
        xoff = (cli_width-text_w)/2;
        yoff = (cli_height-text_h)/2;
        dc->DrawText(text, xoff, yoff);
    }
     else if (id == ID_MoreColorsButton)
    {
        int text_w, text_h;
        wxString text = m_more_colors_button->getText();

        dc->SetTextForeground(*wxBLACK);
        dc->SetFont(text_font);
        dc->GetTextExtent(text, &text_w, &text_h);
        
        // draw the button label
        xoff = (cli_width-text_w)/2;
        yoff = (cli_height-text_h)/2;
        dc->DrawText(text, xoff, yoff);
    }
     else
    {
        xoff = (cli_width-COLOR_SQUARE_SIZE)/2;
        yoff = (cli_height-COLOR_SQUARE_SIZE)/2;

        // draw the color square button
        dc->SetPen(*wxGREY_PEN);
        dc->SetBrush(wxBrush(m_colors[button->GetId()-ID_FirstColor], wxSOLID));
        dc->DrawRectangle(xoff, yoff, COLOR_SQUARE_SIZE, COLOR_SQUARE_SIZE);
    }
}

void ColorPanel::onButtonClicked(kcl::Button* button, int b)
{
    sigColorSelected(m_colors[button->GetId()-ID_FirstColor], m_mode);
}

void ColorPanel::onNoColorClicked(kcl::Button* button, int b)
{
    sigColorSelected(wxNullColour, m_mode);
}

void ColorPanel::onDefaultColorClicked(kcl::Button* button, int b)
{
    sigColorSelected(m_default_color, m_mode);
}

void ColorPanel::setMode(int mode)
{
    m_mode = mode;

    if (m_mode == ModeFill)
    {
        m_more_colors_button->setText(_("More Fill Colors..."));
        
        if (m_no_color_button != NULL)
            m_no_color_button->setText(_("No Fill Color"));
    }
     else if (m_mode == ModeLine)
    {
        m_more_colors_button->setText(_("More Line Colors..."));

        if (m_no_color_button != NULL)
            m_no_color_button->setText(_("No Line Color"));
    }
     else if (m_mode == ModeText)
    {
        m_more_colors_button->setText(_("More Text Colors..."));
    }
     else if (m_mode == ModeNone)
    {
        m_more_colors_button->setText(_("More Colors..."));
    }

    m_last_mode = m_mode;
}

void ColorPanel::setDefaultColor(const wxColor& color, const wxString& label)
{
    if (m_default_color_button)
    {
        if (!label.IsEmpty())
            m_default_color_button->setText(label);
        
        m_default_color = color;
    }
}

void ColorPanel::onMoreColorsClicked(kcl::Button* button, int b)
{
    kcl::PopupContainer* popup = NULL;
    
    if (GetParent()->IsKindOf(CLASSINFO(kcl::PopupContainer)))
    {
        popup = (kcl::PopupContainer*)GetParent();
        popup->Show(false);
        popup->setDestroyChild(false);
    }

    // fire a request for custom colors
    sigCustomColorsRequested(m_custom_colors);

    wxColourData init_cd;
    int i = 0;

    // fill out the dialog's custom colors from the m_custom_colors vector
    std::vector<wxColour>::iterator it;
    for (it = m_custom_colors.begin(); it != m_custom_colors.end(); ++it)
    {
        init_cd.SetCustomColour(i, *it);
        if (++i == 16)
            break;
    }

    init_cd.SetChooseFull(true);

    wxColourDialog dlg(wxTheApp->GetTopWindow(), &init_cd);
    
    if (m_mode == ModeFill)
        dlg.SetTitle(_("More Fill Colors"));
     else if (m_mode == ModeLine)
        dlg.SetTitle(_("More Line Colors"));
    if (m_mode == ModeText)
        dlg.SetTitle(_("More Text Colors"));
    if (m_mode == ModeNone)
        dlg.SetTitle(_("More Colors"));
        
    dlg.Centre();

    if (dlg.ShowModal() == wxID_OK)
    {
        wxColourData& cd = dlg.GetColourData();

        if (cd.GetColour().Ok())
        {
            // -- pass the dialog's custom colors
            //    back to the m_custom_colors vector --
            i = 0;
            for (it = m_custom_colors.begin();
                 it != m_custom_colors.end();
                 ++it)
            {
                *it = cd.GetCustomColour(i);
                if (++i == 16)
                    break;
            }

            sigColorSelected(cd.GetColour(), m_mode);
            sigCustomColorsChanged(m_custom_colors);
            sigCustomColorDialogOk();
        }
    }
    
    if (popup)
    {
        // -- we are responsible for deleting ourselves --
        popup->Destroy();
    }
}

void ColorPanel::setCustomColors(const std::vector<wxColour>& custom_colors)
{
    m_custom_colors = custom_colors;
}

void ColorPanel::getCustomColors(std::vector<wxColour>& custom_colors)
{
    custom_colors = m_custom_colors;
}

void ColorPanel::populateColorVector()
{
    if (::wxDisplayDepth() < 16)
    {
        m_colors.push_back(wxColour(0,0,0));
        m_colors.push_back(wxColour(128,0,0));
        m_colors.push_back(wxColour(0,128,0));
        m_colors.push_back(wxColour(0,0,128));
        m_colors.push_back(wxColour(0,128,128));
        m_colors.push_back(wxColour(128,0,128));
        m_colors.push_back(wxColour(128,128,0));
        m_colors.push_back(wxColour(128,128,128));

        m_colors.push_back(wxColour(255,255,255));
        m_colors.push_back(wxColour(255,0,0));
        m_colors.push_back(wxColour(0,255,0));
        m_colors.push_back(wxColour(0,0,255));
        m_colors.push_back(wxColour(0,255,255));
        m_colors.push_back(wxColour(255,0,255));
        m_colors.push_back(wxColour(255,255,0));
        m_colors.push_back(wxColour(0,0,0));

        m_colors.push_back(wxColour(58,110,165));
        m_colors.push_back(wxColour(192,220,192));
        m_colors.push_back(wxColour(0,192,192));
        m_colors.push_back(wxColour(255,255,255));
        m_colors.push_back(wxColour(0,0,0));
        m_colors.push_back(wxColour(128,128,128));
        m_colors.push_back(wxColour(192,192,192));
        m_colors.push_back(wxColour(255,255,255));
    }
     else
    {
        m_colors.push_back(wxColour(0,0,0));
        m_colors.push_back(wxColour(255,180,0));
        m_colors.push_back(wxColour(0,102,0));
        m_colors.push_back(wxColour(0,51,153));
        m_colors.push_back(wxColour(51,0,102));
        m_colors.push_back(wxColour(102,0,102));
        m_colors.push_back(wxColour(153,0,0));
        m_colors.push_back(wxColour(102,51,0));

        m_colors.push_back(wxColour(153,153,153));
        m_colors.push_back(wxColour(255,222,0));
        m_colors.push_back(wxColour(0,180,51));
        m_colors.push_back(wxColour(0,0,255));
        m_colors.push_back(wxColour(102,0,180));
        m_colors.push_back(wxColour(204,0,204));
        m_colors.push_back(wxColour(204,0,0));
        m_colors.push_back(wxColour(204,102,0));

        m_colors.push_back(wxColour(204,204,204));
        m_colors.push_back(wxColour(255,255,0));
        m_colors.push_back(wxColour(0,255,51));
        m_colors.push_back(wxColour(0,102,255));
        m_colors.push_back(wxColour(153,0,255));
        m_colors.push_back(wxColour(255,0,255));
        m_colors.push_back(wxColour(255,0,0));
        m_colors.push_back(wxColour(255,153,0));

        m_colors.push_back(wxColour(248,248,248));
        m_colors.push_back(wxColour(255,255,178));
        m_colors.push_back(wxColour(178,255,193));
        m_colors.push_back(wxColour(178,209,255));
        m_colors.push_back(wxColour(224,178,255));
        m_colors.push_back(wxColour(255,178,255));
        m_colors.push_back(wxColour(255,178,178));
        m_colors.push_back(wxColour(255,224,178));

        m_colors.push_back(wxColour(255,255,255));
        m_colors.push_back(wxColour(255,255,240));
        m_colors.push_back(wxColour(240,255,243));
        m_colors.push_back(wxColour(240,246,255));
        m_colors.push_back(wxColour(249,240,255));
        m_colors.push_back(wxColour(255,240,255));
        m_colors.push_back(wxColour(255,240,240));
        m_colors.push_back(wxColour(255,249,240));
    }
}




};




