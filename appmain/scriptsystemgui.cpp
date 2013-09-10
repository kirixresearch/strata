/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#include "appmain.h"
#include "scripthost.h"
#include "scriptgui.h"
#include "scriptfont.h"
#include "scriptsystemgui.h"
#include <wx/settings.h>
#include <kl/thread.h>


// (CLASS) SystemColors
// Category: System
// Derives:
// Description: A class that provides access to system colors.
// Remarks: The SystemColors class provides access to system colors.
//
// Property(SystemColors.ActiveBorder):        The color of the active window's border.
// Property(SystemColors.ActiveCaption):       The color of the background in the the active window's title bar.
// Property(SystemColors.ActiveCaptionText):   The color of the text in the active window's title bar.
// Property(SystemColors.AppWorkspace):        The color of the application workspace.
// Property(SystemColors.ButtonFace):          The face color of a button element.
// Property(SystemColors.ButtonHighlight):     The highlight color of a button element.
// Property(SystemColors.ButtonShadow):        The shadow color of a button element.
// Property(SystemColors.ButtonText):          The text color of a button element.
// Property(SystemColors.ControlDarkShadow):   The dark shadow color of a 3D element.
// Property(SystemColors.ControlFace):         The face color of a 3D element.
// Property(SystemColors.ControlHighlight):    The highlight color of a 3D element.
// Property(SystemColors.ControlLight):        The light color of a 3D element.
// Property(SystemColors.ControlShadow):       The shadow color of a 3D element.
// Property(SystemColors.ControlText):         The color of text in a 3D element.
// Property(SystemColors.Desktop):             The color of the desktop.
// Property(SystemColors.GrayText):            The color of dimmed text.
// Property(SystemColors.Highlight):           The color of the background of selected items.
// Property(SystemColors.HighlightText):       The color of the text of selected items.
// Property(SystemColors.InactiveBorder):      The color of an inactive window's border.
// Property(SystemColors.InactiveCaption):     The color of the background of an inactive window's title bar.
// Property(SystemColors.InactiveCaptionText): The color of the text in an inactive window's title bar.
// Property(SystemColors.Info):                The color of the background of a ToolTip.
// Property(SystemColors.InfoText):            The color of the text of a ToolTip.
// Property(SystemColors.Menu):                The color of a menu's background.
// Property(SystemColors.MenuText):            The color of a menu's text.
// Property(SystemColors.ScrollBar):           The color of the background of a scroll bar.
// Property(SystemColors.Window):              The color of the background in the client area of a window.
// Property(SystemColors.WindowFrame):         The color of a window frame.
// Property(SystemColors.WindowText):          The color of the text in the client area of a window.

SystemColors::SystemColors()
{
    m_populated = false;
}

SystemColors::~SystemColors()
{
}

void SystemColors::addColor(const std::wstring& var_name,
                            const wxColor& color)
{
    Color* c = Color::createObject(getParser());
    c->getMember(L"red")->setInteger(color.Red());
    c->getMember(L"green")->setInteger(color.Green());
    c->getMember(L"blue")->setInteger(color.Blue());
    getMember(var_name)->setObject(c);
}

void SystemColors::populate()
{
    m_populated = true;
    
    #if APP_GUI==1
    addColor(L"ActiveBorder",        wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVEBORDER));
    addColor(L"ActiveCaption",       wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION));
    addColor(L"ActiveCaptionText",   wxSystemSettings::GetColour(wxSYS_COLOUR_CAPTIONTEXT));
    addColor(L"AppWorkspace",        wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));
    addColor(L"ButtonFace",          wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    addColor(L"ButtonHighlight",     wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT));
    addColor(L"ButtonShadow",        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
    addColor(L"ButtonText",          wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    addColor(L"ControlDarkShadow",   wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW));
    addColor(L"ControlFace",         wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
    addColor(L"ControlHighlight",    wxSystemSettings::GetColour(wxSYS_COLOUR_3DHIGHLIGHT));
    addColor(L"ControlLight",        wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT));
    addColor(L"ControlShadow",       wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW));
    addColor(L"ControlText",         wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    addColor(L"Desktop",             wxSystemSettings::GetColour(wxSYS_COLOUR_DESKTOP));
    addColor(L"GrayText",            wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    addColor(L"Highlight",           wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
    addColor(L"HighlightText",       wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    addColor(L"InactiveBorder",      wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVEBORDER));
    addColor(L"InactiveCaption",     wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTION));
    addColor(L"InactiveCaptionText", wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT));
    addColor(L"Info",                wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));
    addColor(L"InfoText",            wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));
    addColor(L"Menu",                wxSystemSettings::GetColour(wxSYS_COLOUR_MENU));
    addColor(L"MenuText",            wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT));
    addColor(L"ScrollBar",           wxSystemSettings::GetColour(wxSYS_COLOUR_SCROLLBAR));
    addColor(L"Window",              wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    addColor(L"WindowFrame",         wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME));
    addColor(L"WindowText",          wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    #endif
}


kscript::Value* SystemColors::getMember(const std::wstring& name)
{
    if (!m_populated)
    {
        populate();
    }
    
    return BaseClass::getMember(name);
}




