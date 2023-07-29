/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-26
 *
 */


#include "grid.h"
#include "gridmemmodel.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <wx/clipbrd.h>
#include <wx/tokenzr.h>
#include <wx/tooltip.h>
#include <wx/renderer.h>
#include <kl/math.h>
#include "util.h"


#ifdef WIN32
#ifdef NOMINMAX
#if _MSC_VER < 1600
#define max _cpp_max
#define min _cpp_min
#endif
#endif
#endif


#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif


#ifdef __WXMSW__
#include <wx/msw/uxtheme.h>
#endif

// event definitions

DEFINE_EVENT_TYPE(wxEVT_KCLGRID_SELECTION_CHANGE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_BEGIN_EDIT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_END_EDIT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_BEGIN_CAPTION_EDIT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_END_CAPTION_EDIT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_EDIT_CHANGE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_CELL_LEFT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_CELL_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_CELL_LEFT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_CELL_RIGHT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_LEFT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_ROW_LEFT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_ROW_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_ROW_RESIZE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_ROW_SASH_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_PRE_CURSOR_MOVE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_CURSOR_MOVE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_RESIZE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_HIDE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_INSERT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_MOVE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_MODEL_CHANGE)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_PRE_GHOST_ROW_INSERT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_GHOST_ROW_INSERT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_PRE_INVALID_AREA_INSERT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_INVALID_AREA_INSERT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_INVALID_AREA_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_BUTTON_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_COLOR_CLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_KEY_DOWN)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_NEED_TOOLTIP_TEXT)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_BEGIN_DRAG)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_HSCROLL)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_VSCROLL)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_LINK_LEFTCLICK)
DEFINE_EVENT_TYPE(wxEVT_KCLGRID_LINK_MIDDLECLICK)


namespace kcl
{


/* XPM */
static const char* xpm_cursorrowmarker[] = {
"9 13 2 1",
"x c #000000",
"  c None",
"         ",
"         ",
"  x      ",
"  xx     ",
"  xxx    ",
"  xxxx   ",
"  xxxxx  ",
"  xxxx   ",
"  xxx    ",
"  xx     ",
"  x      ",
"         ",
"         ",
};

/* XPM */
static const char* xpm_ghostrowmarker[] = {
"9 13 2 1",
"x c #000000",
"  c None",
"         ",
"         ",
"         ",
"    x    ",
"    x    ",
"    x    ",
" xxxxxxx ",
"    x    ",
"    x    ",
"    x    ",
"         ",
"         ",
"         ",
};

/* XPM */
static const char* xpm_checkon[] = {
"13 13 5 1",
"  c None",
"! c #000000",
"# c #808080",
"$ c #C0C0C0",
"% c #FFFFFF",
"!!!!!!!!!!!!!",
"!##########$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%!%$!",
"!#%%%%%%!!%$!",
"!#%!%%%!!!%$!",
"!#%!!%!!!%%$!",
"!#%!!!!!%%%$!",
"!#%%!!!%%%%$!",
"!#%%%!%%%%%$!",
"!#%%%%%%%%%$!",
"!$$$$$$$$$$$!",
"!!!!!!!!!!!!!"};

/* XPM */
static const char* xpm_checkoff[] = {
"13 13 5 1",
"  c None",
"! c #000000",
"# c #808080",
"$ c #C0C0C0",
"% c #FFFFFF",
"!!!!!!!!!!!!!",
"!##########$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%%%$!",
"!#%%%%%%%%%$!",
"!$$$$$$$$$$$!",
"!!!!!!!!!!!!!"};

/* XPM */
static const char* xpm_checkneutral[] = {
"13 13 5 1",
"  c None",
"! c #000000",
"# c #808080",
"$ c #C0C0C0",
"% c #FFFFFF",
"!!!!!!!!!!!!!",
"!##########$!",
"!#!%!%!%!%!$!",
"!#%!%!%!%!%$!",
"!#!%!%!%!%!$!",
"!#%!%!%!%!%$!",
"!#!%!%!%!%!$!",
"!#%!%!%!%!%$!",
"!#!%!%!%!%!$!",
"!#%!%!%!%!%$!",
"!#!%!%!%!%!$!",
"!$$$$$$$$$$$!",
"!!!!!!!!!!!!!"};


// utility functions

wxColor getDefaultGreenBarColor(const wxColor& color, int ialpha)
{
    // make sure our default greenbar color is not too light
    wxColor c = kcl::stepColor(color, ialpha);
    if (c == *wxWHITE)
        return wxColor(248,248,248);
    if (c.Red() <= 240 || c.Green() <= 240 || c.Blue() <= 240)
        return c;
    if (c.Red() <= 248 && c.Green() <= 248 && c.Blue() <= 248)
        return c;
    if ((255-c.Red()) + (255-c.Green()) + (255-c.Blue()) <= 20)
        return getDefaultGreenBarColor(color, ialpha-1);
    return c;
}




static bool DrawUxThemeCheckBox(wxWindow* wnd,
                                wxDC& dc,
                                const wxRect& rect,
                                int flags)
{
#if defined(__WXMSW__)
#if wxUSE_UXTHEME

#if wxCHECK_VERSION(3,1,1)
    if (true)
#else
    if (wxUxThemeEngine::Get())
#endif
    {
        wxUxThemeHandle hTheme(wnd, L"BUTTON");
        if (hTheme)
        {
            RECT r;
            wxCopyRectToRECT(rect, r);
            
            int part = 3 /*BP_CHECKBOX*/;
            int state;
            if (flags & wxCONTROL_CHECKED)
                state = 5; /*CBS_CHECKEDNORMAL*/
                 else
                state = 1; /*CBS_UNCHECKEDNORMAL*/
            
            // draw the themed checkbox button
#if wxCHECK_VERSION(3,1,1)
            ::DrawThemeBackground(hTheme, (HDC)getHdcFrom(dc), part, state, &r, NULL);
#else
            wxUxThemeEngine::Get()->DrawThemeBackground(
                        hTheme, 
                        (HDC)getHdcFrom(dc), 
                        part,
                        state,
                        &r, NULL);
#endif

            return true;
        }
    }
    
#endif  // wxUSE_UXTHEME
#endif  // defined(__WXMSW__)

    return false;
}

static wxBitmap getCheckBoxBitmap(wxWindow* win, bool checked)
{
    int flags = 0;
    if (checked)
        flags = wxCONTROL_CHECKED;
        
    wxRect rect(0,0,13,13);
    wxBitmap bmp(13,13);
    wxMemoryDC memdc;
    memdc.SelectObject(bmp);
    
#ifdef WIN32
    if (!DrawUxThemeCheckBox(win, memdc, rect, flags))
        return wxBitmap(checked ? xpm_checkon : xpm_checkoff);
         else
        return bmp;
#else
    return wxBitmap(checked ? xpm_checkon : xpm_checkoff);
#endif
}




#define MAX_SELECT 2147483647

const int EditCtrlId = 29923;



// ModelColumn implementation

ModelColumn::ModelColumn()
{
}

ModelColumn::~ModelColumn()
{
}

void ModelColumn::setName(const wxString& newval)
{
    m_name = newval;
}

wxString ModelColumn::getName()
{
    return m_name;
}

void ModelColumn::setType(int newval)
{
    m_type = newval;
}

int ModelColumn::getType()
{
    return m_type;
}

void ModelColumn::setWidth(int newval)
{
    m_width = newval;
}

int ModelColumn::getWidth()
{
    return m_width;
}

void ModelColumn::setScale(int newval)
{
    m_scale = newval;
}

int ModelColumn::getScale()
{
    return m_scale;
}


// TestModel implementation

TestModel::TestModel()
{
    m_row = 0;
}

void TestModel::initModel(kcl::Grid* grid)
{
}

void TestModel::refresh()
{
}

void TestModel::reset()
{
    m_row = 0;
}

int TestModel::getColumnCount()
{
    return 255;
}

IModelColumnPtr TestModel::getColumnInfo(int col)
{
    ModelColumn* m = new ModelColumn;
    
    wxString buf = wxString::Format(wxT("COLUMN_%02d"), col);
    
    m->setName(buf);
    m->setType(0);
    m->setWidth(20);
    m->setScale(0);

    return static_cast<IModelColumn*>(m);
}

int TestModel::getColumnIndex(const wxString& col_name)
{
    wxString p = col_name.AfterFirst(wxT('_'));
    int coln = wxAtoi(p);

    if (coln >= getColumnCount())
        return -1;

    return coln;
}



void TestModel::setColumnBitmap(int col,
                                const wxBitmap& bitmap,
                                int alignment)
{
}

void TestModel::getColumnBitmap(int col,
                               wxBitmap* bitmap,
                               int* alignment)
{
}



bool TestModel::setColumnProperties(int col,
                                    CellProperties* cell_props)
{
    return false;
}

void TestModel::getCellProperties(int row,
                                  int col,
                                  CellProperties* cell_props)
{
}

bool TestModel::setCellProperties(int row,
                                  int col,
                                  CellProperties* cell_props)
{
    return false;
}

intptr_t TestModel::getRowData(int row)
{
    return 0;
}

void TestModel::setRowData(int row, intptr_t data)
{
}

void TestModel::getCellBitmap(int row,
                              int col,
                              wxBitmap* bitmap,
                              int* alignment)
{
}

wxString TestModel::getCellString(int row, int col)
{
    return wxString::Format(wxT("Record %d, Field %d"), row, col);
}

double TestModel::getCellDouble(int row, int col)
{
    return 0.0;
}

int TestModel::getCellInteger(int row, int col)
{
    return 0;
}

bool TestModel::getCellBoolean(int row, int col)
{
    return false;
}

int TestModel::getCellComboSel(int row, int col)
{
    return -1;
}

bool TestModel::isNull(int row, int col)
{
    return false;
}

bool TestModel::setCellBitmap(int row,
                              int col,
                              const wxBitmap& bitmap,
                              int alignment)
{
    return false;
}

bool TestModel::setCellString(int row,
                              int col,
                              const wxString& value)
{
    return false;
}

bool TestModel::setCellDouble(int row,
                              int col,
                              double value)
{
    return false;
}

bool TestModel::setCellInteger(int row,
                               int col,
                               int value)
{
    return false;
}

bool TestModel::setCellBoolean(int row,
                               int col,
                               bool value)
{
    return false;
}

bool TestModel::setCellComboSel(int row,
                                int col,
                                int sel)
{
    return false;
}

int TestModel::createColumn(int position,
                            const wxString& name,
                            int type,
                            int width,
                            int scale)
{
    // this model doesn't support column creation
    return -1;
}

bool TestModel::modifyColumn(int position,
                             const wxString& str,
                             int type,
                             int width,
                             int scale)
{
    return false;
}

bool TestModel::deleteColumn(int position)
{
    return false;
}

int TestModel::insertRow(int position)
{
    // this model doesn't support row insertion
    return -1;
}

bool TestModel::deleteRow(int position)
{
    // this model doesn't support row deletion
    return false;
}

bool TestModel::deleteAllRows()
{
    // this model doesn't support row deletion
    return false;
}

int TestModel::getRowCount()
{
    return 10000;
}

bool TestModel::isRowValid(int row)
{
    return (row >= 0 && row < 10000 ? true : false);
}

void TestModel::onCursorRowChanged()
{
}

bool TestModel::getGroupBreak()
{
    return false;
}

bool TestModel::getGroupBreaksActive()
{
    return false;
}




// cell editor controls

class GridTextCtrl : public wxTextCtrl
{

public:
    GridTextCtrl(Grid* grid, wxWindow* parent,
        wxWindowID id, const wxString& value,
        const wxPoint& pos, const wxSize& size, long style)
            : wxTextCtrl(parent, id, value, pos, size, style | wxTE_PROCESS_ENTER)
    {
        m_grid = grid;
        m_last_key = -1;
        m_focus_lost = false;
        m_user_action = true;
        m_deleting = false;
        m_numbers_only = false;
    }

    ~GridTextCtrl()
    {
        // attempt to tell grid to write out the value
        m_deleting = true;
        wxString value = GetValue();
        m_grid->onEditDestroy(value,
                              m_last_key,
                              m_focus_lost,
                              m_user_action);
    }

    void setNumbersOnly(bool val = true)
    {
        m_numbers_only = val;
    }
    
    void destroyNow(bool accept)
    {
        if (!accept)
        {
            m_last_key = WXK_ESCAPE;
        }

        m_user_action = false;
        Destroy();
    }

private:

    void onKeyDown(wxKeyEvent& event)
    {
        m_last_key = event.GetKeyCode();
        if (m_last_key == WXK_RETURN         ||
            m_last_key == WXK_NUMPAD_ENTER   ||
            m_last_key == WXK_TAB            ||
            m_last_key == WXK_UP             ||
            m_last_key == WXK_NUMPAD_UP      ||
            m_last_key == WXK_DOWN           ||
            m_last_key == WXK_NUMPAD_DOWN    ||
            m_last_key == WXK_ESCAPE)
        {
            // make Shift-Enter go up instead of down
            if (m_last_key == WXK_RETURN ||
                m_last_key == WXK_NUMPAD_ENTER)
            {
                if (event.ShiftDown())
                {
                    m_last_key = WXK_UP;
                }
            }
            
            // translate numpad up/down to regular up/down
            if (m_last_key == WXK_NUMPAD_UP)
                m_last_key = WXK_UP;
            if (m_last_key == WXK_NUMPAD_DOWN)
                m_last_key = WXK_DOWN;
            
            // make Shift-Tab go left instead of right
            if (m_last_key == WXK_TAB)
            {
                if (event.ShiftDown())
                {
                    m_last_key = WXK_LEFT;
                }
            }

            Destroy();
            return;
        }
        event.Skip();
    }

    void onChar(wxKeyEvent& evt)
    {
        int keycode = evt.GetKeyCode();
        if (m_numbers_only && keycode != wxT('0')
                           && keycode != wxT('1')
                           && keycode != wxT('2')
                           && keycode != wxT('3')
                           && keycode != wxT('4')
                           && keycode != wxT('5')
                           && keycode != wxT('6')
                           && keycode != wxT('7')
                           && keycode != wxT('8')
                           && keycode != wxT('9')
                           && keycode != WXK_BACK
                           && keycode != WXK_DELETE
                           && keycode != WXK_NUMPAD_DELETE
                           && keycode != WXK_LEFT
                           && keycode != WXK_NUMPAD_LEFT
                           && keycode != WXK_RIGHT
                           && keycode != WXK_NUMPAD_RIGHT)
        {
            return;
        }
        
        wxTextCtrl::OnChar(evt);
    }

    void onTextChanged(wxCommandEvent& evt)
    {
        if (IsModified())
        {
            m_grid->onEditChanged(GetValue());
        }
    }

    void onKillFocus(wxFocusEvent& evt)
    {
        wxWindow* focus_wnd = wxWindow::FindFocus();
        
        if (focus_wnd != NULL &&
            focus_wnd != (wxWindow*)m_grid &&
            !m_deleting)
        {
            m_last_key = -1;
            m_focus_lost = true;
            m_user_action = false;

            Destroy();
        }
    }

private:

    Grid* m_grid;
    int m_last_key;
    bool m_focus_lost;
    bool m_user_action;
    bool m_deleting;
    bool m_numbers_only;
    
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(GridTextCtrl, wxTextCtrl)
    EVT_KEY_DOWN(GridTextCtrl::onKeyDown)
    EVT_CHAR(GridTextCtrl::onChar)
    //EVT_MENU(50000, GridTextCtrl::onTextChanged)
    EVT_TEXT(-1, GridTextCtrl::onTextChanged)
    EVT_KILL_FOCUS(GridTextCtrl::onKillFocus)
END_EVENT_TABLE()




class GridChoiceCtrl : public wxChoice
{

public:

    GridChoiceCtrl(Grid* grid,
                   wxWindow* parent,
                   wxWindowID id,
                   const wxPoint& pos,
                   const wxSize& size,
                   int choice_count,
                   const wxString choices[],
                   long style)
            : wxChoice(parent,
                       id,
                       pos,
                       size,
                       choice_count,
                       choices,
                       style)
    {
        m_grid = grid;
        m_start_idx = -1;
        m_last_key = -1;
        m_focus_lost = false;
        m_user_action = true;
        m_deleting = false;
        m_search_idx = 0;
    }

    ~GridChoiceCtrl()
    {
        // attempt to tell grid to write out the value
        m_deleting = true;
        m_grid->onEditDestroy(GetValue(),
                              m_last_key,
                              m_focus_lost,
                              m_user_action);
    }

    wxString GetValue() const
    {
        int selection = GetSelection();
        if (selection == -1)
            return wxEmptyString;
            
        return GetString(selection);
    }

    void setStartIndex(int idx)
    {
        m_start_idx = idx;
    }

    void destroyNow(bool accept)
    {
        if (!accept)
        {
            m_last_key = WXK_ESCAPE;
        }

        m_user_action = false;
        Destroy();
    }

private:

    void onKeyDown(wxKeyEvent& event)
    {
        m_last_key = event.GetKeyCode();
        if (m_last_key == WXK_RETURN         ||
            m_last_key == WXK_NUMPAD_ENTER   ||
            m_last_key == WXK_TAB            ||
            m_last_key == WXK_LEFT           ||
            m_last_key == WXK_NUMPAD_LEFT    ||
            m_last_key == WXK_RIGHT          ||
            m_last_key == WXK_NUMPAD_RIGHT   ||
            m_last_key == WXK_UP             ||
            m_last_key == WXK_NUMPAD_UP      ||
            m_last_key == WXK_DOWN           ||
            m_last_key == WXK_NUMPAD_DOWN)
        {
            // make Shift-Enter go up instead of down
            if (m_last_key == WXK_RETURN ||
                m_last_key == WXK_NUMPAD_ENTER)
            {
                if (event.ShiftDown())
                {
                    m_last_key = WXK_UP;
                }
            }
            
            // translate numpad up/down to regular up/down
            if (m_last_key == WXK_NUMPAD_UP)
                m_last_key = WXK_UP;
            if (m_last_key == WXK_NUMPAD_DOWN)
                m_last_key = WXK_DOWN;

            // translate numpad left/right to regular left/right
            if (m_last_key == WXK_NUMPAD_LEFT)
                m_last_key = WXK_LEFT;
            if (m_last_key == WXK_NUMPAD_RIGHT)
                m_last_key = WXK_RIGHT;

            // make Shift-Tab go left instead of right
            if (m_last_key == WXK_TAB)
            {
                if (event.ShiftDown())
                {
                    m_last_key = WXK_LEFT;
                }
            }

            // make sure we set the focus back to the grid
            m_grid->SetFocus();

            Destroy();
            return;
        }
        else if (m_last_key == WXK_ESCAPE)
        {
            SetSelection(m_start_idx);
            
            wxCommandEvent event(wxEVT_COMMAND_CHOICE_SELECTED, GetId());
            event.SetInt(m_start_idx);
            event.SetString(GetString(m_start_idx));
            event.SetEventObject(this);
            
            GetEventHandler()->ProcessEvent(event);
            
            // make sure we set the focus back to the grid
            m_grid->SetFocus();

            Destroy();
            return;
        }
        
        event.Skip();
    }
    
    void onChar(wxKeyEvent& evt)
    {
        // do a lookup in the box when a key is pressed

        wxChar key_code = evt.GetKeyCode();
        key_code = wxToupper(key_code);
        
        int count = GetCount();
        int i;
        
        for (i = 0; i < count; ++i)
        {
            int idx = m_search_idx + i + 1;
            if (idx >= count)
                idx -= count;
                
            wxString s = GetString(idx);
            if (s.IsEmpty())
                continue;

            s.MakeUpper();
            wxChar ch = s.GetChar(0);
            
            if (ch == key_code)
            {
                if (GetSelection() == idx)
                    return;
                    
                m_search_idx = idx;
                SetSelection(idx);
                
                wxCommandEvent evt(wxEVT_COMMAND_CHOICE_SELECTED, GetId());
                evt.SetInt(idx);
                evt.SetString(GetString(idx));
                evt.SetEventObject(this);
                
                GetEventHandler()->ProcessEvent(evt);
                
                return;
            }
        }
    }

    void onKillFocus(wxFocusEvent& evt)
    {
        wxWindow* focus_wnd = wxWindow::FindFocus();

        if (focus_wnd != NULL &&
            focus_wnd != m_grid &&
            !m_deleting)
        {
            m_focus_lost = true;
            Destroy();
        }
    }

    void onMouseWheel(wxMouseEvent& evt)
    {
    }

private:

    Grid* m_grid;
    int m_start_idx;
    int m_last_key;
    bool m_focus_lost;
    bool m_user_action;
    bool m_deleting;

    int m_search_idx;
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(GridChoiceCtrl, wxChoice)
    EVT_KEY_DOWN(GridChoiceCtrl::onKeyDown)
    EVT_CHAR(GridChoiceCtrl::onChar)
    EVT_KILL_FOCUS(GridChoiceCtrl::onKillFocus)
    EVT_MOUSEWHEEL(GridChoiceCtrl::onMouseWheel)
END_EVENT_TABLE()




class GridComboCtrl : public wxComboBox
{

public:

    GridComboCtrl(Grid* grid,
                  wxWindow* parent,
                  wxWindowID id,
                  const wxPoint& pos,
                  const wxSize& size,
                  int choice_count,
                  const wxString choices[],
                  long style)
            : wxComboBox(parent,
                         id,
                         wxT(""),
                         pos,
                         size,
                         choice_count,
                         choices,
                         style)
    {
        m_grid = grid;
        m_start_idx = -1;
        m_last_key = -1;
        m_focus_lost = false;
        m_user_action = true;
        m_deleting = false;
    }

    ~GridComboCtrl()
    {
        // attempt to tell grid to write out the value
        m_deleting = true;
        m_grid->onEditDestroy(GetValue(),
                              m_last_key,
                              m_focus_lost,
                              m_user_action);
    }

    void setStartIndex(int idx)
    {
        m_start_idx = idx;
    }
    
    void destroyNow(bool accept)
    {
        if (!accept)
        {
            m_last_key = WXK_ESCAPE;
        }

        m_user_action = false;
        Destroy();
    }

private:

    void onKeyDown(wxKeyEvent& evt)
    {
        m_last_key = evt.GetKeyCode();
        if (m_last_key == WXK_RETURN         ||
            m_last_key == WXK_NUMPAD_ENTER   ||
            m_last_key == WXK_TAB            ||
            m_last_key == WXK_LEFT           ||
            m_last_key == WXK_NUMPAD_LEFT    ||
            m_last_key == WXK_RIGHT          ||
            m_last_key == WXK_NUMPAD_RIGHT   ||
            m_last_key == WXK_UP             ||
            m_last_key == WXK_NUMPAD_UP      ||
            m_last_key == WXK_DOWN           ||
            m_last_key == WXK_NUMPAD_DOWN)
        {
            // make Shift-Enter go up instead of down
            if (m_last_key == WXK_RETURN ||
                m_last_key == WXK_NUMPAD_ENTER)
            {
                if (evt.ShiftDown())
                {
                    m_last_key = WXK_UP;
                }
            }
            
            // translate numpad up/down to regular up/down
            if (m_last_key == WXK_NUMPAD_UP)
                m_last_key = WXK_UP;
            if (m_last_key == WXK_NUMPAD_DOWN)
                m_last_key = WXK_DOWN;

            // translate numpad left/right to regular left/right
            if (m_last_key == WXK_NUMPAD_LEFT)
                m_last_key = WXK_LEFT;
            if (m_last_key == WXK_NUMPAD_RIGHT)
                m_last_key = WXK_RIGHT;

            // make Shift-Tab go left instead of right
            if (m_last_key == WXK_TAB)
            {
                if (evt.ShiftDown())
                {
                    m_last_key = WXK_LEFT;
                }
            }

            // make sure we set the focus back to the grid
            m_grid->SetFocus();

            Destroy();
            return;
        }
        else if (m_last_key == WXK_ESCAPE)
        {
            SetSelection(m_start_idx);
            
            wxCommandEvent evt(wxEVT_COMMAND_CHOICE_SELECTED, GetId());
            evt.SetInt(m_start_idx);
            evt.SetString(GetString(m_start_idx));
            evt.SetEventObject(this);
            
            GetEventHandler()->ProcessEvent(evt);
            
            // make sure we set the focus back to the grid
            m_grid->SetFocus();

            Destroy();
            return;
        }
        
        evt.Skip();
    }

    void onKillFocus(wxFocusEvent& evt)
    {
        wxWindow* focus_wnd = wxWindow::FindFocus();

        if (focus_wnd != NULL &&
            focus_wnd != m_grid &&
            focus_wnd != this &&
            !m_deleting)
        {
            m_focus_lost = true;
            Destroy();
        }
    }

    void onMouseWheel(wxMouseEvent& evt)
    {
    }

private:

