/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2008-02-06
 *
 */


#include "appmain.h"
#include "relationdiagram.h"
#include "dlgdatabasefile.h"
#include "relationdnd.h"
#include "dbdoc.h"
#include "jsonconfig.h"

#if defined(__WXMSW__)
    #include <wx/msw/uxtheme.h>
#endif


/* XPM */
static const char* xpm_relationshipbox_close[] = {
"16 16 2 1",
"  c None",
"x c #000000",
"                ",
"                ",
"                ",
"                ",
"                ",
"   xxx   xxx    ",
"    xxx xxx     ",
"     xxxxx      ",
"      xxx       ",
"     xxxxx      ",
"    xxx xxx     ",
"   xxx   xxx    ",
"                ",
"                ",
"                ",
"                "};



const int BOX_CAPTION_HEIGHT = 18;
const int BOX_BORDER_SIZE = 3;
const int BOX_DEFAULT_WIDTH = 160;
const int BOX_DEFAULT_HEIGHT = 180;
const int BOX_MIN_WIDTH = 100;
const int BOX_MIN_HEIGHT = 120;

const int DIAGRAM_SCROLL_WIDTH = 1600;
const int DIAGRAM_SCROLL_HEIGHT = 1200;

const int DRAGDROP_X_OFFSET = 16;
const int DRAGDROP_Y_OFFSET = 16;


// -- utility functions --

static inline bool IsUxThemed()
{
    #if defined(__WXMSW__)
    #if wxUSE_UXTHEME
        return true;
    #endif
    #endif

    return false;
}

static bool DrawUxThemeCloseButton(wxDC& dc,
                            const wxWindow* wnd,
                            const wxRect& rect,
                            int button_state)
{
#if defined(__WXMSW__)
#if wxUSE_UXTHEME

#if wxCHECK_VERSION(3,1,1)
    if (true)
#else
    if (wxUxThemeEngine::Get())
#endif
    {
        wxUxThemeHandle hTheme(wnd, L"WINDOW");
        if (hTheme)
        {
            int state = 1;
            switch (button_state)
            {
                case RelationBox::ButtonStateNormal:
                    state = 1; // CBS_NORMAL
                    break;
                case RelationBox::ButtonStateHover:
                    state = 2; // CBS_HOT
                    break;
                case RelationBox::ButtonStatePressed:
                    state = 3; // CBS_PUSHED
                    break;
                case RelationBox::ButtonStateDisabled:
                    state = 4; //CBS_DISABLED
                    break;
                default:
                    wxASSERT_MSG(false, wxT("Unknown state"));
            }

            RECT rc;
            wxCopyRectToRECT(rect, rc);
            rc.top += (rect.height-wnd->FromDIP(13))/2;
            rc.right = rc.left+wnd->FromDIP(13);
            rc.bottom = rc.top+wnd->FromDIP(13);
            
            // draw the themed close button
#if wxCHECK_VERSION(3,1,1)
            ::DrawThemeBackground(hTheme, (HDC)kcl::getHdcFrom(dc), WP_SMALLCLOSEBUTTON, state, &rc, NULL);
#else
            wxUxThemeEngine::Get()->DrawThemeBackground(
                                hTheme, 
                                (HDC)kcl::getHdcFrom(dc), 
                                WP_SMALLCLOSEBUTTON,
                                state, &rc, NULL);
#endif

            return true;
        }
    }
    
#endif  // wxUSE_UXTHEME
#endif  // defined(__WXMSW__)

    return false;
}

void showAddTableDialog(RelationDiagram* diagram)
{
    if (diagram == NULL)
        return;
            
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeOpenTable);

    dlg.setCaption(_("Add Table"));
    dlg.setAffirmativeButtonLabel(_("Add"));
    
    if (dlg.ShowModal() == wxID_OK)
    {
        if (isValidTable(towstr(dlg.getPath())))
        {
            wxString path = dlg.getPath();
            wxString caption = path.AfterLast(wxT('/'));

            bool allow = true;
            diagram->sigSetAdded(path, &allow);
            if (!allow)
                return;
            
            diagram->addBox(path, caption);
        }
         else
        {
            appMessageBox(_("The table could not be opened. It may currently be in use."),
                               APPLICATION_NAME,
                               wxOK | wxICON_INFORMATION | wxCENTER);
        }
    }
}

static bool sortA(const wxString& l, const wxString& r)
{
    return l.CmpNoCase(r) < 0;
}

static bool sortD(const wxString& l, const wxString& r)
{
    return l.CmpNoCase(r) >= 0;
}




// -- RelationBox class implementation --

BEGIN_EVENT_TABLE(RelationBox, wxControl)
    EVT_PAINT(RelationBox::onPaint)
    EVT_SIZE(RelationBox::onSize)
    EVT_MOUSE_EVENTS(RelationBox::onMouse)
    EVT_TIMER(RelationBox::ID_CursorTimer, RelationBox::onCursorTimer)
    EVT_ERASE_BACKGROUND(RelationBox::onEraseBackground)
    EVT_KCLGRID_BEGIN_DRAG(RelationBox::onBeginDrag)
    EVT_KCLGRID_VSCROLL(RelationBox::onVScroll)
    EVT_KCLGRID_CELL_LEFT_CLICK(RelationBox::onLeftClick)
    EVT_KCLGRID_CELL_LEFT_DCLICK(RelationBox::onLeftDblClick)
END_EVENT_TABLE()


RelationBox::RelationBox(RelationDiagram* parent,
                         wxWindowID id,
                         const wxString& caption,
                         const wxPoint& position,
                         const wxSize& size)
                     : wxControl(parent, id, position, size,
                                 wxNO_BORDER |
                                 wxCLIP_CHILDREN |
                                 wxCLIP_SIBLINGS |
                                 wxNO_FULL_REPAINT_ON_RESIZE)
{
    m_diagram = parent;
    m_caption = caption;
    m_sort_order = RelationBox::SortNone;
    
    m_caption_font = *wxNORMAL_FONT;
    m_caption_font.SetPointSize(8);
    
    m_base_color = kcl::getBaseColor();
    wxColor border_color = kcl::getBorderColor();
    wxColor caption_color = kcl::getCaptionColor();
    
    m_border_pen = wxPen(border_color);
    m_caption_textcolor = caption_color;

    m_sortbutton_rect = wxRect(0,0,0,0);
    m_closebutton_rect = wxRect(0,0,0,0);
    m_closebutton_bitmap = wxBitmap(xpm_relationshipbox_close);
    
    m_box_caption_height = FromDIP(BOX_CAPTION_HEIGHT);
    m_box_border_size = FromDIP(BOX_BORDER_SIZE);
    m_box_min_height = FromDIP(BOX_MIN_HEIGHT);
    m_box_min_width = FromDIP(BOX_MIN_WIDTH);

    m_action = RelationBox::ActionNone;

    m_grid = new kcl::Grid(this, ID_FieldList, wxPoint(0,0), wxSize(0,0));
    m_grid->setOptionState(kcl::Grid::optHorzGridLines |
                           kcl::Grid::optVertGridLines |
                           kcl::Grid::optSelect |
                           kcl::Grid::optColumnResize |
                           kcl::Grid::optEdit, false);
    m_grid->setOptionState(kcl::Grid::optContinuousScrolling |
                           kcl::Grid::optSelectionList, true);
    m_grid->setRowLabelSize(0);
    m_grid->setHeaderSize(0);
    m_grid->setCursorType(kcl::Grid::cursorRowOutline);
    m_grid->setCursorVisible(false);
    m_grid->setBorderType(wxNO_BORDER);
    m_grid->createModelColumn(-1, _("Name"), kcl::Grid::typeCharacter, 0, 0);
    m_grid->createDefaultView();
    m_grid->setColumnProportionalSize(0, 1);
    m_grid->setRowHeight(m_grid->getRowHeight() - 2);
    
    RelationLineDropTarget* drop_target = new RelationLineDropTarget;
    drop_target->sigDragDrop.connect(this, &RelationBox::onCreateLink);
    m_grid->SetDropTarget(drop_target);

    doSizing();

    m_diagram->m_boxes.push_back(this);

    // it is necessary for the RelationDiagram to have
    // the focus so keyboard event can be processed
    m_diagram->SetFocusIgnoringChildren();
}

RelationBox::~RelationBox()
{
}

const xd::Structure& RelationBox::getStructure()
{
    return m_structure;
}

void RelationBox::setStructure(const xd::Structure& s)
{
    m_structure = s;
    populate();
}

void RelationBox::setCaption(const wxString& caption)
{
    m_caption = caption;
    recalcCaption();
}

void RelationBox::setSetPath(const wxString& set_path)
{
    m_set_path = set_path;
}

wxString RelationBox::getSetPath()
{
    return m_set_path;
}

bool RelationBox::getItemExists(const wxString& expr)
{
    int i;
    int row_count = m_grid->getRowCount();

    for (i = 0; i < row_count; ++i)
    {
        wxString s = m_grid->getCellString(i, 0);
        if (s.CmpNoCase(expr) == 0)
        {
            return true;
        }
    }

    return false;
}

int RelationBox::getItemY(const wxString& expr)
{
    int row_count = m_grid->getRowCount();
    int row_height = FromDIP(m_grid->getRowHeight());
    int row_offset = m_grid->getRowOffset();
    int vis_rows = m_grid->getVisibleRowCount();
    int item = -1;
    int i;

    int x, y;
    int cli_width, cli_height;
    x = 0;
    y = 0;
    bool found = false;

    GetClientSize(&cli_width, &cli_height);

    for (i = 0; i < row_count; ++i)
    {
        wxString s = m_grid->getCellString(i, 0);
        if (s.CmpNoCase(expr) == 0)
        {
            if (i < row_offset)
            {
                // before the begin
                return m_box_border_size+(m_box_caption_height/2);
            }

            if (i > row_offset + vis_rows)
            {
                return cli_height;
            }

            found = true;
            item = i;
            break;
        }

        y += row_height;
    }

    if (!found)
    {
        // before the begin
        return m_box_border_size+(m_box_caption_height/2);
    }

    y -= (row_offset*row_height);
    y += (row_height/2);

    m_grid->ClientToScreen(&x, &y);
    ScreenToClient(&x, &y);

    // if it's close enough to the bottom, just put it
    // completely at the bottom.  This cleans up the way
    // it looks when multiple lines go to the same location

    if (abs(cli_height - y) <= 2)
    {
        return cli_height;
    }

    return y;
}

