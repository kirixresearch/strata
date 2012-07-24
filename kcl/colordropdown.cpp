/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2004-02-04
 *
 */


#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/image.h>
#include "button.h"
#include "colordropdown.h"
#include "popupcontainer.h"
#include "colorpanel.h"


namespace kcl
{


ColorDropDown::ColorDropDown(wxWindow* parent,
                             wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size)
                              : kcl::Button(parent, id, pos, size, wxEmptyString, wxNullBitmap)
{
    m_color_panel = NULL;
    setDropDown(true);
}

wxBitmap ColorDropDown::createColorBitmap(const wxBitmap& bmp)
{
    wxBitmap bitmap(bmp);
    int bmp_width = bitmap.GetWidth();
    int bmp_height = bitmap.GetHeight();
    int y;

    if (bmp_height < 4)
        return bmp;

    wxMemoryDC dc;
    dc.SelectObject(bitmap);

    if (m_color.Ok())
    {
        wxPen pen(m_color, 1, wxSOLID);
        dc.SetPen(pen);

        for (y = bmp_height-4; y < bmp_height; ++y)
        {
            dc.DrawLine(0, y, bmp_width, y);
        }
    }
     else
    {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxLIGHT_GREY_BRUSH);

        dc.DrawRectangle(0, bmp_height-4, bmp_width, 4);
    }

    return bitmap;
}

void ColorDropDown::onDropDownClicked()
{
    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    wxPoint pt = ClientToScreen(wxPoint(0,cli_height-1));

    m_popup_container = new kcl::PopupContainer(pt);
    m_color_panel = new ColorPanel(m_popup_container);
    m_color_panel->setMode(ColorPanel::ModeNone);
    m_color_panel->setCustomColors(m_custom_colors);

    m_color_panel->sigColorSelected.connect(this, &ColorDropDown::onColorSelected);
    m_color_panel->sigCustomColorDialogOk.connect(this, &ColorDropDown::onCustomColorDialogOk);

    m_popup_container->doPopup(m_color_panel);
}

void ColorDropDown::onButtonClicked()
{
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, GetId());
    event.SetInt(0);
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
}

void ColorDropDown::onColorSelected(wxColor color, int mode)
{
    setColor(color);
    m_popup_container->Destroy();

    onButtonClicked();
    Refresh(false);
    
    sigColorSelected(color);
}

void ColorDropDown::onCustomColorDialogOk()
{
    // -- get the custom colors from the Color Dialog --
    m_color_panel->getCustomColors(m_custom_colors);
    sigCustomColorDialogOk(this);
}

wxColor ColorDropDown::getColor()
{
    return m_color;
}

void ColorDropDown::setColor(const wxColor& color)
{
    m_color = color;
}

void ColorDropDown::setCustomColors(const std::vector<wxColour>& custom_colors)
{
    m_custom_colors = custom_colors;
}

void ColorDropDown::getCustomColors(std::vector<wxColour>& custom_colors)
{
    custom_colors = m_custom_colors;
}




};