    Grid* m_grid;
    int m_start_idx;
    int m_last_key;
    bool m_focus_lost;
    bool m_user_action;
    bool m_deleting;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(GridComboCtrl, wxComboBox)
    EVT_KEY_DOWN(GridComboCtrl::onKeyDown)
    EVT_KILL_FOCUS(GridComboCtrl::onKillFocus)
    EVT_MOUSEWHEEL(GridComboCtrl::onMouseWheel)
END_EVENT_TABLE()




// Selection class implementation

Selection::Selection()
{
}

Selection::~Selection()
{
    clear();
}

bool Selection::getCellSelected(int row, int col)
{
    int sr, er, sc, ec;
    int start_row, start_col, end_row, end_col;

    for (std::vector<SelectionRect*>::iterator it = m_pieces.begin();
            it != m_pieces.end(); ++it)
    {
        sr = (*it)->m_start_row;
        sc = (*it)->m_start_col;
        er = (*it)->m_end_row == -1 ? MAX_SELECT : (*it)->m_end_row;
        ec = (*it)->m_end_col == -1 ? MAX_SELECT : (*it)->m_end_col;

        start_row = std::min(sr, er);
        start_col = std::min(sc, ec);
        end_row = std::max(sr, er);
        end_col = std::max(sc, ec);

        if (row >= start_row && col >= start_col &&
            row <= end_row && col <= end_col)
        {
            return true;
        }
    }
    return false;
}


int Selection::lookupColumnSelection(int col)
{
    int i = 0;
    for (std::vector<SelectionRect*>::iterator it = m_pieces.begin();
            it != m_pieces.end(); ++it)
    {
        // the entire column is not selected, move on
        if ((*it)->m_start_row != 0 || (*it)->m_end_row != -1)
        {
            i++;
            continue;
        }
        
        // the column comes before the selection, move on
        if (col < (*it)->m_start_col)
        {
            i++;
            continue;
        }
        
        if ((*it)->m_end_col == -1)
            return i;
            
        if (col <= (*it)->m_end_col)
            return i;

        i++;
    }

    return -1;
}


int Selection::lookupRowSelection(int row)
{
    int i = 0;
    for (std::vector<SelectionRect*>::iterator it = m_pieces.begin();
            it != m_pieces.end(); ++it)
    {
        // the entire row is not selected, move on
        if ((*it)->m_start_col != 0 || (*it)->m_end_col != -1)
        {
            i++;
            continue;
        }

        // the row comes before the selection, move on
        if (row < (*it)->m_start_row)
        {
            i++;
            continue;
        }

        if ((*it)->m_end_row == -1)
            return i;

        if (row <= (*it)->m_end_row)
            return i;

        i++;
    }

    return -1;
}

bool Selection::columnContainsSelection(int col)
{
    // NOTE: tests whether any cell in a column is selected, not just
    // if the whole column is selected; used for drawing column
    // selection highlight
    int sc, ec;
    int start_col, end_col;

    for (std::vector<SelectionRect*>::iterator it = m_pieces.begin();
            it != m_pieces.end(); ++it)
    {
        sc = (*it)->m_start_col;
        ec = (*it)->m_end_col == -1 ? MAX_SELECT : (*it)->m_end_col;

        start_col = std::min(sc, ec);
        end_col = std::max(sc, ec);

        if (col >= start_col && col <= end_col)
            return true;
    }

    return false;
}

bool Selection::rowContainsSelection(int row)
{
    // NOTE: tests whether any cell in a row is selected, not just
    // if the whole row is selected; used for drawing row
    // selection highlight
    int sr, er;
    int start_row, end_row;
        
    for (std::vector<SelectionRect*>::iterator it = m_pieces.begin();
            it != m_pieces.end(); ++it)
    {
        sr = (*it)->m_start_row;
        er = (*it)->m_end_row == -1 ? MAX_SELECT : (*it)->m_end_row;

        start_row = std::min(sr, er);
        end_row = std::max(sr, er);

        if (row >= start_row && row <= end_row)
            return true;
    }

    return false;
}

bool Selection::getColumnSelected(int col)
{
    return (lookupColumnSelection(col) != -1 ? true : false);
}

bool Selection::getRowSelected(int row)
{
    return (lookupRowSelection(row) != -1 ? true : false);
}




bool Selection::setColumnSelected(int col, bool selected)
{
    int idx = lookupColumnSelection(col);

    if (selected)
    {
        // if already selected, we are done
        if (idx != -1)
            return true;

        SelectionRect* sel = addSelection();
        sel->m_start_col = col;
        sel->m_end_col = col;
        sel->m_start_row = 0;
        sel->m_end_row = -1;
    }
    else
    {
        // if selection does not exist, we are done
        if (idx == -1)
            return true;

        removeSelection(m_pieces[idx]);
    }

    return true;
}




bool Selection::setRowSelected(int row, bool selected)
{
    int idx = lookupRowSelection(row);

    if (selected)
    {
        // if already selected, we are done
        if (idx != -1)
            return true;

        SelectionRect* sel = addSelection();
        sel->m_start_col = 0;
        sel->m_end_col = -1;
        sel->m_start_row = row;
        sel->m_end_row = row;
    }
    else
    {
        // if selection does not exist, we are done
        if (idx == -1)
            return true;

        removeSelection(m_pieces[idx]);
    }

    return true;
}




SelectionRect* Selection::addSelection()
{
    SelectionRect* r = new SelectionRect;
    r->m_start_row = 0;
    r->m_start_col = 0;
    r->m_end_row = 0;
    r->m_end_col = 0;
    m_pieces.push_back(r);
    return *m_pieces.rbegin();
}

int Selection::getSelectionCount()
{
    return m_pieces.size();
}

SelectionRect* Selection::getSelection(int idx)
{
    if (idx < 0 || idx >= (int)m_pieces.size())
        return NULL;

    return m_pieces[idx];
}


bool Selection::removeSelection(SelectionRect* sel_rect)
{
    std::vector<SelectionRect*>::iterator it;
    it = std::find(m_pieces.begin(), m_pieces.end(), sel_rect);
    if (it == m_pieces.end())
        return false;
    delete (*it);
    m_pieces.erase(it);
    return true;
}

void Selection::clear()
{
    std::vector<SelectionRect*>::iterator it;
    
    for (it = m_pieces.begin();
         it != m_pieces.end();
         ++it)
    {
        delete (*it);
    }

    m_pieces.clear();
}




// grid static members
wxBitmap Grid::m_bmp_checkbox_on = wxNullBitmap;
wxBitmap Grid::m_bmp_checkbox_off = wxNullBitmap;
wxBitmap Grid::m_bmp_checkbox_neutral = wxNullBitmap;
wxBitmap Grid::m_bmp_cursorrowmarker = wxNullBitmap;
wxBitmap Grid::m_bmp_ghostrowmarker = wxNullBitmap;



const int EndEditCleanupId = 23976;
const int ScrollTimerId = 23977;


BEGIN_EVENT_TABLE(Grid, wxNavigationEnabled<wxControl>)
    EVT_PAINT(Grid::onPaint)
    EVT_SIZE(Grid::onSize)
    EVT_SCROLLWIN(Grid::onScroll)
    EVT_MOUSE_EVENTS(Grid::onMouse)
    EVT_KEY_DOWN(Grid::onKeyDown)
    EVT_CHAR(Grid::onChar)
    EVT_ERASE_BACKGROUND(Grid::onEraseBackground)
    EVT_COMBOBOX(EditCtrlId, Grid::onComboSelectionChanged)
    EVT_CHOICE(EditCtrlId, Grid::onComboSelectionChanged)
    EVT_MENU(EndEditCleanupId, Grid::onEndEditCleanup)
    EVT_TIMER(ScrollTimerId, Grid::onScrollTimer)
END_EVENT_TABLE()



Grid::Grid(wxWindow* parent,
           wxWindowID id,
           const wxPoint& pos,
           const wxSize& size,
           long style)
{
    construct();
    Create(parent, id, pos, size, style);
}

Grid::Grid()
{
    construct();
}

void Grid::construct()
{
    if (!Grid::m_bmp_cursorrowmarker.Ok())
        Grid::m_bmp_cursorrowmarker = wxBitmap(xpm_cursorrowmarker);

    if (!Grid::m_bmp_ghostrowmarker.Ok())
        Grid::m_bmp_ghostrowmarker = wxBitmap(xpm_ghostrowmarker);

    if (!Grid::m_bmp_checkbox_on.Ok())
        Grid::m_bmp_checkbox_on = getCheckBoxBitmap(this, true);

    if (!Grid::m_bmp_checkbox_off.Ok())
        Grid::m_bmp_checkbox_off = getCheckBoxBitmap(this, false);

    m_destroying = false;
    m_model = NULL;
    m_gui_initialized = false;
    m_cursor_visible = true;
    m_control = NULL;
    m_drag_format = wxEmptyString;
    m_options = optSelect |
                optEdit |
                optColumnMove |
                optColumnResize |
                optRowResize |
                optHorzGridLines |
                optVertGridLines |
                optActivateHyperlinks;
    m_cursor_type = cursorNormal;
    m_dip_row_height = 0;
    m_row_height = 0;
    m_dip_rowlabel_width = 12;
    m_rowlabel_width = fromDIP(m_dip_rowlabel_width);
    m_border_type = wxNO_BORDER;
    m_bmp_alloc_width = 0;
    m_bmp_alloc_height = 0;

    m_overlay_font = wxFont(9, wxSWISS, wxNORMAL, wxNORMAL, false);

    m_base_color = kcl::getBaseColor();
    m_def_bgcolor.Set(255, 255, 255);
    m_def_fgcolor.Set(0,0,0);
    m_def_gridline_color = kcl::stepColor(m_base_color, 85);
    m_def_invalid_area_color.Set(255,255,255);
    
    m_def_greenbar_color = getDefaultGreenBarColor(kcl::getHighlightColor(), 195);
    /*
    // Windows Vista's highlight color is wretched
    int major, minor;
    ::wxGetOsVersion(&major, &minor);
    if (major == 6)
        m_def_greenbar_color = getDefaultGreenBarColor(wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION), 185);
         else
        m_def_greenbar_color = getDefaultGreenBarColor(kcl::getHighlightColor(), 195);
    */
    
    m_hc_barfg_color.Set(0,0,0);
    //m_hc_barbg_color.Set(225,225,255);
    m_hc_barbg_color = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    m_hc_cellfg_color = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
    m_hc_cellbg_color = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    //m_select_bgcolor.Set(160, 160, 214);
    //m_select_fgcolor.Set(0,0,0);
    m_select_fgcolor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
    m_select_bgcolor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

    m_ghostrow_text = wxEmptyString;
    m_overlay_text = wxEmptyString;
    m_last_edit_allowed = false;
    m_caption_editing = false;
    m_mask_paint = false;

    m_greenbar_interval = 0;

    // initialize empty cell
    m_empty_cell.fgcolor.Set(255,255,255);
    m_empty_cell.bgcolor.Set(255,255,255);
    m_empty_cell.m_null = false;
    m_empty_cell.m_strvalue = wxT("");
    m_empty_cell.m_type = Grid::typeCharacter;
    m_empty_cell.m_width = 10;
    m_empty_cell.m_scale = 0;
    m_empty_cell.m_intvalue = 0;
    m_empty_cell.m_dblvalue = 0.0;
    m_empty_cell.m_boolvalue = false;
    m_empty_cell.mask = 0;
    m_empty_cell.ctrltype = Grid::ctrltypeText;
    m_empty_cell.alignment = Grid::alignLeft;
    m_empty_cell.bitmap_alignment = Grid::alignLeft;
    m_empty_cell.text_wrap = Grid::wrapDefault;
    m_empty_cell.editable = false;
    m_empty_cell.visible = true;

    setModel(new GridMemModel);
}


Grid::~Grid()
{
    // we are destroying the grid, so set the m_destroying flag
    m_destroying = true;

    // destroy edit control, if it exists
    if (m_control)
        m_control->Destroy();

    // clear out view columns
    std::vector<ViewColumn*>::iterator it;
    for (it = m_viewcols.begin(); it != m_viewcols.end(); ++it)
        delete (*it);
    m_viewcols.clear();

    // release model reference
    if (m_model)
        m_model->unref();
}

void Grid::setDragFormat(const wxString& format)
{
    m_drag_format = format;
}

void Grid::setOptionState(int option_mask, bool state)
{
    if (state)
        m_options |= option_mask;
         else
        m_options &= ~option_mask;
}

bool Grid::getOptionState(int option)
{
    return (m_options & option) ? true : false;
}


void Grid::setBorderType(int border_type)
{
    m_border_type = border_type;

    if (m_gui_initialized)
    {
        // get present style
        int style = GetWindowStyleFlag();

        // remove any existing border type
        style &= ~wxBORDER_MASK;

        // add the new border type
        style |= m_border_type;

        // set our window's style flags back
        SetWindowStyleFlag(style);
        Refresh();
    }
}



bool Grid::Create(wxWindow* parent,
                  wxWindowID id,
                  const wxPoint& position,
                  const wxSize& size,
                  long style,
                  int visible_state)
{
    if (visible_state == -1)
        visible_state = stateVisible;

    wxPoint p(0,0);
    wxSize s(100, 100);

    if (position != wxDefaultPosition)
        p = position;
    if (size != wxDefaultSize)
        s = size;

    bool res = wxControl::Create(parent,
                                 id,
                                 p,
                                 s,
                                 wxCLIP_CHILDREN | wxHSCROLL | wxVSCROLL | wxWANTS_CHARS | m_border_type);
    if (!res)
    {
        return false;
    }

    m_visible_state = visible_state;
    SetBackgroundColour(wxColour(255,255,255));

    m_scroll_timer.SetOwner(this, ScrollTimerId);

    initGui();
    updateData();
    render();
    repaint();
    updateScrollbars();

    m_gui_initialized = true;

    return true;
}

void Grid::updateData()
{
    int col_count = m_model->getColumnCount();
    if (m_empty_row.m_coldata.size() != (size_t)col_count)
    {
        // initialize our empty row
        m_empty_row.m_coldata.resize(col_count, m_empty_cell);
        m_empty_row.m_blank = false;
        m_empty_row.m_model_row = -1;
    }


    if (!m_gui_initialized)
        return;
    if (m_row_height == 0)
        return;
    if (m_visible_state == stateDisabled)
        return;

    int vis_rows = getVisibleRowCount() + 1;
    int row, col;
    int model_row;
    IModelColumn* model_col;
    std::vector<IModelColumnPtr> model_cols;

    m_rowdata.clear();

    if (vis_rows == 0)
    {
        return;
    }

    m_row_count = m_model->getRowCount();
    
    if (m_row_count == 0 || m_row_offset >= m_row_count)
    {
        return;
    }

    m_rowdata.resize(vis_rows);



    CellProperties cellprops;

    for (col = 0; col < col_count; col++)
    {
        model_cols.push_back(m_model->getColumnInfo(col));
    }

    row = 0;
    model_row = m_row_offset;

    while (1)
    {
        if (!m_model->isRowValid(model_row))
        {
            m_rowdata.resize(row);
            m_row_count = model_row;

            updateScrollbars(wxVERTICAL);
            
            return;
        }

        if (m_model->getGroupBreak())
        {
            m_rowdata[row].m_blank = true;
            m_rowdata[row].m_model_row = -1;
            row++;
            if (row == vis_rows)
                break;
        }

        m_rowdata[row].m_blank = false;
        m_rowdata[row].m_model_row = model_row;
        m_rowdata[row].m_coldata.resize(col_count);


        for (col = 0; col < col_count; col++)
        {
            model_col = model_cols[col].p;

            CellData& cell_data = m_rowdata[row].m_coldata[col];
            
            /*
            cell_data.mask =    CellProperties::cpmaskFgColor |
                                CellProperties::cpmaskBgColor |
                                CellProperties::cpmaskCtrlType |
                                CellProperties::cpmaskAlignment |
                                CellProperties::cpmaskEditable |
                                CellProperties::cpmaskVisible |
                                CellProperties::cpmaskTextWrap;
            */
            cell_data.mask = 0;

            cell_data.bgcolor = wxNullColour;
            cell_data.fgcolor = wxNullColour;

            // set default cell properties
            cell_data.ctrltype = ctrltypeText;
            cell_data.editable = true;
            cell_data.visible = true;
            
            cell_data.m_type = model_col->getType();
            cell_data.m_width = model_col->getWidth();
            cell_data.m_scale = model_col->getScale();

            cell_data.text_wrap = Grid::wrapDefault;
            cell_data.alignment = Grid::alignDefault;
            cell_data.bitmap_alignment = Grid::alignLeft;

            m_model->getCellProperties(model_row, col, &cell_data);

            cell_data.m_null = false;

            if (!cell_data.m_null)
            {
                cell_data.m_null = m_model->isNull(model_row, col);
                
                // get cell data
                switch (cell_data.m_type)
                {
                    default:
                    case typeCharacter:
                        cell_data.m_strvalue = m_model->getCellString(model_row, col);
                        break;
                    case typeDate:
                    case typeDateTime:
                        cell_data.m_strvalue = m_model->getCellString(model_row, col);
                        break;
                    case typeDouble:
                        cell_data.m_dblvalue = m_model->getCellDouble(model_row, col);
                        break;
                    case typeInteger:
                        cell_data.m_intvalue = m_model->getCellInteger(model_row, col);
                        break;
                    case typeBoolean:
                        cell_data.m_boolvalue = m_model->getCellBoolean(model_row, col);
                        break;
                }
            }

            // get cell bitmap
            m_model->getCellBitmap(model_row,
                                   col,
                                   &cell_data.m_bitmap,
                                   &cell_data.bitmap_alignment);
        }

        row++;
        model_row++;

        if (model_row >= m_row_count)
            break;

        if (row == vis_rows)
            break;
    }

    if (row != m_rowdata.size())
    {
        m_rowdata.resize(row);
    }
}


void Grid::refresh(unsigned int refresh_level)
{
    if (!m_gui_initialized)
        return;

    if (refresh_level & refreshColumnView)
    {
        std::vector<ViewColumn*>::iterator it;
        int idx;
        for (it = m_viewcols.begin(); it != m_viewcols.end(); ++it)
        {
            idx = m_model->getColumnIndex((*it)->m_colname);
            if (idx == -1)
            {
                (*it)->m_modelcol = -1;
                continue;
            }

            (*it)->m_modelcol = idx;
        }

        if (refresh_level == refreshColumnView)
            return;
    }

    if (refresh_level & refreshScrollbars)
    {
        if (!(refresh_level & refreshPaint))
        {
            updateScrollbars(wxVERTICAL);
            updateScrollbars(wxHORIZONTAL);
        }

        if (refresh_level == refreshScrollbars)
            return;
    }

    if (refresh_level & refreshData)
    {
        updateData();

        if (refresh_level == refreshData)
            return;
    }

    render();

    if (refresh_level & refreshPaint)
        repaint();

    updateScrollbars();
}


void Grid::refreshColumn(unsigned int refresh_level, int column_idx)
{
    if (refresh_level & refreshData)
    {
        updateData();
    }

    wxRect rect;
    getCellRect(0, column_idx, &rect);
    rect.SetY(0);
    rect.SetHeight(m_cliheight);

    render(&rect);

    if (refresh_level & refreshPaint)
    {
        repaint();
    }
}



void Grid::setVisibleState(int visible_state)
{
    m_visible_state = visible_state;

    Enable(m_visible_state == stateVisible ? true : false);

    // we have to do this here because the lightenMemDC() function selects a
    // different (lightened) bitmap and never sets it back to m_bmp anywhere
    if (m_visible_state == stateVisible)
        m_memdc.SelectObject(m_bmp);

    render();
    repaint();
}

int Grid::getVisibleState()
{
    return m_visible_state;
}


void Grid::setCursorType(int cursor_type)
{
    m_cursor_type = cursor_type;

    render();
    repaint();
}

int Grid::getCursorType()
{
    return m_cursor_type;
}

void Grid::setCursorVisible(bool show)
{
    m_cursor_visible = show;
}

bool Grid::SetFont(const wxFont& _font)
{
    if (!wxControl::SetFont(_font))
        return false;

    m_font = _font;
    initFont();

    return true;
}

bool Grid::setCaptionFont(const wxFont& font)
{
    m_caption_font = font;
    return true;
}

bool Grid::setOverlayFont(const wxFont& font)
{
    m_overlay_font = font;
    return true;
}

void Grid::initFont()
{
    m_underlined_font = m_font;
    m_underlined_font.SetUnderlined(true);
    
    if (m_bmp.Ok())
    {
        m_memdc.SetFont(m_font);

        int x, y;
        m_memdc.GetTextExtent(wxT("ABCDXHM"), &x, &y);

        m_row_height = y + fromDIP(6);
        m_dip_row_height = toDIP(m_row_height);

        m_dip_header_height = m_dip_row_height - 1;
        m_header_height = fromDIP(m_dip_header_height);
    }
    else
    {
        // bitmap could not be created, so set some default dumb values
        // so that there is no division by zero
        m_dip_row_height = 18;
        m_row_height = fromDIP(m_dip_row_height);

        m_dip_header_height = 18;
        m_header_height = fromDIP(m_dip_header_height);
    }
}



void Grid::createDefaultView()
{
    //  create default view columns

    std::vector<ViewColumn*>::iterator it;
    for (it = m_viewcols.begin(); it != m_viewcols.end(); ++it)
        delete (*it);
    m_viewcols.clear();
    m_viewcol_lookup.clear();

    kcl::IModelColumnPtr col;

    ViewColumn* vc;
    int col_count = m_model->getColumnCount();

    m_total_width = 0;
    for (int i = 0; i < col_count; i++)
    {
        col = m_model->getColumnInfo(i);

        vc = new ViewColumn;
        vc->m_fgcolor = wxNullColour;
        vc->m_bgcolor = wxNullColour;
        vc->m_caption = col->getName();
        vc->m_colname = col->getName();
        vc->m_dip_pixwidth = 80;
        vc->m_pixwidth = fromDIP(vc->m_dip_pixwidth);
        vc->m_prop_size = 0;
        vc->m_modelcol = i;
        vc->m_draw = true;
        vc->m_shaded = false;
        vc->m_separator = false;
        vc->m_resizable = true;
        vc->m_text_wrap = wrapDefault;
        vc->m_alignment = alignDefault;
        vc->m_bitmap_alignment = alignLeft;

        m_model->getColumnBitmap(i, &(vc->m_bitmap), &(vc->m_bitmap_alignment));

        m_total_width += vc->m_pixwidth;
        m_viewcols.push_back(vc);
    }
}


void Grid::setModel(IModelPtr pModel)
{
    if (m_model)
    {
        m_model->unref();
    }

    m_model = pModel;
    if (m_model)
    {
        m_model->ref();
        m_model->initModel(this);

        hideAllColumns();
      
        m_cursor_row = 0;
        m_cursor_col = 0;
        m_cursor_modelcol = 0;

        m_model->onCursorRowChanged();

        m_row_count = 0;
        m_row_offset = 0;
    }
}

void Grid::reset()
{
    m_row_count = 0;
    m_row_offset = 0;
    m_cursor_row = 0;
    m_cursor_col = 0;
    m_cursor_modelcol = 0;
    m_selection.clear();

    m_model->reset();

    m_viewcol_lookup.clear();

    if (m_viewcols.size() > 0)
    {
        m_cursor_modelcol = m_viewcols[m_cursor_col]->m_modelcol;
    }
}




IModelPtr Grid::getModel()
{
    return m_model;
}

void Grid::refreshModel()
{
    if (m_model != NULL)
    {
        m_model->refresh();
    }
}

int Grid::getColumnViewIdx(int model_col)
{
    int idx = 0;
    std::vector<ViewColumn*>::iterator col_it;
    for (col_it = m_viewcols.begin(); col_it != m_viewcols.end(); ++col_it)
    {
        if ((*col_it)->m_modelcol == model_col)
            return idx;
        idx++;
    }

    return -1;
}


int Grid::getColumnViewIdxByName(const wxString& col_name)
{
    if (m_viewcol_lookup.size() != m_viewcols.size())
    {
        m_viewcol_lookup.clear();

        // create a column lookup cache
        int idx = 0;
        std::vector<ViewColumn*>::iterator col_it;
        for (col_it = m_viewcols.begin();
             col_it != m_viewcols.end();
             ++col_it)
        {
            wxString s = (*col_it)->m_colname;
            s.MakeUpper();
            // only insert the first instance of the column
            if (m_viewcol_lookup.find(s) == m_viewcol_lookup.end())
            {
                m_viewcol_lookup[s] = idx;
            }
            idx++;
        }
    }

    wxString s = col_name;
    s.MakeUpper();
    
    wxStringToColIdxHashMap::iterator it;
    it = m_viewcol_lookup.find(s);
    if (it == m_viewcol_lookup.end())
        return -1;

    return it->second;
}


int Grid::getColumnModelIdx(int col)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return -1;

    return m_viewcols[col]->m_modelcol;
}

int Grid::getColumnModelIdxByName(const wxString& col_name)
{
    return m_model->getColumnIndex(col_name);
}

void Grid::hideColumn(int col)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return;

    delete m_viewcols[col];
    m_viewcols.erase(m_viewcols.begin() + col);
    m_viewcol_lookup.clear();


    // since a view column was hidden, the m_cursor_modelcol
    // member might have changed
    if (m_cursor_col >= 0 && m_cursor_col < (int)m_viewcols.size())
    {
        m_cursor_modelcol = m_viewcols[m_cursor_col]->m_modelcol;
    }
    else
    {
        m_cursor_modelcol = 0;
    }


    GridEvent evt;
    evt.SetColumn(col);
    evt.SetUserEvent(false);
    fireEvent(wxEVT_KCLGRID_COLUMN_HIDE, evt);
}


int Grid::insertColumnSeparator(int position)
{
    m_viewcol_lookup.clear();

    ViewColumn* vc = new ViewColumn;
    vc->m_fgcolor = wxNullColour;
    vc->m_bgcolor = wxNullColour;
    vc->m_alignment = alignDefault;
    vc->m_bitmap_alignment = alignLeft;
    vc->m_text_wrap = wrapDefault;
    vc->m_colname = wxT("");
    vc->m_prop_size = 0;
    vc->m_draw = true;
    vc->m_shaded = false;
    vc->m_separator = true;
    vc->m_resizable = true;
    vc->m_modelcol = -1;
    vc->m_dip_pixwidth = 100;
    vc->m_pixwidth = fromDIP(vc->m_dip_pixwidth);
    vc->m_caption = wxT("");

    if (position >= (int)m_viewcols.size())
        position = -1;

    if (position != -1)
    {
        m_viewcols.insert(m_viewcols.begin() + position, vc);
        return position;
    }

    m_viewcols.push_back(vc);

    // since a view column was inserted, the m_cursor_modelcol
    // member might have changed
    if (m_cursor_col >= 0 && m_cursor_col < (int)m_viewcols.size())
        m_cursor_modelcol = m_viewcols[m_cursor_col]->m_modelcol;

    return m_viewcols.size()-1;
}   

int Grid::insertColumn(int position, int modelcol_idx)
{
    m_viewcol_lookup.clear();

    kcl::IModelColumnPtr col = m_model->getColumnInfo(modelcol_idx);

    ViewColumn* vc = new ViewColumn;
    vc->m_fgcolor = wxNullColour;
    vc->m_bgcolor = wxNullColour;
    vc->m_colname = col->getName();
    vc->m_prop_size = 0;
    vc->m_draw = true;
    vc->m_shaded = false;
    vc->m_separator = false;
    vc->m_resizable = true;
    vc->m_text_wrap = wrapDefault;
    vc->m_alignment = alignDefault;
    vc->m_bitmap_alignment = alignLeft;
    vc->m_modelcol = modelcol_idx;
    vc->m_dip_pixwidth = 100;
    vc->m_pixwidth = fromDIP(vc->m_dip_pixwidth);
    vc->m_caption = col->getName();
    
    /*
    vc->m_caption.MakeLower();
    if (!vc->m_caption.IsEmpty())
        vc->m_caption.SetChar(0, wxToupper(vc->m_caption.GetChar(0)));
    */
    
    m_model->getColumnBitmap(modelcol_idx,
                             &(vc->m_bitmap),
                             &(vc->m_bitmap_alignment));

    if (position >= 0 && position <= (int)m_viewcols.size())
    {
        m_viewcols.insert(m_viewcols.begin() + position, vc);
        return position;
    }

    m_viewcols.push_back(vc);

    // since a view column was inserted, the m_cursor_modelcol
    // member might have changed
    if (m_cursor_col >= 0 && m_cursor_col < (int)m_viewcols.size())
        m_cursor_modelcol = m_viewcols[m_cursor_col]->m_modelcol;

    return m_viewcols.size()-1;
}


void Grid::setColumn(int position, int modelcol_idx)
{
    if (position < 0 || position >= (int)m_viewcols.size())
        return;

    if (modelcol_idx < 0 || modelcol_idx >= m_model->getColumnCount())
        return;

    // remove old lookup entry

    m_viewcol_lookup.clear();

    kcl::IModelColumnPtr col = m_model->getColumnInfo(modelcol_idx);

    ViewColumn* vc = m_viewcols[position];
    vc->m_modelcol = modelcol_idx;
    vc->m_caption = col->getName();
    vc->m_colname = col->getName();
    
    /*
    vc->m_caption.MakeLower();
    if (!vc->m_caption.IsEmpty())
    {
        vc->m_caption.SetChar(0, wxToupper(vc->m_caption.GetChar(0)));
    }
    */

    m_model->getColumnBitmap(modelcol_idx,
                             &(vc->m_bitmap),
                             &(vc->m_bitmap_alignment));
}


void Grid::hideAllColumns()
{
    // clear out view columns
    std::vector<ViewColumn*>::iterator it;
    for (it = m_viewcols.begin(); it != m_viewcols.end(); ++it)
        delete (*it);
        
    m_viewcols.clear();
    m_viewcol_lookup.clear();
}


int Grid::getCursorColumn()
{
    return m_cursor_col;
}

int Grid::getCursorRow()
{
    return m_cursor_row;
}

int Grid::createModelColumn(int position,
                            const wxString& name,
                            int type,
                            int width,
                            int scale)
{
    return m_model->createColumn(position, name, type, width, scale);
}

bool Grid::setModelColumnProperties(int position, kcl::CellProperties* props)
{
    if (!props)
        return false;

    return m_model->setColumnProperties(position, props);
}


static void copyCellProps(kcl::CellProperties* dest, kcl::CellProperties* src)
{
    if (src->mask & kcl::CellProperties::cpmaskCtrlType)
    {
        dest->ctrltype = src->ctrltype;
        dest->mask |= kcl::CellProperties::cpmaskCtrlType;
    }
    if (src->mask & kcl::CellProperties::cpmaskFgColor)
    {
        dest->fgcolor = src->fgcolor;
        dest->mask |= kcl::CellProperties::cpmaskFgColor;
    }
    if (src->mask & kcl::CellProperties::cpmaskBgColor)
    {
        dest->bgcolor = src->bgcolor;
        dest->mask |= kcl::CellProperties::cpmaskBgColor;
    }
    if (src->mask & kcl::CellProperties::cpmaskAlignment)
    {
        dest->alignment = src->alignment;
        dest->mask |= kcl::CellProperties::cpmaskAlignment;
    }
    if (src->mask & kcl::CellProperties::cpmaskBitmapAlignment)
    {
        dest->bitmap_alignment = src->bitmap_alignment;
        dest->mask |= kcl::CellProperties::cpmaskBitmapAlignment;
    }
    if (src->mask & kcl::CellProperties::cpmaskCbChoices)
    {
        dest->cbchoices = src->cbchoices;
        dest->mask |= kcl::CellProperties::cpmaskCbChoices;
    }
    if (src->mask & kcl::CellProperties::cpmaskEditable)
    {
        dest->editable = src->editable;
        dest->mask |= kcl::CellProperties::cpmaskEditable;
    }
    if (src->mask & kcl::CellProperties::cpmaskVisible)
    {
        dest->visible = src->visible;
        dest->mask |= kcl::CellProperties::cpmaskVisible;
    }
    if (src->mask & kcl::CellProperties::cpmaskTextWrap)
    {
        dest->text_wrap = src->text_wrap;
        dest->mask |= kcl::CellProperties::cpmaskTextWrap;
    }
    if (src->mask & kcl::CellProperties::cpmaskHyperlink)
    {
        dest->hyperlink = src->hyperlink;
        dest->mask |= kcl::CellProperties::cpmaskHyperlink;
    }
}

bool Grid::setCellProperties(int row, 
                             int model_col,
                             kcl::CellProperties* props)
{
    if (!props)
        return false;

    bool result;
    result = m_model->setCellProperties(row, model_col, props);

    int rowdata_row = row - m_row_offset;
    if (rowdata_row >= 0 && rowdata_row < (int)m_rowdata.size())
    {
        if (model_col < (int)m_rowdata[rowdata_row].m_coldata.size())
        {
            copyCellProps(&m_rowdata[rowdata_row].m_coldata[model_col], props);
        }
    }

    return result;
}

void Grid::getCellProperties(int row,
                             int model_col,
                             kcl::CellProperties* props)
{
    m_model->getCellProperties(row, model_col, props);
}

void Grid::insertRow(int position)
{
    int newrow_pos;
    
    newrow_pos = m_model->insertRow(position);

    m_row_count++;
}

bool Grid::deleteRow(int position)
{
    if (position < 0 || position >= getRowCount())
        return false;

    if (isEditing())
    {
        endEdit(false);
    }

    bool res = m_model->deleteRow(position);
    
    if (!res)
    {
        return false;
    }

    m_row_count--;

    if (m_row_offset > position)
        m_row_offset--;

    if (m_cursor_row > position)
        m_cursor_row--;

    if (m_cursor_row >= m_row_count && m_row_count > 0)
        m_cursor_row = m_row_count-1;

    return true;
}

bool Grid::deleteAllRows()
{
    if (isEditing())
    {
        endEdit(false);
    }


    bool res = m_model->deleteAllRows();
    
    if (!res)
    {
        return false;
    }

    m_row_count = 0;
    m_row_offset = 0;
    m_cursor_row = 0;

    return true;
}

int Grid::getRowCount()
{
    // note that this return value is not always accurate.  It depends on
    // the model.  Some model's may choose not to report the true rowcount,
    // because of performance reasons.  In this case, the grid will choose to
    // give it's internal row count. This is the number of rows that the grid
    // has discovered thus far. However, if the model does return the exact row
    // count, it will be passed along precisely.

    if (!m_model)
        return 0;
        
    int rc = m_model->getRowCount();
    if (rc == -1)
    {
        // return our row count
        return getKnownRowCount();
    }

    return rc;
}


int Grid::getKnownRowCount()
{
    return m_row_count;
}

int Grid::getColumnCount()
{
    return m_viewcols.size();
}

int Grid::getVisibleRowCount()
{
    if (m_row_height == 0)
        return 0;

    return (m_cliheight - m_header_height) / m_row_height;
}

int Grid::getVisibleGroupBreakCount()
{
    return m_row_break_count;
}

void Grid::setRowHeight(int row_height)
{
    if (row_height == -1)
    {
        if (!m_gui_initialized)
            return;

        initFont();
    }

    m_dip_row_height = row_height;
    m_row_height = fromDIP(row_height);

    GridEvent evt;
    evt.SetInt(row_height);
    evt.SetUserEvent(false);
    fireEvent(wxEVT_KCLGRID_ROW_RESIZE, evt);
}

int Grid::getRowHeight()
{
    return m_dip_row_height;
}



bool Grid::getRowRect(int row, wxRect& rect)
{
    int rowdata_size = m_rowdata.size();
    int y = -1;
    int i, r = 0;

    // we go one more than the rowdata size to make ghost
    // row cursors work properly
    for (i = 0; i <= rowdata_size; ++i)
    {
        if (i < rowdata_size)
        {
            if (m_rowdata[i].m_blank)
                continue;
        }

        if (m_row_offset+r == row)
        {
            y = (i*m_row_height) + m_header_height;
            if (y > m_cliheight)
                return false;

            break;
        }
        r++;
    }

    if (y == -1)
        return false;

    rect.x = m_rowlabel_width;
    rect.y = y;
    rect.width = m_cliwidth - rect.x;
    rect.height = m_row_height;

    return true;
}


bool Grid::getColumnRect(int col, wxRect& rect)
{
    for (std::vector<ColumnRange>::iterator it = m_colranges.begin();
            it != m_colranges.end(); ++it)
    {
        if (col == it->viewcol_idx && it->viewcol)
        {
            rect.x = it->xoff;
            rect.y = m_header_height;
            rect.width = it->viewcol->m_pixwidth;
            rect.height = m_cliheight - rect.y;
            return true;
        }
    }
    
    return false;
}
    



void Grid::setRowData(int row, intptr_t data)
{
    m_model->setRowData(row, data);
}

intptr_t Grid::getRowData(int row)
{
    return m_model->getRowData(row);
}


void Grid::setGhostRowText(const wxString& new_value)
{
    m_ghostrow_text = new_value;
}

wxString Grid::getGhostRowText()
{
    return m_ghostrow_text;
}


void Grid::setOverlayText(const wxString& new_value)
{
    m_overlay_text = new_value;
}

wxString Grid::getOverlayText()
{
    return m_overlay_text;
}


void Grid::getColumnBitmap(int model_col,
                           wxBitmap* bitmap,
                           int* alignment)
{
    m_model->getColumnBitmap(model_col, bitmap, alignment);
}

void Grid::setColumnBitmap(int model_col,
                           const wxBitmap& bitmap,
                           int alignment)
{
    m_model->setColumnBitmap(model_col, bitmap, alignment);

    
    std::vector<ViewColumn*>::iterator it;
    for (it = m_viewcols.begin(); it != m_viewcols.end(); ++it)
    {
        if ((*it)->m_modelcol == model_col)
        {
            m_model->getColumnBitmap(model_col,
                                     &((*it)->m_bitmap),
                                     &((*it)->m_bitmap_alignment));
        }
    }
}


void Grid::setCellBitmap(int row,
                         int model_col,
                         const wxBitmap& bitmap,
                         int alignment)
{
    if (alignment == kcl::Grid::alignDefault)
    {
        CellProperties cell_props;
        m_model->getCellProperties(row, model_col, &cell_props);

        // if the cell's bitmap alignment is specified, use it
        if (cell_props.mask & CellProperties::cpmaskBitmapAlignment)
        {
            alignment = cell_props.bitmap_alignment;
        }
        else
        {
            alignment = kcl::Grid::alignLeft;
        }
    }
    
    m_model->setCellBitmap(row, model_col, bitmap, alignment);
}

void Grid::setCellString(int row,
                         int model_col,
                         const wxString& value)
{
    CellProperties cell_props;
    m_model->getCellProperties(row, model_col, &cell_props);

    // if it is a combo, do a lookup
    if (cell_props.ctrltype == ctrltypeDropList ||
        cell_props.ctrltype == ctrltypeComboBox)
    {
        // first, try to do a lookup of the string in the combo choices
        int idx = 0;
        std::vector<wxString>::iterator it;
        for (it = cell_props.cbchoices.begin();
             it != cell_props.cbchoices.end(); ++it)
        {
            if (*it == value)
            {
                setCellComboSel(row, model_col, idx);
                return;
            }

            idx++;
        }
        
        // if the cell is a combobox, we need to set the combobox's text
        // in addition to setting the text in the model below
        if (cell_props.ctrltype == ctrltypeComboBox &&
            m_control != NULL)
        {
            GridComboCtrl* ctrl = (GridComboCtrl*)m_control;
            ctrl->SetValue(value);
            ctrl->SetInsertionPointEnd();
        }    
    }

    m_model->setCellString(row, model_col, value);
}

void Grid::setCellDouble(int row, int model_col, double value)
{
    m_model->setCellDouble(row, model_col, value);
}

void Grid::setCellInteger(int row, int model_col, int value)
{
    m_model->setCellInteger(row, model_col, value);
}

void Grid::setCellBoolean(int row, int model_col, bool value)
{
    m_model->setCellBoolean(row, model_col, value);
}

void Grid::setCellComboSel(int row, int model_col, int sel)
{
    m_model->setCellComboSel(row, model_col, sel);

    CellProperties cell_props;
    m_model->getCellProperties(row, model_col, &cell_props);
    if (sel >= 0 && sel < (int)cell_props.cbchoices.size())
    {
        IModelColumnPtr col_info;
        col_info = m_model->getColumnInfo(model_col);
        if (col_info)
        {
            switch (col_info->getType())
            {
                case typeCharacter:
                    m_model->setCellString(row,
                                           model_col,
                                           cell_props.cbchoices[sel]);
                    break;
                case typeDouble:
                    m_model->setCellDouble(row,
                                           model_col,
                                           wxAtof(cell_props.cbchoices[sel]));
                    break;
                case typeInteger:
                    m_model->setCellInteger(row,
                                            model_col,
                                            wxAtoi(cell_props.cbchoices[sel]));
                    break;
                case typeDate:
                case typeDateTime:
                case typeBoolean:
                    break;
            }
        }
    }
}

wxString Grid::getCellString(int row,
                             int model_col)
{
    wxString s = m_model->getCellString(row, model_col);
    return s;
}

void Grid::getCellBitmap(int row,
                         int model_col,
                         wxBitmap* bitmap,
                         int* alignment)
{
    m_model->getCellBitmap(row, model_col, bitmap, alignment);
}

double Grid::getCellDouble(int row, int model_col)
{
    return m_model->getCellDouble(row, model_col);
}

int Grid::getCellInteger(int row, int model_col)
{
    return m_model->getCellInteger(row, model_col);
}

bool Grid::getCellBoolean(int row, int model_col)
{
    return m_model->getCellBoolean(row, model_col);
}

int Grid::getCellComboSel(int row, int model_col)
{
    return m_model->getCellComboSel(row, model_col);
}


// this method will resize a column or columns in the grid based upon the
// data in the column itself.  If -1 is passed to the resize_col parameter,
// all columns will be resized.  Otherwise, only the column indicated
// by the resize_col parameter will be resized

void Grid::autoColumnResize(int resize_col)
{
    std::vector<int> columns;

    if (resize_col == -1)
    {
        int col_count = m_viewcols.size();
        for (int i = 0; i < col_count; ++i)
            columns.push_back(i);
    }
    else
    {
        if (resize_col >= 0 && resize_col < (int)m_viewcols.size())
            columns.push_back(resize_col);
    }

    autoColumnResizeInternal(columns);
}


void Grid::autoColumnResizeInternal(std::vector<int>& columns)
{
    IModelPtr model = getModel();

    int row = 0;

    wxCoord w, h;
    std::vector<int> col_sizes;
    std::vector<int>::iterator it;
    wxString text;

    col_sizes.resize(columns.size());

    // get the width of the field headings
    int i = 0;
    for (it = columns.begin(); it != columns.end(); ++it)
    {
        m_memdc.GetTextExtent(getColumnCaption(*it).c_str(), &w, &h);
        col_sizes[i] = w;
        ++i;
    }

    // now take a 100-row sample record set to find correct column width
    for (row = m_row_offset; row < m_row_offset+100; row++)
    {
        if (row >= m_row_count)
            break;

        i = 0;
        for (it = columns.begin(); it != columns.end(); ++it)
        {
            int model_col = getColumnModelIdx(*it);
            if (model_col == -1)
            {
                col_sizes[i] = -1;
                ++i;
                continue;
            }

            m_memdc.GetTextExtent(model->getCellString(row, model_col).c_str(), &w, &h);

            if (w > col_sizes[i])
                col_sizes[i] = w;
            ++i;
        }
    }

    // set the column widths to the new size
    
    i = 0;
    for (it = columns.begin(); it != columns.end(); ++it)
    {
        if (col_sizes[i] == -1)
        {
            ++i;
            continue;
        }
        
        if (!m_viewcols[*it]->m_resizable)
        {
            ++i;
            continue;
        }
        
        if (col_sizes[i] > fromDIP(980))
            col_sizes[i] = fromDIP(980);

        if (m_viewcols[*it]->m_pixwidth != col_sizes[i]+20)
        {
            m_viewcols[*it]->m_pixwidth = col_sizes[i]+20;
            m_viewcols[*it]->m_dip_pixwidth = toDIP(m_viewcols[*it]->m_pixwidth);

            GridEvent evt;
            evt.SetColumn(*it);
            evt.SetInt(m_viewcols[*it]->m_dip_pixwidth);
            evt.SetUserEvent(false);
            fireEvent(wxEVT_KCLGRID_COLUMN_RESIZE, evt);
        }

        ++i;
    }

    updateData();
    render();
    updateScrollbars(wxHORIZONTAL);
    repaint();
}


void Grid::setRowLabelSize(int new_value)
{
    m_dip_rowlabel_width = new_value;
    m_rowlabel_width = fromDIP(new_value);
}

int Grid::getRowLabelSize()
{
    return m_dip_rowlabel_width;
}


void Grid::setHeaderSize(int new_val)
{
    m_dip_header_height = new_val;
    m_header_height = fromDIP(new_val);
}

int Grid::getHeaderSize()
{
    return m_dip_header_height;
}


void Grid::setColumnSize(int col, int pixwidth)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return;