void RelationBox::fireSizedMovedSignal()
{
    wxRect rect;
    rect = GetRect();

    int vx, vy;
    m_diagram->GetViewStart(&vx, &vy);

    vx *= 10;
    vy *= 10;

    rect.x += vx;
    rect.y += vy;

    m_diagram->sigBoxSizedMoved(m_set_path, rect);
}

void RelationBox::raise()
{
    // make sure the boxes are in the right order in the m_boxes
    // vector in the RelationDiagram -- this is really important
    // since the boxes will draw in whatever order they are in
    std::vector<RelationBox*>::iterator it;

    for (it = m_diagram->m_boxes.begin(); it != m_diagram->m_boxes.end(); ++it)
    {
        if ((*it) == this)
        {
            (*it)->m_grid->Raise();
            (*it)->Raise();
        }
        else
        {
            (*it)->m_grid->Lower();
            (*it)->Lower();
        }
    }

    for (it = m_diagram->m_boxes.begin(); it != m_diagram->m_boxes.end(); ++it)
    {
        if ((*it) == this)
        {
            // remove this box from its current location in the vector
            m_diagram->m_boxes.erase(it);
            
            // add this box to the end of the boxes vector (this will
            // make sure it is drawn last, which is proper behavior
            // since it was the last raised box)
            m_diagram->m_boxes.push_back(this);
            break;
        }
    }

    ::wxYield();
}

void RelationBox::redraw()
{
    m_grid->refresh(kcl::Grid::refreshAll);
    Refresh(false);
}

void RelationBox::repopulate()
{
    populate();
}

void RelationBox::clearSelection()
{
    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void RelationBox::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);

    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);


    // draw the background area for the fields
    
    dc.SetPen(m_border_pen);
    dc.SetBrush(*wxWHITE);
    dc.DrawRectangle(0,
                     m_box_caption_height+m_box_border_size-1,
                     cli_width,
                     cli_height-(m_box_caption_height+m_box_border_size-1));


    // draw the caption background

    wxRect r(1, 1, cli_width, m_box_caption_height+m_box_border_size);
    r.height /= 2;
    r.height++;

    // caption top gradient
    wxColor top_color = m_base_color;
    wxColor bottom_color = kcl::stepColor(top_color, 170);
    dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);
    
    r.y += r.height;
    r.y--;
    
    // caption bottom fill
    top_color = kcl::getBaseColor();
    bottom_color = top_color;
    dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);

    // caption border
    dc.SetPen(m_border_pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0,0, cli_width, m_box_caption_height+m_box_border_size+1);

    // create the close button rect for future use
    if (IsUxThemed())
    {
        m_closebutton_rect = wxRect(cli_width-m_box_border_size-FromDIP(15),
                                    m_box_border_size+FromDIP(1),
                                    FromDIP(13), FromDIP(13));

        // create the sort button rect for future use
        m_sortbutton_rect = m_closebutton_rect;
        m_sortbutton_rect.x -= FromDIP(17);
        m_sortbutton_rect.y -= FromDIP(1);
    }
     else
    {
        m_closebutton_rect = wxRect(cli_width-m_box_border_size-FromDIP(16),
                                    m_box_border_size-FromDIP(1),
                                    FromDIP(16), FromDIP(16));

        // create the sort button rect for future use
        m_sortbutton_rect = m_closebutton_rect;
        m_sortbutton_rect.x -= FromDIP(16);
        m_sortbutton_rect.y += FromDIP(1);
    }

    wxCoord caption_width, caption_height;
    dc.GetTextExtent(m_paint_caption, &caption_width, &caption_height);

    int caption_y = (m_box_caption_height+m_box_border_size-caption_height+2)/2;
    dc.SetFont(m_caption_font);
    dc.SetTextForeground(m_caption_textcolor);
    dc.SetClippingRegion(0, 0, cli_width, FromDIP(21));
    dc.DrawText(m_paint_caption, m_box_border_size+4, caption_y);


    wxBitmap sort_bmp;
    switch (m_sort_order)
    {
        case 1: sort_bmp = GETBMPSMALL(gf_columnsortup); break;
        case 2: sort_bmp = GETBMPSMALL(gf_columnsortdown); break;
        default:
        case 3: sort_bmp = GETBMPSMALL(gf_columnsortnone); break;
    }

    dc.DrawBitmap(sort_bmp,
                  m_sortbutton_rect.GetX(),
                  m_sortbutton_rect.GetY(),
                  true);

    if (!DrawUxThemeCloseButton(dc, this, m_closebutton_rect,
                                RelationBox::ButtonStateNormal))
    {
        dc.DrawBitmap(m_closebutton_bitmap,
                      m_closebutton_rect.GetX(),
                      m_closebutton_rect.GetY(),
                      true);
    }
    
    dc.DestroyClippingRegion();
}

void RelationBox::onEraseBackground(wxEraseEvent& evt)
{
}

void RelationBox::doSizing()
{
    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    // handle redraw issues outside the grid

    wxSize oldsize = GetClientSize();

    wxRect r1(1, oldsize.y-3, oldsize.x-1, oldsize.y-1);
    Refresh(FALSE, &r1);

    wxRect r2(oldsize.x-3, m_box_caption_height+m_box_border_size+2, oldsize.x-1, oldsize.y-1);
    Refresh(FALSE, &r2);

    recalcCaption();

    // redraw grid

    m_grid->SetSize(1,
                    m_box_caption_height+m_box_border_size+1,
                    cli_width-m_box_border_size-1,
                    cli_height-m_box_caption_height-((m_box_border_size*2))-1);

    m_grid->refresh(kcl::Grid::refreshPaint);
}

void RelationBox::recalcCaption()
{
    // handle long captions with ellipses

    wxSize oldsize = GetClientSize();

    wxClientDC dc(this);
    int caption_width, caption_height;
    wxString caption, temp_caption;

    caption = m_caption;
    temp_caption = caption;
    dc.GetTextExtent(caption, &caption_width, &caption_height);

    while (caption_width+(m_box_border_size*2)+24 > oldsize.x)
    {
        caption = caption.RemoveLast();
        if (caption.IsEmpty())
            break;
        temp_caption = caption;
        temp_caption += wxT("...");

        dc.GetTextExtent(temp_caption, &caption_width, &caption_height);
    }

    if (temp_caption != caption || caption != m_paint_caption)
    {
        caption = temp_caption;

        int caption_y = (m_box_caption_height+m_box_border_size-caption_height+2)/2;

        m_paint_caption = caption;
        wxRect r3(1, 1, oldsize.x-1, m_box_caption_height-1);
        Refresh(FALSE, &r3);
    }
}

void RelationBox::onSize(wxSizeEvent& evt)
{
    doSizing();
}

void RelationBox::cycleSortOrder()
{
    m_sort_order++;
    if (m_sort_order > RelationBox::SortNone)
        m_sort_order = RelationBox::SortAscending;
    populate();
    Refresh();
    m_diagram->repaint();
}