// (CLASS) SystemMetrics
// Category: System
// Derives:
// Description: A class that provides access to system metrics.
// Remarks: The SystemMetrics class provides access to system metrics.
//
// Property(SystemMetrics.BorderHeight):                The height of a single border.
// Property(SystemMetrics.BorderWidth):                 The width of a single border.
// Property(SystemMetrics.CaptionHeight):               The height of a window caption area.
// Property(SystemMetrics.CursorHeight):                The height of the cursor.
// Property(SystemMetrics.CursorWidth):                 The width of the cursor.
// Property(SystemMetrics.DoubleClickHeight):           The maximum vertical distance between two mouse clicks in order to generate a double-click.
// Property(SystemMetrics.DoubleClickWidth):            The maximum horizontal distance between two mouse clicks in order to generate a double-click.
// Property(SystemMetrics.DragHeight):                  The minimum vertical distance the mouse must move from a drag point in order to generate a drag event.
// Property(SystemMetrics.DragWidth):                   The minimum horizontal distance the mouse must move from a drag point in order to generate a drag event.
// Property(SystemMetrics.EdgeHeight):                  The height of a 3D border.
// Property(SystemMetrics.EdgeWidth):                   The width of a 3D border.
// Property(SystemMetrics.FrameSizeHeight):             The height of the frame of a thick-framed window.
// Property(SystemMetrics.FrameSizeWidth):              The width of the frame of a thick-framed window.
// Property(SystemMetrics.HorizontalScrollArrowHeight): The height of the arrow bitmap on a horizontal scrollbar.
// Property(SystemMetrics.HorizontalScrollArrowWidth):  The width of the arrow bitmap on a horizontal scrollbar.
// Property(SystemMetrics.HorizontalScrollbarHeight):   The height of a horizontal scrollbar.
// Property(SystemMetrics.HorizontalThumbWidth):        The width of a horizontal scrollbar thumb tracker.
// Property(SystemMetrics.IconHeight):                  The default height of an icon.
// Property(SystemMetrics.IconWidth):                   The default width of an icon.
// Property(SystemMetrics.IconSpacingHeight):           The height of a grid cell for items tiled in large icon view.
// Property(SystemMetrics.IconSpacingWidth):            The width of a grid cell for items tiled in large icon view.
// Property(SystemMetrics.MenuBarHeight):               The height of a menu bar.
// Property(SystemMetrics.ScreenHeight):                The height of the screen.
// Property(SystemMetrics.ScreenWidth):                 The width of the screen.
// Property(SystemMetrics.SmallIconHeight):             The recommended height of a small icon.
// Property(SystemMetrics.SmallIconWidth):              The recommended width of a small icon.
// Property(SystemMetrics.VerticalScrollArrowHeight):   The height of the arrow bitmap on a vertical scrollbar.
// Property(SystemMetrics.VerticalScrollArrowWidth):    The width of the arrow bitmap on a vertical scrollbar.
// Property(SystemMetrics.VerticalScrollbarWidth):      The width of a vertical scrollbar.
// Property(SystemMetrics.VerticalThumbHeight):         The height of a vertical scrollbar thumb tracker.
// Property(SystemMetrics.WindowMinimumHeight):         The minimum height of a window.
// Property(SystemMetrics.WindowMinimumWidth):          The minimum width of a window.

SystemMetrics::SystemMetrics()
{
    m_populated = false;
}

SystemMetrics::~SystemMetrics()
{
}