    m_viewcols[col]->m_dip_pixwidth = pixwidth;
    m_viewcols[col]->m_pixwidth = fromDIP(m_viewcols[col]->m_dip_pixwidth);

    GridEvent evt;
    evt.SetColumn(col);
    evt.SetInt(pixwidth);
    evt.SetUserEvent(false);
    fireEvent(wxEVT_KCLGRID_COLUMN_RESIZE, evt);
}

void Grid::setColumnProportionalSize(int col, int new_val)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return;
    m_viewcols[col]->m_prop_size = new_val;
}

void Grid::setColumnAlignment(int col, int alignment)
{    
    if (col < 0 || col >= (int)m_viewcols.size())
        return;
    m_viewcols[col]->m_alignment = alignment;
}

void Grid::setColumnTextWrapping(int col, int new_value)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return;
    m_viewcols[col]->m_text_wrap = new_value;
}

void Grid::setColumnResizable(int col, bool new_value)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return;
    m_viewcols[col]->m_resizable = new_value;
}

void Grid::setColumnCaption(int col, const wxString& caption)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return;

    if (caption.IsEmpty())
    {
        ViewColumn* vc = m_viewcols[col];
        vc->m_caption = vc->m_colname;
        
        /*
        vc->m_caption.MakeLower();
        if (!vc->m_caption.IsEmpty())
        {
            vc->m_caption.SetChar(0, wxToupper(vc->m_caption.GetChar(0)));
        }
        */
    }
    else
    {
        m_viewcols[col]->m_caption = caption;
    }
}

void Grid::setColumnShaded(int col, bool shaded)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return;
    m_viewcols[col]->m_shaded = shaded;
}

void Grid::setColumnColors(int col, const wxColor& fg, const wxColor& bg)
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return;
    m_viewcols[col]->m_fgcolor = fg;
    m_viewcols[col]->m_bgcolor = bg;
}

void Grid::setDefaultBackgroundColor(const wxColor& bg)
{
    m_def_bgcolor = bg;
}

void Grid::setDefaultForegroundColor(const wxColor& fg)
{
    m_def_fgcolor = fg;
}

void Grid::setSelectBackgroundColor(const wxColor& bg)
{
    m_select_bgcolor = bg;
}

void Grid::setSelectForegroundColor(const wxColor& fg)
{
    m_select_fgcolor = fg;
}

void Grid::setDefaultLineColor(const wxColor& lc)
{
    m_def_gridline_color = lc;

    m_gridline_pen.SetColour(m_def_gridline_color);
    m_gridline_pen.SetStyle(wxSOLID);
}

void Grid::setDefaultInvalidAreaColor(const wxColor& iac)
{
    m_def_invalid_area_color = iac;
}

void Grid::setGreenBarColor(const wxColor& gbc)
{
    m_def_greenbar_color = gbc;
}

void Grid::setGreenBarInterval(int new_value)
{
    m_greenbar_interval = new_value;
}

int Grid::getColumnSize(int col) const
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return -1;

    return m_viewcols[col]->m_dip_pixwidth;
}

int Grid::getColumnProportionalSize(int col) const
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return -1;

    return m_viewcols[col]->m_prop_size;
}

int Grid::getColumnAlignment(int col) const
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return -1;

    return m_viewcols[col]->m_alignment;
}

int Grid::getColumnTextWrapping(int col) const
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return false;

    return m_viewcols[col]->m_text_wrap;
}

wxString Grid::getColumnCaption(int col) const
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return wxEmptyString;

    return m_viewcols[col]->m_caption;
}

bool Grid::getColumnShaded(int col) const
{
    if (col < 0 || col >= (int)m_viewcols.size())
        return false;
    return m_viewcols[col]->m_shaded;
}

void Grid::getColumnColors(int col, wxColor& fg, wxColor& bg) const
{
    bg = wxNullColour;
    fg = wxNullColour;

    if (col < 0 || col >= (int)m_viewcols.size())
        return;

    bg = m_viewcols[col]->m_bgcolor;
    fg = m_viewcols[col]->m_fgcolor;
}

int Grid::getColumnDefaultAlignment(int col)
{
    int model_colidx = getColumnModelIdx(col);
    if (model_colidx == -1)
        return alignLeft;
    
    IModelPtr model = getModel();
    if (model.isNull())
        return alignLeft;
    
    IModelColumnPtr model_col = model->getColumnInfo(model_colidx);
    if (model_col.isNull())
        return alignLeft;
    
    int type = model_col->getType();
    switch (type)
    {
        default:
            return alignLeft;
        case typeDouble:
        case typeInteger:
            return alignRight;
        case typeDate:
        case typeDateTime:
        case typeBoolean:
            return alignCenter;
    }

    return alignLeft;
}

wxColor Grid::getDefaultBackgroundColor() const
{
    return m_def_bgcolor;
}

wxColor Grid::getDefaultForegroundColor() const
{
    return m_def_fgcolor;
}

wxColor Grid::getDefaultLineColor() const
{
    return m_def_gridline_color;
}

wxColor Grid::getGreenBarColor() const
{
    return m_def_greenbar_color;
}

int Grid::getGreenBarInterval() const
{
    return m_greenbar_interval;
}

wxColor Grid::getDefaultInvalidAreaColor() const
{
    return m_def_invalid_area_color;
}

bool Grid::isCellSelected(int row, int col)
{
    return m_selection.getCellSelected(row, col);
}

bool Grid::isColumnSelected(int col)
{
    return m_selection.getColumnSelected(col);
}

bool Grid::setColumnSelected(int col, bool selected)
{
    return m_selection.setColumnSelected(col, selected);
}

bool Grid::setRowSelected(int row, bool selected)
{
    return m_selection.setRowSelected(row, selected);
}

void Grid::selectAll()
{
    if (isEditing())
    {
        if (m_control->IsKindOf(CLASSINFO(GridTextCtrl)))
        {
            GridTextCtrl* ctrl = (GridTextCtrl*)m_control;
            ctrl->SetSelection(-1, -1);
            return;
        }
        
        endEdit(true);
    }
    
    clearSelection();

    kcl::SelectionRect s;
    s.m_start_col = 0;
    s.m_end_col = -1;
    s.m_start_row = 0;
    s.m_end_row = -1;
    addSelection(&s);
    
    refresh(kcl::Grid::refreshAll);
}

bool Grid::isRowSelected(int row)
{
    return m_selection.getRowSelected(row);
}

void Grid::copySelection()
{
    // disable text qualifier for now since the delimiter
    // is a tab and since it saves a step of removing the 
    // qualifiers when pasting into a text editor
    bool use_text_qualifier = false;

    // caption headers
    bool use_column_captions = true;

    // get the selection count; if we don't have any selections,
    // use the cursor cell, and we're done
    int selection_count = getSelectionCount();
    if (selection_count == 0)
    {
        if (m_viewcols[m_cursor_col]->m_modelcol == -1)
            return;

        wxString result = getCellString(m_cursor_row, m_cursor_modelcol);
        wxTheClipboard->SetData(new wxTextDataObject(result));
        wxTheClipboard->Close();
        return;
    }

    // copy the selections and keep track of the bounds
    // of the selection
    int start_row = INT_MAX;
    int start_col = INT_MAX;
    int end_row = 0;
    int end_col = 0;

    std::vector<wxRect> selections;
    for (int i = 0; i < selection_count; ++i)
    {
        // get the selection rectangle    
        SelectionRect sel;
        getSelection(i, &sel);
        
        wxRect rect(sel.m_start_col,
                    sel.m_start_row,
                    sel.m_end_col - sel.m_start_col + 1,
                    sel.m_end_row - sel.m_start_row + 1);
                    
        selections.push_back(rect);

        // keep track of the bounds
        if (sel.m_start_row < start_row)
            start_row = sel.m_start_row;

        if (sel.m_start_col < start_col)
            start_col = sel.m_start_col;

        if (sel.m_end_row > end_row)
            end_row = sel.m_end_row;

        if (sel.m_end_col > end_col)
            end_col = sel.m_end_col;
    }
    

    wxString result;

    // estimate the size of the result string
    double bytes_per_row = (end_col - start_col + 1) * 10;
    double estimated_bytes = bytes_per_row * ((double)(end_row - start_row + 1 + 1));   // # of rows plus column captions

    if (estimated_bytes > 15000000.0)
    {
        int max_rows = int(15000000.0/bytes_per_row);

        if (max_rows > 100000)
            max_rows = 100000;

        if (end_row < start_row + max_rows - 1)
            return;
        
        end_row = start_row + max_rows - 1;
        estimated_bytes = bytes_per_row * ((double)(end_row - start_row + 1));
    }


    result.Alloc((int)estimated_bytes);


    // column captions
    std::vector<wxString> captions;
    captions.resize(end_col - start_col + 1);


    // build a string
    wxString cell_text;

    std::vector<wxRect>::iterator it, it_begin, it_end;
    it_begin = selections.begin();
    it_end = selections.end();

    for (int row = start_row; row <= end_row; row++)
    {
        for (int col = start_col; col <= end_col; col++)
        {
            if (m_viewcols[col]->m_modelcol == -1)
                continue;

            // if the cell is contained in a selection, get
            // the cell value; otherwise, use a blank value
            cell_text = wxEmptyString;
            for (it = it_begin; it != it_end; ++it)
            {
                if (it->Contains(col, row))
                {
                    // get the cell caption
                    captions[col - start_col] = m_viewcols[col]->m_caption;
                
                    // get the cell text
                    cell_text = m_model->getCellString(row, m_viewcols[col]->m_modelcol);
                    break;
                }
            }

            const wxChar* p = cell_text.c_str();
            
            if (use_text_qualifier)
                result += wxT("\"");

            while (*p)
            {
                if (*p == 0x0d)
                {
                    p++;
                    continue;
                }

                if (*p == wxT('"'))
                {
                    // if we're using a text qualifier, double quote any embedded
                    // quotes; otherwise, simply add the character, since there's
                    // not conflict
                    if (use_text_qualifier)
                        result += wxT("\"\"");
                         else
                        result += *p;
                }
                else
                {
                    result += *p;
                }
                p++;
            }
            
            if (use_text_qualifier)
                result += wxT("\"");

            if (col < end_col)
            {
                result += wxT("\t");
            }
        }

        if (row < end_row)
        {
            result += wxT("\r\n");
        }        
    }

    // create the caption string
    wxString caption_row;
    std::vector<wxString>::iterator it_str, it_str_end;
    it_str_end = captions.end();
    
    for (it_str = captions.begin(); it_str != it_str_end; ++it_str)
    {
        if (it_str != captions.begin())
            caption_row += wxT("\t");
        
        if (use_text_qualifier)
            caption_row += wxT("\"");
            
        caption_row += *it_str;

        if (use_text_qualifier)
            caption_row += wxT("\"");
    }

    caption_row += wxT("\r\n");


    if (use_column_captions)
        result = caption_row + result;


    // write text to the clipboard
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(result));
        wxTheClipboard->Close();
    }
}


int Grid::getSelectionCount()
{
    return m_selection.getSelectionCount();
}

void Grid::getSelection(unsigned int idx, SelectionRect* selrect)
{
    if ((int)idx >= m_selection.getSelectionCount())
        return;

    SelectionRect* s = m_selection.getSelection(idx);

    int start_row = s->m_start_row;
    int end_row = s->m_end_row;
    int start_col = s->m_start_col;
    int end_col = s->m_end_col;

    if (end_row == -1)
        end_row = m_row_count-1;

    if (end_row == -1)  // protect when m_row_count == 0
        end_row = 0;

    if (end_col == -1)
        end_col = m_viewcols.size()-1;

    if (end_col == -1)  // protect when end_col == 0
        end_col = 0;

    selrect->m_start_row = std::min(start_row, end_row);
    selrect->m_start_col = std::min(start_col, end_col);
    selrect->m_end_row = std::max(start_row, end_row);
    selrect->m_end_col = std::max(start_col, end_col);
}

void Grid::getAllSelections(std::vector<SelectionRect>& selrect)
{
    int sel_count = m_selection.getSelectionCount();
    for (int idx = 0; idx < sel_count; ++idx)
    {
        SelectionRect s = *m_selection.getSelection(idx);
        selrect.push_back(s);
    }
}

void Grid::clearSelection()
{
    m_selection.clear();
}

bool Grid::addSelection(SelectionRect* selrect)
{
    SelectionRect* s = m_selection.addSelection();
    s->m_start_col = selrect->m_start_col;
    s->m_end_col = selrect->m_end_col;
    s->m_start_row = selrect->m_start_row;
    s->m_end_row = selrect->m_end_row;
    return true;
}

bool Grid::removeSelection(unsigned int idx)
{
    if ((int)idx > m_selection.getSelectionCount())
        return false;

    m_selection.removeSelection(m_selection.getSelection(idx));
    return true;
}



// event dispatchers

bool Grid::fireEvent(int event_type, int row, int col, bool* allowed)
{
    GridEvent evt(GetId(), event_type, this);
    evt.m_row = row;
    evt.m_col = col;

    bool h = GetEventHandler()->ProcessEvent(evt);
    if (allowed)
    {
        *allowed = evt.IsAllowed();
    }

    return h;
}


bool Grid::fireEvent(int event_type, GridEvent& evt, bool* allowed)
{
    evt.SetEventType(event_type);
    evt.SetId(GetId());
    evt.SetEventObject(this);

    bool h = GetEventHandler()->ProcessEvent(evt);
    if (allowed)
    {
        *allowed = evt.IsAllowed();
    }

    return h;
}






void Grid::onEraseBackground(wxEraseEvent& event)
{
}

int Grid::fromDIP(int d)
{
    return this->FromDIP(d);
}

int Grid::toDIP(int d)
{
    return this->ToDIP(d);
}



void Grid::drawColumnHeaderRectangle(int x1, int y1, int width, int height, bool selected)
{
    m_memdc.GradientFillLinear(wxRect(x1,y1,width,height),
                               m_base_color,
                               kcl::stepColor(m_base_color, 170),
                               wxNORTH);
                               
    m_memdc.SetPen(m_darkgrey_pen);
    m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
    m_memdc.DrawRectangle(x1, y1, width, height);
}

void Grid::drawRowGripperRectangle(int x1, int y1, int width, int height, bool selected)
{
    m_memdc.GradientFillLinear(wxRect(x1,y1,width,height),
                               m_base_color,
                               kcl::stepColor(m_base_color, 170),
                               wxWEST);
                               
    m_memdc.SetPen(m_darkgrey_pen);
    m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
    m_memdc.DrawRectangle(x1, y1, width, height);
    
#if 0
    m_memdc.SetPen(m_darkgrey_pen);
    m_memdc.SetBrush(m_grey_brush);
/*
    // TODO: to finish column and row selection highlights: 1) uncomment this
    // section, 2) determine a suitable color for bgbrush, 3) refresh columns 
    // and rows on mouse move (currently only refreshing actual selection area 
    // in grid), 4) add a grid flag so column and row selections aren't added
    // when we don't want them
    
    if (selected)
    {
        wxBrush bgbrush(m_select_bgcolor, wxSOLID);
        m_memdc.SetBrush(bgbrush);
    }
*/
    m_memdc.DrawRectangle(x1, y1, width, height);
    
    m_memdc.SetPen(*wxWHITE_PEN);
    m_memdc.DrawLine(x1+1, y1+1, x1+width-1, y1+1);
    m_memdc.DrawLine(x1+1, y1+1, x1+1, y1+height-1);
/*
    m_memdc.SetPen(*wxGREY_PEN);
    m_memdc.DrawLine(x1+width-2, y1+1, x1+width-2, y1+height-1);
    m_memdc.DrawLine(x1+1, y1+height-2, x1+width-2, y1+height-2);
*/
#endif
}

/*  experimental drawChiselRectangle() for GTK
void Grid::drawChiselRectangle(int x1, int y1, int width, int height, bool selected)
{
    static GtkWidget *s_button = NULL;
    static GtkWidget *s_window = NULL;

    if (s_button == NULL)
    {
        s_window = gtk_window_new(GTK_WINDOW_POPUP);
        gtk_widget_realize(s_window);

        s_button = gtk_toolbar_new();
        gtk_container_add(GTK_CONTAINER(s_window), s_button);

        gtk_widget_realize(s_button);
    }


    gtk_paint_box
    (
        s_button->style,
        m_memdc.m_window,
        GTK_STATE_NORMAL,
        GTK_SHADOW_OUT,
        NULL,
        s_button,
        "toolbar",
        x1, y1, width+1, height+1
    );
}
*/


bool Grid::hitTest(int x, int y, int* model_row, int* view_col, int* cell_xoff)
{
    bool result = true;

    int row = -1;
    int col = -1;

    if (m_row_height > 0)
    {
        int yoff = m_header_height;
        int rowdata_size = m_rowdata.size();

        for (int i = 0; i < rowdata_size; ++i)
        {
            if (m_rowdata[i].m_blank)
            {
                yoff += m_row_height;
                continue;
            }

            if (y > yoff && y <= yoff+m_row_height)
            {
                row = m_rowdata[i].m_model_row;
                break;
            }

            yoff += m_row_height;
        }

        if (row == -1)
        {
            result = false;
        }
    }
    
    if (cell_xoff)
    {
       *cell_xoff = 0;
    }

    std::vector<ColumnRange>::iterator it;
    for (it = m_colranges.begin(); it != m_colranges.end(); ++it)
    {
        if (x > (it->xoff) && x < (it->xoff + it->width))
        {
            col = it->viewcol_idx;
            if (cell_xoff)
            {
                *cell_xoff = x - it->xoff;
            }
            break;
        }
    }

    if (col == -1)
    {
        result = false;
    }

    if (model_row)
    {
        *model_row = row;
    }

    if (view_col)
    {
        *view_col = col;
    }

    return result;
}


void Grid::getCellRect(int row, int col, wxRect* rect)
{
    if (col >= (int)m_viewcols.size())
        return;
    
    int i;

    int x = -m_xoff + m_frozen_width;
    for (i = 0; i < col; i++)
    {
        x += m_viewcols[i]->m_pixwidth;
    }


    int y = 5000;
    
    if (row >= m_row_offset)
    {
        int r = 0;

        int rowdata_size = m_rowdata.size();

        // we go one more than the rowdata size to make ghost
        // row cursors work properly
        for (i = 0; i <= rowdata_size; ++i)
        {
            if (i < rowdata_size)
            {
                if (m_rowdata[i].m_blank)
                    continue;
            }

            if (m_row_offset+r == row)
            {
                y = (i*m_row_height) + m_header_height;
                break;
            }
            r++;
        }
    }
    else
    {
        y = -5000;
    }

    rect->SetX(x);
    rect->SetY(y);
    rect->SetWidth(m_viewcols[col]->m_pixwidth);
    rect->SetHeight(m_row_height);
}

void Grid::getBoundingBoxRect(int row1,
                              int col1,
                              int row2,
                              int col2,
                              wxRect* rect)
{
    int start_row = std::min(row1, row2);
    int start_col = std::min(col1, col2);
    int end_row = std::max(row1, row2);
    int end_col = std::max(col1, col2);

    wxRect ul_rect;     // upper-left cell's rectangle
    wxRect lr_rect;     // lower-right cell's rectangle

    getCellRect(start_row, start_col, &ul_rect);
    getCellRect(end_row, end_col, &lr_rect);


    rect->SetX(ul_rect.GetX());
    rect->SetY(ul_rect.GetY());
    rect->SetWidth(lr_rect.GetX()-ul_rect.GetX() + lr_rect.GetWidth());
    rect->SetHeight(lr_rect.GetY()-ul_rect.GetY() + lr_rect.GetHeight());
}



void Grid::drawBoundingBox(int row1,
                           int col1,
                           int row2,
                           int col2,
                           bool thin,
                           bool stipple)
{
    wxRect rect;
    getBoundingBoxRect(row1, col1, row2, col2, &rect);

    int bb_xoff = rect.x;
    int bb_yoff = rect.y;
    int bb_width = rect.width;
    int bb_height = rect.height;

    m_memdc.SetClippingRegion(m_frozen_width, m_header_height,
            m_cliwidth-m_frozen_width, m_cliheight-m_header_height);

    if (thin)
    {
        if (stipple)
            m_memdc.SetPen(*wxLIGHT_GREY_PEN);
             else
            m_memdc.SetPen(*wxGREY_PEN);
        m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
        
        if (isRowSelected(m_cursor_row))
        {
            int color_lightness = m_hc_barbg_color.Red()+
                                  m_hc_barbg_color.Green()+
                                  m_hc_barbg_color.Blue();
            if (color_lightness < 480)
            {
                // since we have a dark row highlight color, it looks better
                // to have the bounding rectangle moved in one pixel
                bb_yoff += 1;
                bb_height -= 2;

                // we have a dark row highlight color,
                // so draw a light bounding rectangle
                m_memdc.SetPen(kcl::stepColor(m_hc_barbg_color,160));
            }
            else
            {
                m_memdc.SetPen(kcl::stepColor(m_hc_barbg_color,60));
            }
        }
        
        // if the grid has vertical lines, move the rectangle in
        // one pixel so it doesn't overlap the vertical grid lines
        if (m_options & optVertGridLines)
        {
            bb_xoff += 1;
            bb_width -= 2;
            
            // the first column's x-offset is off by one
            if (col1 == 0 || col2 == 0)
            {
                bb_xoff -= 1;
                bb_width += 1;
            }
        }
        
        // if the grid has horizontal lines, move the rectangle in
        // one pixel so it doesn't overlap the horizontal grid lines
        if (m_options & optHorzGridLines)
        {
            bb_yoff += 1;
            bb_height -= 1;
            
            // the first row's y-offset is off by one
            if (row1 == 0 || row2 == 0)
            {
                bb_yoff -= 1;
                bb_height += 1;
            }
        }
        
        if (!isEditing())
            m_memdc.DrawRectangle(bb_xoff, bb_yoff, bb_width+1, bb_height);
    }
    else
    {
        m_memdc.SetPen(*wxBLACK_PEN);
        m_memdc.SetBrush(*wxBLACK_BRUSH);

        m_memdc.DrawRectangle(bb_xoff, bb_yoff - 1, bb_width, 3);
        m_memdc.DrawRectangle(bb_xoff, bb_yoff + bb_height - 1,bb_width, 3);
        m_memdc.DrawRectangle(bb_xoff - 1, bb_yoff - 1, 3, bb_height + 3);
        m_memdc.DrawRectangle(bb_xoff + bb_width - 1, bb_yoff - 1, 3, bb_height + 3);
    }

    m_memdc.DestroyClippingRegion();
}


static void lightenMemDC(wxMemoryDC& dc, wxBitmap& bmp)
{
    wxImage img = bmp.ConvertToImage();
    unsigned char* data = img.GetData();
    
    int height = img.GetHeight();
    int width = img.GetWidth();

#if wxCHECK_VERSION(2,9,0)
    if (img.HasAlpha())
        img.ClearAlpha();
#endif

    int x, y;
    wxColour col;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            unsigned char* r = data + (y*(width*3))+(x*3);
            unsigned char* g = r+1;
            unsigned char* b = r+2;

            *r = (unsigned char)blendColor((double)*r, 255.0, 0.3);
            *g = (unsigned char)blendColor((double)*g, 255.0, 0.3);
            *b = (unsigned char)blendColor((double)*b, 255.0, 0.3);
        }
    }
    
    wxBitmap new_bmp(img);
    dc.SelectObject(new_bmp);
}