void RelationBox::onMouse(wxMouseEvent& evt)
{
    int evt_type = evt.GetEventType();
    
    if (evt_type == wxEVT_RIGHT_DOWN)
    {
        raise();
        redraw();
    }
     else if (evt_type == wxEVT_LEFT_DCLICK)
    {
        cycleSortOrder();
        return;
    }
     else if (evt_type == wxEVT_LEFT_DOWN)
    {
        raise();
        redraw();

        int cli_width, cli_height;
        GetClientSize(&cli_width, &cli_height);

        if (m_closebutton_rect.Contains(evt.GetPosition()))
        {
            m_action = RelationBox::ActionClose;
            CaptureMouse();

            wxClientDC dc(this);
            
            if (!DrawUxThemeCloseButton(dc, this, m_closebutton_rect,
                                        RelationBox::ButtonStatePressed))
            {
                dc.DrawBitmap(m_closebutton_bitmap,
                              m_closebutton_rect.GetX(),
                              m_closebutton_rect.GetY(),
                              true);
            }
            
            return;
        }

        if (m_sortbutton_rect.Contains(evt.GetPosition()))
        {
            cycleSortOrder();
            return;
        }

        if (abs(evt.m_y - cli_height) <= 4 &&
            abs(evt.m_x - cli_width) <= 4)
        {
            m_action = RelationBox::ActionSEResize;
            SetCursor(wxCURSOR_SIZENWSE);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }
         else if (abs(evt.m_y - cli_height) <= 4 &&
                  evt.m_x <= 4)
        {
            m_action = RelationBox::ActionSWResize;
            SetCursor(wxCURSOR_SIZENESW);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }
         else if (evt.m_y <= 4 &&
                  evt.m_x <= 4)
        {
            m_action = RelationBox::ActionNWResize;
            SetCursor(wxCURSOR_SIZENWSE);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }
         else if (evt.m_y <= 4 &&
                  abs(evt.m_x - cli_width) <= 4)
        {
            m_action = RelationBox::ActionNEResize;
            SetCursor(wxCURSOR_SIZENESW);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }
         else if (abs(evt.m_y - cli_height) <= 4 &&
                  abs(evt.m_x - cli_width) <= 4)
        {
            m_action = RelationBox::ActionSEResize;
            SetCursor(wxCURSOR_SIZENWSE);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }
         else if (abs(evt.m_y - cli_height) <= 4)
        {
            m_action = RelationBox::ActionSResize;
            SetCursor(wxCURSOR_SIZENS);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }
         else if (abs(evt.m_x - cli_width) <= 4)
        {
            m_action = RelationBox::ActionEResize;
            SetCursor(wxCURSOR_SIZEWE);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }
         else if (evt.m_x <= 4)
        {
            m_action = RelationBox::ActionWResize;
            SetCursor(wxCURSOR_SIZEWE);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }
         else if (evt.m_y <= 4)
        {
            m_action = RelationBox::ActionNResize;
            SetCursor(wxCURSOR_SIZENS);
            m_cursor_timer.Stop();
            CaptureMouse();
            return;
        }

        if (evt.m_y < m_box_caption_height)
        {
            m_action = RelationBox::ActionMove;
            m_action_offx = evt.m_x;
            m_action_offy = evt.m_y;
            CaptureMouse();
            return;
        }

    }
     else if (evt_type == wxEVT_LEFT_UP)
    {
        if (m_action == RelationBox::ActionMove)
        {
            m_diagram->refresh();
            ReleaseMouse();
            m_action = RelationBox::ActionNone;

            raise();
            redraw();
        }
         else if (m_action == RelationBox::ActionSEResize ||
                  m_action == RelationBox::ActionSWResize ||
                  m_action == RelationBox::ActionNWResize ||
                  m_action == RelationBox::ActionNEResize ||
                  m_action == RelationBox::ActionSResize ||
                  m_action == RelationBox::ActionEResize ||
                  m_action == RelationBox::ActionWResize ||
                  m_action == RelationBox::ActionNResize)
        {
            ReleaseMouse();
            SetCursor(wxNullCursor);
            m_action = RelationBox::ActionNone;
        }
         else if (m_action == RelationBox::ActionClose)
        {
            m_action = RelationBox::ActionNone;
            ReleaseMouse();

            int cli_width, cli_height;
            GetClientSize(&cli_width, &cli_height);

            if (m_closebutton_rect.Contains(evt.GetPosition()))
            {
                m_diagram->sigSetRemoved(m_set_path);

                Refresh();

                bool allow = true;
                m_diagram->onBoxClosing(this, &allow);

                if (allow)
                {
                    std::vector<RelationBox*>::iterator it;
                    it = std::find(m_diagram->m_boxes.begin(),
                                   m_diagram->m_boxes.end(),
                                   this);
                    if (it != m_diagram->m_boxes.end())
                    {
                        m_diagram->m_boxes.erase(it);
                    }

                    m_diagram->repaint();
                    Destroy();
                }

                return;
            }

            Refresh();
        }
    }
     else if (evt_type == wxEVT_MOTION)
    {
        if (m_action == RelationBox::ActionNone)
        {
            raise();
        }

        wxCursor cursor = wxNullCursor;

        int cli_width, cli_height;
        GetClientSize(&cli_width, &cli_height);

        if (m_action == RelationBox::ActionMove)
        {
            wxSize clisize = GetParent()->GetClientSize();

            wxPoint pt = ::wxGetMousePosition();
            pt = GetParent()->ScreenToClient(pt);
            int newx, newy;
            newx = pt.x - m_action_offx;
            newy = pt.y - m_action_offy;

            if (newx+cli_width > clisize.x)
                newx = clisize.x - cli_width;
            if (newy+cli_height > clisize.y)
                newy = clisize.y - cli_height;
            if (newx < 0)
                newx = 0;
            if (newy < 0)
                newy = 0;

            Move(newx, newy);
            diagramRefresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionSEResize)
        {
            wxSize oldsize = GetClientSize();
            wxPoint pt = ::wxGetMousePosition();
            pt = ScreenToClient(pt);
            if (pt.x < m_box_min_width)
                pt.x = m_box_min_width;
            if (pt.y < m_box_min_height)
                pt.y = m_box_min_height;
            SetSize(pt.x, pt.y);
            wxRect r(oldsize.x-50, 0, pt.x-(oldsize.x-50)+1, pt.y);
            Refresh(FALSE, &r);
            Update();
            m_diagram->refresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionSResize)
        {
            wxSize oldsize = GetClientSize();

            wxPoint pt = ::wxGetMousePosition();
            pt = ScreenToClient(pt);
            if (pt.y < m_box_min_height)
                pt.y = m_box_min_height;
            SetSize(-1, pt.y);
            wxRect r(0, oldsize.y, oldsize.x, pt.y-oldsize.y+1);
            Refresh(FALSE, &r);
            Update();
            m_diagram->refresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionEResize)
        {
            wxSize oldsize = GetClientSize();
            wxPoint pt = ::wxGetMousePosition();
            pt = ScreenToClient(pt);
            if (pt.x < m_box_min_width)
                pt.x = m_box_min_width;
            SetSize(pt.x, -1);
            wxRect r(oldsize.x-50, 0, pt.x-(oldsize.x-50)+1, pt.y);
            Refresh(FALSE, &r);
            Update();
            m_diagram->refresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionWResize)
        {
            int old_x, old_y;
            int old_w, old_h;
            GetPosition(&old_x, &old_y);
            GetSize(&old_w, &old_h);
            wxPoint pt = ::wxGetMousePosition();
            pt = GetParent()->ScreenToClient(pt);
            int new_w = old_w + (old_x - pt.x);
            if (new_w < m_box_min_width)
                new_w = m_box_min_width;
            if (pt.x > old_x+old_w-new_w)
                pt.x = old_x+old_w-new_w;
            SetSize(pt.x, old_y, new_w, old_h);
            Refresh(FALSE);
            Update();
            m_diagram->refresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionNResize)
        {
            int old_x, old_y;
            int old_w, old_h;
            GetPosition(&old_x, &old_y);
            GetSize(&old_w, &old_h);
            wxPoint pt = ::wxGetMousePosition();
            pt = GetParent()->ScreenToClient(pt);
            if (pt.y < 0)
                pt.y = 0;
            int new_h = old_h + (old_y - pt.y);
            if (new_h < m_box_min_height)
                new_h = m_box_min_height;
            if (pt.y > old_y+old_h-new_h)
                pt.y = old_y+old_h-new_h;
            SetSize(old_x, pt.y, old_w, new_h);
            Refresh(FALSE);
            Update();
            m_diagram->refresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionSWResize)
        {
            int old_x, old_y;
            int old_w, old_h;
            GetPosition(&old_x, &old_y);
            GetSize(&old_w, &old_h);
            wxPoint pt = ::wxGetMousePosition();
            pt = GetParent()->ScreenToClient(pt);
            int new_h = pt.y - old_y;
            if (new_h < m_box_min_height)
                new_h = m_box_min_height;
            int new_w = old_w + (old_x - pt.x);
            if (new_w < m_box_min_width)
                new_w = m_box_min_width;
            if (pt.x > old_x+old_w-new_w)
                pt.x = old_x+old_w-new_w;
            SetSize(pt.x, old_y, new_w, new_h);
            Refresh(FALSE);
            Update();
            m_diagram->refresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionNWResize)
        {
            int old_x, old_y;
            int old_w, old_h;
            GetPosition(&old_x, &old_y);
            GetSize(&old_w, &old_h);
            wxPoint pt = ::wxGetMousePosition();
            pt = GetParent()->ScreenToClient(pt);
            if (pt.y < 0)
                pt.y = 0;
            int new_h = old_h + (old_y - pt.y);
            if (new_h < m_box_min_height)
                new_h = m_box_min_height;
            int new_w = old_w + (old_x - pt.x);
            if (new_w < m_box_min_width)
                new_w = m_box_min_width;
            if (pt.x > old_x+old_w-new_w)
                pt.x = old_x+old_w-new_w;
            if (pt.y > old_y+old_h-new_h)
                pt.y = old_y+old_h-new_h;
            SetSize(pt.x, pt.y, new_w, new_h);
            Refresh(FALSE);
            Update();
            m_diagram->refresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionNEResize)
        {
            int old_x, old_y;
            int old_w, old_h;
            GetPosition(&old_x, &old_y);
            GetSize(&old_w, &old_h);
            wxPoint pt = ::wxGetMousePosition();
            pt = GetParent()->ScreenToClient(pt);
            if (pt.y < 0)
                pt.y = 0;
            int new_h = old_h + (old_y - pt.y);
            if (new_h < m_box_min_height)
                new_h = m_box_min_height;
            int new_w = pt.x - old_x;
            if (new_w < m_box_min_width)
                new_w = m_box_min_width;
            if (pt.y > old_y+old_h-new_h)
                pt.y = old_y+old_h-new_h;
            SetSize(old_x, pt.y, new_w, new_h);
            Refresh(FALSE);
            Update();
            m_diagram->refresh();
            fireSizedMovedSignal();
        }
         else if (m_action == RelationBox::ActionClose)
        {
            wxClientDC dc(this);

            if (m_closebutton_rect.Contains(evt.GetPosition()))
            {
                if (!DrawUxThemeCloseButton(dc, this, m_closebutton_rect,
                                            RelationBox::ButtonStatePressed))
                {
                    dc.DrawBitmap(m_closebutton_bitmap,
                                  m_closebutton_rect.GetX(),
                                  m_closebutton_rect.GetY(),
                                  true);
                }
            }
             else
            {
                if (!DrawUxThemeCloseButton(dc, this, m_closebutton_rect,
                                            RelationBox::ButtonStateNormal))
                {
                    dc.DrawBitmap(m_closebutton_bitmap,
                                  m_closebutton_rect.GetX(),
                                  m_closebutton_rect.GetY(),
                                  true);
                }
            }
        }

        if (m_action == RelationBox::ActionNone && !cursor.Ok())
        {
            if (abs(evt.m_y - cli_height) <= 4 &&
                abs(evt.m_x - cli_width) <= 4)
            {
                cursor = wxCURSOR_SIZENWSE;
                m_cursor_timer.SetOwner(this, ID_CursorTimer);
                m_cursor_timer.Start(100, true);
            }
             else if (abs(evt.m_y - cli_height) <= 4 &&
                      evt.m_x <= 4)
            {
                cursor = wxCURSOR_SIZENESW;
                m_cursor_timer.SetOwner(this, ID_CursorTimer);
                m_cursor_timer.Start(100, true);
            }
             else if (evt.m_y <= 4 &&
                      abs(evt.m_x - cli_width) <= 4)
            {
                cursor = wxCURSOR_SIZENESW;
                m_cursor_timer.SetOwner(this, ID_CursorTimer);
                m_cursor_timer.Start(100, true);
            }
             else if (evt.m_y <= 4 &&
                      evt.m_x <= 4)
            {
                cursor = wxCURSOR_SIZENWSE;
                m_cursor_timer.SetOwner(this, ID_CursorTimer);
                m_cursor_timer.Start(100, true);
            }
             else if (abs(evt.m_y - cli_height) <= 4)
            {
                cursor = wxCURSOR_SIZENS;
                m_cursor_timer.SetOwner(this, ID_CursorTimer);
                m_cursor_timer.Start(100, true);
            }
             else if (abs(evt.m_x - cli_width) <= 4)
            {
                cursor = wxCURSOR_SIZEWE;
                m_cursor_timer.SetOwner(this, ID_CursorTimer);
                m_cursor_timer.Start(100, true);
            }
             else if (evt.m_x <= 4)
            {
                cursor = wxCURSOR_SIZEWE;
                m_cursor_timer.SetOwner(this, ID_CursorTimer);
                m_cursor_timer.Start(100, true);
            }
             else if (evt.m_y <= 4)
            {
                cursor = wxCURSOR_SIZENS;
                m_cursor_timer.SetOwner(this, ID_CursorTimer);
                m_cursor_timer.Start(100, true);
            }

        }

        SetCursor(cursor);
    }
}