void SystemMetrics::populate()
{
    m_populated = true;
    
    #if APP_GUI==1
    addMetric(L"BorderHeight",                wxSystemSettings::GetMetric(wxSYS_BORDER_Y));
    addMetric(L"BorderWidth",                 wxSystemSettings::GetMetric(wxSYS_BORDER_X));
    addMetric(L"CaptionHeight",               wxSystemSettings::GetMetric(wxSYS_CAPTION_Y));
    addMetric(L"CursorHeight",                wxSystemSettings::GetMetric(wxSYS_CURSOR_Y));
    addMetric(L"CursorWidth",                 wxSystemSettings::GetMetric(wxSYS_CURSOR_X));
    addMetric(L"DoubleClickHeight",           wxSystemSettings::GetMetric(wxSYS_DCLICK_Y));
    addMetric(L"DoubleClickWidth",            wxSystemSettings::GetMetric(wxSYS_DCLICK_X));
    addMetric(L"DragHeight",                  wxSystemSettings::GetMetric(wxSYS_DRAG_Y));
    addMetric(L"DragWidth",                   wxSystemSettings::GetMetric(wxSYS_DRAG_X));
    addMetric(L"EdgeHeight",                  wxSystemSettings::GetMetric(wxSYS_EDGE_Y));
    addMetric(L"EdgeWidth",                   wxSystemSettings::GetMetric(wxSYS_EDGE_X));
    addMetric(L"FrameSizeHeight",             wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_Y));
    addMetric(L"FrameSizeWidth",              wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_X));
    addMetric(L"HorizontalScrollArrowHeight", wxSystemSettings::GetMetric(wxSYS_HSCROLL_ARROW_Y));
    addMetric(L"HorizontalScrollArrowWidth",  wxSystemSettings::GetMetric(wxSYS_HSCROLL_ARROW_X));
    addMetric(L"HorizontalScrollbarHeight",   wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y));
    addMetric(L"HorizontalThumbWidth",        wxSystemSettings::GetMetric(wxSYS_HTHUMB_X));
    addMetric(L"IconHeight",                  wxSystemSettings::GetMetric(wxSYS_ICON_Y));
    addMetric(L"IconWidth",                   wxSystemSettings::GetMetric(wxSYS_ICON_X));
    addMetric(L"IconSpacingHeight",           wxSystemSettings::GetMetric(wxSYS_ICONSPACING_Y));
    addMetric(L"IconSpacingWidth",            wxSystemSettings::GetMetric(wxSYS_ICONSPACING_X));
    addMetric(L"MenuBarHeight",               wxSystemSettings::GetMetric(wxSYS_MENU_Y));
    addMetric(L"ScreenHeight",                wxSystemSettings::GetMetric(wxSYS_SCREEN_Y));
    addMetric(L"ScreenWidth",                 wxSystemSettings::GetMetric(wxSYS_SCREEN_X));
    addMetric(L"SmallIconHeight",             wxSystemSettings::GetMetric(wxSYS_SMALLICON_Y));
    addMetric(L"SmallIconWidth",              wxSystemSettings::GetMetric(wxSYS_SMALLICON_X));
    addMetric(L"VerticalScrollArrowHeight",   wxSystemSettings::GetMetric(wxSYS_VSCROLL_ARROW_Y));
    addMetric(L"VerticalScrollArrowWidth",    wxSystemSettings::GetMetric(wxSYS_VSCROLL_ARROW_X));
    addMetric(L"VerticalScrollbarWidth",      wxSystemSettings::GetMetric(wxSYS_VSCROLL_X));
    addMetric(L"VerticalThumbHeight",         wxSystemSettings::GetMetric(wxSYS_VTHUMB_Y));
    addMetric(L"WindowMinimumHeight",         wxSystemSettings::GetMetric(wxSYS_WINDOWMIN_Y));
    addMetric(L"WindowMinimumWidth",          wxSystemSettings::GetMetric(wxSYS_WINDOWMIN_X));
    #endif
}

void SystemMetrics::addMetric(const std::wstring& var_name,
                              const int metric)
{
    getMember(var_name)->setInteger(metric);
}

kscript::Value* SystemMetrics::getMember(const std::wstring& name)
{
    if (!m_populated)
    {
        populate();
    }
    
    return BaseClass::getMember(name);
}



// (CLASS) SystemFonts
// Category: System
// Derives:
// Description: A class that provides access to system fonts.
// Remarks: The SystemFonts class provides access to system fonts.
//
// Property(SystemFonts.AnsiFixedFont):     Windows fixed-width font.
// Property(SystemFonts.AnsiVariableFont):  Windows variable-width font.
// Property(SystemFonts.DefaultFont):       Default font.
// Property(SystemFonts.OemFixedFont):      Original equipment manufacturer fixed-width font.
// Property(SystemFonts.SystemFont):        System font.
// Property(SystemFonts.SwissFont):         Swiss font.

SystemFonts::SystemFonts()
{
    m_populated = false;
}

SystemFonts::~SystemFonts()
{
}

void SystemFonts::populate()
{
    m_populated = true;
    
    #if APP_GUI==1
    addFont(L"AnsiFixedFont", wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));
    addFont(L"AnsiVariableFont", wxSystemSettings::GetFont(wxSYS_ANSI_VAR_FONT));
    addFont(L"DefaultFont", *wxNORMAL_FONT);
    addFont(L"OemFixedFont", wxSystemSettings::GetFont(wxSYS_OEM_FIXED_FONT));
    addFont(L"SystemFont", wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
    addFont(L"SwissFont", *wxSWISS_FONT);
    #endif
}

void SystemFonts::addFont(const std::wstring& var_name,
                          const wxFont& font)
{
    zFont* f = zFont::createObject(getParser());
    f->setWxFont(font);
    getMember(var_name)->setObject(f);
}

kscript::Value* SystemFonts::getMember(const std::wstring& name)
{
    if (!m_populated)
    {
        populate();
    }
    
    return BaseClass::getMember(name);
}



// (METHOD) SystemFonts.getInstalledFonts
//
// Description: Returns an array of the installed font facenames.
//
// Syntax: static function SystemFonts.getInstalledFonts() : Array(String)
//
// Remarks: Returns an array of the installed font facenames.
//
// Returns: An array containing string values which represent the facenames
//     of all the fonts that are installed on the system.

void SystemFonts::getInstalledFonts(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setArray(env);

    wxArrayString fonts = g_app->getFontNames();
    size_t i, count = fonts.Count();
    for (i = 0; i < count; ++i)
    {
        kscript::Value val;
        val.setString(towstr(fonts.Item(i)));
        retval->appendMember(&val);
    }
}