void Grid::drawDisabledStipple()
{
    lightenMemDC(m_memdc, m_bmp);
}


void Grid::initGui()
{
    allocBitmap(100, 100);

    SetFont(*wxNORMAL_FONT);
    
    m_memdc.SetPen(*wxTRANSPARENT_PEN);
    m_memdc.SetBrush(*wxWHITE_BRUSH);
    m_memdc.DrawRectangle(-1, -1, 105, 105);

    // create commonly used graphics objects

    m_white_brush = *wxWHITE_BRUSH;

    m_grey_brush = *wxTheBrushList->FindOrCreateBrush(
                        m_base_color,
                        wxSOLID);

    m_grey_pen = *wxThePenList->FindOrCreatePen(
                        m_base_color,
                        1,
                        wxSOLID);

    m_darkgrey_pen = kcl::getBorderPen();
                        
    m_gridline_pen = *wxThePenList->FindOrCreatePen(
                        m_def_gridline_color,
                        1,
                        wxSOLID);

    unsigned char stipple_data[2];
    stipple_data[0] = 0xaa;
    stipple_data[1] = 0x55;
    wxBitmap stipple_bitmap((char*)stipple_data, 8, 2, 1);

    m_stipple_pen.SetColour(wxColor(0,0,0));
#ifdef WIN32
    m_stipple_pen.SetStipple(stipple_bitmap);
#endif

    // initialize edit control member variable
    m_control = NULL;

    // set dimension and position member variables
    m_xoff = 0;
    m_total_width = 0;
    m_frozen_width = 0;
    m_row_offset = 0;
    m_dip_header_height = 18;
    m_header_height = fromDIP(m_dip_header_height);

    // set row break count (group break count)
    m_row_break_count = 0;

    // set action variables
    m_mouse_action = actionNone;
    m_action_xpos = 0;
    m_action_ypos = 0;
    m_action_col = NULL;

    m_last_tooltip_row = 0;
    m_last_tooltip_col = 0;

    // get size
    GetClientSize(&m_cliwidth, &m_cliheight);

    // initialize font
    initFont();
}


bool _ptInRect(wxRect* rect, int x, int y)
{
    return (x > rect->x &&
            x < rect->x + rect->width &&
            y > rect->y &&
            y < rect->y + rect->height);
}


bool Grid::rectInRect(wxRect* rect, int x, int y, int width, int height)
{
    if (x+width < 0 || y+height < 0)
        return false;
    if (x > m_cliwidth || y > m_cliheight)
        return false;
    if (!rect)
        return true;

    if (x+width < rect->x)
        return false;
    if (y+height < rect->y)
        return false;
    if (x > rect->x+rect->width)
        return false;
    if (y > rect->y+rect->height)
        return false;

    return true;
}


void darkenColor(wxColor& c, int amount = 70)
{
    int r, g, b;
    r = c.Red();
    g = c.Green();
    b = c.Blue();
    c.Set((r*amount)/100, (g*amount)/100, (b*amount)/100);
}