void RelationBox::onCursorTimer(wxTimerEvent& evt)
{
    if (m_action == RelationBox::ActionSEResize ||
        m_action == RelationBox::ActionSResize ||
        m_action == RelationBox::ActionEResize)
    {
        return;
    }

    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);
    
    wxPoint pt = ::wxGetMousePosition();
    pt = ScreenToClient(pt);

    if (abs(pt.x - cli_height) <= 4 &&
        abs(pt.y - cli_width) <= 4)
    {
        return;
    }
     else
    {
        SetCursor(wxNullCursor);
    }
}

void RelationBox::populate()
{
    int offset = m_grid->getRowOffset();

    m_grid->deleteAllRows();

    std::vector<wxString> items;

    size_t col_count = m_structure.getColumnCount();
    int i;
    int item_idx = 0;

    for (i = 0; i < (int)col_count; ++i)
    {
        items.push_back(m_structure.getColumnName(i));
    }

    if (m_sort_order == RelationBox::SortAscending)
    {
        std::sort(items.begin(), items.end(), sortA);
    }
     else if (m_sort_order == RelationBox::SortDescending)
    {
        std::sort(items.begin(), items.end(), sortD);
    }
     else
    {
        // no sort
    }

    std::vector<wxString>::iterator it;

    i = 0;
    for (it = items.begin(); it != items.end(); ++it)
    {
        const xd::ColumnInfo& colinfo = m_structure.getColumnInfo(towstr(*it));

        wxBitmap icon;

        if (colinfo.calculated)
        {
            icon = GETBMPSMALL(gf_lightning);
        }
         else
        {
            icon = GETBMPSMALL(gf_field);
        }
        
        wxString text = colinfo.name;


        m_grid->insertRow(-1);
        m_grid->setCellString(i, 0, text);
        m_grid->setCellBitmap(i, 0, icon, kcl::Grid::alignLeft);

        if (isLinked(text))
        {
            kcl::CellProperties props;
            props.mask = kcl::CellProperties::cpmaskBgColor;
            props.bgcolor = kcl::getLightHighlightColor();
            m_grid->setCellProperties(i, 0, &props);
        }

        i++;
    }

    m_grid->setRowOffset(offset);
    m_grid->refresh(kcl::Grid::refreshAll);
}

void RelationBox::onBeginDrag(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    if (row == -1)
        return;

    m_grid->refresh(kcl::Grid::refreshAll);

    wxString fields, tag = m_set_path.AfterLast(L'/');
    
    int i, row_count = m_grid->getRowCount();
    for (i = 0; i < row_count; ++i)
    {
        if (m_grid->isRowSelected(i))
        {
            wxString s;
            s = tag;
            s += wxT(".");
            s += m_grid->getCellString(i, 0);
            
            if (!fields.IsEmpty())
            {
                fields += wxT("\t");
            }
            fields += s;
        }
    }

    RelationLineDataObject data;
    data.setInfo(this, row, fields);
    wxDropSource dragSource(m_grid);
    dragSource.SetData(data);
    wxDragResult result = dragSource.DoDragDrop(TRUE);
}

static RelationBox* findTopmostBoxByPosition(const wxPoint& pt,
                                      std::vector<RelationBox*>& boxes)
{
    std::vector<RelationBox*>::reverse_iterator rit;
    for (rit = boxes.rbegin(); rit != boxes.rend(); ++rit)
    {
        wxRect box_rect = (*rit)->GetClientRect();
        (*rit)->ClientToScreen(&box_rect.x, &box_rect.y);
        
        if (box_rect.Contains(pt))
            return (*rit);
    }

    return NULL;
}

void RelationBox::onCreateLink(wxDragResult& drag_result,
                               RelationLineDataObject* data)
{
    std::vector<RelationBox*>::iterator it;

    RelationBox* src_box;
    int src_line;

    src_box = data->getBox();
    src_line = data->getLine();

    if (src_box == NULL || src_box == this)
        return;

    int tot_selected_lines = 0;
    int tot_src_lines = src_box->m_grid->getRowCount();
    for (int i = 0; i < tot_src_lines; ++i)
    {
        if (src_box->m_grid->isRowSelected(i))
            tot_selected_lines++;
    }

    if (tot_selected_lines > 1)
    {
        appMessageBox (_("Multi-field relationships cannot be created at one time.  Please connect each pair of fields individually."),
                              APPLICATION_NAME,
                              wxICON_EXCLAMATION | wxCENTER);
        src_box->clearSelection();
        return;
    }

    for (it = m_diagram->m_boxes.begin();
         it != m_diagram->m_boxes.end();
         ++it)
    {
        (*it)->clearSelection();
        (*it)->m_grid->refresh(kcl::Grid::refreshAll);
    }

    int row, col;
    
    wxPoint pt = m_grid->ScreenToClient(wxGetMousePosition());
    int item = m_grid->hitTest(pt.x, pt.y, &row, &col);

    if (row == -1)
        return;

    wxString left_path, left_expr, right_path, right_expr;

    left_path = src_box->getSetPath();
    left_expr = src_box->m_grid->getCellString(src_line, 0);
    right_path = m_set_path;
    right_expr = m_grid->getCellString(row, 0);


    // make sure the line does not already exist
    std::vector<RelationLine*> existing_lines;
    std::vector<RelationLine*>::iterator line_it;
    m_diagram->getLines(left_path, right_path, existing_lines);

    for (line_it = existing_lines.begin();
         line_it != existing_lines.end();
         ++line_it)
    {
        if ((*line_it)->left_expr.CmpNoCase(left_expr) == 0 &&
            (*line_it)->right_expr.CmpNoCase(right_expr) == 0)
        {
            return;
        }
    }

    // add the line to the diagram
    m_diagram->addLine(left_path,
                       left_expr,
                       right_path,
                       right_expr);

    m_diagram->refresh();


    src_box->populate();
    populate();


    m_diagram->sigDiagramUpdated();
}

void RelationBox::onLeftClick(kcl::GridEvent& evt)
{
    // make sure the topmost box is being clicked
    if (m_diagram->m_boxes.size() == 0)
        return;
    
    wxPoint pt = ::wxGetMousePosition();
    RelationBox* click_box = findTopmostBoxByPosition(pt, m_diagram->m_boxes);
    if (!click_box)
        return;
    
    if (click_box != this)
    {
        evt.SetEventObject(click_box);
        ::wxPostEvent(click_box, evt);
        return;
    }

    raise();
    redraw();

    std::vector<RelationBox*>::iterator it;
    for (it = m_diagram->m_boxes.begin();
         it != m_diagram->m_boxes.end();
         ++it)
    {
        if ((*it) != this &&
            (*it)->m_grid->getSelectionCount() > 0)
        {
            (*it)->clearSelection();
            (*it)->m_grid->refresh(kcl::Grid::refreshAll);
        }
    }

    evt.Skip();
}

void RelationBox::onLeftDblClick(kcl::GridEvent& evt)
{
    // make sure the topmost box is being clicked
    if (m_diagram->m_boxes.size() == 0)
        return;
    
    wxPoint click_pt = ::wxGetMousePosition();
    RelationBox* click_box = findTopmostBoxByPosition(click_pt, m_diagram->m_boxes);
    if (!click_box)
        return;
    
    if (click_box != this)
    {
        evt.SetEventObject(click_box);
        ::wxPostEvent(click_box, evt);
        return;
    }

    raise();
    redraw();

    int row = evt.GetRow();
    if (row < 0 || row >= m_grid->getRowCount())
        return;

    m_diagram->sigFieldActivated(m_set_path,
                                 m_grid->getCellString(row, 0));
}

void RelationBox::onVScroll(kcl::GridEvent& evt)
{
    m_diagram->refresh();
}

