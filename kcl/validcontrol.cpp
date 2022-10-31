/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2002-01-19
 *
 */


#include "validcontrol.h"
#include "util.h"


namespace kcl
{


const int INDENT = 3;
const int SPACER = 4;


/* XPM */
static const char* xpm_valid_checkmark[] = {
"16 16 5 1",
"  c None",
"! c #000000",
"# c #008000",
"$ c #808080",
"% c #00FF00",
"           %##! ",
"           %##! ",
"          %##!  ",
"          %##!  ",
"         %##!   ",
"         %##!   ",
"        %##!    ",
"  %     %##!    ",
" %#    %##!     ",
"%###   %##!     ",
" #### %##!      ",
"  ####%##!      ",
"   #####!       ",
"    ####!       ",
"     #$!        ",
"                "};

/* XPM */
static const char* xpm_valid_xmark[] = {
"16 16 5 1",
"  c None",
"! c #000000",
"# c #800000",
"$ c #808080",
"% c #FF0000",
"                ",
" %#        %#   ",
" ###      %###  ",
" %###    %###   ",
"  %###  %###    ",
"   %###%###     ",
"    %#####      ",
"     %###       ",
"    %#####      ",
"   %###$###     ",
"  %###  $###    ",
" %###    $###   ",
" ###      $###  ",
"  #        $#   ",
"                ",
"                "};




BEGIN_EVENT_TABLE(ValidControl, wxControl)
    EVT_PAINT(ValidControl::onPaint)
    EVT_SIZE(ValidControl::onSize)
END_EVENT_TABLE()


ValidControl::ValidControl(wxWindow* parent, wxWindowID id,
                           const wxPoint& pos, const wxSize& size) :
                               wxControl(parent, id, pos, size, wxNO_BORDER),
                                   m_valid_bitmap(xpm_valid_checkmark),
                                   m_invalid_bitmap(xpm_valid_xmark)

{
    m_valid_label = _("Valid");
    m_invalid_label = _("Invalid");
    m_valid_state = true;
    m_border_visible = true;
    m_label_visible = true;
    m_alignment = ValidControl::alignCenter;
    m_bgbrush = wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE), wxBRUSHSTYLE_SOLID);
    SetFont(*wxNORMAL_FONT);
    
    wxSize s = GetSize();
    m_minWidth = s.x;
    m_minHeight = wxMax(s.y, FromDIP(16 + 6));
}

void ValidControl::setValidLabel(const wxString& label)
{
    m_valid_label = label;
    Refresh(true);
}

void ValidControl::setInvalidLabel(const wxString& label)
{
    m_invalid_label = label;
    Refresh(true);
}

void ValidControl::setValidBitmap(const wxBitmap& bitmap)
{
    m_valid_bitmap = bitmap;
    m_minHeight = wxMax(m_minHeight, m_valid_bitmap.GetHeight() + FromDIP(6));
    Refresh(true);
}

void ValidControl::setInvalidBitmap(const wxBitmap& bitmap)
{
    m_invalid_bitmap = bitmap;
    m_minHeight = wxMax(m_minHeight, m_invalid_bitmap.GetHeight() + FromDIP(6));
    Refresh(true);
}

void ValidControl::setValid(bool state)
{
    m_valid_state = state;
    Refresh(true);
}

void ValidControl::setBorderVisible(bool state)
{
    m_border_visible = state;
    Refresh(true);
}

void ValidControl::setLabelVisible(bool state)
{
    m_label_visible = state;
    Refresh(true);
}

void ValidControl::setAlignment(int align)
{
    m_alignment = align;
    Refresh(true);
}

void ValidControl::onPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    
    wxSize size = GetClientSize();
    int cli_height = size.GetHeight();
    int cli_width = size.GetWidth();

    dc.SetFont(GetFont());
    dc.SetBrush(m_bgbrush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, cli_width, cli_height);

    if (m_border_visible)
    {
        // since we're going to be drawing a border around the control,
        // it's OK to add a nice gradient background -- we only want this
        // gradient backound if we have a border, though, because otherwise
        // the control may be placed in a solid background panel and the
        // background gradient would look out of place
        dc.GradientFillLinear(wxRect(0, 0, cli_width, cli_height),
                              stepColor(getBaseColor(), 160),
                              stepColor(getBaseColor(), 95),
                              wxSOUTH);
        
        dc.SetPen(*wxGREY_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(0, 0, cli_width, cli_height);
        dc.SetPen(*wxWHITE_PEN);
        dc.DrawRectangle(1, 1, cli_width-2, cli_height-2);
    }

    wxString label;
    wxBitmap bitmap;

    if (m_valid_state)
    {
        label = m_valid_label;
        bitmap = m_valid_bitmap;
    }
     else
    {
        label = m_invalid_label;
        bitmap = m_invalid_bitmap;
    }

    int xoff = 0;
    int bmp_width = bitmap.GetWidth(), bmp_height = bitmap.GetHeight();
    wxCoord text_width = 0, text_height = 0;
    
    if (m_label_visible)
    {
        dc.GetTextExtent(label, &text_width, &text_height);
    }

    switch (m_alignment)
    {
        case ValidControl::alignLeft:
        {
            xoff = INDENT;
            break;
        }
        case ValidControl::alignCenter:
        {
            xoff = (cli_width-bmp_width)/2;
            if (m_label_visible)
            {
                xoff -= (text_width)/2;
                xoff -= SPACER;
            }
            break;
        }
        case ValidControl::alignRight:
        {
            xoff = cli_width - INDENT;
            if (m_label_visible)
            {
                xoff -= (text_width)/2;
                xoff -= SPACER;
            }
            break;
        }
    }

    dc.DrawBitmap(bitmap, xoff, (cli_height-bmp_height)/2 + 1, true);

    xoff += bmp_width;
    xoff += SPACER;
    
    if (m_label_visible)
    {
        dc.DrawText(label, xoff,(cli_height-text_height)/2);
    }
}

void ValidControl::onSize(wxSizeEvent& event)
{
    Refresh(true);
}


}; // namespace kcl