void Grid::calcColumnWidths()
{
    int xoff, col, col_count;

    col_count = m_viewcols.size();

    // calculate proportional sizes
    xoff = m_frozen_width;
    m_total_pwidth = 0;
    for (col = 0; col < col_count; col++)
    {
        if (m_viewcols[col]->m_prop_size == 0)
        {
            xoff += m_viewcols[col]->m_pixwidth;
        }
        else
        {
            m_total_pwidth += m_viewcols[col]->m_prop_size;
        }
    }

    if (m_total_pwidth > 0)
    {
        for (col = 0; col < col_count; ++col)
        {
            if (m_viewcols[col]->m_prop_size > 0)
            {
                m_viewcols[col]->m_pixwidth = (((m_cliwidth - xoff - 1) * (m_viewcols[col]->m_prop_size * 100)) / (m_total_pwidth * 100));
                m_viewcols[col]->m_dip_pixwidth = toDIP(m_viewcols[col]->m_pixwidth);
            }
        }

        // determine total width
        m_total_width = 0;
        for (col = 0; col < col_count; col++)
            m_total_width += m_viewcols[col]->m_pixwidth;

        // if there are a few remaining pixels, add them on to the
        // last column with a proportional width
        if (m_total_width < m_cliwidth)
        {
            int adj = m_cliwidth-m_total_width;
            if (adj > 0)
            {
                for (col = col_count-1; col >= 0; --col)
                {
                    if (m_viewcols[col]->m_prop_size > 0)
                    {
                        m_viewcols[col]->m_pixwidth += adj;
                        m_viewcols[col]->m_dip_pixwidth = toDIP(m_viewcols[col]->m_pixwidth);
                        m_total_width += adj;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        m_total_width = xoff-m_frozen_width;
    }
}


bool Grid::setCellData(int row, int col, CellData* data)
{
    CellData c = *data;
    
    if (row >= (int)m_rowdata.size())
        return false;
        
    RowData& rowdata = m_rowdata[row];
    if (col >= (int)rowdata.m_coldata.size())
        return false;

    rowdata.m_coldata[col] = c;
    
    // this will set the cell properties in the model
    setCellProperties(row, col, data);
    return true;
}


CellData* Grid::getCellData(int row, int col)
{
    row -= m_row_offset;

    if (row < 0 || row >= (int)m_rowdata.size() || col < 0)
    {
        return &m_empty_cell;
    }

    int r = 0;
    int rowdata_size = m_rowdata.size();
    for (int i = 0; i < rowdata_size; ++i)
    {
        if (m_rowdata[i].m_blank)
        {
            continue;
        }

        if (r == row)
        {
            RowData& rowdata = m_rowdata[i];
            if (col >= (int)rowdata.m_coldata.size())
            {
                return &m_empty_cell;
            }

            return &rowdata.m_coldata[col];
        }

        r++;
    }

    return &m_empty_cell;
}



bool Grid::allocBitmap(int width, int height)
{
    if (width < 16)
        width = 16;

    if (height < 16)
        height = 16;

    if (width <= m_bmp_alloc_width && height <= m_bmp_alloc_height)
    {
        // allocated bitmap is already big enough
        return false;
    }

    wxClientDC cdc(this);
    m_bmp.Create(width, height, cdc);
    m_memdc.SelectObject(m_bmp);

    m_bmp_alloc_width = width;
    m_bmp_alloc_height = height;
    
    // it was necessary to allocate a new bitmap
    return true;
}


void Grid::drawColumnDropHighlight(int col)
{
    wxRect r;
    

    if (!getColumnRect(col, r))
    {
        getColumnRect(m_viewcols.size() - 1, r);
        r.x += r.width;
    }
    
    wxClientDC dc(this);
    repaint(&dc);
    dc.SetBrush(*wxBLACK_BRUSH);
    dc.SetPen(*wxBLACK_PEN);

    dc.DrawRectangle(r.x,
                     0,
                     2,
                     m_cliheight);
}

void Grid::drawRowDropHighlight(int row, int style)
{
    if (style == 0)
    {
        wxRect r;
        getCellRect(row, 0, &r);
        
        wxClientDC dc(this);
        repaint(&dc);
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.SetPen(*wxBLACK_PEN);

        dc.DrawRectangle(r.x,
                         r.y,
                         m_cliwidth-r.x,
                         2);
/*
        int yy = r.y-4;
        int hh = 10;

        if (yy < m_header_height)
        {
            yy = m_header_height;
            hh = 5;
        }


        dc.DrawRectangle(r.x,
                         yy,
                         2,
                         hh);

        dc.DrawRectangle(m_cliwidth-r.x-2,
                         yy,
                         2,
                         hh);
*/

    }
    else if (style == 1)
    {
        SelectionRect* s;
        s = m_selection.addSelection();
        s->m_start_row = row;
        s->m_end_row = row;
        s->m_start_col = 0;
        s->m_end_col = -1;

        refresh(kcl::Grid::refreshAll);

        m_selection.removeSelection(s);
    }
}




inline wxString filterControlChars(const wxString& str)
{
    const wxChar* p = str.c_str();
    while (*p)
    {
        if (*p <= 13)
        {
            // must translate
            wxString result;
            result.Alloc(255);

            p = str.c_str();
            while (*p)
            {
                if (*p > 13)
                {
                    result += *p;
                }
                else
                {
                    result += wxT(' ');
                }

                p++;
            }

            return result;
        }
        p++;
    }

    return str;
}


static void drawTextWrap(wxDC& dc,
                    const wxString& str,
                    int x,
                    int y,
                    int width,
                    int height)
{
    wxArrayString arr;


    const wxChar* s = str.c_str();
    wxString piece;
    bool non_space_found = false;
    while (*s)
    {
        bool is_space = wxIsspace(*s) ? true : false;

        if (is_space && non_space_found)
        {
            arr.Add(piece);
            piece = wxT("");
            non_space_found = false;
        }
        else
        {
            if (!is_space)
                non_space_found = true;

            piece += *s;
        }

        ++s;
    }

    if (!piece.IsEmpty())
        arr.Add(piece);

    int text_x, text_y, line_height;
    dc.GetTextExtent(wxT("ABCDXHM"), &text_x, &line_height);

    //line_height += 2;
    int count = arr.Count();

    int yoff = 0;
    int xoff = x;
    int w = width;
    int line = 1;
    int i;

    int max_lines = height/line_height;

    for (i = 0; i < count; ++i)
    {
        wxString piece = arr.Item(i);
        if (piece.IsEmpty())
            continue;

        piece += wxT(" ");

        dc.GetTextExtent(piece, &text_x, &text_y);

        if (w < text_x && xoff > x && line < max_lines)
        {
            w = width;
            xoff = x;
            yoff += line_height;
            line++;
        }

        w -= text_x;
        xoff += text_x;
    }

    int total_height = yoff+line_height;

    yoff = y+(height/2)-(total_height/2);
    xoff = x;
    w = width;
    line = 1;

    for (i = 0; i < count; ++i)
    {
        wxString piece = arr.Item(i);
        if (piece.IsEmpty())
            continue;

        piece += wxT(" ");

        dc.GetTextExtent(piece, &text_x, &text_y);

        if (w < text_x && xoff > x && line < max_lines)
        {
            w = width;
            xoff = x;
            yoff += line_height;
            line++;
        }

        dc.DrawText(piece, xoff, yoff);

        w -= text_x;
        xoff += text_x;
    }
}


static wxString dbl2fstr(double d, int dec_places)
{
    // the first time we are run, get locale information about the system's
    // decimal point character and thousands separator character

    static wxChar thousands_sep = 0;
    static wxChar decimal_point = 0;
    if (!decimal_point)
    {
        struct lconv* l = localeconv();
        thousands_sep = (unsigned char)*(l->thousands_sep);
        decimal_point = (unsigned char)*(l->decimal_point);

        if (thousands_sep == 0)
            thousands_sep = wxT(',');
        if (decimal_point == 0)
            decimal_point = wxT('.');
    }

    // initialize result area
    wxChar result[256];
    wxChar* string_start;
    memset(result, 0, 256*sizeof(wxChar));

    double decp, intp;
    int digit;
    bool negative = false;

    if (d < 0.0)
    {
        negative = true;
        d = fabs(d);
    }

    // split the number up into integer and fraction portions
    d = kl::dblround(d, dec_places);
    decp = modf(d, &intp);

    int i = 0;

    int pos = 80;
    while (pos > 1)
    {
        digit = (int)((modf(intp/10, &intp)+0.01)*10);

        result[pos] = wxT('0') + digit;
        pos--;

        if (intp < 1.0)
        {
            break;
        }

        if (++i == 3)
        {
            result[pos] = thousands_sep;
            pos--;
            i = 0;
        }
    }

    if (negative)
    {
        result[pos] = wxT('-');
        --pos;
    }


    string_start = result+pos+1;


    if (dec_places > 0)
    {
        pos = 81;
        result[pos] = decimal_point;
        pos++;
        wxSnprintf(result+pos, 80, wxT("%0*.0f"), dec_places, decp*kl::pow10(dec_places));
    }

    return string_start;
}

static void getColorRectangles(int cell_x, int cell_y, int row_height,
                               wxRect* fgcolor_rect, wxRect* bgcolor_rect)
{
    int x1, y1, h;
    x1 = cell_x+6;
    y1 = cell_y+3;
    h = row_height-6;
    h *= 2;
    h /= 3;
    
    //set the foreground rectangle area
    *fgcolor_rect = wxRect(x1, y1, h, h);

    //set the background rectangle area
    *bgcolor_rect = wxRect(x1+(h/2), y1+(h/2), h, h);
}


void Grid::render(wxRect* update_rect, bool cursor_visible)
{
    if (!m_gui_initialized)
        return;

    if (allocBitmap(m_cliwidth + 200, m_cliheight))
    {
        // allocBitmap returned true, which indicates that it was
        // necessay to allocate a new bitmap.  This means that
        // we need to disregard the update_rect that was passed
        // to us, and re-render the entire grid
        
        update_rect = NULL;
    }

    if (!m_cursor_visible)
    {
        cursor_visible = false;
    }

    m_render_size.Set(m_cliwidth, m_cliheight);

    if (m_visible_state == stateHidden)
    {
        m_memdc.SetPen(*wxWHITE_PEN);
        m_memdc.SetBrush(*wxWHITE_BRUSH);
        m_memdc.DrawRectangle(-1,-1, m_cliwidth+2, m_cliheight+2);
        return;
    }

    int col_count = m_viewcols.size();
    int row, col;
    int xoff, yoff;
    int invalid_xoffset = -1;
    int invalid_yoffset = -1;

    int rowdata_size = m_rowdata.size();

    if (m_options & Grid::optGhostRow &&
        !(m_options & Grid::optInvalidAreaClickable))
    {
        // paint an extra "ghost" row
        rowdata_size++;
    }

    m_memdc.SetBackgroundMode(wxTRANSPARENT);

    // determine header height

    m_frozen_width = m_rowlabel_width;
    
    calcColumnWidths();

    if (col_count == 0)
    {
        invalid_xoffset = m_frozen_width-1;
    }

    // draw column headers and figure out column ranges

    // if we've specified a caption font, use it, otherwise stick with m_font
    if (m_caption_font.IsOk())
        m_memdc.SetFont(m_caption_font);
        
    m_colranges.clear();
    ColumnRange range;

    xoff = -m_xoff + m_frozen_width;
    int col_width;
    for (col = 0; col < col_count; col++)
    {
        col_width = m_viewcols[col]->m_pixwidth;

        if (xoff+col_width >= m_frozen_width && xoff < m_cliwidth)
        {
            // add a column range entry
            range.viewcol = m_viewcols[col];
            range.xoff = xoff;
            range.width = col_width;
            range.viewcol_idx = col;
            m_colranges.push_back(range);
        }

        if (m_header_height > 0 &&
            rectInRect(update_rect, xoff, 0, col_width, m_header_height))
        {
            // don't draw any columns that have a width of 0
            if (col_width == 0)
                continue;
            
            // draw the column header
            int colx, colw;

            colx = xoff;
            colw = col_width+1;
            
            // if this is the leftmost visible column, move it one pixel
            // to the left so we don't have a double border with either
            // the row label's border or the window border
            if (xoff == m_rowlabel_width)
            {
                colx--;
                colw++;
            }

            wxCoord text_width;
            wxCoord text_height;
            int textx, texty;
            const wxChar* str = m_viewcols[col]->m_caption.c_str();
            m_memdc.GetTextExtent(str, &text_width, &text_height);

            textx = colx+(col_width/2)-(text_width/2);
            textx = std::max(textx, xoff+3);

            texty = (m_header_height/2)-(text_height/2)-1;
            
            // if there's a selection in the column, set the column selection flag
            bool selected = false;
            if (m_selection.columnContainsSelection(col))
                selected = true;

            m_memdc.SetClippingRegion(colx, -1, colw, m_header_height+1);
            drawColumnHeaderRectangle(colx, -1, colw, m_header_height+1, selected);
            m_memdc.SetTextForeground(wxColour(0,0,0));
            m_memdc.DrawText(str, textx, texty);

            if (m_viewcols[col]->m_bitmap.Ok())
            {
                int bitmapx;
                int bitmapy = (m_header_height / 2) - (m_viewcols[col]->m_bitmap.GetHeight() / 2);

                switch (m_viewcols[col]->m_bitmap_alignment)
                {
                    default:
                    case alignLeft:
                        bitmapx = colx + FromDIP(2);
                        break;

                    case alignCenter:
                        bitmapx = textx - m_viewcols[col]->m_bitmap.GetWidth() - FromDIP(2);
                        if (bitmapx < colx+2)
                            bitmapx = colx+2;
                        break;

                    case alignRight:
                        bitmapx = colx+colw - m_viewcols[col]->m_bitmap.GetWidth() - FromDIP(2);
                        break;
                }


                m_memdc.DrawBitmap(m_viewcols[col]->m_bitmap, bitmapx, bitmapy, true);
            }

            m_memdc.DestroyClippingRegion();
        }

        xoff += col_width;

        if (col+1 == col_count)
        {
            invalid_xoffset = xoff;
            break;
        }

        if (xoff > m_cliwidth)
            break;
    }
    
    // set the font back to m_font (if we specified a caption font)
    m_memdc.SetFont(m_font);
        

    
    // draw rows
    wxColour fgcolor, bgcolor;
    wxString text;
    int alignment;
    int text_wrap;

    CellData* celldata;
    row = 0;


    std::vector<wxRect> group_separator_rects;
    m_row_break_count = 0;

    yoff = m_header_height;
    while (1)
    {
        if (row == rowdata_size)
        {
            invalid_yoffset = yoff;
            break;
        }

        RowData* rowdata;

        if (row < (int)m_rowdata.size())
            rowdata = &m_rowdata[row];
             else
            rowdata = &m_empty_row;


        if (rowdata->m_blank)
        {
            int t = 0;
            if (row > 0 && m_options & optHorzGridLines)
                t = 1;

            group_separator_rects.push_back(wxRect(m_rowlabel_width,
                                                   yoff+t,
                                                   m_cliwidth-m_rowlabel_width,
                                                   m_row_height-t));

            // increment the row count and y offset
            row++;
            yoff += m_row_height;

            // increment the row break count
            m_row_break_count++;

            if (yoff > m_cliheight)
                break;

            continue;
        }



        if (rectInRect(update_rect,
                       m_frozen_width,
                       yoff,
                       m_cliwidth-m_frozen_width,
                       m_row_height))
        {
            xoff = -m_xoff + m_frozen_width;
            for (col = 0; col < col_count; col++)
            {
                col_width = m_viewcols[col]->m_pixwidth;

                if (!(m_options & optVertGridLines))
                {
                    // for painting reasons, last column is 1 pixel
                    // bigger when there are no grid lines
                    if (col+1 == col_count)
                    {
                        col_width++;
                    }
                }

                if (rectInRect(update_rect,
                               xoff,
                               yoff,
                               col_width,
                               m_row_height))
                {
                    int model_row = rowdata->m_model_row;

                    if (m_viewcols[col]->m_modelcol != -1)
                    {
                        celldata = &(rowdata->m_coldata[m_viewcols[col]->m_modelcol]);
                    }
                    else
                    {
                        celldata = &m_empty_cell;
                    }

                    if (celldata->ctrltype == ctrltypeColor)
                    {
                        if (m_greenbar_interval > 0)
                        {
                            if ((m_row_offset+row+1) % m_greenbar_interval == 0)
                            {
                                bgcolor = m_def_greenbar_color;
                            }
                            else
                            {
                                bgcolor = m_def_bgcolor;
                            }
                        }
                        else
                        {
                            bgcolor = m_def_bgcolor;
                        }
                        
                        fgcolor = celldata->fgcolor;
                    }
                    else
                    {
                        bgcolor = celldata->bgcolor;
                        fgcolor = celldata->fgcolor;
                    }

                    // if the model did not specify an alignment, use
                    // the view's alignment
                    alignment = celldata->alignment;
                    
                    if (alignment == alignDefault)
                    {
                        alignment = m_viewcols[col]->m_alignment;

                        if (alignment == alignDefault)
                            alignment = getColumnDefaultAlignment(col);
                    }
                    

                    // if the model did not specify a text wrap setting,
                    // use the view's text wrap setting
                    text_wrap = wrapDefault;

                    if (!(celldata->mask & CellProperties::cpmaskTextWrap))
                    {
                        text_wrap = m_viewcols[col]->m_text_wrap;
                    }
                    

                    // if the model did not specify a color, use
                    // the view's column color
                    if (!bgcolor.Ok())
                    {
                        bgcolor = m_viewcols[col]->m_bgcolor;
                        if (!bgcolor.Ok())
                        {
                            if (m_greenbar_interval > 0)
                            {
                                if ((m_row_offset+row+1) % m_greenbar_interval == 0)
                                {
                                    bgcolor = m_def_greenbar_color;
                                }
                                else
                                {
                                    bgcolor = m_def_bgcolor;
                                }
                            }
                            else
                            {
                                bgcolor = m_def_bgcolor;
                            }
                        }
                    }

                    if (!fgcolor.Ok())
                    {
                        fgcolor = m_viewcols[col]->m_fgcolor;
                        if (!fgcolor.Ok())
                        {
                            fgcolor = m_def_fgcolor;
                        }
                    }


                    // darken the color a bit if the cell is selected
                    if (m_selection.getCellSelected(model_row, col) &&
                        !(((m_options & optSelectionList) == 0) &&
                           m_cursor_row == model_row &&
                           m_cursor_col == col &&
                           cursor_visible))
                    {
                        if ((bgcolor.Red() > 240 &&
                             bgcolor.Green() > 240 &&
                             bgcolor.Blue() > 240) ||
                            bgcolor == m_def_greenbar_color)
                        {
                            if (m_select_bgcolor.Ok())
                            {
                                bgcolor = m_select_bgcolor;
                            }
                            
                            if (m_select_fgcolor.Ok())
                            {
                                fgcolor = m_select_fgcolor;
                            }
                            else
                            {
                                darkenColor(fgcolor);
                            }
                        }
                        else
                        {
                            darkenColor(bgcolor);
                            darkenColor(fgcolor);
                        }
                    }

                    if (m_viewcols[col]->m_shaded)
                    {
                        darkenColor(bgcolor, 90);
                        darkenColor(fgcolor, 90);
                    }

                    bool highlight_visible = (cursor_visible && 
                                                (m_cursor_type == cursorHighlight ||
                                                 m_cursor_type == cursorRowHighlight));
                                                 
                    if (highlight_visible)
                    {
                        if (model_row == m_cursor_row)
                        {
                            if (col == m_cursor_col || m_cursor_type == cursorRowHighlight)
                            {
                                fgcolor = m_hc_cellfg_color;
                                bgcolor = m_hc_cellbg_color;
                            }
                            else
                            {
                                fgcolor = m_hc_barfg_color;
                                bgcolor = m_hc_barbg_color;
                            }
                        }
                    }


                    // put cell text into "text" variable

                    text = wxT("");

                    if (celldata->m_null)
                    {
                        if (celldata->m_type == typeDateTime ||
                            celldata->m_type == typeDate)
                        {
                            text = wxT("  /  /  ");
                        }
                        else
                        {
                            text = wxT("<null>");
                        }
                    }
                    else
                    {
                        switch (celldata->m_type)
                        {
                            default:
                            case typeDate:
                            case typeDateTime:
                            case typeCharacter:
                            {
                                text = filterControlChars(celldata->m_strvalue);
                                break;
                            }
                            case typeDouble:
                            {
                                // for now, print numbers with thousands separators
                                // if they have decimal places, otherwise not

                                if (celldata->m_scale > 0)
                                {
                                	text = dbl2fstr(celldata->m_dblvalue,
                                                    celldata->m_scale);
                                }
                                else
                                {
                                    text = wxString::Format(wxT("%.*f"),
                                                            celldata->m_scale,
                                                            celldata->m_dblvalue);
                                }

                                break;
                            }
                            case typeInteger:
                            {
                                text = wxString::Format(wxT("%d"), celldata->m_intvalue);
                                break;
                            }
                            case typeBoolean:
                            {
                                text = celldata->m_boolvalue ? wxT("T") : wxT("F");
                                break;
                            }
                        }
                    }


                    // identify urls; if the hyperlink option is active,
                    // them draw them with a special font
                    bool is_url = false;
                    if ((m_options & optActivateHyperlinks) || celldata->hyperlink)
                    {
                        int url_pos = text.Find(wxT("://"));
                        if (celldata->hyperlink || (url_pos >= 4 && url_pos <= 10))
                        {
                            // text is a url
                            is_url = true;
                            if ((m_cursor_type == cursorRowHighlight && model_row == m_cursor_row) || isRowSelected(model_row))
                            {
                                fgcolor = m_hc_cellfg_color;
                            }
                            else
                            {
                                fgcolor = wxColour(0, 0, 255);
                            }
                            m_memdc.SetFont(m_underlined_font);
                        }
                    }

                    int text_width, text_height;
                    int textx, texty;
                    int bmpx, bmpy;

                    m_memdc.GetTextExtent(text, &text_width, &text_height);
                    
                    if (celldata->ctrltype == ctrltypeCheckBox)
                    {
                        text_width = 16;
                    }

                    switch (alignment)
                    {
                        default:
                        case alignLeft:
                            textx = xoff+4;
                            break;
                        case alignCenter:
                            textx = (xoff+(col_width/2))-(text_width/2);
                            break;
                        case alignRight:
                            textx = (xoff+col_width-4)-text_width;
                            break;
                    }

                    texty = yoff+((m_row_height-text_height+1)/2);

                    if (celldata->m_bitmap.Ok())
                    {
                        int bitmap_width = celldata->m_bitmap.GetWidth();
                        int bitmap_height = celldata->m_bitmap.GetHeight();

                        switch (celldata->bitmap_alignment)
                        {
                            default:
                            case alignLeft:
                                bmpx = xoff+4;
                                break;
                            case alignCenter:
                                bmpx = (xoff+(col_width/2))-(bitmap_width/2);
                                break;
                            case alignRight:
                                bmpx = (xoff+col_width-4)-bitmap_width;
                                break;
                        }

                        bmpy = yoff+((m_row_height+1-bitmap_height)/2);

                        // bitmap and text are both aligned the same
                        if (celldata->bitmap_alignment == alignment)
                        {
                            if (alignment == alignRight)
                                textx -= (bitmap_width+2);
                                 else
                                textx = bmpx + bitmap_width + 2;
                        }
                    }

                    
                    // if editing, draw white background on the cell
                    if (m_control && col == m_cursor_col && m_cursor_row == model_row)
                    {
                        bgcolor.Set(255,255,255);
                        fgcolor.Set(0,0,0);
                    }

                    wxBrush bgbrush(bgcolor, wxSOLID);
                    m_memdc.SetBrush(bgbrush);
                    m_memdc.SetTextForeground(fgcolor);

                    if (xoff < m_frozen_width)
                    {
                        m_memdc.SetClippingRegion(m_frozen_width,
                                                  yoff,
                                                  col_width-(m_frozen_width-xoff),
                                                  m_row_height);
                    }
                    else
                    {
                        m_memdc.SetClippingRegion(xoff,
                                                  yoff,
                                                  col_width,
                                                  m_row_height);
                    }

                    m_memdc.DrawRectangle(xoff-1, yoff-1, col_width+2, m_row_height+2);

                    switch (celldata->ctrltype)
                    {
                        case ctrltypeCheckBox:
                        {       
                            if (celldata->visible)
                            {
                                int bitmap_height = (celldata->m_boolvalue ? m_bmp_checkbox_on.GetHeight()
                                                                           : m_bmp_checkbox_off.GetHeight());
                                int bmpx = textx+2;
                                int bmpy = yoff+((m_row_height+1-bitmap_height)/2);

                                m_memdc.DrawBitmap(celldata->m_boolvalue
                                          ? m_bmp_checkbox_on
                                          : m_bmp_checkbox_off,
                                              bmpx, bmpy, true);
                            }
                            break;
                        }
                        
                        case ctrltypeButton:
                        {
                            bool pressed = false;
                            if (m_mouse_action == actionPressButton &&
                                m_action_viewcol == col &&
                                m_action_row == m_row_offset+row)
                            {
                                pressed = true;
                            }

                            int x1, y1, width, height;
                            width = col_width-8;
                            height = m_row_height-8;
                            x1 = xoff+(col_width/2)-(width/2);
                            y1 = yoff+(m_row_height/2)-(height/2);

                            if (celldata->visible)
                            {
                                if (pressed)
                                {
                                    wxRendererNative::Get().DrawPushButton(
                                                     this, m_memdc,
                                                     wxRect(x1, y1, width, height),
                                                     wxCONTROL_PRESSED);
                                }
                                else
                                {
                                    wxRendererNative::Get().DrawPushButton(
                                                     this, m_memdc,
                                                     wxRect(x1, y1, width, height),
                                                     0 /* not pressed */);
                                }

                                m_memdc.DrawText(text, textx, texty);
                            }
                            break;
                        }
                        
                        case ctrltypeColor:
                        {
                            wxRect fgcolor_rect;
                            wxRect bgcolor_rect;
                            
                            getColorRectangles(xoff, yoff, m_row_height,
                                               &fgcolor_rect, &bgcolor_rect);

                            wxBrush colorfgbrush(celldata->fgcolor, wxSOLID);
                            wxBrush colorbgbrush(celldata->bgcolor, wxSOLID);
                            
                            if (celldata->bgcolor == wxNullColour)
                                colorbgbrush = wxBrush(*wxWHITE, wxSOLID);
                            
                            if (celldata->visible)
                            {
                                // draw background color box
                                m_memdc.SetPen(*wxBLACK);
                                m_memdc.SetBrush(*wxBLACK);
                                m_memdc.DrawRectangle(bgcolor_rect);
                                m_memdc.SetPen(*wxWHITE);
                                m_memdc.SetBrush(colorbgbrush);
                                bgcolor_rect.Deflate(1,1);
                                m_memdc.DrawRectangle(bgcolor_rect);
                                
                                // draw red line if this is a null color
                                if (celldata->bgcolor == wxNullColour)
                                {
                                    int x1, y1, x2, y2;
                                    x1 = bgcolor_rect.GetLeft();
                                    y1 = bgcolor_rect.GetBottom();
                                    x2 = bgcolor_rect.GetRight()+1;
                                    y2 = bgcolor_rect.GetTop()-1;
                                    m_memdc.SetPen(wxColor(204,0,0));
                                    m_memdc.DrawLine(x1, y1, x2, y2);
                                    m_memdc.DrawLine(x1, y1-1, x2-1, y2);
                                    m_memdc.DrawLine(x1+1, y1, x2, y2+1);
                                }

                                // draw foreground color box
                                m_memdc.SetPen(*wxBLACK);
                                m_memdc.SetBrush(*wxBLACK);
                                m_memdc.DrawRectangle(fgcolor_rect);
                                m_memdc.SetPen(*wxWHITE);
                                m_memdc.SetBrush(colorfgbrush);
                                fgcolor_rect.Deflate(1,1);
                                m_memdc.DrawRectangle(fgcolor_rect);
                            }
                            break;
                        }

                        default:
                            if (celldata->visible)
                            {
                                if (text_wrap == wrapOn)
                                {
                                    drawTextWrap(m_memdc, text, textx, yoff, col_width, m_row_height);
                                }
                                else
                                {
                                    m_memdc.DrawText(text, textx, texty);
                                }
                            }
                            break;

                    }

                    if (celldata->m_bitmap.Ok() && celldata->visible)
                    {
                        m_memdc.DrawBitmap(celldata->m_bitmap, bmpx, bmpy, true);
                    }


                    if (is_url)
                    {
                        // set the font back
                        m_memdc.SetFont(m_font);
                    }

                    m_memdc.DestroyClippingRegion();
                }
                if (xoff >= m_cliwidth)
                    break;

                xoff += col_width;
            }
        }

        row++;
        yoff += m_row_height;

        if (yoff > m_cliheight)
            break;
    }

    // draw column lines

    if (m_options & optVertGridLines)
    {
        m_memdc.SetPen(m_gridline_pen);
        xoff = -m_xoff + m_frozen_width;
        int col_width;
        for (col = 0; col < col_count; col++)
        {
            col_width = m_viewcols[col]->m_pixwidth;
            xoff += col_width;

            if (xoff > m_cliwidth)
                break;

            if (xoff > m_frozen_width)
            {
                m_memdc.DrawLine(xoff,
                                 m_header_height,
                                 xoff,
                                 invalid_yoffset == -1 ? m_cliheight : invalid_yoffset);
            }
        }
    }

    // draw row lines

    if (m_options & optHorzGridLines)
    {
        m_memdc.SetPen(m_gridline_pen);
        yoff = m_header_height;
        row = 0;
        while (1)
        {
            if (row == rowdata_size)
                break;

            yoff += m_row_height;

            if (rectInRect(update_rect, 0, yoff, m_cliwidth, m_row_height))
            {
                m_memdc.DrawLine(m_frozen_width,
                                 yoff,
                                 invalid_xoffset == -1 ? m_cliwidth : invalid_xoffset, yoff);
            }
            
            if (yoff > m_cliheight)
                break;

            row++;
        }
    }


    if (invalid_yoffset != -1)
    {
        // paint out-of-bounds area
        if (rowdata_size > 0)
        {
            // fix off-by-one exception case
            if (m_options & optHorzGridLines)
            {
                invalid_yoffset++;
            }
        }
        
        if (m_greenbar_interval > 0 &&
            (m_options & optFullGreenbar))
        {
            // draw the invalid area in a manner that makes the user believe
            // that the invalid area is actually part of the valid area

            wxBrush greenbarbrush(m_def_greenbar_color, wxSOLID);
            wxPen greenbarpen(m_def_greenbar_color, 1, wxSOLID);
            
            wxBrush bgbrush(m_def_bgcolor, wxSOLID);
            wxPen bgpen(m_def_bgcolor, 1, wxSOLID);
            
            int greenrow = (m_row_count+1) % m_greenbar_interval;
            
            m_memdc.SetClippingRegion(0, invalid_yoffset, m_cliwidth, m_cliheight-invalid_yoffset);

            int invalid_row_yoffset = invalid_yoffset;
            while (invalid_row_yoffset <= m_cliheight)
            {
                if (greenrow % m_greenbar_interval == 0)
                {
                    m_memdc.SetBrush(greenbarbrush);
                    m_memdc.SetPen(greenbarpen);
                    m_memdc.DrawRectangle(0, invalid_row_yoffset, m_cliwidth, m_row_height);
                }
                else
                {
                    m_memdc.SetBrush(bgbrush);
                    m_memdc.SetPen(bgpen);
                    m_memdc.DrawRectangle(0, invalid_row_yoffset, m_cliwidth, m_row_height);
                }
                
                invalid_row_yoffset += m_row_height;
                greenrow++;
            }

            m_memdc.DestroyClippingRegion();
        }
        else
        {
            // draw the invalid area as a "true" invalid area
            
            wxBrush colorinvalidareabrush(m_def_invalid_area_color, wxSOLID);
            wxPen colorinvalidareapen(m_def_invalid_area_color, 1, wxSOLID);

            // paint out-of-bounds area
            m_memdc.SetBrush(colorinvalidareabrush);
            m_memdc.SetPen(colorinvalidareapen);
            m_memdc.SetClippingRegion(0, invalid_yoffset, m_cliwidth, m_cliheight-invalid_yoffset);
            m_memdc.DrawRectangle(0, invalid_yoffset, m_cliwidth, m_cliheight-invalid_yoffset);
            m_memdc.DestroyClippingRegion();
        }
    }

    if (invalid_xoffset != -1)
    {
        int offset = invalid_xoffset;

        //if (m_options & optVertGridLines)
        {
            offset++;
        }

        if (m_greenbar_interval > 0 &&
            (m_options & optFullGreenbar))
        {
            // draw the invalid area in a manner that makes the user believe
            // that the invalid area is actually part of the valid area
            
            wxBrush greenbarbrush(m_def_greenbar_color, wxSOLID);
            wxPen greenbarpen(m_def_greenbar_color, 1, wxSOLID);
            
            wxBrush bgbrush(m_def_bgcolor, wxSOLID);
            wxPen bgpen(m_def_bgcolor, 1, wxSOLID);
            
            int greenrow = (m_row_offset+1) % m_greenbar_interval;
            
            m_memdc.SetClippingRegion(offset, 0, m_cliwidth-offset+1, m_cliheight);

            int yoffset = m_header_height;
            while (yoffset <= m_cliheight)
            {
                if (greenrow % m_greenbar_interval == 0)
                {
                    m_memdc.SetBrush(greenbarbrush);
                    m_memdc.SetPen(greenbarpen);
                    m_memdc.DrawRectangle(offset, yoffset, m_cliwidth-offset+1, m_row_height);
                }
                else
                {
                    m_memdc.SetBrush(bgbrush);
                    m_memdc.SetPen(bgpen);
                    m_memdc.DrawRectangle(offset, yoffset, m_cliwidth-offset+1, m_row_height);
                }
                
                yoffset += m_row_height;
                greenrow++;
            }
            
            m_memdc.DestroyClippingRegion();
        }
        else
        {
            // draw the invalid area as a "true" invalid area
            
            wxBrush colorinvalidareabrush(m_def_invalid_area_color, wxSOLID);
            wxPen colorinvalidareapen(m_def_invalid_area_color, 1, wxSOLID);

            // paint out-of-bounds area
            m_memdc.SetBrush(colorinvalidareabrush);
            m_memdc.SetPen(colorinvalidareapen);
            m_memdc.SetClippingRegion(offset, 0, m_cliwidth-offset+1, m_cliheight);
            m_memdc.DrawRectangle(offset, -1, m_cliwidth-offset+1, m_cliheight+2);
            m_memdc.DestroyClippingRegion();
        }


        // draw the rest of the row selection for selection lists
        
        bool highlight_visible = (cursor_visible && 
                                    (m_cursor_type == cursorHighlight ||
                                     m_cursor_type == cursorRowHighlight));
                                     
        if ((m_options & optSelectionList) || highlight_visible)
        {
            if (col == m_cursor_col || m_cursor_type == cursorRowHighlight)
            {
                m_memdc.SetPen(m_hc_cellbg_color);
                m_memdc.SetBrush(m_hc_cellbg_color);
            }
            else
            {
                m_memdc.SetPen(m_hc_barbg_color);
                m_memdc.SetBrush(m_hc_barbg_color);
            }

            int counter = 0, vis_row_count = getVisibleRowCount();
            int row = getRowOffset();

            int x = invalid_xoffset;
            int w = m_cliwidth-invalid_xoffset;
            int y = m_header_height;
            
            while (counter++ <= vis_row_count)
            {
                // if the row is selected, draw the selection color all the
                // way across the grid's client area (including invalid area)
                if (isRowSelected(row++))
                    m_memdc.DrawRectangle(x,y,w,m_row_height);
                
                y += m_row_height;
            }
        }
        
        // paint chisel rectangle column header
        drawColumnHeaderRectangle(invalid_xoffset, -1, m_cliwidth-invalid_xoffset+1, m_header_height+1, false);
    }

    // save invalid y offset
    m_invalid_yoffset = invalid_yoffset;



    // draw column separators

    m_memdc.SetPen(*wxTRANSPARENT_PEN);

    xoff = -m_xoff + m_frozen_width;
    for (col = 0; col < col_count; col++)
    {
        col_width = m_viewcols[col]->m_pixwidth;

        if (m_viewcols[col]->m_separator)
        {
            //if (rectInRect(update_rect, xoff, 0, col_width, m_cliheight))
            if (xoff+col_width >= m_frozen_width && xoff < m_cliwidth)
            {
                int x = xoff+1;
                int width = col_width-1;

                if (x < m_frozen_width)
                {
                    width -= (m_frozen_width-x);
                    x = m_frozen_width;
                }

                wxColor bg = m_def_invalid_area_color;
                if (m_viewcols[col]->m_bgcolor.Ok())
                    bg = m_viewcols[col]->m_bgcolor;
                m_memdc.SetBrush(wxBrush(bg, wxSOLID));
                m_memdc.DrawRectangle(x, m_header_height, width, invalid_yoffset == -1 ? m_cliheight : invalid_yoffset);
            }
        }

        xoff += col_width;
        if (xoff > m_cliwidth)
            break;
    }



    // draw row labels

    if (m_rowlabel_width > 0)
    {
        // NOTE: originally, this whole section was included in an if()
        // statement with the condition:
        //     if (rectInRect(update_rect, 0, 0, m_rowlabel_width, m_cliheight))
        // however, with this condition, the cursor row marker won't always update
        // properly when the mouse moves around; to make sure the cursor row is
        // always marked correctly, always update all the row markers
        {
            drawRowGripperRectangle(-1, -1, m_frozen_width+1, m_header_height+1, false);

            row = 0;
            yoff = m_header_height;
            int group_row_count = 0;
            
            while (1)
            {
                if (row == rowdata_size)
                    break;

                // if there's a selection in the row, set the row selection flag
                bool blank_row = false;
                bool selected = false;
                RowData* rowdata;

                if (row < (int)m_rowdata.size())
                {
                    rowdata = &m_rowdata[row];
                    int model_row = rowdata->m_model_row;
                    if (m_selection.rowContainsSelection(model_row)) 
                        selected = true;
                        
                    // increment the group row count
                    if (m_rowdata[row].m_blank)
                    {
                        blank_row = true;
                        group_row_count++;
                    }
                }

                if (row == 0)
                {
                    drawRowGripperRectangle(-1, yoff-1, m_rowlabel_width+1, m_row_height+2, selected);
                }
                else
                {
                    drawRowGripperRectangle(-1, yoff, m_rowlabel_width+1, m_row_height+1, selected);
                }

                // draw the cursor row and ghost row markers, but only
                // if there's enough space
                if (m_rowlabel_width > fromDIP(8))
                {
                    if (row < (int)m_rowdata.size())
                    {
                        // if we're on a cursor row, but not the ghost row, and the
                        // option is set, draw the cursor marker
                        if (!blank_row && m_row_offset + row - group_row_count == m_cursor_row &&
                            m_options & optCursorRowMarker)
                        {
                            m_memdc.DrawBitmap(Grid::m_bmp_cursorrowmarker, 1, yoff+((m_row_height-9)/2)-1, true);
                        }
                    }
                    else
                    {
                        // if we're on the row past the end of the data and the
                        // option is set, draw the ghost row
                        if (m_options & optGhostRow)
                            m_memdc.DrawBitmap(Grid::m_bmp_ghostrowmarker, 1, yoff+((m_row_height-9)/2)-1, true);
                    }
                }

                row++;
                yoff += m_row_height;
                if (yoff > m_cliheight)
                    break;
            }
        }
    }


    // white out hidden columns

    m_memdc.SetBrush(*wxWHITE_BRUSH);
    m_memdc.SetPen(*wxWHITE_PEN);

    xoff = -m_xoff + m_frozen_width;
    for (col = 0; col < col_count; col++)
    {
        col_width = m_viewcols[col]->m_pixwidth;

        if (xoff >= m_frozen_width && xoff < m_cliwidth)
        {
            if (!m_viewcols[col]->m_draw)
            {
                m_memdc.DrawRectangle(xoff+1, 0, col_width-1, m_cliheight);
            }
        }

        xoff += col_width;
        if (xoff > m_cliwidth)
            break;
    }


    // draw group separators
    {
        wxBrush colorinvalidareabrush(m_def_invalid_area_color, wxSOLID);
        
        m_memdc.SetBrush(colorinvalidareabrush);
        m_memdc.SetPen(*wxTRANSPARENT_PEN);

        std::vector<wxRect>::iterator it;
        for (it = group_separator_rects.begin();
                it != group_separator_rects.end(); ++it)
        {
            m_memdc.DrawRectangle(it->x, it->y, it->width, it->height);
        }

    }


    // render cursor
    if (cursor_visible && rowdata_size > 0)
    {
        bool done = false;

        if (m_options & optGhostRow)
        {
            if (m_cursor_row >= m_row_count)
            {
                if (m_cursor_type != cursorRowOutline)
                {
                    drawBoundingBox(m_cursor_row,
                                    m_cursor_col,
                                    m_cursor_row,
                                    m_cursor_col,
                                    true,
                                    true);
                }
                else
                {
                    // draw row outline cursor
                    wxRect cursor_cell_rect;
                    getCellRect(m_cursor_row, m_cursor_col, &cursor_cell_rect);

                    //m_memdc.SetPen(m_stipple_pen);
                    m_memdc.SetPen(*wxGREY_PEN);
                    m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
                    m_memdc.DrawRectangle(m_frozen_width,
                                          cursor_cell_rect.y,
                                          m_total_width-m_frozen_width+1,
                                          m_row_height);
                }

                done = true;
            }
        }

        // render cursor
        if (m_cursor_type == cursorHighlight ||
            m_cursor_type == cursorRowHighlight)
        {
            done = true;
        }

        if (!done)
        {
            if (m_selection.getSelectionCount() == 1 &&
                m_cursor_type == cursorNormal &&
                m_cursor_visible &&
                m_viewcols.size() > 0)
            {
                bool separator = false;
                if (m_cursor_col >= 0 && m_cursor_col < (int)m_viewcols.size())
                {
                    if (m_viewcols[m_cursor_col]->m_separator)
                        separator = true;
                }

                SelectionRect* r = m_selection.getSelection(0);

                int end_row = (r->m_end_row == -1 ? MAX_SELECT : r->m_end_row);
                end_row = std::min(end_row, m_row_count-1);

                int end_col = (r->m_end_col == -1 ? m_viewcols.size()-1 : r->m_end_col);

                if (!separator)
                {
                    drawBoundingBox(m_cursor_row,
                                    m_cursor_col,
                                    m_cursor_row,
                                    m_cursor_col,
                                    true);
                }

                drawBoundingBox(r->m_start_row, r->m_start_col, end_row, end_col);
            }
            else
            {
                if (m_cursor_col >= 0 && m_cursor_col < (int)m_viewcols.size())
                {
                    if (m_cursor_type == cursorRowOutline)
                    {
                        wxRect cursor_cell_rect;
                        getCellRect(m_cursor_row, m_cursor_col, &cursor_cell_rect);

                        //m_memdc.SetPen(m_stipple_pen);
                        m_memdc.SetPen(*wxLIGHT_GREY_PEN);
                        m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
                        m_memdc.DrawRectangle(m_frozen_width,
                                              cursor_cell_rect.y,
                                              m_total_width-m_frozen_width+1,
                                              m_row_height);
                    }
                    else
                    {
                        bool thin = false;
                        
                        if (m_cursor_type == cursorThin ||
                            m_selection.getSelectionCount() > 1)
                        {
                            thin = true;
                        }
                        
                        drawBoundingBox(m_cursor_row,
                                        m_cursor_col,
                                        m_cursor_row,
                                        m_cursor_col,
                                        thin);
                    }
                }
            }
        }
    }


    // draw ghost row text, if any
    if (m_ghostrow_text.Length() > 0)
    {
        int text_w, text_h;
        m_memdc.GetTextExtent(wxT("ABCDXHM"), &text_w, &text_h);

        wxRect row_rect;
        getRowRect(m_row_count, row_rect);

        int yoff = row_rect.GetY();
        yoff += (row_rect.GetHeight()-text_h)/2;
        
        m_memdc.SetTextForeground(wxColor(128,128,128));
        m_memdc.SetClippingRegion(m_rowlabel_width,
                                  row_rect.GetY(),
                                  m_cliwidth-m_rowlabel_width,
                                  m_row_height);

        m_memdc.DrawText(m_ghostrow_text, 4, yoff);
        m_memdc.DestroyClippingRegion();
    }


    // draw overlay text, if any
    if (m_overlay_text.Length() > 0)
    {
        // store the font we've been using and set the font to the overlay font
        
        wxFont dc_font = m_memdc.GetFont();
        m_memdc.SetFont(m_overlay_font);
        
        wxArrayString arr;
        wxStringTokenizer tkz(m_overlay_text, wxT("\n"));
        while (tkz.HasMoreTokens())
        {
            arr.Add(tkz.GetNextToken());
        }

        int text_x, text_y, line_height;
        m_memdc.GetTextExtent(wxT("ABCDXHM"), &text_x, &line_height);

        line_height += 5;
        int line_count = arr.Count();

        int yoff = ((m_cliheight-m_header_height)/2) -
                   ((line_height * line_count)/2) +
                   m_header_height;

        m_memdc.SetTextForeground(*wxBLACK);
        m_memdc.SetClippingRegion(m_rowlabel_width,
                                  m_header_height,
                                  m_cliwidth-m_rowlabel_width,
                                  m_cliheight-m_header_height);

        for (int i = 0; i < line_count; ++i)
        {
            m_memdc.GetTextExtent(arr.Item(i), &text_x, &text_y);
            
            m_memdc.DrawText(arr.Item(i), (m_cliwidth-text_x)/2, yoff);

            yoff += line_height;
        }

        m_memdc.DestroyClippingRegion();
        
        // restore the original font the dc was using
        m_memdc.SetFont(dc_font);
    }

    // if disabled, draw the disable stipple
    if (m_visible_state == stateDisabled)
        drawDisabledStipple();
}


void Grid::onSize(wxSizeEvent& event)
{
    if (m_gui_initialized && m_model)
    {
        if (isEditing())
            endEdit(true);

        GetClientSize(&m_cliwidth, &m_cliheight);
        
        updateScrollbars(wxVERTICAL);
        updateScrollbars(wxHORIZONTAL);

        // if the new size is bigger than our rendered size,
        // we have to re-render

        if (m_cliwidth > m_render_size.GetWidth() ||
            m_cliheight > m_render_size.GetHeight() ||
            m_overlay_text.length() > 0)
        {
            // only need to get more data if the vertical size was changed
            if (m_cliheight > m_render_size.GetHeight())
            {
                updateData();
            }

            render();
            repaint();
        }
        else
        {
            if (m_total_pwidth > 0)
            {
                render();
                repaint();
            }
        }
    }
}



void Grid::scrollHorz(int pixels)
{
    if (pixels == 0)
        return;
    
    int new_xoffset = m_xoff + pixels;

    if (pixels < 0)
    {
        if (new_xoffset < 0)
        {
            pixels += -(new_xoffset);
        }

        m_xoff += pixels;
    }
    else
    {
        int max_offset = m_total_width - (m_cliwidth - m_frozen_width);
        if (new_xoffset > max_offset)
        {
            pixels -= (new_xoffset-max_offset);
        }

        m_xoff += pixels;
    }

    updateScrollbars(wxHORIZONTAL);

    if (m_cliwidth-m_frozen_width-pixels <= 0 || m_xoff == 0)
    {
        render(NULL);
    }
    else
    {
        if (pixels < 0)
        {
            pixels = -pixels;
            
            m_memdc.Blit(m_frozen_width+pixels,
                         0,
                         m_cliwidth-m_frozen_width-pixels,
                         m_cliheight,
                         &m_memdc,
                         m_frozen_width,
                         0);
                         
            wxRect rect(m_frozen_width, 0, pixels, m_cliheight);
            render(&rect);
        }
        else
        {
            m_memdc.Blit(m_frozen_width, 0, m_cliwidth-m_frozen_width-pixels, m_cliheight,
                    &m_memdc, m_frozen_width+pixels, 0);
            wxRect rect(m_cliwidth-pixels, 0, pixels, m_cliheight);
            render(&rect);
        }
    }

    fireEvent(wxEVT_KCLGRID_HSCROLL, m_row_offset, m_xoff);
}

void Grid::scrollVert(int rows)
{
    m_row_offset += rows;

    if (m_row_offset < 0)
    {
        m_row_offset = 0;
    }

    if (m_model->getGroupBreaksActive())
    {
        if (m_row_offset > m_row_count-1)
        {
            m_row_offset = m_row_count-1;
        }
    }
    else
    {

        int visrows = getVisibleRowCount();
        if (m_row_offset > m_row_count-visrows+1)
        {
            m_row_offset = m_row_count-visrows+1;
            if (m_row_offset < 0)
            {
                m_row_offset = 0;
            }
        }
    }

    fireEvent(wxEVT_KCLGRID_VSCROLL, m_row_offset, m_xoff);
}


void Grid::updateScrollbars(int scrollbar)
{
    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    if (scrollbar == wxVERTICAL || scrollbar == -1)
    {
        if (m_row_height == 0)
            return;

        int visrows = getVisibleRowCount();
        int row_count = m_row_count;

        if (m_options & optGhostRow)
        {
            row_count++;
        }

        SetScrollbar(wxVERTICAL, m_row_offset, visrows, row_count, true);

        // client width may have changed since we modified scrollbar. refetch
        // information if we are going to be modifying the horizontal
        // parameters as well

        if (scrollbar == -1)
        {
            GetClientSize(&m_cliwidth, &m_cliheight);
        }
    }

    if (scrollbar == wxHORIZONTAL || scrollbar == -1)
    {
        calcColumnWidths();

        int scroll_width = m_total_width;

        if (m_xoff+cli_width-m_frozen_width > scroll_width)
            scroll_width = m_xoff+cli_width-m_frozen_width;

        SetScrollbar(wxHORIZONTAL,
                     m_xoff,
                     cli_width-m_frozen_width,
                     scroll_width, true);
    }
}



void Grid::onScroll(wxScrollWinEvent& event)
{
    if (isEditing())
    {
        endEdit(true);
        if (!m_last_edit_allowed)
        {
            return;
        }
    }

    int newpos;

    if (event.GetOrientation() == wxHORIZONTAL)
    {
        int oldpos = m_xoff;
        newpos = m_xoff;

        if (event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            newpos -= 30;
            if (newpos < 0)
                newpos = 0;
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            newpos = std::min(newpos+30, m_total_width - m_cliwidth + m_frozen_width);
        }
        
        if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            newpos -= (m_cliwidth*2)/3;
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            newpos += (m_cliwidth*2)/3;
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
        {
            #ifdef __WXGTK__
      
            int mousex, mousey;
            ::wxGetMousePosition(&mousex, &mousey);
            ScreenToClient(&mousex, &mousey);
            
            if (mousex < 20 || mousex > m_cliwidth-20)
            {
                // GTK scrolling always sends a thumbtrack,
                // so we have to handle it a little differently
                int p = event.GetPosition();
                
                if (p == m_xoff+1)
                    newpos = m_xoff+30;
                 else if (p == m_xoff-1)
                    newpos = m_xoff-30;
                 else newpos = event.GetPosition();
                 
                if (newpos > m_total_width - m_cliwidth + m_frozen_width)
                    newpos = m_total_width - m_cliwidth + m_frozen_width;
                if (newpos < 0)
                    newpos = 0;
            }
            else
            {
                newpos = event.GetPosition();
            }
            
            #else
            newpos = event.GetPosition();
            #endif
        }

        if (newpos != oldpos)
        {
            scrollHorz(newpos - oldpos);
            repaint();
        }
    }

    if (event.GetOrientation() == wxVERTICAL)
    {
        newpos = m_row_offset;
        int visrows = getVisibleRowCount();

        wxEventType event_type = event.GetEventType();

        if (event_type == wxEVT_SCROLLWIN_LINEUP)
        {
            if (newpos > 0)
            {
                newpos--;
            }
        }
        
        if (event_type == wxEVT_SCROLLWIN_LINEDOWN)
        {
            newpos++;
        }

        if (event_type == wxEVT_SCROLLWIN_PAGEUP)
        {
            int scroll_rows = visrows-1;

            if (m_model->getGroupBreaksActive())
            {
                scroll_rows /= 2;
            }

            newpos -= scroll_rows;
        }

        if (event_type == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            int scroll_rows = visrows-1; // (default)

            int y;
            for (y = m_cliheight; y >= m_row_height; --y)
            {
                int row;
                int col;
                if (hitTest(m_rowlabel_width+1, y, &row, &col))
                {
                    scroll_rows = (row - m_row_offset) - 1;
                    break;
                }
            }

            newpos += scroll_rows;
        }


        if (event_type == wxEVT_SCROLLWIN_THUMBTRACK)
        {
            if (m_options & optContinuousScrolling)
            {
                newpos = event.GetPosition();
            }
            else
            {
                #ifdef __WXGTK__
                /*
                GtkWidget* widget = GetHandle();
                if (widget)
                {
                    GdkWindow* wnd = widget->window;
                    if (wnd)
                    {
                        GdkModifierType state;
                        gint x = 0, y = 0;
                        
                        gdk_window_get_pointer (wnd,
                                                &x,
                                                &y,
                                                &state);
                        
                        if ((state & GDK_BUTTON1_MASK) == 0)
                        {
                            event_type = wxEVT_SCROLLWIN_THUMBRELEASE;
                        }
                    }
                }
                */
                event_type = wxEVT_SCROLLWIN_THUMBRELEASE;
                #endif
            }
        }

        if (event_type == wxEVT_SCROLLWIN_THUMBRELEASE)
        {
            newpos = event.GetPosition();

            if (m_model->getGroupBreaksActive() &&
                newpos == m_row_count-visrows)
            {
                newpos = m_row_count-1;
            }
        }


        if (newpos != m_row_offset)
        {
            bool busy_cursor = (abs(newpos - m_row_offset) > 30000);
            if (busy_cursor)
            {
                ::wxBeginBusyCursor();
            }

            scrollVert(newpos - m_row_offset);
            updateData();
            render();
            updateScrollbars(wxVERTICAL);
            repaint();

            if (busy_cursor)
            {
                ::wxEndBusyCursor();
            }
        }
    }
}

int Grid::getRowOffset()
{
    return m_row_offset;
}

void Grid::setRowOffset(int new_value)
{
    m_row_offset = new_value;
}

int Grid::getHorizontalOffset()
{
    return m_xoff;
}

void Grid::setHorizontalOffset(int new_value)
{
    m_xoff = new_value;
}



void Grid::moveCursor(int row, int col, bool repaint_flag)
{
    // note: for a long time, we returned if the cursor row and column
    // didn't change; however, it caused the cursor not to be drawn in 
    // some instances when switching between tables; specifically, when 
    // switching from cell (x,y) in table A to another cell in table B, 
    // then back to cell (x,y) in table A, the cursor wouldn't appear 
    // in table A, because when the focus moved away from a table A, 
    // the cursor would be hidden, and when clicking back on cell (x,y), 
    // although the cursor would be set back to visible, the cursor would 
    // still be on the same cell, preventing it from being drawn, since
    // the following logic simply returns if the cell is the same

    //if (row == m_cursor_row && col == m_cursor_col)
    //    return;

    // some bounds checking
    if (row < 0)
        row = 0;

    if (row >= m_row_count)
    {
        if (m_options & optGhostRow)
        {
            if (row > m_row_count)
                return;
        }
        else
        {
            return;
        }
    }

    if (col < 0 || col >= (int)m_viewcols.size())
        return;

    bool allow = true;
    kcl::GridEvent evt;
    evt.SetRow(m_cursor_row);
    evt.SetColumn(m_cursor_col);
    evt.SetDestinationRow(row);
    evt.SetDestinationColumn(col);
    fireEvent(wxEVT_KCLGRID_PRE_CURSOR_MOVE, evt, &allow);
    if (!allow)
    {
        return;
    }

    int old_row = m_cursor_row;
    int old_col = m_cursor_col;

    int cursor_modelcol = m_viewcols[col]->m_modelcol;
    int ctrl_type = ctrltypeText;
 

    CellData* celldata = getCellData(row, cursor_modelcol);
    if (celldata)
    {
        ctrl_type = celldata->ctrltype;
    }

    bool is_combo = (ctrl_type == ctrltypeComboBox ||
                     ctrl_type == ctrltypeDropList);


    // destroy edit control, if one exists
    if (isEditing())
    {
        endEdit(true);
    }

    m_cursor_row = row;
    m_cursor_col = col;
    m_cursor_modelcol = cursor_modelcol;

    if (m_cursor_row != old_row)
    {
        m_model->onCursorRowChanged();
    }

    if (repaint_flag)
    {
        if (m_cursor_type == cursorHighlight ||
            m_cursor_type == cursorRowHighlight ||
            m_cursor_type == cursorRowOutline)
        {
            render();
        }
        else
        {

            wxRect oldcell_rect;
            wxRect newcell_rect;
            getCellRect(old_row, old_col, &oldcell_rect);
            getCellRect(m_cursor_row, m_cursor_col, &newcell_rect);

            render(&oldcell_rect, false);
            if (!is_combo)
            {
                render(&newcell_rect);
            }
            
        }
    }

    if (is_combo)
    {
        beginEdit();
    }

    if (repaint_flag)
    {
        repaint();
    }
    
    // fire a move event
    kcl::GridEvent evt2;
    evt2.SetRow(old_row);
    evt2.SetColumn(old_col);
    evt2.SetDestinationRow(m_cursor_row);
    evt2.SetDestinationColumn(m_cursor_col);
    fireEvent(wxEVT_KCLGRID_CURSOR_MOVE, evt2);
}


void Grid::scrollToBottom()
{
    int visrows = getVisibleRowCount();
    m_row_offset = m_row_count - visrows;
    if (m_row_offset < 0)
        m_row_offset = 0;
}

void Grid::scrollToTop()
{
    m_row_offset = 0;
}

bool Grid::scrollVertToCursor()
{
    // returns true if cursor is outside visible range
    // and row offset is adjusted; false otherwise

    return scrollVertToRow(m_cursor_row);
}

bool Grid::scrollHorzToCursor()
{
    // returns true if cursor is outside visible range
    // and x offset is adjusted; false otherwise

    return scrollHorzToColumn(m_cursor_col);
}

bool Grid::scrollVertToRow(int row)
{
    // returns true if new row is outside visible range
    // and row offset is adjusted to scroll to the new
    // row; false otherwise

    bool offset_adjusted = false;
    int visrows = getVisibleRowCount();
    int breakrows = getVisibleGroupBreakCount();

    // note: don't do anything if the row is already visible

    // if the row position is before the top row, scroll
    // so that the new row is the top row
    if (row <= m_row_offset)
    {
        m_row_offset = row;
        offset_adjusted = true;
    }

    // if the row position is after the bottom row, scroll
    // so that the new row is the bottom row
    if (row >= m_row_offset + visrows - breakrows)
    {
        m_row_offset = row - visrows + breakrows + 1;
        offset_adjusted = true;
    }
    
    // make sure the row offset isn't less than zero
    if (m_row_offset < 0)
        m_row_offset = 0;

    return offset_adjusted;
}

bool Grid::scrollHorzToColumn(int col)
{
    // returns true if new column is outside visible range
    // and x offset is adjusted to scroll to the new
    // column; false otherwise

   if (col < 0 || col >= (int)m_viewcols.size())
        return false;

    bool offset_adjusted = false;
    int xoff = 0;
    
    for (int idx = 0; idx < col; idx++)
    {
        xoff += m_viewcols[idx]->m_pixwidth;
    }

    if (xoff-m_xoff < 0)
    {
        m_xoff = xoff+1;
        offset_adjusted = true;        
    }
    else
    {
        int width = m_viewcols[col]->m_pixwidth;
        if ((xoff-m_xoff) + width + m_frozen_width > m_cliwidth)
        {
            m_xoff = (xoff+width-(m_cliwidth-m_frozen_width));
            offset_adjusted = true;
        }
    }
    
    return offset_adjusted;
}

bool Grid::isCursorVisible()
{
    int visrows = getVisibleRowCount();

    return (m_cursor_row >= m_row_offset &&
            m_cursor_row < m_row_offset+visrows) ? true : false;
}


void Grid::moveCol(ViewColumn* from_col, ViewColumn* to_col)
{
    m_viewcol_lookup.clear();

    std::vector<ViewColumn*>::iterator col_it;
    int insert_idx = 0;
    if (to_col != NULL)
    {
        // insert the column in it's new place
        for (col_it = m_viewcols.begin(); col_it != m_viewcols.end(); ++col_it)
        {
            if (*col_it == to_col)
                break;

            insert_idx++;
        }
    }

    for (col_it = m_viewcols.begin(); col_it != m_viewcols.end(); ++col_it)
    {
        if (*col_it == from_col)
        {
            // delete from column
            m_viewcols.erase(col_it);
            break;
        }
    }

    std::vector<ViewColumn*>::iterator insert_it;
    if (to_col == NULL)
        insert_it = m_viewcols.end();
         else
        insert_it = m_viewcols.begin() + insert_idx;

    m_viewcols.insert(insert_it, m_action_col);
}


void Grid::onCellClicked(int row, int col)
{
    int modelcol = m_viewcols[col]->m_modelcol;

    CellData* data = getCellData(row, modelcol);
    
    if (data->ctrltype == ctrltypeCheckBox && data->editable)
    {
        // first, make sure they hit the checkbox
        wxRect rect;
        getCellRect(row, col, &rect);
        if (rect.IsEmpty())
            return;
        
        int mouse_x, mouse_y;
        ::wxGetMousePosition(&mouse_x, &mouse_y);
        this->ScreenToClient(&mouse_x, &mouse_y);
        while (rect.GetWidth() > 16)
            rect.Deflate(1, 0);
        while (rect.GetHeight() > 16)
            rect.Deflate(0, 1);
        if (!rect.Contains(mouse_x, mouse_y))
            return;

        // since we're inside the checkbox rectangle, make the value change
        bool bval = data->m_boolvalue;
        bool allow;
        
        fireEvent(wxEVT_KCLGRID_BEGIN_EDIT, row, col, &allow);
        if (!allow)
            return;

        bval = !data->m_boolvalue;

        kcl::GridEvent evt;
        evt.SetRow(m_cursor_row);
        evt.SetColumn(m_cursor_col);
        evt.SetBoolean(bval);
        
        fireEvent(wxEVT_KCLGRID_END_EDIT, evt, &allow);
        if (!allow)
            return;

        data = getCellData(row, modelcol);
        data->m_boolvalue = bval;

        m_model->setCellBoolean(row, modelcol, bval);

        fireEvent(wxEVT_KCLGRID_MODEL_CHANGE, row, modelcol);

        refresh(refreshPaint);
        return;
    }

}

int Grid::getColumnFromXpos(int xpos)
{
    int xoff = -m_xoff + m_frozen_width;

    for (size_t col = 0; col < m_viewcols.size(); col++)
    {
        if (xpos >= xoff && xpos < xoff+m_viewcols[col]->m_pixwidth)
            return (int)col;
        xoff += m_viewcols[col]->m_pixwidth;
    }
    return -1;
}

void Grid::doGhostRowInsert()
{
    bool allow = true;

    fireEvent(wxEVT_KCLGRID_PRE_GHOST_ROW_INSERT, m_cursor_row, m_cursor_col, &allow);

    if (!allow)
    {
        return;
    }

    insertRow(-1);
    refresh(refreshAll);

    fireEvent(wxEVT_KCLGRID_GHOST_ROW_INSERT, m_cursor_row, m_cursor_col);
}

void Grid::doInvalidAreaInsert(int mouse_x, int mouse_y)
{
    // determine what column was clicked on (row will be -1)
    int row, col;
    hitTest(mouse_x, mouse_y, &row, &col);

    // calculate how many rows we want to insert
    int rows_to_insert = 0;
    int invalid_y = m_invalid_yoffset;
    while (invalid_y <= mouse_y)
    {
        rows_to_insert++;
        invalid_y += m_row_height;
    }
    
    // the row is going to be -1 because the user has clicked past
    // the grid's row count, so reassign the row variable to be
    // the hypothetical row that the user clicked on
    row = getRowCount()+rows_to_insert-1;
    
    // fire the pre invalid area insert event to see if the
    // caller is going to allow this event to occur
    bool allow = true;
    
    fireEvent(wxEVT_KCLGRID_PRE_INVALID_AREA_INSERT, row, col, &allow);

    if (!allow)
    {
        return;
    }

    // insert the rows
    while (rows_to_insert-- > 0)
        insertRow(-1);
    
    refresh(refreshAll);

    fireEvent(wxEVT_KCLGRID_INVALID_AREA_INSERT, row, col);
}

bool Grid::doButtonClick(int mouse_x, int mouse_y)
{
    int row, col;
    if (!hitTest(mouse_x, mouse_y, &row, &col))
    {
        return false;
    }

    CellData* celldata = getCellData(row, m_viewcols[col]->m_modelcol);
    if (!celldata)
    {
        return false;
    }

    // find out if the cell is a button

    if (celldata->ctrltype == ctrltypeButton)
    {
        // it's a button

        m_mouse_action = actionPressButton;
        m_action_col = m_viewcols[col];
        m_action_colxoff = 0;
        m_action_xpos = mouse_x;
        m_action_lastxpos = mouse_x;
        m_action_viewcol = col;
        m_action_row = row;

        wxRect cellrect;
        getCellRect(row, col, &cellrect);
        render(&cellrect);
        repaint();

        CaptureMouse();
        return true;
    }

    if (celldata->ctrltype == ctrltypeColor)
    {
        // it's a color control

        m_mouse_action = actionPressColor;
        m_action_col = m_viewcols[col];
        m_action_colxoff = 0;
        m_action_xpos = mouse_x;
        m_action_lastxpos = mouse_x;
        m_action_ypos = mouse_y;
        m_action_lastypos = mouse_y;
        m_action_viewcol = col;
        m_action_row = row;

        wxRect cellrect;
        getCellRect(row, col, &cellrect);
        render(&cellrect);
        repaint();

        CaptureMouse();
        return true;
    }

    return false;
}


void Grid::onMouse(wxMouseEvent& event)
{
    if (event.GetEventType() == wxEVT_LEFT_DOWN)
    {
        m_action_xpos = 0;
        m_action_ypos = 0;
    
        if (isEditing())
            endEdit(true);

        // any left button click sets the focus to the grid
        SetFocus();

        if (m_options & optGhostRow)
        {
            if (m_invalid_yoffset != -1)
            {
                if (event.m_y > m_invalid_yoffset)
                {
                    int row, col;
                    hitTest(event.m_x, event.m_y, &row, &col);
                    if (col != -1)
                    {
                        moveCursor(getRowCount(), col);
                    }
                }
            }
        }

        ViewColumn* mouse_column;
        mouse_column = NULL;

        if (event.m_y < m_header_height)
        {
            if (event.m_x < m_rowlabel_width)
            {
                // user clicked on the table label (upper-left corner of grid)

                m_selection.clear();
                m_action_selrect = m_selection.addSelection();
                m_action_selrect->m_start_row = 0;
                m_action_selrect->m_start_col = 0;
                m_action_selrect->m_end_row = -1;
                m_action_selrect->m_end_col = -1;

                render();
                repaint();
                return;
            }
            else
            {
                // user clicked on one of the column labels

                // determine column
                std::vector<ColumnRange>::iterator it;
                for (it = m_colranges.begin(); it != m_colranges.end(); ++it)
                {
                    if (!it->viewcol->m_resizable)
                        continue;
                    
                    if (event.m_x > (it->xoff + it->width - 3) &&
                        event.m_x < (it->xoff + it->width + 3))
                    {
                        if (m_options & optColumnResize)
                        {
                            m_mouse_action = actionResizeCol;
                            m_action_col = it->viewcol;
                            m_action_colxoff = it->xoff;
                            m_action_xpos = event.m_x;
                            m_action_lastxpos = event.m_x;
                            m_action_viewcol = it->viewcol_idx;
                            CaptureMouse();
                            return;
                        }
                    }


                    if (event.m_x > it->xoff && event.m_x < it->xoff+it->width)
                    {
                        if (fireEvent(wxEVT_KCLGRID_COLUMN_LEFT_CLICK, -1, it->viewcol_idx))
                            return;
                    }


                    if (m_options & optSelect && 
                        event.m_x > it->xoff && event.m_x < it->xoff+it->width)
                    {
                        m_cursor_col = it->viewcol_idx;
                        
                        m_mouse_action = actionSelectCol;
                        m_action_col = it->viewcol;
                        m_action_colxoff = it->xoff;
                        m_action_xpos = event.m_x;
                        m_action_lastxpos = event.m_x;
                        m_action_viewcol = it->viewcol_idx;
                        
                        // clear any selections we might have, if ctrl is not depressed
                        if (!event.ControlDown() && !event.ShiftDown() &&
                            m_selection.getSelectionCount() > 0)
                        {
                            m_selection.clear();

                            // render off-screen buffer without cursor
                            render(NULL, false);
                            repaint();
                        }
                        else
                        {
                            // render off-screen buffer without cursor
                            render(NULL, false);
                        }

                        CaptureMouse();
                        return;
                    }
                }
            }
        }
        else
        {
            int col;
            int row = 0;

            hitTest(event.m_x, event.m_y, &row, &col);
            if (row == -1)
            {
                if (m_options & optInvalidAreaClickable)
                    fireEvent(wxEVT_KCLGRID_INVALID_AREA_CLICK, row, col);
                return;
            }
            
            if (event.m_x < m_rowlabel_width)
            {
                // check if the user is resizing the rows

                int yoff = m_header_height;
                int rowdata_size = m_rowdata.size();
                int i;

                for (i = 0; i < rowdata_size; ++i)
                {
                    yoff += m_row_height;

                    if (abs(event.m_y-yoff) <= 1)
                    {
                        m_mouse_action = actionResizeRow;
                        m_action_ypos = event.m_y;
                        m_action_row = event.m_y;
                        CaptureMouse();
                        return;
                    }
                }


                // user clicked on row label

                if (fireEvent(wxEVT_KCLGRID_ROW_LEFT_CLICK, row, -1))
                    return;


                {
                    m_mouse_action = actionSelectRows;
                        
                    if (event.ShiftDown())
                    {
                        // create a starting point for our shift-selection
                        if (m_selection.getSelectionCount() == 0)
                        {
                            m_action_selrect = m_selection.addSelection();
                            m_action_selrect->m_start_row = m_cursor_row;
                            m_action_selrect->m_end_row = m_cursor_row;
                            m_action_selrect->m_start_col = 0;
                            m_action_selrect->m_end_col = -1;
                        }
                        else
                        {
                            m_action_selrect = m_selection.getSelection(m_selection.getSelectionCount()-1);
                        }
                        
                        if (m_action_selrect->m_start_col == 0 &&
                            m_action_selrect->m_end_col == -1)
                        {
                            m_action_selrect->m_start_row = std::min(row, m_cursor_row);
                            m_action_selrect->m_end_row = std::max(row, m_cursor_row);
                            
                            render();
                            repaint();

                            kcl::GridEvent evt;
                            evt.SetRow(m_cursor_row);
                            evt.SetColumn(m_cursor_col);
                            evt.SetUserEvent(true);
                            fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE, evt);

                            return;
                        }
                    }


                    // at this point the user might be making a row selection,
                    // so set the mode to actionSelectRows

                    m_mouse_action = actionSelectRows;
                    if (!event.ControlDown())
                    {
                        m_selection.clear();
                        render();
                        repaint();
                    }
                    m_action_xpos = event.m_x;
                    m_action_selrect = m_selection.addSelection();
                    m_action_selrect->m_start_row = row;
                    m_action_selrect->m_start_col = 0;
                    m_action_selrect->m_end_row = row;
                    m_action_selrect->m_end_col = -1;

                    moveCursor(m_action_selrect->m_start_row, 0);
                    render();
                    repaint();

                    CaptureMouse();
                }

                return;
            }
            else
            {
                // user clicked on cell
                if (col == -1)
                    return;

                if (col >= 0 && col <=(int) m_viewcols.size())
                {
                    if (m_viewcols[col]->m_separator)
                        return;
                }


                if (m_options & optSelectionList)
                {
                    if (event.ShiftDown() && m_selection.getSelectionCount() > 0)
                    {
                        SelectionRect* last_sel = m_selection.getSelection(m_selection.getSelectionCount()-1);
                        
                        if (last_sel->m_start_col == 0 &&
                            last_sel->m_end_col == -1)
                        {
                            if (row < last_sel->m_start_row)
                            {
                                last_sel->m_start_row = row;
                            }
                            else
                            {
                                last_sel->m_end_row = row;
                            }

                            render();
                            repaint();

                            kcl::GridEvent evt;
                            evt.SetRow(m_cursor_row);
                            evt.SetColumn(m_cursor_col);
                            evt.SetUserEvent(true);
                            fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE, evt);

                            return;
                        }
                    }

                    // if we are clicking on a cell that is not already
                    // selected, we need to clear the existing selection
                    bool cell_already_selected = 
                                  m_selection.getCellSelected(row, col);
                                  
                    if (!event.ControlDown() &&
                        m_selection.getSelectionCount() > 0 &&
                        !cell_already_selected)
                    {
                        m_selection.clear();
                    }
                    
                    // a click on a cell will automatically select the
                    // entire row -- but only do so if the cell that
                    // was selected was not already selected
                    if (!cell_already_selected)
                    {
                        m_mouse_action = actionSelectRows;

                        m_action_xpos = event.m_x;
                        m_action_selrect = m_selection.addSelection();
                        m_action_selrect->m_start_row = row;
                        m_action_selrect->m_start_col = 0;
                        m_action_selrect->m_end_row = row;
                        m_action_selrect->m_end_col = -1;

                        // fire selection change event
                        kcl::GridEvent evt;
                        evt.SetRow(m_cursor_row);
                        evt.SetColumn(m_cursor_col);
                        evt.SetUserEvent(true);
                        fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE, evt);

                        render();
                        repaint();
                    }
                }
                else   // not a selection list
                {
                    // at this point the user might be making a cell
                    // selection, so set the mode to actionSelect
                    if (m_options & optSelect)
                    {
                        int sel_count = m_selection.getSelectionCount();
                        
                        // handle shift-selection
                        if (event.ShiftDown() && !event.ControlDown() && sel_count < 2)
                        {
                            kcl::GridEvent evt;
                            evt.SetRow(m_cursor_row);
                            evt.SetColumn(m_cursor_col);
                            evt.SetUserEvent(true);
                            fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE, evt);

                            // get a selection started
                            m_mouse_action = actionSelect;
                            m_action_selrect = (sel_count == 0) ? m_selection.addSelection()
                                                                : m_selection.getSelection(0);
                            m_action_selrect->m_start_row = std::min(m_cursor_row, row);
                            m_action_selrect->m_end_row = std::max(m_cursor_row, row);
                            m_action_selrect->m_start_col = std::min(m_cursor_col, col);
                            m_action_selrect->m_end_col = std::max(m_cursor_col, col);
                            
                            render();
                            repaint();

                            CaptureMouse();
                            return;
                        }
                    }
                }
                
                if (fireEvent(wxEVT_KCLGRID_CELL_LEFT_CLICK, row, col))
                {
                    return;
                }

                if (doButtonClick(event.m_x, event.m_y))
                {
                    return;
                }

                // move the cursor to that point
                moveCursor(row, col);

                onCellClicked(row, col);

                if (m_options & optSingleClickEdit)
                {
                    if (!isEditing())
                    {
                        beginEditWithPositioning(event.m_x, event.m_y);
                    }

                    return;
                }

                m_action_xpos = event.m_x;
                m_action_ypos = event.m_y;

                // at this point the user might be making a cell selection,
                // so set the mode to actionSelect

                if (m_options & optSelect)
                {
                    if (!event.ControlDown() && m_selection.getSelectionCount() > 0)
                    {
                        m_selection.clear();
                        render();
                        repaint();

                        // fire selection change event
                        kcl::GridEvent evt;
                        evt.SetRow(m_cursor_row);
                        evt.SetColumn(m_cursor_col);
                        evt.SetUserEvent(true);
                        fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE, evt);
                    }


                    m_mouse_action = actionSelect;
                    m_action_selrect = m_selection.addSelection();
                    m_action_selrect->m_start_row = row;
                    m_action_selrect->m_start_col = col;
                    m_action_selrect->m_end_row = row;
                    m_action_selrect->m_end_col = col;


                    // so the selection redraws correctly
                    if (m_selection.getSelectionCount() == 2)
                    {
                        render();
                        repaint();
                    }

                    CaptureMouse();
                }
            }
        }
    }

    if (event.GetEventType() == wxEVT_LEFT_UP)
    {
        if (GetCapture() == this)
        {
            ReleaseMouse();
        }

        int action_xpos = m_action_xpos;
        int action_ypos = m_action_ypos;

        m_action_xpos = 0;
        m_action_ypos = 0;

        if (m_scroll_timer.IsRunning())
        {
            m_scroll_timer.Stop();
        }
        
        if (m_mouse_action == actionSelectCol)
        {
            m_mouse_action = actionNone;

            // if there are no rows, don't add any new selection
            if (m_row_count == 0)
            {
                render();
                repaint();
                return;
            }

            int col = 0;

            std::vector<ColumnRange>::iterator it;
            for (it = m_colranges.begin();
                 it != m_colranges.end();
                 ++it)
            {
                if (event.m_x > (it->xoff) &&
                    event.m_x < (it->xoff + it->width))
                {
                    break;
                }

                col++;
            }


            col = m_colranges[col].viewcol_idx;

            if (event.ShiftDown() && m_selection.getSelectionCount() > 0)
            {
                SelectionRect* last_sel = m_selection.getSelection(m_selection.getSelectionCount()-1);
                if (last_sel->m_end_row == -1)
                {
                    if (col < last_sel->m_start_col)
                    {
                        last_sel->m_start_col = col;
                    }
                    else
                    {
                        last_sel->m_end_col = col;
                    }

                    render();
                    repaint();

                    kcl::GridEvent evt;
                    evt.SetRow(m_cursor_row);
                    evt.SetColumn(m_cursor_col);
                    evt.SetUserEvent(true);
                    fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE, evt);

                    return;
                }
            }

            // don't select a separator column
            if (m_viewcols[col]->m_separator)
            {
                return;
            }

            // if column is already selected, don't add any new selection
            if (m_selection.getColumnSelected(col))
            {
                return;
            }

            m_action_selrect = m_selection.addSelection();
            m_action_selrect->m_start_row = 0;
            m_action_selrect->m_start_col = col;
            m_action_selrect->m_end_row = -1;
            m_action_selrect->m_end_col = col;

            render();
            repaint();

            kcl::GridEvent evt;
            evt.SetRow(m_cursor_row);
            evt.SetColumn(m_cursor_col);
            evt.SetUserEvent(true);
            fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE, evt);
        }
        else if (m_mouse_action == actionMoveCol)
        {
            // deselect the move column bitmap, allowing it
            // to be destroyed
            m_movecol_dc.SelectObject(wxNullBitmap);

            m_action_col->m_draw = true;
            m_mouse_action = actionNone;

            updateData();
            render();
            Refresh(false);
            updateScrollbars(wxHORIZONTAL);
        }
        else if (m_mouse_action == actionResizeCol)
        {
        }
        else if (m_mouse_action == actionResizeRow)
        {
            if (m_options & optRowResize)
            {
                int new_row_height = m_row_height + (event.m_y - action_ypos);
                
                if (new_row_height < 3)
                    new_row_height = 3;
                if (new_row_height > 200)
                    new_row_height = 200;

                if (m_row_height != new_row_height)
                {
                    m_row_height = new_row_height;
                    m_dip_row_height = toDIP(m_row_height);
                
                    updateData();
                    render();
                    repaint();

                    GridEvent evt;
                    evt.SetInt(m_row_height);
                    evt.SetUserEvent(true);
                    fireEvent(wxEVT_KCLGRID_ROW_RESIZE, evt);
                }
            }
        }
        else if (m_mouse_action == actionSelect)
        {
            // this is where normal clicks end up when
            // the main mouse button is released
            
            m_mouse_action = actionNone;


            int row = m_action_selrect->m_end_row, col = m_action_selrect->m_end_col;


            if (m_action_selrect->m_start_row == m_action_selrect->m_end_row &&
                m_action_selrect->m_start_col == m_action_selrect->m_end_col)
            {
                // a selection really ended up not happening
                m_selection.removeSelection(m_action_selrect);
                
                
                int model_col = getColumnModelIdx(col);
                if (model_col != -1)
                {
                    int r,c, cell_xoff = 0;
                    hitTest(event.m_x, event.m_y, &r, &c, &cell_xoff);

                    wxString cell_str = getCellString(row, model_col);

                    kcl::CellProperties props;
                    getCellProperties(row, model_col, &props);
                    
                    // if the hyperlink option is active and we find something
                    // that looks like a hyperlink, then trigger a left-click
                    // event on a link if the link is clicked on
                    if ((m_options & optActivateHyperlinks) || props.hyperlink)
                    {
                        int url_pos = cell_str.Find(wxT("://"));
                        if (props.hyperlink || (url_pos >= 4 && url_pos <= 10))
                        {
                            wxClientDC dc(this);
                            wxSize text_extent = dc.GetTextExtent(cell_str);

                            if (cell_xoff < text_extent.GetWidth())
                            {
                                GridEvent evt;
                                evt.SetColumn(model_col);
                                evt.SetRow(row);
                                evt.SetString(cell_str);
                                evt.SetUserEvent(true);
                                fireEvent(wxEVT_KCLGRID_LINK_LEFTCLICK, evt);
                            }
                        }
                    }
                }
            }
        }
        else if (m_mouse_action == actionSelectRows)
        {
            m_mouse_action = actionNone;

            if (!event.ControlDown())
            {
                int i = 0;
                for (i = 0; i < m_selection.getSelectionCount(); ++i)
                {
                    SelectionRect* r = m_selection.getSelection(i);
                    if (r != m_action_selrect)
                    {
                        m_selection.removeSelection(r);
                        i = -1;
                        continue;
                    }
                }

                render();
                repaint();
            }

        }
        else if (m_mouse_action == actionPressButton)
        {
            m_mouse_action = actionNone;
            wxRect cellrect;
            getCellRect(m_action_row, m_action_viewcol, &cellrect);
            render(&cellrect);
            repaint();

            fireEvent(wxEVT_KCLGRID_BUTTON_CLICK, m_action_row, m_action_viewcol);
        }
        else if (m_mouse_action == actionPressColor)
        {
            m_mouse_action = actionNone;
            wxRect cellrect;
            getCellRect(m_action_row, m_action_viewcol, &cellrect);
            render(&cellrect);
            repaint();

            // determine if the foreground or background color was clicked
            bool fgcolor_clicked = false;
            wxRect fgcolor_rect, bgcolor_rect;
            getColorRectangles(cellrect.x, cellrect.y, m_row_height,
                               &fgcolor_rect, &bgcolor_rect);
            
            wxPoint action_pt(action_xpos, action_ypos);
            wxPoint mouseup_pt = event.GetPosition();
            
            if (fgcolor_rect.Contains(action_pt) &&
                fgcolor_rect.Contains(mouseup_pt))
            {
                fgcolor_clicked = true;
            }
            else if (bgcolor_rect.Contains(action_pt) &&
                      bgcolor_rect.Contains(mouseup_pt))
            {
                fgcolor_clicked = false;
            }
            else
            {
                return;
            }
            
            // get the celldata's foreground and background colors
            int modelcol = getColumnModelIdx(m_action_viewcol);
            CellData* celldata = getCellData(m_action_row, modelcol);

            GridEvent evt;
            evt.SetColumn(m_action_viewcol);
            evt.SetRow(m_action_row);
            evt.SetForegroundColor(celldata->fgcolor);
            evt.SetBackgroundColor(celldata->bgcolor);
            evt.SetBoolean(fgcolor_clicked);
            evt.SetUserEvent(true);
            fireEvent(wxEVT_KCLGRID_COLOR_CLICK, evt);
        }
        else
        {
            // see if the user clicked on the column header row
            if (event.m_y < m_header_height)
            {
                if (m_options & optCaptionsEditable &&
                    m_options & optSingleClickEdit)
                {
                    std::vector<ColumnRange>::iterator it;
                    for (it = m_colranges.begin();
                         it != m_colranges.end(); ++it)
                    {
                        if (event.m_x > (it->xoff) &&
                            event.m_x < (it->xoff + it->width))
                        {
                            // check to see if this action is allowed
                            bool allow = true;
                            
                            if (fireEvent(wxEVT_KCLGRID_BEGIN_CAPTION_EDIT,
                                          -1, it->viewcol_idx, &allow))
                            {
                                if (!allow)
                                    return;
                                    
                                beginCaptionEdit(it->viewcol_idx);
                                return;
                            }
                        }
                    }
                }
            }
        }

        m_mouse_action = actionNone;
    }

    if (event.GetEventType() == wxEVT_MIDDLE_UP)
    {
        // if we're middle-clicking on a link, fire the middle-click link event
        int row, col, cell_xoff = 0;
        hitTest(event.m_x, event.m_y, &row, &col);

        int model_col = getColumnModelIdx(col);
        if (row != -1 && model_col != -1)
        {
            wxString cell_str = getCellString(row, model_col);

            kcl::CellProperties props;
            getCellProperties(row, model_col, &props);

            // if the hyperlink option is active and we find something
            // that looks like a hyperlink, then trigger a left-click
            // event on a link if the link is clicked on
            if (props.hyperlink || (m_options & optActivateHyperlinks))
            {
                int url_pos = cell_str.Find(wxT("://"));
                if (props.hyperlink || (url_pos >= 4 && url_pos <= 10))
                {
                    wxClientDC dc(this);
                    wxSize text_extent = dc.GetTextExtent(cell_str);

                    if (cell_xoff < text_extent.x)
                    {
                        GridEvent evt;
                        evt.SetColumn(model_col);
                        evt.SetRow(row);
                        evt.SetString(cell_str);
                        evt.SetUserEvent(true);
                        fireEvent(wxEVT_KCLGRID_LINK_MIDDLECLICK, evt);
                    }
                }
            }
        }
    }

    if (event.GetEventType() == wxEVT_RIGHT_DOWN)
    {
        if (m_mouse_action != actionNone)
            return;
            
        if (m_invalid_yoffset != -1)
        {
            if (event.m_y >= m_invalid_yoffset)
                return;
        }

        if (isEditing())
        {
            endEdit(true);
        }

        int row, col;
        hitTest(event.m_x, event.m_y, &row, &col);
        
        if (event.m_x < m_rowlabel_width)
        {
            if (row != -1)
            {
                if (fireEvent(wxEVT_KCLGRID_ROW_RIGHT_CLICK, row, -1))
                    return;
            }
        }

        if (col == -1)
        {
            return;
        }

        if (event.m_y < m_header_height)
        {
            // fire a right click event
            fireEvent(wxEVT_KCLGRID_COLUMN_RIGHT_CLICK, -1, col);
            return;
        }
        else
        {
            if (row == -1)
                return;
            
            // if we didn't right-click on a selection area,
            // clear the selection and move the cursor to the
            // cell we right-clicked
            if (!isCellSelected(row,col))
            {
                clearSelection();
                moveCursor(row, col);
            }
            
            fireEvent(wxEVT_KCLGRID_CELL_RIGHT_CLICK, row, col);
        }
    }

    if (event.GetEventType() == wxEVT_LEFT_DCLICK)
    {
        // this takes care of some wxGTK mouse-click order issues, but
        // might possibly be valid for windows as well
        if (m_mouse_action == actionSelect ||
            m_mouse_action == actionSelectCol ||
            m_mouse_action == actionSelectRows)
        {
            if (GetCapture() == this)
            {
                ReleaseMouse();
            }

            m_mouse_action = actionNone;
        }

        // first, see if the user clicked on the column header row
        if (event.m_y < m_header_height)
        {
            for (std::vector<ColumnRange>::iterator it = m_colranges.begin();
                    it != m_colranges.end(); ++it)
            {
                if (!it->viewcol->m_resizable)
                    continue;
                
                if (m_options & optColumnResize)
                {
                    if (event.m_x > (it->xoff + it->width - 5) &&
                        event.m_x < (it->xoff + it->width + 5))
                    {
                        // user double-clicked on a column_resizer,
                        // so resize all selected column

                        int col_count = m_viewcols.size();
                        std::vector<int> to_resize;
                        for (int i = 0; i < col_count; ++i)
                        {
                            if (isColumnSelected(i))
                                to_resize.push_back(i);
                        }

                        if (to_resize.size() == 0)
                            to_resize.push_back(it->viewcol_idx);

                        autoColumnResizeInternal(to_resize);
                        return;
                    }
                }
                
                if (m_options & optCaptionsEditable &&
                    !(m_options & optSingleClickEdit))
                {
                    if (event.m_x > (it->xoff) &&
                        event.m_x < (it->xoff + it->width))
                    {
                        // check to see if this action is allowed
                        bool allow = true;
                        
                        if (fireEvent(wxEVT_KCLGRID_BEGIN_CAPTION_EDIT, -1, it->viewcol_idx, &allow))
                        {
                            if (!allow)
                                return;
                                
                            beginCaptionEdit(it->viewcol_idx);
                            return;
                        }
                    }
                }
            }

            return;
        }
        
        if (event.m_x < m_rowlabel_width)
        {
            // double click on row label sash
            
            int yoff = m_header_height;
            int rowdata_size = m_rowdata.size();
            int i;
            bool allow;

            for (i = 0; i < rowdata_size; ++i)
            {
                yoff += m_row_height;

                if (abs(event.m_y-yoff) <= 1)
                {
                    fireEvent(wxEVT_KCLGRID_ROW_SASH_DCLICK, i, -1, &allow);
                    return;
                }
            }
        
        }
        

        int row, col;

        bool do_ghost_row_insert = false;
        bool do_invalid_area_insert = false;

        if (!hitTest(event.m_x, event.m_y, &row, &col))
        {
            if (!m_model->getGroupBreaksActive() &&
                (m_options & optGhostRow) &&
                event.m_y >= m_invalid_yoffset-m_row_height &&
                event.m_y < m_invalid_yoffset)
            {
                do_ghost_row_insert = true;
                row = getRowCount();
            }
            else if (!m_model->getGroupBreaksActive() &&
                      (m_options & optInvalidAreaClickable) &&
                      row == -1)
            {
                do_invalid_area_insert = true;
            }
            else
            {
                return;
            }
        }

        if (col == -1 &&  !(m_options & optInvalidAreaClickable))
            return;

        // this check catches a problem where the the user is using the mouse
        // wheel and the double-click speed is very slow.  In the abscence of
        // this check, bad things can happen, like a cell edit starting that
        // the user didn't want

        if (row != m_cursor_row &&
             !(m_options & optGhostRow || m_options & optInvalidAreaClickable))
        {
            return;
        }

        bool dblclick_handled = false;

        if (!do_ghost_row_insert && !do_invalid_area_insert)
        {
            // if it's a button, do nothing
            if (doButtonClick(event.m_x, event.m_y))
                return;

            dblclick_handled = fireEvent(wxEVT_KCLGRID_CELL_LEFT_DCLICK, row, col);
        }
               
        if (!dblclick_handled)
        {
            if (do_ghost_row_insert)
            {
                doGhostRowInsert();
                row = getRowCount()-1;
                moveCursor(row, (col == -1) ? 0 : col);
            }
            
            if (do_invalid_area_insert)
            {
                doInvalidAreaInsert(event.m_x, event.m_y);
                row = getRowCount()-1;
                moveCursor(row, (col == -1) ? 0 : col);
            }

            if (col != -1)
                onCellClicked(row, col);

            if (!(m_options & optSingleClickEdit))
            {
                // if do_invalid_area_insert is true, an editor control
                // should have already been created
                if (!do_invalid_area_insert)
                {
                    beginEditWithPositioning(event.m_x, event.m_y);
                }
            }
        }
    }

    if (event.GetEventType() == wxEVT_MOTION)
    {
        int scroll_vert = 0;
        int scroll_horz = 0;


        if (m_mouse_action == actionSelect ||
            m_mouse_action == actionSelectRows)
        {
            if (!event.LeftIsDown())
            {
                // this fixes a problem when a drag operation is happening.
                // sometimes an additional select operation can start,
                // confusing the grid.  This is a fix, but probably doesn't
                // fix the root of the problem.

                if (GetCapture() == this)
                {
                    ReleaseMouse();
                }

                m_mouse_action = actionNone;
                return;
            }

            if (event.m_y > m_cliheight)
            {
                scroll_vert = 1;
            }
            else if (event.m_y < 0)
            {
                scroll_vert = -1;
            }
            
            if (m_total_width > m_cliwidth)
            {
                if (event.m_x > m_cliwidth)
                {
                    scroll_horz = 30;
                }
                else if (event.m_x < 0)
                {
                    scroll_horz = -30;
                }
            }
        }

        if (scroll_vert == 0 && scroll_horz == 0)
        {
            m_scroll_timer_voffset = 0;
            m_scroll_timer_hoffset = 0;

            if (m_scroll_timer.IsRunning())
            {
                m_scroll_timer.Stop();
            }
        }
        else
        {
            m_scroll_timer_voffset = scroll_vert;
            m_scroll_timer_hoffset = scroll_horz;

            if (!m_scroll_timer.IsRunning())
            {
                m_scroll_timer.Start(30);
            }
        }

        if ((m_mouse_action == actionNone ||
             m_mouse_action == actionSelectRows)
                && event.LeftIsDown()
                && m_action_xpos > m_rowlabel_width
                && m_action_xpos != 0)
        {
            // a spurious move event used to reach here on wxMSW.  Steps
            // to reproduce are 1) create a control, 2) click in the invalid
            // area.  Control destroys, and then a move event gets fired.  For
            // now, we mask this event by testing for m_action_xpos != 0.
            // m_action_xpos is now initialized to zero at the beginning on a
            // left down operation.
            m_mouse_action = actionNone;

            if (m_action_xpos > 0 && m_action_xpos < m_cliwidth &&
                m_action_ypos > 0 && m_action_ypos < m_cliheight &&
                abs(event.m_x - m_action_xpos) + abs(event.m_y - m_action_ypos) > 5)
            {
                int row, col;
                hitTest(m_action_xpos, m_action_ypos, &row, &col);
                fireEvent(wxEVT_KCLGRID_BEGIN_DRAG, row, col);
                return;
            }
        }


        if (m_mouse_action == actionResizeCol)
        {
            if (m_action_col->m_prop_size != 0)
            {
                if (m_action_viewcol < (int)m_viewcols.size())
                {
                    ViewColumn* resize_viewcol = m_viewcols[m_action_viewcol];
                    if (resize_viewcol->m_prop_size == 0)
                    {
                        
                    }
                }
                return;
            }
            
            if (m_action_viewcol < (int)m_viewcols.size())
            {
                ViewColumn* resize_viewcol = m_viewcols[m_action_viewcol];
                if (!resize_viewcol->m_resizable)
                    return;
            }

            int old_width = m_action_col->m_pixwidth;
            int new_width = std::max(fromDIP(5), event.m_x - m_action_colxoff);

            if (new_width != old_width)
            {
                int new_dip_width = toDIP(new_width);
                m_action_col->m_pixwidth = new_width;
                m_action_col->m_dip_pixwidth = new_dip_width;

                m_memdc.Blit(m_action_colxoff+new_width, 0, m_cliwidth-(m_action_colxoff+old_width), m_cliheight,
                        &m_memdc, m_action_colxoff+old_width, 0);

                if (new_width < old_width)
                {
                    wxRect rect(m_cliwidth-(old_width-new_width), 0, old_width-new_width, m_cliheight);
                    render(&rect);
                }

                wxRect rect(m_action_colxoff, 0, m_action_col->m_pixwidth, m_cliheight);

                render(&rect);
                repaint();

                updateScrollbars(wxHORIZONTAL);

                GridEvent evt;
                evt.SetColumn(m_action_viewcol);
                evt.SetInt(new_dip_width);
                evt.SetUserEvent(true);
                fireEvent(wxEVT_KCLGRID_COLUMN_RESIZE, evt);
            }
        }
        else if (m_mouse_action == actionResizeRow)
        {
            if (m_options & optRowResize)
            {
                if (event.m_y != m_action_row)
                {
                    m_action_row = event.m_y;

                    wxClientDC dc(this);
                    repaint(&dc);
                    dc.SetPen(*wxBLACK_PEN);
                    dc.DrawLine(m_rowlabel_width, event.m_y, m_cliwidth, event.m_y);
                }
            }
        }
        else if (m_mouse_action == actionSelectCol)
        {
            int xoff = event.m_x - m_action_xpos;

            if (xoff < 3 && xoff > -3)
            {
                // don't do anything if the user doesn't move the mouse very much
            }
            else
            {
                if (!(m_options & optColumnMove))
                    return;

                // the user is trying to move the column
                m_mouse_action = actionMoveCol;

                for (std::vector<ColumnRange>::iterator it = m_colranges.begin();
                    it != m_colranges.end(); ++it)
                {
                    if (it->viewcol == m_action_col)
                    {
                        m_action_col->m_draw = false;

                        // create move column bitmap
                        m_movecol_bmp.Create(it->width+1, m_cliheight, -1);
                        m_movecol_dc.SelectObject(m_movecol_bmp);
                        m_movecol_dc.Blit(0, 0, it->width+1, m_cliheight, &m_memdc, it->xoff, 0);


                        lightenMemDC(m_movecol_dc, m_movecol_bmp);
                        
                        m_memdc.SetBrush(*wxWHITE_BRUSH);
                        m_memdc.SetPen(*wxTRANSPARENT_PEN);
                        m_memdc.DrawRectangle(it->xoff+1, 0, it->width-1, m_cliheight);
                        repaint();

                        wxClientDC dc(this);
                        dc.Blit(event.m_x - (m_action_xpos - m_action_colxoff), 0,
                               it->width+1, m_cliheight, &m_movecol_dc, 0, 0);

                        return;
                    }
                }
            }
        }
        else if (m_mouse_action == actionMoveCol)
        {
            int xoff = event.m_x - (m_action_xpos - m_action_colxoff);
            int width = m_action_col->m_pixwidth;

            wxClientDC dc(this);

            int mousex, mousey;
            ::wxGetMousePosition(&mousex, &mousey);
            ScreenToClient(&mousex, &mousey);

            int visrows = getVisibleRowCount();

            // direction indicates:
            //      -2 = after a scroll operation to the left
            //      -1 = mouse sweeping to the left
            //       0 = mouse x position unchanged
            //       1 = mouse sweeping to the right
            //       2 = after a scroll operation to the right

            int direction = 0;

            if (mousex > m_action_lastxpos)
                direction = 1;
            if (mousex < m_action_lastxpos)
                direction = -1;

            m_action_lastxpos = mousex;

            while (m_xoff > 0 && mousex < 20+m_frozen_width)
            {
                ::wxMilliSleep(5);

                scrollHorz(-(visrows + 2));

                repaint(&dc);
                ::wxGetMousePosition(&mousex, &mousey);
                ScreenToClient(&mousex, &mousey);

                direction = -2;
            }


            int max_offset = m_total_width - (m_cliwidth - m_frozen_width);

            while (mousex > m_cliwidth-20 && m_xoff < max_offset)
            {
                ::wxMilliSleep(5);

                scrollHorz(visrows + 2);
                repaint(&dc);

                ::wxGetMousePosition(&mousex, &mousey);
                ScreenToClient(&mousex, &mousey);

                direction = 2;
            }
 

            // determine the destination column index, if any
            ViewColumn* dest_vc = NULL;
            int dest_vc_num = 0;

            bool past_action_col = false;

            for (std::vector<ColumnRange>::iterator it = m_colranges.begin();
                    it != m_colranges.end(); ++it)
            {
                if (direction == -2)
                {
                    dest_vc = it->viewcol;
                    dest_vc_num = it->viewcol_idx;
                    break;
                }

                if (direction == -1 &&
                    xoff > it->xoff &&
                    xoff < it->xoff+(it->width/5) &&
                    !past_action_col)
                {
                    dest_vc = it->viewcol;
                    dest_vc_num = it->viewcol_idx;
                    break;
                }

                if (direction == 1 &&
                    xoff+width > it->xoff+((it->width*4)/5) &&
                    xoff+width < it->xoff+it->width &&
                    past_action_col)
                {
                    dest_vc = it->viewcol;
                    dest_vc_num = it->viewcol_idx;
                    break;
                }

                if (direction == 2 &&
                    xoff > it->xoff &&
                    xoff < it->xoff+it->width)
                {
                    dest_vc = it->viewcol;
                    dest_vc_num = it->viewcol_idx;
                    break;
                }

                if (it->viewcol == m_action_col)
                    past_action_col = true;
            }

            if (dest_vc && m_action_col != dest_vc)
            {
                moveCol(m_action_col, dest_vc);
                render(NULL, false);
            }

            dc.Blit(0, 0, xoff, m_cliheight, &m_memdc, 0, 0);
            
            dc.Blit(xoff+width,
                    0,
                    m_cliwidth-(xoff+width),
                    m_cliheight,
                    &m_memdc,
                    xoff+width,
                    0);
                    
            dc.Blit(xoff, 0, width+1, m_cliheight, &m_movecol_dc, 0, 0);

            if (dest_vc && m_action_col != dest_vc)
            {
                GridEvent evt;
                evt.SetColumn(m_action_viewcol);
                evt.SetDestinationColumn(dest_vc_num);
                fireEvent(wxEVT_KCLGRID_COLUMN_MOVE, evt);

                m_action_viewcol = dest_vc_num;
            }
        }
        else if (m_mouse_action == actionSelect)
        {
            int row, col;
            int mouse_x = event.m_x;
            int mouse_y = event.m_y;

            if (mouse_y > m_cliheight-1)
                mouse_y = m_cliheight-1;

            if (mouse_y < m_header_height+1)
                mouse_y = m_header_height+1;

            if (mouse_x > m_cliwidth-1)
                mouse_x = m_cliwidth-1;

            if (mouse_x < m_frozen_width+1)
                mouse_x = m_frozen_width+1;

            hitTest(mouse_x, mouse_y, &row, &col);

            if (row == -1)
                return;

            if (col == -1)
            {
                if (event.m_x > (m_total_width - m_frozen_width + m_xoff))
                {
                    col = m_viewcols.size() - 1;
                }
            }

            if (col != -1)
            {
                int old_start_row = std::min(m_action_selrect->m_start_row,
                                             m_action_selrect->m_end_row);
                                             
                int old_start_col = std::min(m_action_selrect->m_start_col,
                                             m_action_selrect->m_end_col);
                                             
                int old_end_row = std::max(m_action_selrect->m_start_row,
                                           m_action_selrect->m_end_row);
                                           
                int old_end_col = std::max(m_action_selrect->m_start_col,
                                           m_action_selrect->m_end_col);

                int new_start_row, new_start_col;
                int new_end_row, new_end_col;
                
                if (!event.ShiftDown())
                {
                    m_action_selrect->m_end_row = row;
                    m_action_selrect->m_end_col = col;
                    
                    new_start_row = std::min(m_action_selrect->m_start_row,
                                             m_action_selrect->m_end_row);
                                                 
                    new_start_col = std::min(m_action_selrect->m_start_col,
                                             m_action_selrect->m_end_col);
                                                 
                    new_end_row = std::max(m_action_selrect->m_start_row,
                                           m_action_selrect->m_end_row);
                                               
                    new_end_col = std::max(m_action_selrect->m_start_col,
                                           m_action_selrect->m_end_col);
                }
                else
                {
                    if (row <= m_cursor_row)
                        m_action_selrect->m_start_row = row;
                    if (row >= m_cursor_row)
                        m_action_selrect->m_end_row = row;

                    if (col <= m_cursor_col)
                        m_action_selrect->m_start_col = col;
                    if (col >= m_cursor_col)
                        m_action_selrect->m_end_col = col;
                    
                    // make sure that the corner of the selection is
                    // always anchored at the cursor position
                    new_start_row = std::min(m_action_selrect->m_start_row,
                                             m_cursor_row);
                                                 
                    new_start_col = std::min(m_action_selrect->m_start_col,
                                             m_cursor_col);
                                                 
                    new_end_row = std::max(m_action_selrect->m_end_row,
                                           m_cursor_row);
                                               
                    new_end_col = std::max(m_action_selrect->m_end_col,
                                           m_cursor_col);
                }
                
                // if the scroll timer is running, the
                // repaint will happen in onScrollTimer

                if (!m_scroll_timer.IsRunning())
                {
                    wxRect rect_paint;
                    getBoundingBoxRect(
                                    std::min(old_start_row, new_start_row),
                                    std::min(old_start_col, new_start_col),
                                    std::max(old_end_row, new_end_row),
                                    std::max(old_end_col, new_end_col),
                                    &rect_paint);

                    render(&rect_paint);
                    repaint();
                }
                
                // the selection changed in some way; fire an event
                if (old_start_col != new_start_col ||
                    old_start_row != new_start_row ||
                    old_end_col != new_end_col ||
                    old_end_row != new_end_row)
                {
                    fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE,
                              m_cursor_row,
                              m_cursor_col);
                }
            }
        }
        else if (m_mouse_action == actionSelectRows)
        {
            // determine row and column
            int row;
            
            int mx = event.m_x;
            int my = event.m_y;

            if (my < m_header_height+1)
                my = m_header_height+1;

            if (my > m_cliheight-1)
                my = m_cliheight-1;

            hitTest(mx, my, &row, NULL);

            if (row != -1)
            {
                if (m_action_selrect->m_end_row != row)
                {
                    m_action_selrect->m_end_row = row;
                    render();
                    repaint();

                    fireEvent(wxEVT_KCLGRID_SELECTION_CHANGE,
                              m_cursor_row,
                              m_cursor_col);
                }
            }
        }


        // determine cursor
        
        wxCursor cursor;
        cursor = wxNullCursor;
        bool sizing_cursor = false;

        if (event.m_y < m_header_height && m_mouse_action == actionNone)
        {
            if (m_options & optColumnResize)
            {
                for (std::vector<ColumnRange>::iterator it = m_colranges.begin();
                        it != m_colranges.end(); ++it)
                {
                    if (!it->viewcol->m_resizable)
                        continue;
                    
                    if (event.m_x > (it->xoff + it->width - 3) &&
                        event.m_x < (it->xoff + it->width + 3))
                    {
                        cursor = wxCursor(wxCURSOR_SIZEWE);
                        sizing_cursor = true;
                        break;
                    }
                }
            }
        }

        if (event.m_x < m_rowlabel_width && m_mouse_action == actionNone)
        {
            if (m_options & optRowResize)
            {
                int yoff = m_header_height;
                int rowdata_size = m_rowdata.size();
                int i;

                for (i = 0; i < rowdata_size; ++i)
                {
                    yoff += m_row_height;

                    if (abs(event.m_y-yoff) <= 1)
                    {
                        cursor = wxCursor(wxCURSOR_SIZENS);
                        sizing_cursor = true;
                        break;
                    }
                }
            }
        }


        {
            int row, col, cell_xoff = 0;
            
            if (hitTest(event.m_x, event.m_y, &row, &col, &cell_xoff))
            {
                int model_col = getColumnModelIdx(col);
                if (model_col != -1)
                {
                    kcl::CellProperties cellprops;
                    getCellProperties(row, model_col, &cellprops);

                    // if the user is hovering over a hyperlink, show an
                    // appropriate cursor
                    if (cellprops.hyperlink || (m_options & optActivateHyperlinks))
                    {
                        wxString cell_str = getCellString(row, model_col);
                        int url_pos = cell_str.Find(wxT("://"));
                        if (cellprops.hyperlink || (url_pos >= 4 && url_pos <= 10))
                        {
                            wxClientDC dc(this);
                            wxSize text_extent = dc.GetTextExtent(cell_str);

                            if (cell_xoff < text_extent.x)
                            {
                                cursor = wxCursor(wxCURSOR_HAND);
                            }
                        }
                    }
                }
            }
        }


        SetCursor(cursor);