bool RelationBox::isLinked(const wxString& expr)
{
    std::vector<RelationLine>::iterator line_it;

    for (line_it = m_diagram->m_lines.begin();
         line_it != m_diagram->m_lines.end();
         ++line_it)
    {
        if (m_set_path == line_it->left_path &&
            0 == expr.CmpNoCase(line_it->left_expr))
        {
            return true;
        }

        if (m_set_path == line_it->right_path &&
            0 == expr.CmpNoCase(line_it->right_expr))
        {
            return true;
        }
    }

    return false;
}

void RelationBox::diagramRefresh()
{ 
    // win32 can redraw this stuff faster than other platforms
    
    #ifdef WIN32
        m_diagram->refresh();
    #else    
        static clock_t last = 0;
        clock_t c = clock();
        if (c-last >= CLOCKS_PER_SEC/80)
        {
            last = c;
            m_diagram->refresh();
        }
    #endif
}




// RelationshipDiagram class implementation

enum
{
    ID_AddExpression = 15000,
    ID_DeleteExpression,
    ID_DeleteRelation,
    ID_EditExpression,
    ID_CloseBox
};


BEGIN_EVENT_TABLE(RelationDiagram, wxScrolledWindow)
    EVT_PAINT(RelationDiagram::onPaint)
    EVT_KEY_DOWN(RelationDiagram::onKeyDown)
    EVT_MOUSE_EVENTS(RelationDiagram::onMouse)
    EVT_SIZE(RelationDiagram::onSize)
    EVT_ERASE_BACKGROUND(RelationDiagram::onEraseBackground)
    EVT_SCROLLWIN(RelationDiagram::onScroll)
END_EVENT_TABLE()


RelationDiagram::RelationDiagram(wxWindow* parent,
                                 wxWindowID id,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 bool x_scroll,
                                 bool y_scroll)
                            : wxScrolledWindow(parent,
                                              id,
                                              pos,
                                              size,
                                              wxNO_BORDER |
                                              wxCLIP_CHILDREN |
                                              wxNO_FULL_REPAINT_ON_RESIZE)
{
    SetScrollbars(10, 10,
                  x_scroll ? (DIAGRAM_SCROLL_WIDTH/10) : 0,
                  y_scroll ? (DIAGRAM_SCROLL_HEIGHT/10) : 0);
    SetBackgroundColour(*wxWHITE);
    
    EnableScrolling(x_scroll, y_scroll);

    m_overlay_font = wxFont(16, wxSWISS, wxNORMAL, wxNORMAL, false);
    m_overlay_text = _("To add tables, double-click here\nor drag in tables from the Project Panel");

    m_thick_black_pen = wxPen(*wxBLACK, 3, wxSOLID);
    m_thin_black_pen = wxPen(*wxBLACK, 1, wxSOLID);
    m_thick_red_pen = wxPen(wxColour(192,0,0), 3, wxSOLID);
    m_thin_red_pen = wxPen(wxColour(192,0,0), 1, wxSOLID);

    m_changed = false;

    FsDataDropTarget* drop_target = new FsDataDropTarget;
    drop_target->sigDragDrop.connect(this, &RelationDiagram::onTreeDataDropped);
    drop_target->sigDragLeave.connect(this, &RelationDiagram::onDragLeave);
    drop_target->sigDragOver.connect(this, &RelationDiagram::onDragOver);
    SetDropTarget(drop_target);
    
    m_first_time = true;
    m_selected_line = NULL;
}

RelationDiagram::~RelationDiagram()
{
}

bool RelationDiagram::save()
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    int vx, vy;
    GetViewStart(&vx, &vy);

    vx *= 10;
    vy *= 10;

    // store the relationship info a json node
    kl::JsonNode node;
    kl::JsonNode metadata_node = node["metadata"];
    metadata_node["type"] = L"application/vnd.kx.relmgrpanel";
    metadata_node["version"] = 1;
    metadata_node["description"] = L"";

    kl::JsonNode boxes_node = node["boxes"];
    boxes_node.setArray();

    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
    {
        int x, y, width, height;
        (*it)->GetPosition(&x, &y);
        (*it)->GetClientSize(&width, &height);

        x += vx;
        y += vy;

        kl::JsonNode boxes_child_node = boxes_node.appendElement();
        boxes_child_node["path"].setString(towstr((*it)->getSetPath()));
        boxes_child_node["xpos"].setInteger(ToDIP(x));
        boxes_child_node["ypos"].setInteger(ToDIP(y));
        boxes_child_node["width"].setInteger(ToDIP(width));
        boxes_child_node["height"].setInteger(ToDIP(height));
    }

    // save the job
    std::wstring path = kl::stdswprintf(L"/.appdata/%ls/panels/relmgrpanel",
                                        g_app->getDatabase()->getActiveUid().c_str());

    if (!JsonConfig::saveToDb(node, g_app->getDatabase(), path, L"application/vnd.kx.relmgrpanel"))
        return false;

    return true;
}


bool RelationDiagram::load()
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    kl::JsonNode node;

    // try to load the manager info from the new location
    std::wstring path = kl::stdswprintf(L"/.appdata/%ls/panels/relmgrpanel",
                            g_app->getDatabase()->getActiveUid().c_str());

    node = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (node.isOk())
    {
        // file exists in the location; make sure it's in the right format
        if (!isValidFileVersion(node, L"application/vnd.kx.relmgrpanel", 1))
            return false;

        kl::JsonNode boxes_node = node["boxes"];
        if (!boxes_node.isOk())
            return false;

        std::vector<kl::JsonNode> boxes_children_node = boxes_node.getChildren();
        std::vector<kl::JsonNode>::iterator it, it_end;
        it_end = boxes_children_node.end();

        for (it = boxes_children_node.begin(); it != it_end; ++it)
        {
            wxString path;
            int x, y, width, height;

            kl::JsonNode boxes_child_node = *it;
            path = boxes_child_node["path"].getString();
            x = FromDIP(boxes_child_node["xpos"].getInteger());
            y = FromDIP(boxes_child_node["ypos"].getInteger());
            width = FromDIP(boxes_child_node["width"].getInteger());
            height = FromDIP(boxes_child_node["height"].getInteger());

            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;
            if (width > FromDIP(500))
                width = FromDIP(500);
            if (height > FromDIP(500))
                height = FromDIP(500);
            if (x > DIAGRAM_SCROLL_WIDTH-width)
                x = DIAGRAM_SCROLL_WIDTH-width;
            if (y > DIAGRAM_SCROLL_HEIGHT-height)
                y = DIAGRAM_SCROLL_HEIGHT-height;

            wxString caption = path.AfterLast(wxT('/'));
            addBox(path, caption, *wxBLACK, x, y, width, height);
        }

        return true;
    }

    // file isn't in the new location; try the old location in the old format
    wxString old_location = kl::stdswprintf(L"/.appdata/%ls/dcfe/relmgrpanel",
                                            g_app->getDatabase()->getActiveUid().c_str());

    node = JsonConfig::loadFromDb(g_app->getDatabase(), towstr(old_location));
    if (node.isOk())
    {
        kl::JsonNode root_node = node["root"];
        if (!root_node.isOk())
            return false;

        // load the boxes from the old format; note: this parallels the way the
        // data is loaded from the new format at this time; keep implementation
        // separate in case new fomat changes
        kl::JsonNode boxes_node = root_node["boxes"];
        if (!boxes_node.isOk())
            return false;

        std::vector<kl::JsonNode> boxes_children_node = boxes_node.getChildren();
        std::vector<kl::JsonNode>::iterator it, it_end;
        it_end = boxes_children_node.end();

        for (it = boxes_children_node.begin(); it != it_end; ++it)
        {
            wxString path;
            int x, y, width, height;

            kl::JsonNode boxes_child_node = *it;
            path = boxes_child_node["path"].getString();
            x = boxes_child_node["xpos"].getInteger();
            y = boxes_child_node["ypos"].getInteger();
            width = boxes_child_node["width"].getInteger();
            height = boxes_child_node["height"].getInteger();

            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;
            if (width > 500)
                width = 500;
            if (height > 500)
                height = 500;
            if (x > DIAGRAM_SCROLL_WIDTH-width)
                x = DIAGRAM_SCROLL_WIDTH-width;
            if (y > DIAGRAM_SCROLL_HEIGHT-height)
                y = DIAGRAM_SCROLL_HEIGHT-height;

            wxString caption = path.AfterLast(wxT('/'));
            addBox(path, caption, *wxBLACK, x, y, width, height);
        }

        // save the info in the new location and format
        save();

        // delete the old
        if (db->getFileExist(towstr(old_location)))
            db->deleteFile(towstr(old_location));

        return true;
    }

    // couldn't load the file in the new or the old format
    return false;
}

void RelationDiagram::setOverlayText(const wxString& overlay_text)
{
    m_overlay_text = overlay_text;
}

void RelationDiagram::onScroll(wxScrollWinEvent& evt)
{
    repaint();
    evt.Skip();
}

RelationLine* RelationDiagram::addLine(const wxString& left_path,
                                       const wxString& left_expr,
                                       const wxString& right_path,
                                       const wxString& right_expr)
{
    RelationLine line;
    line.left_path = left_path;
    line.left_expr = left_expr;
    line.right_path = right_path;
    line.right_expr = right_expr;
    line.x1 = 0;
    line.y1 = 0;
    line.x2 = 0;
    line.y2 = 0;
    line.user = 0;
    line.selected = false;
    line.valid = true;

    m_lines.push_back(line);

    bool allow = true;
    sigLineAdded(&(m_lines.back()), &allow);

    if (!allow)
    {
        m_lines.pop_back();
        return NULL;
    }

    return &(m_lines.back());
}


void RelationDiagram::getBoxPaths(std::vector<wxString>& paths)
{
    paths.clear();

    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
        paths.push_back((*it)->getSetPath());
}

RelationBox* RelationDiagram::getBox(unsigned int idx)
{
    if (idx < 0 || idx >= m_boxes.size())
        return NULL;
        
    return m_boxes[idx];
}

