/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams; Aaron L. Williams
 * Created:  2009-12-11
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
#include "borderpanel.h"
#include "popupcontainer.h"
#include "util.h"


namespace kcl
{


/* XPM */
static const char* icon_border_bottom_16[] = {
"16 16 4 1",
"  c #000000",
"! c #808080",
"# c #A0A0A4",
"$ c #FFFFFF",
"$$$$$$$$$$$$$$$$",
"!$!$!$!$!$!$!$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$#$#$#$#$#$#$#$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"               $"};

/* XPM */
static const char* icon_border_bottom_dbl_16[] = {
"16 16 4 1",
"  c #000000",
"! c #808080",
"# c #A0A0A4",
"$ c #FFFFFF",
"$$$$$$$$$$$$$$$$",
"!$!$!$!$!$!$!$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$#$#$#$#$#$#$#$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"               $",
"!$$$$$$$$$$$$$!$",
"               $"};

/* XPM */
static const char* icon_border_bottom_dbl_t_16[] = {
"16 16 4 1",
"  c #000000",
"! c #808080",
"# c #A0A0A4",
"$ c #FFFFFF",
"$$$$$$$$$$$$$$$$",
"               $",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$#$#$#$#$#$#$#$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"               $",
"!$$$$$$$$$$$$$!$",
"               $"};

/* XPM */
static const char* icon_border_bottom_thick_16[] = {
"16 16 4 1",
"  c #000000",
"! c #808080",
"# c #A0A0A4",
"$ c #FFFFFF",
"$$$$$$$$$$$$$$$$",
"!$!$!$!$!$!$!$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$#$#$#$#$#$#$#$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"               $",
"               $"};

/* XPM */
static const char* icon_border_bottom_tk_t_16[] = {
"16 16 4 1",
"  c #000000",
"! c #808080",
"# c #A0A0A4",
"$ c #FFFFFF",
"$$$$$$$$$$$$$$$$",
"               $",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$#$#$#$#$#$#$#$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"               $",
"               $"};

/* XPM */
static const char* icon_border_bottom_top_16[] = {
"16 16 4 1",
"  c #000000",
"! c #808080",
"# c #A0A0A4",
"$ c #FFFFFF",
"$$$$$$$$$$$$$$$$",
"               $",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$#$#$#$#$#$#$#$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"!$$$$$$$$$$$$$!$",
"$$$$$$$#$$$$$$$$",
"               $"};

/* XPM */
static const char* icon_border_left_16[] = {
"16 16 4 1",
"  c #000000",
"! c #808080",
"# c #A0A0A4",
"$ c #FFFFFF",
"$$$$$$$$$$$$$$$$",
" $!$!$!$!$!$!$!$",
" $$$$$$#$$$$$$$$",
" $$$$$$$$$$$$$!$",
" $$$$$$#$$$$$$$$",
" $$$$$$$$$$$$$!$",
" $$$$$$#$$$$$$$$",
" $$$$$$$$$$$$$!$",
" #$#$#$#$#$#$#$$",
" $$$$$$$$$$$$$!$",
" $$$$$$#$$$$$$$$",
" $$$$$$$$$$$$$!$",
" $$$$$$#$$$$$$$$",
" $$$$$$$$$$$$$!$",
" $$$$$$#$$$$$$$$",
" $!$!$!$!$!$!$!$"};

/* XPM */
static const char* icon_border_noborder_16[] = {
"16 16 3 1",
"  c #808080",
"! c #A0A0A4",
"# c #FFFFFF",
"################",
" # # # # # # # #",
"#######!########",
" ############# #",
"#######!########",
" ############# #",
"#######!########",
" ############# #",
"#!#!#!#!#!#!#!##",
" ############# #",
"#######!########",
" ############# #",
"#######!########",
" ############# #",
"#######!########",
" # # # # # # # #"};

/* XPM */
static const char* icon_border_outside_16[] = {
"16 16 3 1",
"  c #000000",
"! c #A0A0A4",
"# c #FFFFFF",
"################",
"               #",
" ######!###### #",
" ############# #",
" ######!###### #",
" ############# #",
" ######!###### #",
" ############# #",
" !#!#!#!#!#!#! #",
" ############# #",
" ######!###### #",
" ############# #",
" ######!###### #",
" ############# #",
" ######!###### #",
"               #"};

/* XPM */
static const char* icon_border_right_16[] = {
"16 16 4 1",
"  c #000000",
"! c #808080",
"# c #A0A0A4",
"$ c #FFFFFF",
"$$$$$$$$$$$$$$$$",
"!$!$!$!$!$!$!$ $",
"$$$$$$$#$$$$$$ $",
"!$$$$$$$$$$$$$ $",
"$$$$$$$#$$$$$$ $",
"!$$$$$$$$$$$$$ $",
"$$$$$$$#$$$$$$ $",
"!$$$$$$$$$$$$$ $",
"$#$#$#$#$#$#$# $",
"!$$$$$$$$$$$$$ $",
"$$$$$$$#$$$$$$ $",
"!$$$$$$$$$$$$$ $",
"$$$$$$$#$$$$$$ $",
"!$$$$$$$$$$$$$ $",
"$$$$$$$#$$$$$$ $",
"!$!$!$!$!$!$!$ $"};

/* XPM */
static const char* icon_border_thick_out_16[] = {
"16 16 3 1",
"  c #000000",
"! c #A0A0A4",
"# c #FFFFFF",
"################",
"               #",
"               #",
"  ###########  #",
"  #####!#####  #",
"  ###########  #",
"  #####!#####  #",
"  ###########  #",
"  #!#!#!#!#!#  #",
"  ###########  #",
"  #####!#####  #",
"  ###########  #",
"  #####!#####  #",
"  ###########  #",
"               #",
"               #"};

/* XPM */
static const char* icon_borders_all_16[] = {
"16 16 2 1",
"  c #000000",
"! c #FFFFFF",
"!!!!!!!!!!!!!!!!",
"               !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
"               !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
" !!!!!! !!!!!! !",
"               !"};


enum
{
    ID_FirstBorder = wxID_HIGHEST+1
};


BEGIN_EVENT_TABLE(BorderPanel, wxPanel)
    EVT_SIZE(BorderPanel::onSize)
END_EVENT_TABLE()


const int BUTTON_SIZE = 25;
const int BORDER_PADDING = 0;
const int BORDER_SQUARE_SIZE = 14;


BorderPanel::BorderPanel(wxWindow* parent,
                         long style) :
                             wxPanel(parent,
                                     wxID_ANY,
                                     wxPoint(0,0),
                                     wxSize(79,60),
                                     wxCLIP_CHILDREN | style)
{
    SetBackgroundColour(*wxWHITE);

    // set the highlight color
    m_highlight_color = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

    // initialize variabled
    m_main_sizer = NULL;
    m_grid_sizer = NULL;

    // set our default number of columns
    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    // create border grid
    int cols = (cli_width-(BORDER_PADDING*2))/BUTTON_SIZE;
    m_grid_sizer = new wxGridSizer(cols);

    // create the bitmaps; note: order of items as they
    // are inserted into the panel must match enum order; 
    // TODO: should remove this dependency
    std::vector<wxBitmap> bitmaps;
    bitmaps.push_back(wxBitmap(icon_border_noborder_16));
    bitmaps.push_back(wxBitmap(icon_border_bottom_16));
    bitmaps.push_back(wxBitmap(icon_border_left_16));
    bitmaps.push_back(wxBitmap(icon_border_right_16));
    //bitmaps.push_back(wxBitmap(icon_border_bottom_dbl_16));
    //bitmaps.push_back(wxBitmap(icon_border_bottom_thick_16));
    bitmaps.push_back(wxBitmap(icon_border_bottom_top_16));
    //bitmaps.push_back(wxBitmap(icon_border_bottom_dbl_t_16));
    //bitmaps.push_back(wxBitmap(icon_border_bottom_tk_t_16));
    //bitmaps.push_back(wxBitmap(icon_borders_all_16));
    bitmaps.push_back(wxBitmap(icon_border_outside_16));
    //bitmaps.push_back(wxBitmap(icon_border_thick_out_16));


    // create the panel
    std::vector<wxBitmap>::iterator it, it_end;
    it_end = bitmaps.end();
    
    int i = 0;
    for (it = bitmaps.begin(); it != it_end; ++it)
    {
        kcl::Button* b = new kcl::Button(this,
                                         ID_FirstBorder + (i++),
                                         wxDefaultPosition,
                                         wxSize(BUTTON_SIZE, BUTTON_SIZE),
                                         wxEmptyString,
                                         *it);

        b->SetBackgroundColour(*wxWHITE);
        b->setMode(kcl::Button::modeFlat);
        b->setRightClickAllowed(true);
        b->sigCustomRender.connect(this, &BorderPanel::onButtonRender);
        b->sigButtonClicked.connect(this, &BorderPanel::onButtonClicked);
        m_grid_sizer->Add(b);
    }

    // create main sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->AddSpacer(2);
    m_main_sizer->Add(m_grid_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, BORDER_PADDING);
    m_main_sizer->AddSpacer(2);
    SetSizer(m_main_sizer);
    Layout();
}

BorderPanel::~BorderPanel()
{
    sigDestructing(this);
}

void BorderPanel::onSize(wxSizeEvent& event)
{
    int cli_height, cli_width;
    GetClientSize(&cli_width, &cli_height);

    if (m_grid_sizer)
        m_grid_sizer->SetCols((cli_width-(BORDER_PADDING*2))/BUTTON_SIZE);
        
    event.Skip();
}

void BorderPanel::onButtonRender(kcl::Button* button,
                                wxDC* dc,
                                int flags)
{
    int xoff, yoff;
    int cli_width, cli_height;
    button->GetClientSize(&cli_width, &cli_height);

    bool color_background = true;
    wxColor selected_background = kcl::stepColor(m_highlight_color, 140);
    wxColor hover_background = kcl::stepColor(m_highlight_color, 170);

    if (flags & kcl::Button::buttonDepressed)
    {
        // draw selected background (darker blue)
        dc->SetPen(m_highlight_color);
        dc->SetBrush(selected_background);
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
            dc->SetBrush(hover_background);
            dc->DrawRectangle(0, 0, cli_width, cli_height);
        }
         else
        {
            // draw no background
            color_background = false;
            dc->SetPen(*wxWHITE);
            dc->SetBrush(*wxWHITE);
            dc->DrawRectangle(0, 0, cli_width, cli_height);
        }
    }

    xoff = (cli_width-BORDER_SQUARE_SIZE)/2;
    yoff = (cli_height-BORDER_SQUARE_SIZE)/2;

    wxBitmap bitmap = button->getBitmap();

    if (color_background)
    {
        wxMask* mask = new wxMask(bitmap, *wxWHITE);
        bitmap.SetMask(mask);
    }
    
    dc->DrawBitmap(bitmap, xoff, yoff, true);
}

void BorderPanel::onButtonClicked(kcl::Button* button, int b)
{
    int border_style = button->GetId()-ID_FirstBorder + 1;
    sigBorderSelected(border_style);
}


};