#ifdef WIN32
        // get tooltip for the cell
        int tooltip_row = -1;
        int tooltip_col = -1;
        wxString tip_text;

        hitTest(event.m_x, event.m_y, &tooltip_row, &tooltip_col);

        if ((tooltip_row != -1 || tooltip_col != -1) &&
            !sizing_cursor &&
            (tooltip_row != m_last_tooltip_row ||
             tooltip_col != m_last_tooltip_col))
        {
            m_last_tooltip_row = tooltip_row;
            m_last_tooltip_col = tooltip_col;
            
            if ((tooltip_col == -1 && event.m_x > m_rowlabel_width) ||
                (tooltip_row == -1 && event.m_y > m_header_height))
            {
                removeToolTip();
                return;
            }

            GridEvent evt;
            evt.SetColumn(tooltip_col);
            evt.SetRow(tooltip_row);

            bool allow = true;
            bool handled;

            handled = fireEvent(wxEVT_KCLGRID_NEED_TOOLTIP_TEXT, evt, &allow);
            
            if (allow)
            {
                if (handled)
                {
                    tip_text = evt.GetString();
                    if (tip_text.IsEmpty())
                    {
                        removeToolTip();
                    }
                    else
                    {
                        // set the default tootip delay and the tooltip; 
                        // note: other parts of the application use different 
                        // delays (e.g. report writer), so set the delay
                        // each time the tooltip is used
                        wxToolTip::SetDelay(1100);
                        SetToolTip(tip_text);
                    }
                }
            }
        }
        else
        {
            if (tooltip_row != m_last_tooltip_row ||
                tooltip_col != m_last_tooltip_col)
            {
                removeToolTip();
            }
        }