RelationBox* RelationDiagram::getBoxByName(const wxString& path)
{
    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
    {
        if (path.CmpNoCase((*it)->getSetPath()) == 0)
            return (*it);
    }
    
    return NULL;
}

int RelationDiagram::getBoxCount()
{
    return m_boxes.size();
}


void RelationDiagram::deleteLine(RelationLine* l)
{
    std::vector<RelationLine>::iterator line_it;
    for (line_it = m_lines.begin(); line_it != m_lines.end(); ++line_it)
    {
        if (&(*line_it) == l)
        {
            RelationLine copy = *l;
            m_lines.erase(line_it);
            sigLineDeleted(&copy);
            break;
        }
    }
}

void RelationDiagram::deleteAllLines()
{
    m_lines.clear();
}

void RelationDiagram::deleteLines(const wxString& path)
{
    bool found = true;

    while (found)
    {
        found = false;

        std::vector<RelationLine>::iterator line_it;
        for (line_it = m_lines.begin(); line_it != m_lines.end(); ++line_it)
        {
            if (path.CmpNoCase(line_it->left_path) == 0)
            {
                found = true;
                m_lines.erase(line_it);
                break;
            }

            if (path.CmpNoCase(line_it->right_path) == 0)
            {
                found = true;
                m_lines.erase(line_it);
                break;
            }
        }
    }
}


void RelationDiagram::clear()
{
    m_lines.clear();

    std::vector<RelationBox*> boxes = m_boxes;
    
    std::vector<RelationBox*>::iterator it;
    for (it = boxes.begin(); it != boxes.end(); ++it)
        (*it)->Destroy();

    m_boxes.clear();
}


void RelationDiagram::clearBoxSelection()
{
    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
        (*it)->clearSelection();
}

void RelationDiagram::clearLineSelection()
{
    std::vector<RelationLine>::iterator line_it;
    for (line_it = m_lines.begin(); line_it != m_lines.end(); ++line_it)
    {
        line_it->selected = false;
    }
    
    m_selected_line = NULL;
}

void RelationDiagram::getLines(const wxString& left_path, 
                          const wxString& right_path,
                          std::vector<RelationLine*>& lines)
{
    lines.clear();

    std::vector<RelationLine>::iterator line_it;
    for (line_it = m_lines.begin(); line_it != m_lines.end(); ++line_it)
    {
        bool b = true;

        if (!left_path.IsEmpty())
        {
            if (left_path.CmpNoCase(line_it->left_path) != 0)
                b = false;
        }

        if (!right_path.IsEmpty() && b)
        {
            if (right_path.CmpNoCase(line_it->right_path) != 0)
                b = false;
        }

        if (b)
        {
            lines.push_back(&(*line_it));
        }
    }
}
  
void RelationDiagram::getOrigLines(const wxString& left_path,
                              const wxString& right_path,
                              std::vector<RelationLine*>& lines)
{
    lines.clear();

    std::vector<RelationLine>::iterator line_it;
    for (line_it = m_orig_lines.begin();
         line_it != m_orig_lines.end(); ++line_it)
    {
        bool b = true;

        if (!left_path.IsEmpty())
        {
            if (left_path.CmpNoCase(line_it->left_path) != 0)
                b = false;
        }

        if (!right_path.IsEmpty() && b)
        {
            if (right_path.CmpNoCase(line_it->right_path) != 0)
                b = false;
        }

        if (b)
        {
            lines.push_back(&(*line_it));
        }
    }
}


 
void RelationDiagram::_getRelationInfo(std::vector<RelationLine>& lines,
                                       const wxString& left_path,
                                       std::vector<RelationInfo>& info)
{
    wxString tag;

    info.clear();
    std::vector<RelationLine>::iterator line_it;
    for (line_it = lines.begin(); line_it != lines.end(); ++line_it)
    {
        if (left_path.CmpNoCase(line_it->left_path) != 0)
            continue;

        wxString left_path = line_it->left_path;
        wxString right_path = line_it->right_path;
        
        left_path.MakeUpper();
        right_path.MakeUpper();
        
        RelationInfo* r = NULL;

        std::vector<RelationInfo>::iterator r_it;
        for (r_it = info.begin(); r_it != info.end(); ++r_it)
        {
            if (right_path.CmpNoCase(r_it->right_path) == 0)
            {
                r = &(*r_it);
                break;
            }
        }

        if (!r)
        {
            RelationInfo ri;
            ri.left_path = line_it->left_path;
            ri.right_path = line_it->right_path;
            ri.tag = tag;
            if (ri.tag.empty())
                ri.tag = line_it->right_path.AfterLast(L'/');
            info.push_back(ri);
            r = &(info.back());
        }

        if (r->tag.IsEmpty())
            r->tag = line_it->tag;

        r->lines.push_back(*line_it);
    }
}

void RelationDiagram::getRelationInfo(const wxString& left_path,
                                      std::vector<RelationInfo>& info)
{
    _getRelationInfo(m_lines, left_path, info);
}

void RelationDiagram::getOrigRelationInfo(const wxString& left_path,
                                          std::vector<RelationInfo>& info)
{
    _getRelationInfo(m_orig_lines, left_path, info);
}


void RelationDiagram::onBoxClosing(RelationBox* box, bool* allow)
{
    // if the box is included twice, close the box without
    // thinking twice about it
    wxString box_path = box->getSetPath();

    int box_count = 0;
    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
    {
        if (box_path.CmpNoCase((*it)->getSetPath()) == 0)
            box_count++;
    }

    if (box_count > 1)
    {
        *allow = true;
        return;
    }


    // if closing the box will delete some lines, ask
    // the user's permission

    std::vector<RelationLine*> lines1, lines2;

    getLines(box->getSetPath(), wxT(""), lines1);
    getLines(wxT(""), box->getSetPath(), lines2);

    *allow = true;

    if (lines1.size() == 0 && lines2.size() == 0)
        return;

    int res = appMessageBox(_("Removing this table from the diagram will remove all relationships associated with this table.  Would you like to continue?"),
                                 APPLICATION_NAME,
                                 wxYES_NO | wxICON_QUESTION | wxCENTER,
                                 g_app->getMainWindow());
    if (res == wxNO)
    {
        *allow = false;
        return;
    }

    deleteLines(box->getSetPath());
    sigDiagramUpdated();
}


static double distFromLine(int x1, int y1, int x2, int y2, int px1, int py1)
{
    // find the point on the line where the dot
    // product with the mouse click will be zero

    double pt_dist, x3, y3, dist_from_line;
    int t1, t2;

    t1 = ((px1-x1)*(x2-x1) + (py1-y1)*(y2-y1));
    t2 = ((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));

    if (t2 == 0)
        return 10000.0;

    pt_dist = double(t1)/double(t2);

    x3 = x1 + pt_dist*(x2-x1);
    y3 = y1 + pt_dist*(y2-y1);

    // calculate the distance between the
    // point above and the mouse click

    dist_from_line = sqrt((px1-x3)*(px1-x3) + (py1-y3)*(py1-y3));

    return dist_from_line;
}


#define HIT_DISTANCE 7

RelationLine* RelationDiagram::hitTest(int x, int y)
{
    std::vector<RelationLine>::iterator line_it;
    for (line_it = m_lines.begin(); line_it != m_lines.end(); ++line_it)
    {
        int x1 = std::min(line_it->x1, line_it->x2) - 5;
        int x2 = std::max(line_it->x1, line_it->x2) + 5;
        int y1 = std::min(line_it->y1, line_it->y2) - 5;
        int y2 = std::max(line_it->y1, line_it->y2) + 5;

        if (x < x1 || x > x2)
            continue;

        if (y < y1 || y > y2)
            continue;

        if (distFromLine(line_it->x1, line_it->y1,
                         line_it->x2, line_it->y2,
                         x, y) <= HIT_DISTANCE)
        {
            return &(*line_it);
        }
    }

    return NULL;
}


static bool rectInRect(int rx, int ry, int rwidth, int rheight,
                       int x, int y, int width, int height)
{
    if (x+width < rx)
        return false;
    if (y+height < ry)
        return false;
    if (x > rx+rwidth)
        return false;
    if (y > ry+rheight)
        return false;
    return true;
}



bool RelationDiagram::addBox(const wxString& path,
                             const wxString& caption,
                             const wxColor& color,
                             int x,
                             int y,
                             int width,
                             int height)
{
    // don't allow a table to be added twice (for now?)
    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
    {
        if ((*it)->getSetPath().CmpNoCase(path) == 0)
            return false;
    }

    if (width <= 0)
        width = FromDIP(BOX_DEFAULT_WIDTH);
    if (height <= 0)
        height = FromDIP(BOX_DEFAULT_HEIGHT);

    xd::Structure structure = g_app->getDatabase()->describeTable(towstr(path));
    if (structure.isNull())
        return false;

    if (x <= 0 && y <= 0)
    {
        // look for a free spot
        
        x = 10;
        y = 10;

        while (1)
        {
            bool free = true;

            std::vector<RelationBox*>::iterator it;
            for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
            {
                int bx, by, bwidth, bheight;

                (*it)->GetPosition(&bx, &by);
                (*it)->GetClientSize(&bwidth, &bheight);

                if (rectInRect(bx, by, bwidth, bheight,
                               x, y, width, height))
                {
                    x = bx+bwidth-1;
                    free = false;
                    break;
                }
            }

            if (free)
                break;

            x += 30;
            if (x > 600)
            {
                x = 10;
                y += 20;
            }

            if (y > 500)
                break;
        }
    }


    int vx, vy;
    GetViewStart(&vx, &vy);

    vx *= 10;
    vy *= 10;

    x -= vx;
    y -= vy;

    // when a relation box is created, it adds itself
    // to the diagram's vector of boxes
    RelationBox* box = new RelationBox(this, -1,
                                       caption,
                                       wxPoint(x,y),
                                       wxSize(width, height));

    box->setSetPath(path);
    box->setStructure(structure);

    refresh();

    box->fireSizedMovedSignal();

    return true;
}