#endif

    }

    if (event.GetEventType() == wxEVT_MOUSEWHEEL)
    {
        if (isEditing())
        {
            endEdit(true);
            return;
        }

        if (event.m_wheelRotation > 0)
        {
            scrollVert(-3);
        }
        else
        {
            scrollVert(3);
        }

        updateData();
        render();
        updateScrollbars(wxVERTICAL);
        repaint();
    }
}


void Grid::onScrollTimer(wxTimerEvent& event)
{
    if (m_scroll_timer_voffset == 0 && m_scroll_timer_hoffset == 0)
        return;

    if (m_scroll_timer_voffset)
    {
        scrollVert(m_scroll_timer_voffset);
        updateData();
        updateScrollbars(wxVERTICAL);
    }

    if (m_scroll_timer_hoffset)
    {
        scrollHorz(m_scroll_timer_hoffset);
        updateScrollbars(wxHORIZONTAL);
    }


    wxPoint pt = ScreenToClient(::wxGetMousePosition());
    wxMouseEvent evt(wxEVT_MOTION);
    evt.m_x = pt.x;
    evt.m_y = pt.y;
    evt.m_leftDown = true;
    onMouse(evt);

    render();
    repaint();
}



void Grid::removeToolTip()
{
    wxToolTip* tip = GetToolTip();
    if (!tip)
        return;
    #ifdef WIN32
    tip->SetWindow(NULL);
    #endif
    SetToolTip(NULL);
}

void Grid::onKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_TAB)
    {
        // if the grid is in a dialog, the tab key should be used
        // for navigating controls
        if (GetParent()->IsKindOf(CLASSINFO(wxDialog)))
        {
            Navigate();
            return;
        }
    }

    GridEvent evt;
    evt.m_key_event = event;
    evt.SetRow(m_cursor_row);
    evt.SetColumn(m_cursor_col);
    if (fireEvent(wxEVT_KCLGRID_KEY_DOWN, evt))
    {
        // event was handled
        return;
    }

    int new_col, new_row;
    
    int orig_key_code = event.GetKeyCode();
    int key_code = event.GetKeyCode();
    bool shift_down = event.ShiftDown();
    
    if (key_code == WXK_RETURN ||
        key_code == WXK_NUMPAD_ENTER)
    {
        orig_key_code = WXK_RETURN;
            
        if (event.ShiftDown())
        {
            key_code = WXK_UP;
        }
        else
        {
            key_code = WXK_DOWN;
        }
    }
    
    if (key_code == WXK_TAB)
    {
        if (event.ShiftDown())
        {
            key_code = WXK_LEFT;
        }
        else
        {
            key_code = WXK_RIGHT;
        }
    }

    switch (key_code)
    {
        case WXK_NUMPAD_LEFT:
        case WXK_LEFT:
            {
                new_col = std::max(0, m_cursor_col - 1);
                
                if (!shift_down || orig_key_code == WXK_TAB)
                {
                    // remove any existing selection
                    clearSelection();
                    
                    // if the shift key is not down, move the cursor
                    moveCursor(m_cursor_row, new_col, false);

                    // if the cursor is off the screen, scroll horizontally
                    // to the cursor
                    scrollHorzToCursor();
                }
                else
                {
                    // if the shift key is down, add to the selection
                    int sel_count = getSelectionCount();
                    if (sel_count == 0)
                    {
                        SelectionRect s;
                        s.m_start_row = m_cursor_row;
                        s.m_end_row = m_cursor_row;
                        s.m_start_col = new_col;
                        s.m_end_col = m_cursor_col;
                        addSelection(&s);
                    }
                    else if (sel_count == 1)
                    {
                        new_col = std::max(0, m_cursor_col - 1);
                        
                        SelectionRect* s = m_selection.getSelection(0);
                        if (s->m_end_col > m_cursor_col)
                        {
                            s->m_end_col = std::max(0, s->m_end_col - 1);
                        }
                        else
                        {
                            s->m_start_col = std::max(0, s->m_start_col - 1);
                        }
                    }
                }
                
                render();
                updateScrollbars(wxHORIZONTAL);
                repaint();
            }
            break;

        case WXK_NUMPAD_RIGHT:
        case WXK_RIGHT:
            {
                new_col = std::min((int)(m_viewcols.size()-1), m_cursor_col + 1);
                
                if (!shift_down || orig_key_code == WXK_TAB)
                {
                    // remove any existing selection
                    clearSelection();
                    
                    // if the shift key is not down, move the cursor
                    moveCursor(m_cursor_row, new_col, false);

                    // if the cursor is off the screen, scroll horizontally
                    // to the cursor
                    scrollHorzToCursor();
                }
                else
                {
                    // if the shift key is down, add to the selection
                    int sel_count = getSelectionCount();
                    if (sel_count == 0)
                    {
                        SelectionRect s;
                        s.m_start_row = m_cursor_row;
                        s.m_end_row = m_cursor_row;
                        s.m_start_col = m_cursor_col;
                        s.m_end_col = new_col;
                        addSelection(&s);
                    }
                    else if (sel_count == 1)
                    {
                        SelectionRect* s = m_selection.getSelection(0);
                        if (s->m_start_col < m_cursor_col)
                        {
                            s->m_start_col = std::min((int)(m_viewcols.size()-1), s->m_start_col + 1);
                        }
                        else
                        {
                            s->m_end_col = std::min((int)(m_viewcols.size()-1), s->m_end_col + 1);
                        }
                    }
                }
                
                render();
                updateScrollbars(wxHORIZONTAL);
                repaint();
            }
            break;

        case WXK_NUMPAD_UP:
        case WXK_UP:
            {
                new_row = std::max(0, m_cursor_row - 1);
                
                if (!shift_down || orig_key_code == WXK_RETURN)
                {
                    // remove any existing selection
                    clearSelection();
                    
                    // if the shift key is not down, move the cursor
                    moveCursor(new_row, m_cursor_col, false);

                    // scroll vertically to the new cursor position
                    // and update the data in case the row offset
                    // has changed
                    if (scrollVertToCursor())
                        updateData();
                }
                else
                {
                    // if the shift key is down, add to the selection
                    int sel_count = getSelectionCount();
                    if (sel_count == 0)
                    {
                        SelectionRect s;
                        s.m_start_row = new_row;
                        s.m_end_row = m_cursor_row;
                        s.m_start_col = m_cursor_col;
                        s.m_end_col = m_cursor_col;
                        addSelection(&s);
                    }
                    else if (sel_count == 1)
                    {
                        SelectionRect* s = m_selection.getSelection(0);
                        if (s->m_end_row > m_cursor_row)
                        {
                            s->m_end_row = std::max(0, s->m_end_row - 1);
                        }
                        else
                        {
                            s->m_start_row = std::max(0, s->m_start_row - 1);
                        }
                    }
                }
                
                render();
                updateScrollbars(wxVERTICAL);
                repaint();
            }
            break;

        case WXK_NUMPAD_DOWN:
        case WXK_DOWN:
            {
                new_row = m_cursor_row + 1;
                
                if (!shift_down || orig_key_code == WXK_RETURN)
                {
                    // remove any existing selection
                    clearSelection();

                    // if the shift key is not down, move the cursor
                    moveCursor(new_row, m_cursor_col, false);

                    // scroll vertically to the new cursor position
                    // and update the data in case the row offset
                    // has changed
                    if (scrollVertToCursor())
                        updateData();
                }
                else
                {
                    // if the shift key is down, add to the selection
                    int sel_count = getSelectionCount();
                    if (sel_count == 0)
                    {
                        SelectionRect s;
                        s.m_start_row = m_cursor_row;
                        s.m_end_row = new_row;
                        s.m_start_col = m_cursor_col;
                        s.m_end_col = m_cursor_col;
                        addSelection(&s);
                    }
                    else if (sel_count == 1)
                    {
                        SelectionRect* s = m_selection.getSelection(0);
                        if (s->m_start_row < m_cursor_row)
                        {
                            s->m_start_row = std::min(m_row_count-1, s->m_start_row + 1);
                        }
                        else
                        {
                            s->m_end_row = std::min(m_row_count-1, s->m_end_row + 1);
                        }
                    }
                }
                
                render();
                updateScrollbars(wxVERTICAL);
                repaint();
            }
            break;

        case WXK_NUMPAD_PAGEUP:
        case WXK_PAGEUP:
            {
                int scroll_rows = getVisibleRowCount() - 2;
                int cursor_row_on_screen = m_cursor_row - m_row_offset;


                if (m_model->getGroupBreaksActive())
                {
                    scroll_rows /= 2;
                }


                if (event.ShiftDown() || event.ControlDown())
                {
                    scroll_rows = 1000;
                    if (event.ShiftDown() && event.ControlDown())
                    {
                        scroll_rows = 100000;
                    }
                }

                scrollVert(-scroll_rows);

                if (cursor_row_on_screen >= 0 &&
                    cursor_row_on_screen < scroll_rows)
                {
                    moveCursor(m_row_offset+cursor_row_on_screen,
                               m_cursor_col,
                               false);
                }
                else
                {
                    moveCursor(m_row_offset,
                               m_cursor_col,
                               false);
                }

                updateData();

                render();
                updateScrollbars(wxVERTICAL);
                repaint();
            }
            break;

        case WXK_NUMPAD_PAGEDOWN:
        case WXK_PAGEDOWN:
            {
                int scroll_rows = getVisibleRowCount() - 2;
                int cursor_row_on_screen = m_cursor_row - m_row_offset;

                
                int y;
                for (y = m_cliheight; y >= m_row_height; --y)
                {
                    int row;
                    int col;
                    if (hitTest(m_rowlabel_width+1, y, &row, &col))
                    {
                        scroll_rows = (row - m_row_offset) - 1;
                        break;
                    }
                }



                if (event.ShiftDown() || event.ControlDown())
                {
                    scroll_rows = 1000;
                    if (event.ShiftDown() && event.ControlDown())
                    {
                        scroll_rows = 100000;
                    }
                }

                scrollVert(scroll_rows);
                updateData();

                if (cursor_row_on_screen >= 0 &&
                    cursor_row_on_screen < scroll_rows)
                {
                    moveCursor(m_row_offset+cursor_row_on_screen,
                               m_cursor_col,
                               false);
                }
                else
                {
                    moveCursor(m_row_offset,
                               m_cursor_col,
                               false);
                }

                render();
                updateScrollbars(wxVERTICAL);
                repaint();
            }
            break;

        case WXK_NUMPAD_HOME:
        case WXK_HOME:
            {
                m_xoff = 0;
                moveCursor(m_cursor_row, 0, false);
                render();
                updateScrollbars(wxHORIZONTAL);
                repaint();
            }
            break;

        case WXK_NUMPAD_END:
        case WXK_END:
            {
                m_xoff = m_total_width - m_cliwidth + m_frozen_width;
                if (m_xoff < 0)
                    m_xoff = 0;

                moveCursor(m_cursor_row, m_viewcols.size()-1, false);
                render();
                updateScrollbars(wxHORIZONTAL);
                repaint();
            }
            break;

        case 'T':
        case 'F':
        case WXK_SPACE:
            {
                event.Skip();
                
                CellData* data = getCellData(m_cursor_row, m_cursor_modelcol);
    
                if (data->ctrltype == ctrltypeCheckBox && data->editable)
                {
                    bool bval = data->m_boolvalue;
                    bool allow;
                    
                    fireEvent(wxEVT_KCLGRID_BEGIN_EDIT, m_cursor_row, m_cursor_col, &allow);
                    if (!allow)
                        break;

                    if (key_code == 'T')
                        bval = true;
                    else if (key_code == 'F')
                        bval = false;
                    else
                        bval = !data->m_boolvalue;

                    kcl::GridEvent evt;
                    evt.SetRow(m_cursor_row);
                    evt.SetColumn(m_cursor_col);
                    evt.SetBoolean(bval);
                    
                    fireEvent(wxEVT_KCLGRID_END_EDIT, evt, &allow);
                }
            }



        default:
            // old code:
            //event.Skip();

            // replaced with:
            // for some reason, in wxWidgets 3.1, the EVT_CHAR is not firing as desired if
            // we skip the wxKeyEvent here; To reproduce:
            // 1) open table
            // 2) press key to edit (works)
            // 3) press return; cursor goes down
            // 4) press key to edit (doesn't work, but should)
            wxChar a = event.GetUnicodeKey();

            if (key_code >= WXK_NUMPAD0 && key_code <= WXK_NUMPAD9)
            {
                a = '0' + (key_code - WXK_NUMPAD0);
            }

            if (key_code == WXK_NUMPAD_EQUAL)     a = '=';
            if (key_code == WXK_NUMPAD_MULTIPLY)  a = '*';
            if (key_code == WXK_NUMPAD_SUBTRACT)  a = '-';
            if (key_code == WXK_NUMPAD_DECIMAL)   a = '.';
            if (key_code == WXK_NUMPAD_DIVIDE)    a = '/';
            if (key_code == WXK_NUMPAD_ADD)       a = '+';

            if (a != WXK_NONE)
            {
                if (event.ShiftDown())
                    a = ::wxToupper(a);
                else
                    a = ::wxTolower(a);
                event.m_keyCode = a;
                onChar(event);
            }
            break;
    }
}