void RelationDiagram::onEraseBackground(wxEraseEvent& evt)
{
}

void RelationDiagram::onKeyDown(wxKeyEvent& evt)
{
    int key_code = evt.GetKeyCode();
    
    if (key_code == WXK_DELETE ||
        key_code == WXK_NUMPAD_DELETE)
    {
        if (m_selected_line != NULL)
        {
            deleteLine(m_selected_line);

            // redraw all the boxes
            std::vector<RelationBox*>::iterator it;
            for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
                (*it)->repopulate();

            sigDiagramUpdated();

            clearLineSelection();
            refresh();
        }
    }
}

void RelationDiagram::onDragLeave()
{
    repaint(NULL, false);

    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
        (*it)->redraw();
}

void RelationDiagram::onDragOver(wxDragResult& drag_result)
{
    if (drag_result == wxDragNone)
        return;

    wxPoint pt = ::wxGetMousePosition();
    if (m_last_drag_pt == pt)
        return;

    m_last_drag_pt = pt;
}

void RelationDiagram::onTreeDataDropped(wxDragResult& drag_result,
                                        FsDataObject* data)
{
    IFsItemEnumPtr items = data->getFsItems();

    DbDoc* dbdoc = g_app->getDbDoc();

    // prevent folders from being dragged
    int i,count = items->size();
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        if (item.isNull())
            continue;

        IDbFolderFsItemPtr folder = item;
        if (folder.isOk())
        {
            refresh();
            appMessageBox(_("One or more of the items is a folder.  Folders cannot be added to the relationship diagram."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        if (dbdoc->isFsItemExternal(item))
        {
            refresh();
            appMessageBox(_("One or more of the items is an external table.  External tables cannot be added to the relationship diagram."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            return;
        }

        IDbObjectFsItemPtr obj = item;
        if (obj.isOk())
        {
            if (obj->getType() != dbobjtypeSet)
            {
                refresh();
                appMessageBox(_("One or more of the items is not a table.  Items that are not tables cannot be added to the relationship diagram."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);

                return;
            }
        }
    }

    int viewx = 0, viewy = 0;
    GetViewStart(&viewx, &viewy);
    
    viewx *= 10;
    viewy *= 10;

    wxPoint pt = ::wxGetMousePosition();
    pt = ScreenToClient(pt);

    std::vector<wxString> res;
    std::vector<wxString>::iterator it;
    DbDoc::getFsItemPaths(items, res, true);


    for (it = res.begin(); it != res.end(); ++it)
    {
        wxString path = *it;
        if (!g_app->getDatabase()->getFileExist(towstr(path)))
        {
            // some of the paths couldn't be found, bail out
            appMessageBox(_("One or more of the items could not be found in the project."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
    }
    
    bool existing_set_added = false;
    for (it = res.begin(); it != res.end(); ++it)
    {
        wxString path = *it;
        
        bool allow = true;
        sigSetAdded(path, &allow);
        if (!allow)
            continue;

        wxString caption = path.AfterLast(wxT('/'));
        if (addBox(path, caption, wxColor(0,0,128),
                   pt.x-DRAGDROP_X_OFFSET+viewx,
                   pt.y-DRAGDROP_Y_OFFSET+viewy))
        {
            pt.x += 40;
            pt.y += 60;
        }
         else
        {
            existing_set_added = true;
        }
    }

    refresh();
    
    if (existing_set_added)
    {
        appMessageBox(_("One or more of the tables was not added because it already exists in the relationship diagram."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
    }
}


// when a set is renamed, our internal structures
// must be updated to reflect these changes

void RelationDiagram::onSetRenamed(const wxString& old_path,
                                   const wxString& new_path)
{
    std::vector<RelationLine>::iterator line_it;
    for (line_it = m_lines.begin(); line_it != m_lines.end(); ++line_it)
    {
        if (line_it->left_path.CmpNoCase(old_path) == 0)
            line_it->left_path = new_path;

        if (line_it->right_path.CmpNoCase(old_path) == 0)
            line_it->right_path = new_path;
    }

    for (line_it = m_orig_lines.begin();
         line_it != m_orig_lines.end();
         ++line_it)
    {
        if (line_it->left_path.CmpNoCase(old_path) == 0)
            line_it->left_path = new_path;

        if (line_it->right_path.CmpNoCase(old_path) == 0)
            line_it->right_path = new_path;
    }

    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
    {
        if ((*it)->getSetPath().CmpNoCase(old_path) == 0)
        {
            wxString caption = new_path.AfterLast(wxT('/'));
            (*it)->setSetPath(new_path);
            (*it)->setCaption(caption);
            (*it)->Refresh(FALSE);
        }
    }

    refresh();
}

static bool findFieldInList(std::vector<wxString>& fields,
                            const wxString& name)
{
    std::vector<wxString>::iterator it;
    for (it = fields.begin(); it != fields.end(); ++it)
    {
        if (name.CmpNoCase(*it) == 0)
            return true;
    }
    
    return false;
}

void RelationDiagram::onSetStructureChanged(const wxString& set_path)
{
    RelationBox* box = getBoxByName(set_path);
    if (!box)
        return;
    
    xd::Structure structure = g_app->getDatabase()->describeTable(towstr(set_path));
    if (structure.isNull())
        return;
    
    // populate a list of fieldnames for use below
    std::vector<wxString> fieldnames;
    size_t i, count = structure.getColumnCount();
    for (i = 0; i < count; ++i)
    {
        fieldnames.push_back(structure.getColumnName(i));
    }
    
    // check to see if we need to delete any relationship lines
    
    std::vector<RelationLine*>::iterator it;
    std::vector<RelationLine*> lines, to_delete;
    getLines(wxEmptyString, wxEmptyString, lines);
    wxString line_expr;
    
    for (it = lines.begin(); it != lines.end(); ++it)
    {
        // find and remove any lines that were connected to any
        // fields that no longer exist in the table we've modified
        
        RelationLine* line = (*it);
        if (set_path.CmpNoCase(line->left_path) == 0)
        {
            line_expr = line->left_expr;
            if (!findFieldInList(fieldnames, line_expr))
                to_delete.push_back(*it);
        }
        
        if (set_path.CmpNoCase(line->right_path) == 0)
        {
            line_expr = line->right_expr;
            if (!findFieldInList(fieldnames, line_expr))
                to_delete.push_back(*it);
        }
    }
    
    // update the associated relationship box
    wxString caption = set_path.AfterLast(wxT('/'));
    box->setSetPath(set_path);
    box->setCaption(caption);
    box->setStructure(structure);
    box->repopulate();
    
    // refresh the diagram
    refresh();

    // make sure we save the changes we've made to the diagram
    save();
}




const int TAB_LEN = 14;

void RelationDiagram::drawConnectingLine(wxDC* dc,
                                         int x1,
                                         int y1,
                                         int x2,
                                         int y2,
                                         int direction,
                                         bool bold,
                                         bool valid,
                                         const wxBitmap& from_bmp,
                                         const wxBitmap& to_bmp,
                                         wxPoint* out_pt1,
                                         wxPoint* out_pt2)
{
    int startx, endx;

    wxPen* thin_pen;
    wxPen* thick_pen;

    if (valid)
    {
        thin_pen = &m_thin_black_pen;
        thick_pen = &m_thick_black_pen;
        dc->SetBrush(thick_pen->GetColour());
    }
     else
    {
        thin_pen = &m_thin_red_pen;
        thick_pen = &m_thick_red_pen;
        dc->SetBrush(thick_pen->GetColour());
    }

    // draw thick lines extending out from relationship boxes
    
    dc->SetPen(*thick_pen);

    if (direction & 0x02)
    {
        dc->DrawLine(x1, y1, x1+TAB_LEN, y1);
        if (from_bmp.Ok())
            dc->DrawBitmap(from_bmp, x1, y1-from_bmp.GetHeight(), true);
        startx = x1+TAB_LEN;
    }
     else
    {
        dc->DrawLine(x1, y1, x1-TAB_LEN, y1);
        if (from_bmp.Ok())
            dc->DrawBitmap(from_bmp, x1-TAB_LEN-2, y1-from_bmp.GetHeight(), true);
        startx = x1-TAB_LEN;
    }

    if (direction & 0x01)
    {
        dc->DrawLine(x2+TAB_LEN, y2, x2+2, y2);
        if (to_bmp.Ok())
            dc->DrawBitmap(to_bmp, x2, y2-to_bmp.GetHeight(), true);
        endx = x2+TAB_LEN;
    }
     else
    {
        dc->DrawLine(x2-TAB_LEN, y2, x2-2, y2);
        if (to_bmp.Ok())
            dc->DrawBitmap(to_bmp, x2-to_bmp.GetWidth(), y2 - to_bmp.GetHeight(), true);
        endx = x2-TAB_LEN;
    }

    // draw thin line connecting relationship boxes
    
    if (bold)
        dc->SetPen(*thick_pen);
         else
        dc->SetPen(*thin_pen);

    dc->DrawLine(startx, y1, endx, y2);

    if (out_pt1)
    {
        out_pt1->x = startx;
        out_pt1->y = y1;
    }

    if (out_pt2)
    {
        out_pt2->x = endx;
        out_pt2->y = y2;
    }

    // draw arrows
    
    dc->SetPen(*thin_pen);

    wxPoint pt[3];
    if (direction & 0x01)
    {
        pt[0].x = 0; pt[0].y = 0;
        pt[1].x = 3; pt[1].y = -3;
        pt[2].x = 3; pt[2].y = 3;
        dc->DrawPolygon(3, pt, x2, y2);
    }
     else
    {
        pt[0].x = -1; pt[0].y = 0;
        pt[1].x = -4; pt[1].y = -3;
        pt[2].x = -4; pt[2].y = 3;
        dc->DrawPolygon(3, pt, x2, y2);
    }
}


/*
static bool _rectInRect(wxRect* rect, int x, int y, int width, int height)
{
    if (x+width < 0 || y+height < 0)
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
*/

void RelationDiagram::refresh()
{
    if (!m_bmp.Ok())
        return;

    // win32 can redraw this stuff faster than other platforms

    //Refresh();
    //Update();
    repaint(NULL, true);
}


void RelationDiagram::refreshBoxes()
{
    // repopulate and redraw all the boxes
    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
        (*it)->repopulate();
}


void RelationDiagram::redrawBoxes()
{
    // repopulate and redraw all the boxes
    std::vector<RelationBox*>::iterator it;
    for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
        (*it)->redraw();
}


void RelationDiagram::repaint(wxDC* dest_dc, bool render, wxRect* rect)
{
    int cli_width, cli_height;
    bool tempdc = false;

    GetClientSize(&cli_width, &cli_height);

    wxDC* dc = &m_memdc;
    
    if (render)
    {
        if (rect)
        {
            dc->SetClippingRegion(rect->x, rect->y,
                                  rect->width, rect->height);
        }

        dc->SetBrush(GetBackgroundColour());
        dc->SetPen(*wxTRANSPARENT_PEN);

        // draw background
        dc->DrawRectangle(0, 0, cli_width, cli_height);


        std::vector<RelationBox*>::iterator box_it;
        std::vector<RelationBox*>::reverse_iterator box_rit;
        std::vector<RelationLine>::iterator line_it;
        for (line_it = m_lines.begin();
             line_it != m_lines.end();
             ++line_it)
        {
            wxCoord x1 = -1, y1 = -1, x2 = -1, y2 = -1;
            RelationBox* from_box = NULL;
            RelationBox* to_box = NULL;
        
            int from_box_x, from_box_width;
            int to_box_x, to_box_width;
            int tempi, dir_flags = 0;
            bool valid = line_it->valid;

            for (box_it = m_boxes.begin(); box_it != m_boxes.end(); ++box_it)
            {
                wxString table_path = (*box_it)->getSetPath();

                if (!from_box && isSamePath(towstr(table_path), towstr(line_it->left_path)))
                {
                    from_box = *box_it;
                    from_box->GetClientSize(&from_box_width, &tempi);
                    from_box->GetPosition(&from_box_x, &tempi);
                    x1 = 0;
                    y1 = from_box->getItemY(line_it->left_expr);
                }
            } 


            for (box_rit = m_boxes.rbegin();
                 box_rit != m_boxes.rend();
                 ++box_rit)
            {
                wxString table_path = (*box_rit)->getSetPath();

                if (!to_box && isSamePath(towstr(table_path), towstr(line_it->right_path)))
                {
                    to_box = *box_rit;
                    to_box->GetClientSize(&to_box_width, &tempi);
                    to_box->GetPosition(&to_box_x, &tempi);
                    x2 = 0;
                    y2 = to_box->getItemY(line_it->right_expr);
                }
            }

            if (!from_box || !to_box)
                continue;

            
            // determine validity of the line

            if (!from_box->getItemExists(line_it->left_expr))
                valid = false;

            if (!to_box->getItemExists(line_it->right_expr))
                valid = false;
            

            // find out the direction of the line

            if (from_box_x+from_box_width < to_box_x)
            {
                x1 = from_box_width;
                x2 = 0;
                dir_flags = 2;
            }
             else if (from_box_x < to_box_x+to_box_width)
            {
                x1 = from_box_width;
                x2 = to_box_width;
                dir_flags = 3;
            }
             else
            {
                x1 = 0;
                x2 = to_box_width;
                dir_flags = 1;
            }

            int box_x, box_y;

            from_box->GetPosition(&box_x, &box_y);
            x1 += box_x;
            y1 += box_y;
            
            to_box->GetPosition(&box_x, &box_y);
            x2 += box_x;
            y2 += box_y;

            wxPoint pt1, pt2;
            drawConnectingLine(dc, x1, y1, x2, y2,
                               dir_flags,
                               line_it->selected,
                               valid,
                               line_it->left_bitmap,
                               line_it->right_bitmap,
                               &pt1,
                               &pt2);

            line_it->x1 = pt1.x;
            line_it->y1 = pt1.y;
            line_it->x2 = pt2.x;
            line_it->y2 = pt2.y;
        }


        // draw overlay text
        int tx, ty;
        GetViewStart(&tx, &ty);

        dc->SetFont(*wxNORMAL_FONT);

        if (m_boxes.size() == 0)
        {
            wxFont saved_font = dc->GetFont();
            dc->SetFont(m_overlay_font);

            wxArrayString arr;
            wxStringTokenizer tkz(m_overlay_text, wxT("\n"));
            while (tkz.HasMoreTokens())
                arr.Add(tkz.GetNextToken());

            int text_x, text_y, line_height;
            m_memdc.GetTextExtent(wxT("ABCDXHM"), &text_x, &line_height);

            line_height += 5;
            int line_count = arr.Count();

            int yoff = (cli_height/2) - ((line_height * line_count)/2);

            m_memdc.SetTextForeground(*wxBLACK);

            for (int i = 0; i < line_count; ++i)
            {
                dc->GetTextExtent(arr.Item(i), &text_x, &text_y);
                dc->DrawText(arr.Item(i), (cli_width-text_x)/2 + tx, yoff + ty);
                yoff += line_height;
            }
            
            // restore the saved font
            dc->SetFont(saved_font);
        }
    }


    if (dest_dc)
    {
        dest_dc->Blit(0, 0, cli_width, cli_height, dc, 0, 0);
    }
     else
    {
        wxClientDC dest_dc(this);
        dest_dc.Blit(0, 0, cli_width, cli_height, dc, 0, 0);
    }
}



void RelationDiagram::onPaint(wxPaintEvent& evt)
{    
    wxPaintDC dc(this);

    if (m_first_time)
    {
        repaint(&dc, true);
        m_first_time = false;
    }
     else
    {
        repaint(&dc, false);
    }
}



void RelationDiagram::onSize(wxSizeEvent& evt)
{
    int cli_width, cli_height;
    bool tempdc = false;

    GetClientSize(&cli_width, &cli_height);
    if (cli_width < 10)
        cli_width = 0;
    if (cli_height < 10)
        cli_height = 10;
 
#ifdef __WXGTK__

    // this needs to be investigated more thoroughly;
    // for some reason the code below with the if
    // statement is causing problems on gtk when the
    // window is reduced in size
    
    m_memdc.SelectObject(wxNullBitmap);
    m_bmp.Create(cli_width+1, cli_height+1, -1);
    m_memdc.SelectObject(m_bmp);
#else
    if (!m_bmp.Ok() ||
        cli_width > m_bmp.GetWidth() ||
        cli_height > m_bmp.GetHeight())
    {
        wxClientDC dc(this);

        m_memdc.SelectObject(wxNullBitmap);
        m_bmp.Create(cli_width+1, cli_height+1, dc);
        m_memdc.SelectObject(m_bmp);
    }
#endif

    repaint();
}

void RelationDiagram::onMouse(wxMouseEvent& evt)
{
    if (evt.GetEventType() == wxEVT_LEFT_DOWN)
    {
        // it is necessary for the RelationDiagram to have
        // the focus so keyboard event can be processed
        SetFocusIgnoringChildren();
        
        RelationLine* l = hitTest(evt.m_x, evt.m_y);
        
        if (l)
        {
            bool selected = !l->selected;
            clearLineSelection();
            l->selected = selected;
            if (selected)
                m_selected_line = l;
            refresh();
        }
         else
        {
            clearLineSelection();
            refresh();
        }
    }
     else if (evt.GetEventType() == wxEVT_RIGHT_DOWN)
    {
        // it is necessary for the RelationDiagram to have
        // the focus so keyboard event can be processed
        SetFocusIgnoringChildren();
        
        RelationLine* line;
        line = hitTest(evt.m_x, evt.m_y);

        if (line == NULL)
            return;
        
        clearLineSelection();
        line->selected = true;
        refresh();

        m_selected_line = line;
        
        int command = 0;
        
        // if the signal is active, then this event
        // is being handled elsewhere
        sigLineRightClicked(line, &command);
        if (!sigLineRightClicked.isActive())
        {
            wxMenu menuPopup;
            menuPopup.Append(MenuID_DeleteLine, _("Delete Relation"));
            menuPopup.AppendSeparator();
            menuPopup.Append(MenuID_Cancel, _("Cancel"));

            wxPoint pt_mouse = ::wxGetMousePosition();
            pt_mouse = ScreenToClient(pt_mouse);
            CommandCapture* cc = new CommandCapture;
            PushEventHandler(cc);
            PopupMenu(&menuPopup, pt_mouse);
            command = cc->getLastCommandId();
            PopEventHandler(true);
        }

        if (command == MenuID_DeleteLine)
        {
            deleteLine(line);

            // redraw all the boxes

            std::vector<RelationBox*>::iterator it;
            for (it = m_boxes.begin(); it != m_boxes.end(); ++it)
                (*it)->repopulate();

            sigDiagramUpdated();
        }

        clearLineSelection();
        refresh();
    }
     else if (evt.GetEventType() == wxEVT_LEFT_DCLICK)
    {
        // it is necessary for the RelationDiagram to have
        // the focus so keyboard event can be processed
        SetFocusIgnoringChildren();
        
        RelationLine* line;
        line = hitTest(evt.m_x, evt.m_y);

        if (line == NULL)
        {
            showAddTableDialog(this);
            return;
        }

        sigLineActivated(line);
    }
}

void RelationDiagram::resetModified()
{
    m_orig_lines = m_lines;
}