void Grid::onChar(wxKeyEvent& event)
{
    if (event.AltDown())
    {
        // ignore control keys
        event.Skip();
        return;
    }

    int key_code = event.GetKeyCode();

    if (key_code != WXK_RETURN &&
        key_code != WXK_NUMPAD_ENTER &&
        key_code != WXK_TAB &&
        key_code != WXK_INSERT &&
        key_code != WXK_ESCAPE &&
        key_code != WXK_CLEAR &&
        key_code != WXK_NUMPAD1 &&
        key_code != WXK_NUMPAD2 &&
        key_code != WXK_NUMPAD3 &&
        key_code != WXK_NUMPAD4 &&
        key_code != WXK_NUMPAD5 &&
        key_code != WXK_NUMPAD6 &&
        key_code != WXK_NUMPAD7 &&
        key_code != WXK_NUMPAD8 &&
        key_code != WXK_NUMPAD9 &&
        key_code != WXK_NUMPAD0 &&
        key_code != WXK_NUMPAD_INSERT &&
        key_code != WXK_NUMPAD_DELETE &&
        key_code != WXK_NUMPAD_DECIMAL)
    {
        // if the cursor row is greater than the row count,
        // and ghost row is active, then append a row

        if (m_cursor_row == m_row_count && m_options & optGhostRow)
        {
            doGhostRowInsert();
        }

        // start the edit with no text
        beginEdit(wxT(""));

        if (m_control && m_control->IsKindOf(CLASSINFO(wxTextCtrl)))
        {
            if (key_code != WXK_BACK &&
                key_code != WXK_DELETE)
            {
                wxTextCtrl* text = (wxTextCtrl*)m_control;

                wxChar buf[2];
                buf[0] = event.GetKeyCode();
                buf[1] = 0;

                text->WriteText(buf);
                text->SetInsertionPointEnd();
            }
        }
    }
}


void Grid::onEditChanged(const wxString& value)
{
    kcl::GridEvent evt;
    evt.SetRow(m_cursor_row);
    evt.SetColumn(m_cursor_col);
    evt.SetString(value);
    fireEvent(wxEVT_KCLGRID_EDIT_CHANGE, evt);
}

void Grid::onComboSelectionChanged(wxCommandEvent& event)
{
    if (!m_control)
    {
        // something went wrong, refresh the grid and bail out
        wxASSERT_MSG(m_control != NULL, wxT("m_control is NULL when it shouldn't be!"));
        refresh(refreshAll);
        return;
    }
    
    int sel;
    wxString str;
    
    if (m_control->IsKindOf(CLASSINFO(GridChoiceCtrl)))
    {
        GridChoiceCtrl* ctrl = static_cast<GridChoiceCtrl*>(m_control);
        sel = ctrl->GetSelection();
        str = ctrl->GetValue();
    }
    else if (m_control->IsKindOf(CLASSINFO(GridComboCtrl)))
    {
        GridComboCtrl* ctrl = static_cast<GridComboCtrl*>(m_control);
        sel = ctrl->GetSelection();
        str = ctrl->GetValue();
    }
    else
    {
        return;
    }

    m_edit_combosel = sel;
    setCellComboSel(m_cursor_row, m_cursor_col, sel);

    kcl::GridEvent evt;
    evt.SetRow(m_cursor_row);
    evt.SetColumn(m_cursor_col);
    evt.SetString(str);
    evt.SetExtraLong(sel);

    fireEvent(wxEVT_KCLGRID_EDIT_CHANGE, evt);
    fireEvent(wxEVT_KCLGRID_MODEL_CHANGE, m_cursor_row, m_cursor_modelcol);
}


void Grid::onEditDestroy(const wxString& value,
                         int last_key_code,
                         bool focus_lost,
                         bool user_action)
{
    m_control = NULL;

    m_edit_value = value;
    m_last_key_code = last_key_code;
        
/*
    if (focus_lost)
    {
        kcl::GridEvent evt;
        evt.SetString(wxT(""));
        evt.SetEditCancelled(true);
        evt.SetRow(m_cursor_row);
        evt.SetColumn(m_cursor_col);
        fireEvent(wxEVT_KCLGRID_END_EDIT, evt);
        m_last_edit_allowed = false;
        return;
    }
*/

    if (!m_destroying)
    {
        // this flag prevents flicker when editing cells
        m_mask_paint = true;

        // post event to do end-edit cleanup (we want this to
        // execute slightly later)

        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, EndEditCleanupId);
        event.SetEventObject(this);
        event.SetInt(user_action ? 1 : 0);
        if (user_action)
        {
            ::wxPostEvent(this, event);
        }
        else
        {
            onEndEditCleanup(event);
        }
    }
}

void Grid::onEndEditCleanup(wxCommandEvent& event)
{
    int row = m_cursor_row;
    int col = m_cursor_col;
    int modelcol = m_cursor_modelcol;

    m_mask_paint = false;
    m_control = NULL;

    // check if the edit operation was cancelled
    if (m_last_key_code == WXK_ESCAPE)
    {
        refresh(kcl::Grid::refreshAll);
        
        if (m_caption_editing)
        {
            int viewcol = getColumnViewIdx(m_caption_edit_col->m_modelcol);

            kcl::GridEvent evt;
            evt.SetString(wxT(""));
            evt.SetInt(-1);
            evt.SetEditCancelled(true);
            evt.SetRow(-1);
            evt.SetColumn(viewcol);
            evt.SetUserEvent(event.GetInt() ? true : false);
            fireEvent(wxEVT_KCLGRID_END_CAPTION_EDIT, evt);
            m_caption_editing = false;
            m_last_edit_allowed = false;
            m_caption_edit_col = NULL;
        }
        else
        {
            kcl::GridEvent evt;
            evt.SetString(wxT(""));
            evt.SetInt(-1);
            evt.SetEditCancelled(true);
            evt.SetRow(row);
            evt.SetColumn(col);
            evt.SetUserEvent(event.GetInt() ? true : false);
            fireEvent(wxEVT_KCLGRID_END_EDIT, evt);
            m_last_edit_allowed = false;
        }
        
        return;
    }
    
    bool allow = true;

    // if we were editing a column caption, fire the end caption edit event
    if (m_caption_editing)
    {
        int viewcol = getColumnViewIdx(m_caption_edit_col->m_modelcol);

        kcl::GridEvent evt;
        evt.SetRow(-1);
        evt.SetColumn(viewcol);
        evt.SetString(m_edit_value);
        evt.SetUserEvent(event.GetInt() ? true : false);
        fireEvent(wxEVT_KCLGRID_END_CAPTION_EDIT, evt, &allow);

        if (allow)
        {
            IModelPtr model = getModel();
            if (model)
            {
                int model_idx = m_caption_edit_col->m_modelcol;
                model->modifyColumn(model_idx, m_edit_value, -1, -1, -1);
                model->refresh();
                refresh(kcl::Grid::refreshAll);
            }
        }

        m_caption_editing = false;
        m_last_edit_allowed = allow;    // store for later
        m_caption_edit_col = NULL;
        
        // if the user was editing a column caption and presses the tab key,
        // end the old edit and try to begin a new edit on the next view column
        if (m_last_key_code == WXK_TAB ||
            m_last_key_code == WXK_LEFT)
        {
            int new_viewcol = viewcol;
            
            // tab was pressed
            if (m_last_key_code == WXK_TAB)
                new_viewcol++;
                
            // shift-tab was pressed
            if (m_last_key_code == WXK_LEFT)
                new_viewcol--;
                
            if (new_viewcol >= 0 && new_viewcol < getColumnCount())
            {    
                // check to see if this action is allowed
                bool allow = true;
                
                if (fireEvent(wxEVT_KCLGRID_BEGIN_CAPTION_EDIT,
                              -1, new_viewcol, &allow))
                {
                    if (!allow)
                        return;
                        
                    beginCaptionEdit(new_viewcol);
                    return;
                }
            }
        }

        
        return;
    }


    int tempi = 0;
    double tempd = 0.0;
    bool tempb = false;

    CellData* celldata = getCellData(row, modelcol);

    if (!celldata)
    {
        return;
    }
    
    // if the cell that was being edited was a combobox, do a lookup
    // of the edit text in the combobox's choices -- if we can't find
    // the text in the combobox's choice list, then the index is -1
    bool ctrltype_is_combobox = (celldata->ctrltype == ctrltypeComboBox);
    if (ctrltype_is_combobox)
    {
        int idx = 0;
        bool found = false;
        std::vector<wxString>::iterator it;
        for (it = celldata->cbchoices.begin();
             it != celldata->cbchoices.end(); ++it)
        {
            if (*it == m_edit_value)
            {
                m_edit_combosel = idx;
                found = true;
                break;
            }
            
            idx++;
        }

        // we couldn't find the string in the combo choices
        if (!found)
            m_edit_combosel = -1;
    }

    


    int type = celldata->m_type;

    switch (type)
    {
        case typeDate:
        case typeDateTime:
        case typeCharacter:
        {
            kcl::GridEvent evt;
            evt.SetRow(row);
            evt.SetColumn(col);
            evt.SetString(m_edit_value);
            evt.SetExtraLong(m_edit_combosel);
            evt.SetUserEvent(event.GetInt() ? true : false);
            fireEvent(wxEVT_KCLGRID_END_EDIT, evt, &allow);
        }
        break;

        case typeInteger:
        {
            tempi = wxAtoi(m_edit_value);

            kcl::GridEvent evt;
            evt.SetRow(row);
            evt.SetColumn(col);
            evt.SetInt(tempi);
            evt.SetExtraLong(m_edit_combosel);
            evt.SetUserEvent(event.GetInt() ? true : false);
            fireEvent(wxEVT_KCLGRID_END_EDIT, evt, &allow);
        }
        break;

        case typeDouble:
        {
            wxString s;
            const wxChar* v = m_edit_value.c_str();
            
            while (*v)
            {
#ifdef _UNICODE
                if (wcschr(L"0123456789,.-", *v))
                    s += *v;
#else
                if (strchr("0123456789,.-", *v))
                    s += *v;
#endif
                v++;
            }

            tempd = wxAtof(s);
            kcl::GridEvent evt;
            evt.SetRow(row);
            evt.SetColumn(col);
            evt.SetDouble(tempd);
            evt.SetExtraLong(m_edit_combosel);
            evt.SetUserEvent(event.GetInt() ? true : false);
            fireEvent(wxEVT_KCLGRID_END_EDIT, evt, &allow);
        }
        break;

        case typeBoolean:
        {
            tempb = false;
            if (m_edit_value.Length() > 0)
            {
                tempb = (wxToupper(m_edit_value.GetChar(0)) == wxT('T'));
            }

            kcl::GridEvent evt;
            evt.SetRow(row);
            evt.SetColumn(col);
            evt.SetBoolean(tempb);
            evt.SetExtraLong(m_edit_combosel);
            evt.SetUserEvent(event.GetInt() ? true : false);
            fireEvent(wxEVT_KCLGRID_END_EDIT, evt, &allow);
        }
        break;

    }


    // store this value for later
    m_last_edit_allowed = allow;


    if (allow)
    {
        bool success = false;

        switch (type)
        {
            case typeCharacter:
                success = m_model->setCellString(row, modelcol, m_edit_value);
                break;

            case typeInteger:
                success = m_model->setCellInteger(row, modelcol, tempi);
                break;

            case typeDouble:
                success = m_model->setCellDouble(row, modelcol, tempd);
                break;

            case typeBoolean:
                success = m_model->setCellBoolean(row, modelcol, tempb);
                break;
        }

        // the following code is only meant to handle the case where
        // the user typed some text into a combobox which was not equal
        // to any of the combobox's choices
        if (ctrltype_is_combobox)
        {
            int cur_model_combosel = m_model->getCellComboSel(row, modelcol);
            bool combo_edit_success = false;
            
            // we couldn't find the edit text in the combobox's list
            // of choices, which means the text was edited (since a call
            // to setCellComboSel() would certain be found in the above
            // lookup, so call setCellComboSel() here so that we can
            // update both the grid and the model
            if (cur_model_combosel != -1)
            {
                combo_edit_success = m_model->setCellComboSel(row,
                                                              modelcol,
                                                              m_edit_combosel);
            }
                                                          
            // if any of the model's set calls succeeded, we need
            // to update the data and do the render below
            success = (combo_edit_success || success);
        }
        
        fireEvent(wxEVT_KCLGRID_MODEL_CHANGE, row, modelcol);

        if (success)
        {
            updateData();       // update internal model cache
            render();           // render to memory device context
        }

        if (m_last_key_code != -1)
        {
            if (m_last_key_code == WXK_UP ||
                m_last_key_code == WXK_NUMPAD8)
            {
                // decrement cursor row by one
                moveCursor(m_cursor_row-1, m_cursor_col);
                if (scrollVertToCursor())
                {
                    updateData();
                    updateScrollbars(wxVERTICAL);
                    render();
                }
            }

            if (m_last_key_code == WXK_RETURN ||
                m_last_key_code == WXK_NUMPAD_ENTER ||
                m_last_key_code == WXK_DOWN   ||
                m_last_key_code == WXK_NUMPAD2)
            {
                // advance cursor row by one
                moveCursor(m_cursor_row+1, m_cursor_col);
                if (scrollVertToCursor())
                {
                    updateData();
                    updateScrollbars(wxVERTICAL);
                    render();                
                }
            }

            if (m_last_key_code == WXK_LEFT ||
                m_last_key_code == WXK_NUMPAD4)
            {
                // move cursor back by one column
                moveCursor(m_cursor_row, m_cursor_col-1);
            }

            if (m_last_key_code == WXK_TAB   ||
                m_last_key_code == WXK_RIGHT ||
                m_last_key_code == WXK_NUMPAD6)
            {
                int new_col = m_cursor_col + 1;
                int new_row = m_cursor_row;

                if (new_col >= (int)m_viewcols.size())
                {
                    new_row++;
                    new_col = 0;
                }

                bool repaint_all = false;
                int xoff = 0;
                int width = 0;
                for (int col = 0; col < new_col; col++)
                {
                    xoff += m_viewcols[col]->m_pixwidth;
                }
                width = m_viewcols[new_col]->m_pixwidth;
                if ((xoff-m_xoff) + width + m_frozen_width > m_cliwidth)
                {
                    m_xoff = (xoff+width-(m_cliwidth-m_frozen_width));
                    updateScrollbars(wxHORIZONTAL);
                    repaint_all = true;
                }

                if (new_col == 0)
                {
                    m_xoff = 0;
                    updateScrollbars(wxHORIZONTAL);
                    repaint_all = true;
                }


                int visrows = getVisibleRowCount();
                if (new_row - m_row_offset >= visrows-1)
                {
                    scrollVert(1);
                    updateData();
                    updateScrollbars(wxVERTICAL);
                    render();
                }
                else
                {
                    if (repaint_all)
                    {
                        render();
                    }
                }

                // advance cursor column by one
                moveCursor(new_row, new_col);
            }
            
            #ifdef __WXGTK__
            if (!isEditing())
            {
                // if we are not editing a new
                // cell, set the focus back to the grid
                SetFocus();
            }
            #endif
        }

        repaint(); // paint mem dc on the screen
    }
}


void Grid::beginEdit()
{
    CellData* cell_data = getCellData(m_cursor_row, m_cursor_modelcol);

    // sanity check
    if (cell_data == &m_empty_cell)
        return;

    // ok, we're good

    wxString copy;
    if (cell_data->m_type == typeDouble)
    {
        copy = wxString::Format(wxT("%.*f"), cell_data->m_scale, cell_data->m_dblvalue);
    }
    else if (cell_data->m_type == typeInteger)
    {
        copy = wxString::Format(wxT("%d"), cell_data->m_intvalue);
    }
    else
    {
        copy = (const wxChar*)cell_data->m_strvalue;
    }

    beginEdit(copy);
}


void Grid::beginCaptionEdit(int viewcol_idx)
{
    if (!(m_options & optCaptionsEditable))
        return;

    ViewColumn* viewcol = NULL;
    wxString caption_text;
    int h = m_header_height;
    int x = 0, w = 0;

    // find the column range associated with this view column
    std::vector<ColumnRange>::iterator it;
    for (it = m_colranges.begin();
         it != m_colranges.end(); ++it)
    {
        if (it->viewcol_idx == viewcol_idx)
        {
            viewcol = it->viewcol;
            caption_text = viewcol->m_caption;
            x = it->xoff;
            w = it->width+1;
        }
    }
    
    // we moved the text control off the screen, so scroll the grid
    if (viewcol == NULL)
    {
        moveCursor(0, viewcol_idx);
        scrollHorzToCursor();
        refresh(kcl::Grid::refreshAll);

        // find the column range associated with this view column
        std::vector<ColumnRange>::iterator it;
        for (it = m_colranges.begin();
             it != m_colranges.end(); ++it)
        {
            if (it->viewcol_idx == viewcol_idx)
            {
                viewcol = it->viewcol;
                caption_text = viewcol->m_caption;
                x = it->xoff;
                w = it->width+1;
            }       
        }
    }
    
    // something went wrong, bail out
    if (viewcol == NULL)
        return;
    
    // if the caption text is wider than the column pixel width,
    // enlarge the text control's width to fit the caption text
    wxCoord text_width;
    wxCoord text_height;
    
    if (m_caption_font.IsOk())
    {
        m_memdc.GetTextExtent(caption_text, &text_width, &text_height, NULL, NULL, &m_caption_font);
    }
    else
    {
        m_memdc.GetTextExtent(caption_text, &text_width, &text_height);
    }

    long style = wxTE_CENTER;
    if (text_width > w-12)
    {
        w = text_width+20;
        style = wxTE_LEFT;
    }
        
    GridTextCtrl* text = new GridTextCtrl(this,
                                          this,
                                          EditCtrlId,
                                          caption_text,
                                          wxPoint(x,0),
                                          wxSize(w,h),
                                          style);
    if (m_caption_font.IsOk())
        text->SetFont(m_caption_font);
    text->SetMaxLength(80);
    text->SetFocus();
    text->SetSelection(-1,-1);
    m_control = (wxControl*)text;
    m_caption_editing = true;
    m_caption_edit_col = viewcol;
}


void Grid::beginEdit(const wxString& default_text)
{   
    if (m_cursor_row >= m_row_count)
        return;

    if (m_cursor_col < 0 || m_cursor_col >= (int)m_viewcols.size())
        return;

    if (m_viewcols[m_cursor_col]->m_modelcol == -1)
        return;
    
    if (!isCursorVisible())
    {
        scrollVertToCursor();
        refresh(refreshAll);
    }


    // check to see if editing is disabled
    if (!(m_options & optEdit))
    {
        return;
    }


    // check editable status

    std::vector<RowData>::iterator it;
    for (it = m_rowdata.begin(); it != m_rowdata.end(); ++it)
    {
        if (it->m_model_row == m_cursor_row)
        {
            if (!it->m_coldata[m_cursor_modelcol].editable)
            {
                // cell was not editable
                return;
            }

            break;
        }
    }



    // destroy previous control, if it exists
    
    if (m_control)
    {
        m_control->Destroy();
        m_control = NULL;
    }


    wxRect cellrect;
    getCellRect(m_cursor_row, m_cursor_col, &cellrect);



    // check to see if this action is allowed
    bool allow = true;

    fireEvent(wxEVT_KCLGRID_BEGIN_EDIT, m_cursor_row, m_cursor_col, &allow);

    if (!allow)
    {
        return;
    }

    m_edit_value = wxT("");
    m_edit_combosel = -1;


    CellData* celldata = getCellData(m_cursor_row, m_cursor_modelcol);
    if (!celldata)
    {
        return;
    }


    // alignment
    int alignment = celldata->alignment;

    if (alignment == alignDefault)
    {
        if (m_cursor_col < (int)m_viewcols.size())
        {
            alignment = m_viewcols[m_cursor_col]->m_alignment;
        }
    }

    if (alignment == alignDefault)
    {
        // set default alignment
        switch (celldata->m_type)
        {
            default:
                alignment = alignLeft;
                break;
            case typeDouble:
            case typeInteger:
                alignment = alignRight;
                break;
            case typeDate:
            case typeDateTime:
            case typeBoolean:
                alignment = alignCenter;
                break;
        }
    }


    switch (celldata->ctrltype)
    {
        case ctrltypeText:
        {
            wxCoord text_width;
            wxCoord text_height;
            m_memdc.GetTextExtent(default_text, &text_width, &text_height);


            int x = cellrect.x + 1;
            int y = cellrect.y + 1;
            int width = cellrect.width - 1;
            int height = cellrect.height - 1;

            if (height < text_height+1)
            {
                height = text_height+1;
            }

            if (celldata->m_bitmap.Ok())
            {
                int bitmap_width = celldata->m_bitmap.GetWidth()+2;
                x += bitmap_width;
                width -= bitmap_width;
            }

            int style = 0;

            if (m_options & optEditBoxBorder)
            {
                style |= wxSIMPLE_BORDER;
            }
            else
            {
                style |= wxNO_BORDER;

                if (m_cursor_type == cursorThin ||
                    m_cursor_type == cursorRowOutline)
                {
                    y += 2;
                    height -= 3;
                    x += 3;
                    width -= 4;
                }
            }

            if (alignment == alignCenter)
                style |= wxTE_CENTER;

            if (alignment == alignRight)
                style |= wxTE_RIGHT;

            GridTextCtrl* text = new GridTextCtrl(this,
                                                  this,
                                                  EditCtrlId,
                                                  default_text,
                                                  wxPoint(x,y),
                                                  wxSize(width, height),
                                                  style);
                                                  
            // this will disallow invalid (alpha) characters to be
            // entered in the text control for integer columns
            if (celldata->m_type == typeInteger)
                text->setNumbersOnly(true);
                
            text->SetMaxLength(celldata->m_width);
            text->SetFocus();
            m_control = (wxControl*)text;
            text->SetInsertionPointEnd();
        }
        break;

        case ctrltypeDropList:
        case ctrltypeComboBox:
        {
            // render screen without cursor
            render(NULL, false);
            repaint();

            wxString* choices = new wxString[celldata->cbchoices.size()];
            int i = 0;
            
            std::vector<wxString>::iterator it;
            for (it = celldata->cbchoices.begin();
                 it != celldata->cbchoices.end();
                 ++it)
            {
                choices[i++] = *it;
            }

            
            if (celldata->ctrltype == ctrltypeDropList)
            {
                #ifdef __WXGTK__
                int height = m_row_height+4;
                #else
                int height = m_row_height;
                #endif
                
                GridChoiceCtrl* choice;
                choice = new GridChoiceCtrl(
                             this,
                             this,
                             EditCtrlId,
                             wxPoint(2000,2000),
                             wxSize(cellrect.width+2, height),
                             celldata->cbchoices.size(),
                             choices,
                             0);
            
                int sel = choice->FindString(default_text);
                if (sel != -1)
                {
                    choice->setStartIndex(sel);
                    choice->SetSelection(sel);
                }
                
                wxPoint pt = cellrect.GetPosition();
                pt.y -= ((height/2) - (m_row_height/2));
                
                m_control = (wxControl*)choice;
                m_control->SetPosition(pt);
                m_control->SetFocus();
            }
            else if (celldata->ctrltype == ctrltypeComboBox)
            {
                GridComboCtrl* combo;
                combo = new GridComboCtrl(
                             this,
                             this,
                             EditCtrlId,
                             wxPoint(2000,2000),
                             wxSize(cellrect.width+2, m_row_height),
                             celldata->cbchoices.size(),
                             choices,
                             wxCB_DROPDOWN);
                             
                int sel = combo->FindString(default_text);
                if (sel != -1)
                {
                    combo->setStartIndex(sel);
                    combo->SetSelection(sel);
                }
                else
                {
                    combo->SetValue(default_text);
                    combo->SetInsertionPointEnd();
                }
                
                m_control = (wxControl*)combo;
                m_control->SetPosition(cellrect.GetPosition());
                
                // we has a message here that setting the focus to the combo control
                // was causing some problems, however, i believe the issue is resolved
                // because we were not checking if the combo control's focus had been
                // sent to itself in its onKillFocus() -- if this bug crops up again,
                // feel free to comment out the SetFocus() below until a better
                // solution has been reached.  (DW -- 14.Jul.2007)
                
                m_control->SetFocus();
            }
            
            delete[] choices;
        }
        break;
    }

    // render behind the control
    wxRect cell_rect;
    getCellRect(m_cursor_row, m_cursor_col, &cell_rect);
    render(&cell_rect);
    repaint();
}


void Grid::beginEditWithPositioning(int x, int y)
{
    beginEdit();

    if (m_control && m_control->IsKindOf(CLASSINFO(wxTextCtrl)))
    {
        // position the caret where the user dbl-clicked
        wxTextCtrl* text = (wxTextCtrl*)m_control;

        int mouse_x, mouse_y, edit_x, edit_y;

        // at least on win32, pixels are offset 2
        // centimeters to the left
        mouse_x = x-2;
        mouse_y = y;

        ClientToScreen(&mouse_x, &mouse_y);
        text->GetPosition(&edit_x, &edit_y);
        ClientToScreen(&edit_x, &edit_y);
    
        mouse_x -= edit_x;
        mouse_y -= edit_y;

        // get text from the text control
        wxClientDC dc(this);
        dc.SetFont(text->GetFont());

        wxString edit_str = text->GetValue();
        dc.GetTextExtent(edit_str, &edit_x, &edit_y);


        // do adjustment based on the justification of the text ctrl
        
        unsigned int textctrl_style = text->GetWindowStyleFlag();
        if (textctrl_style & wxTE_CENTER)
        {
            int textctrl_width, textctrl_height;
            text->GetClientSize(&textctrl_width, &textctrl_height);

            mouse_x -= ((textctrl_width-edit_x)/2);
        }
        else if (textctrl_style & wxTE_RIGHT)
        {
            int textctrl_width, textctrl_height;
            text->GetClientSize(&textctrl_width, &textctrl_height);

            mouse_x -= (textctrl_width-edit_x-2);
        }


        wxString test_str;
        int last_x;
        edit_x = 0;
        int len = edit_str.Length();
        for (int i = 1; i <= len; ++i)
        {
            test_str = edit_str.Left(i);
            last_x = edit_x;
            dc.GetTextExtent(test_str, &edit_x, &edit_y);
            if (edit_x > mouse_x)
            {
                if (mouse_x > edit_x-((edit_x-last_x)/2)-1)
                {
                    i++;
                }

                text->SetInsertionPoint(i-1);
                break;
            }
        }
    }
}



void Grid::endEdit(bool accept)
{
    if (!m_control)
        return;

    if (m_control->IsKindOf(CLASSINFO(GridTextCtrl)))
    {
        GridTextCtrl* ctrl = static_cast<GridTextCtrl*>(m_control);
        ctrl->destroyNow(accept);
        return;
    }
    else if (m_control->IsKindOf(CLASSINFO(GridChoiceCtrl)))
    {
        GridChoiceCtrl* ctrl = static_cast<GridChoiceCtrl*>(m_control);
        ctrl->destroyNow(accept);
        return;
    }
    else if (m_control->IsKindOf(CLASSINFO(GridComboCtrl)))
    {
        GridComboCtrl* ctrl = static_cast<GridComboCtrl*>(m_control);
        ctrl->destroyNow(accept);
        return;
    }
}

bool Grid::isEditing()
{
    return m_control ? true : false;
}


void Grid::repaint(wxDC* dc, wxRect* rect)
{
    int x, y, w, h;

    if (!m_gui_initialized)
        return;

    if (!IsShown())
        return;

    x = 0;
    y = 0;
    w = m_cliwidth;
    h = m_cliheight;

    if (rect)
    {
        x = rect->x;
        y = rect->y;
        w = rect->width;
        h = rect->height;
    }

    if (!m_bmp.Ok())
    {
        if (dc)
        {
            // if there is no bitmap
            dc->SetBrush(*wxWHITE_BRUSH);
            dc->SetPen(*wxWHITE_PEN);
            dc->DrawRectangle(x,y,w,h);
        }
        else
        {
            wxClientDC cdc(this);
            cdc.SetBrush(*wxWHITE_BRUSH);
            cdc.SetPen(*wxWHITE_PEN);
            cdc.DrawRectangle(x,y,w,h);
        }
        return;
    }

    if (dc)
    {
        dc->Blit(x, y, w, h, &m_memdc, x, y);
    }
    else
    {
        wxClientDC cdc(this);
        cdc.Blit(x, y, w, h, &m_memdc, x, y);
    }
}

void Grid::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    if (m_mask_paint)
        return;
    wxRect update_rect = GetUpdateClientRect();
    repaint(&dc, &update_rect);
}



};  // namespace kcl


