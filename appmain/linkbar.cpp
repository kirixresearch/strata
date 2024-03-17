/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-05-26
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "bookmarkfs.h"
#include "linkbar.h"
#include "toolbars.h"
#include "dbdoc.h"
#include "dlglinkprops.h"
#include "webdoc.h"
#include <artprovider.h>   // angle brackets to allow overriding
#include <wx/popupwin.h>
#include <wx/stopwatch.h>
#include <wx/dcbuffer.h>


const int LinkBarDropDownId = 6524;


// ripped directly from CfwTabArt
static void drawInactiveTabBackground(wxDC& dc,
                                      const wxRect& rect,
                                      const wxColor& base_color,
                                      const wxPen& border_pen1,
                                      const wxPen& border_pen2)
{
    // border_pen1 is the "normal" border drawing pen (something like
    // a dark gray), and border_pen2 specifies a light off-white which
    // makes the border between inactive tabs only a partial border

    wxRect rect_shine = rect;
    rect_shine.height /= 2;
    rect_shine.height += 2;
            
    // draw the base background color
    dc.SetPen(base_color);
    dc.SetBrush(base_color);
    dc.DrawRectangle(rect);

    // draw the "shine" for the tab
    wxColor c1 = kcl::stepColor(base_color, 125);
    wxColor c2 = kcl::stepColor(base_color, 160);
    dc.GradientFillLinear(rect_shine, c1, c2, wxNORTH);

    // draw left and right borders
    dc.SetPen(border_pen2);
    dc.DrawLine(rect.x, rect.y,
                rect.x, rect.y+rect.height);
    dc.SetPen(border_pen1);
    dc.DrawLine(rect.x, rect.y+2,
                rect.x, rect.y+rect.height-2);
    dc.DrawLine(rect.x+rect.width, rect.y,
                rect.x+rect.width, rect.y+rect.height);

    // draw white chisel outline around side and bottom borders
    dc.SetPen(kcl::stepColor(base_color, 160));
    dc.DrawLine(rect.x+1, rect.y,
                rect.x+1, rect.y+rect.height-1);
    dc.DrawLine(rect.x+rect.width-1, rect.y,
                rect.x+rect.width-1, rect.y+rect.height-1);
    dc.DrawLine(rect.x+1, rect.y+rect.height-1,
                rect.x+rect.width, rect.y+rect.height-1);
}


static wxBitmap rescaleBitmapTo(wxBitmap bitmap, int to_size)
{
    int x = bitmap.GetWidth();
    int y = bitmap.GetHeight();

    if (x != to_size || y != to_size)
    {
        wxImage image = bitmap.ConvertToImage();
        image.Rescale(to_size, to_size);
        return wxBitmap(image);
    }
    else
    {
        return bitmap;
    }
}





#ifdef __WXMSW__
#if !wxCHECK_VERSION(2,9,0)
extern const wxChar* wxCanvasClassName;
#endif
extern "C" WXDLLEXPORT HINSTANCE wxGetInstance();
LRESULT WXDLLEXPORT APIENTRY wxWndProc(HWND, UINT, WPARAM, LPARAM);
#endif


std::wstring xf_get_win32_error_msg();


#if wxCHECK_VERSION(2,9,0) && defined(__WXMSW__)
class DropShadowPopupTransientWindow : public wxPopupTransientWindow
{

public:

#if wxCHECK_VERSION(3,1,1)
    DropShadowPopupTransientWindow() : wxPopupTransientWindow()
    {
    }

    DropShadowPopupTransientWindow(wxWindow *parent, int style = wxBORDER_NONE) : wxPopupTransientWindow(parent, style)
    {
    }
#else
    DropShadowPopupTransientWindow()
    {
        Init();
    }

    DropShadowPopupTransientWindow(wxWindow *parent, int style = wxBORDER_NONE)
    {
        Init();
        (void)Create(parent, style);
    }
#endif



    bool Create(wxWindow *parent, int flags)
    {
        // popup windows are created hidden by default
        Hide();

        return wxPopupWindowBase::Create(parent) &&
                   CreateWithDropShadow(parent, wxID_ANY,
                                    wxDefaultPosition, wxDefaultSize,
                                    flags | wxPOPUP_WINDOW);
    }

    bool CreateWithDropShadow(wxWindow *parent,
                             wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style,
                             const wxString& name = wxPanelNameStr)
    {
        if (!CreateBase(parent, id, pos, size, style, wxDefaultValidator, name))
            return false;

        parent->AddChild(this);

        WXDWORD exstyle;
        DWORD msflags = MSWGetCreateWindowFlags(&exstyle);

        if (IsShown())
            msflags |= WS_VISIBLE;

        if (!MSWCreate(wxApp::GetRegisteredClassName(wxT("wxKxDrop"), COLOR_BTNFACE, 0x00020000 /* CS_DROPSHADOW */),
                        NULL, pos, size, msflags, exstyle))
        {
            return false;
        }

        InheritAttributes();

        return true;
    }
};



#define PopupTransientBase DropShadowPopupTransientWindow

#else

#define PopupTransientBase wxPopupTransientWindow

#endif


class LinkBarPopupWindow : public PopupTransientBase
{
public:

    LinkBarPopupWindow(LinkBar* parent, int flags) : PopupTransientBase()             
    {
        Create(parent, flags);

        m_linkbar = parent;
        m_popup_id = -1;
        
        LinkBarItem* item = m_linkbar->getItemFromCurrentPosition();
        if (item)
            m_popup_id = item->GetId();
    }
    
    void OnDismiss()
    {
        // if we've clicked the same item we clicked to open up
        // this popup window, don't fire the sigDismiss() signal;
        // instead, the destruction of the window should be
        // handled by the LinkBar -- this is necessary, otherwise,
        // the LinkBar will never know when a popup window actually
        // existed or not because this window destroys itself before
        // the LinkBar can check to see whether or not it existed
        LinkBarItem* item = m_linkbar->getItemFromCurrentPosition();
        if (item && item->GetId() == m_popup_id && m_popup_id != -1)
            return;
        
        sigDismiss();
    }
    
    void OnEraseBackground(wxEraseEvent& evt)
    {
    }
    
    void OnPaint(wxPaintEvent& evt)
    {
        int cli_w,cli_h;
        GetClientSize(&cli_w,&cli_h);
        
        wxBufferedPaintDC dc(this);
        
        // draw a border around the popup window
        dc.SetPen(kcl::getBorderPen());
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(0,0,cli_w,cli_h);
    }

public:

    xcm::signal0 sigDismiss;

private:
    
    wxBitmap m_screen_bmp;
    LinkBar* m_linkbar;
    int m_popup_id;
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(LinkBarPopupWindow, wxPopupTransientWindow)
    EVT_PAINT(LinkBarPopupWindow::OnPaint)
    EVT_ERASE_BACKGROUND(LinkBarPopupWindow::OnEraseBackground)
END_EVENT_TABLE()




// utility functions

static std::vector<IFsItemPtr> enum2vec(IFsItemEnumPtr fs_enum)
{
    std::vector<IFsItemPtr> vec;
    
    size_t i, count = fs_enum->size();
    for (i = 0; i < count; ++i)
        vec.push_back(fs_enum->getItem(i));

    return vec;
}


static void activateItem(const std::wstring& path, int open_mask)
{
    Bookmark b;
    if (!g_app->getBookmarkFs()->loadBookmark(path, b))
        return;

    if (b.run_target)
    {
        xd::IDatabasePtr db = g_app->getDatabase();
        if (db.isOk())
        {
            xd::IFileInfoPtr finfo = db->getFileInfo(b.location);
            if (finfo.isOk())
            {
                if (finfo->getType() == xd::filetypeNode || finfo->getType() == xd::filetypeStream)
                {
                    g_app->getAppController()->execute(b.location);
                    return;
                }
            }
        }
    }

    g_app->getAppController()->openAny(b.location, open_mask);
}

static void openItems(std::vector<IFsItemPtr>& vec)
{
    bool first = true;
    
    std::vector<IFsItemPtr>::iterator it;
    for (it = vec.begin(); it != vec.end(); ++it)
    {
        if ((*it)->isFolder())
            continue;

        std::wstring path = g_app->getBookmarkFs()->getBookmarkItemPath(*it);
        if (path.empty())
            continue;

        first = false;
        int open_mask = appOpenForceNewWindow;
        if (first)
            open_mask |= appOpenActivateNewWindow;

        activateItem(path, open_mask);
    }
}


enum RightClickPopupMenuIds
{
    ID_LinkBar_OpenAll = wxID_HIGHEST + 1,
    ID_LinkBar_OpenAllFolder,
    ID_LinkBar_Open,
    ID_LinkBar_OpenTab,
    ID_LinkBar_NewBookmark,
    ID_LinkBar_NewFolder,
    ID_LinkBar_Properties,
    ID_LinkBar_Rename,
    ID_LinkBar_Delete
};


BEGIN_EVENT_TABLE(LinkBar, wxAuiToolBar)
    EVT_TIMER(6901, LinkBar::onPopupTimer)
    EVT_MENU(wxID_ANY, LinkBar::onToolButtonClick)
    EVT_UPDATE_UI(ID_App_ToggleRelationshipSync, LinkBar::onUpdateUI_RelationshipSync)       
    EVT_AUITOOLBAR_TOOL_DROPDOWN(wxID_ANY, LinkBar::onToolDropDownClick)
    EVT_AUITOOLBAR_RIGHT_CLICK(wxID_ANY, LinkBar::onRightClick)
    EVT_AUITOOLBAR_MIDDLE_CLICK(wxID_ANY, LinkBar::onMiddleClick)
    EVT_AUITOOLBAR_OVERFLOW_CLICK(wxID_ANY, LinkBar::onOverflowClick)
    EVT_AUITOOLBAR_BEGIN_DRAG(wxID_ANY, LinkBar::onBeginDrag)
    EVT_LEAVE_WINDOW(LinkBar::OnLeaveWindow)
    EVT_SIZE(LinkBar::OnSize)
END_EVENT_TABLE()


LinkBar::LinkBar(wxWindow* parent,
                 wxWindowID id,
                 const wxPoint& pos,
                 const wxSize& size)
                 : wxAuiToolBar(parent, ID_Toolbar_Link, pos, size,
                                wxAUI_TB_DEFAULT_STYLE |
                                wxAUI_TB_NO_AUTORESIZE |
                                wxAUI_TB_TEXT | 
                                wxNO_BORDER)
{
    // initialize variables
    m_exclude_id_begin = -1;
    m_exclude_id_end = -1;
    m_drop_idx = -1;
    m_drag_id = -1;
    m_last_id = -1;
    m_hover_stopwatch.Pause();

    m_max_popup_windows_size = FromDIP(wxSize(320, 480));
    m_min_popup_windows_size = FromDIP(wxSize(140, 20));

    // set up the linkbar as a drop target
    FsDataDropTarget* drop_target = new FsDataDropTarget;
    drop_target->sigDragLeave.connect(this, &LinkBar::onFsDataLeave);
    drop_target->sigDragOver.connect(this, &LinkBar::onFsDataDragOver);
    drop_target->sigDragDrop.connect(this, &LinkBar::onFsDataDrop);
    SetDropTarget(drop_target);

    // don't allow dragging over these items
    m_exclude_id_begin = -1;
    m_exclude_id_end = ID_App_ToggleRelationshipSync;

    m_popup_window = NULL;
    m_popup_fspanel.clear();
    m_popup_id = 0;
    m_popup_timer.SetOwner(this, 6901);
    m_popup_during_drag = false;
    
    SetArtProvider(new LinkBarArt);
    SetToolTextOrientation(wxAUI_TBTOOL_TEXT_RIGHT);
    SetToolPacking(0);
    SetToolBorderPadding(FromDIP(2));
    
    // top padding needs to be one greater since the top pixel
    // of the linkbar is actually the border line; left padding
    // is 5 pixels to match the left padding of the standard toolbar
    SetMargins(FromDIP(5), FromDIP(2) , FromDIP(3), FromDIP(2));
        
    SetGripperVisible(false);
    SetOverflowVisible(false);
    
    refresh();
}

LinkBarItem* LinkBar::getItemFromCurrentPosition(bool ignore_ypos)
{
    int x,y;
    ::wxGetMousePosition(&x,&y);
    ScreenToClient(&x,&y);

    // ignore vertical padding of items (always try and find
    // the item regardless of the mouse y-coordinate)
    if (ignore_ypos)
        y = GetClientSize().GetHeight()/2;
    
    // find the tool in the toolbar that we're hovering over
    LinkBarItem* item = FindToolByPosition(x,y);
    return item;
}

void LinkBar::onItemActivated(IFsItemPtr item)
{
    if (!m_popup_window)
        return;
    
    if (item->isFolder())
    {
        m_popup_window->Freeze();
        
        if (m_popup_fspanel->isItemExpanded(item))
            m_popup_fspanel->collapse(item);
             else
            m_popup_fspanel->expand(item);

        // reposition and resize the popup window
        // as best we can to the size of its contents
        recalcPopupWindowSize();
        repositionPopupWindow();
        g_app->getMainWindow()->Refresh();

        m_popup_window->Thaw();
        return;
    }
    
    // set the hover item highlight and no pressed item highlight
    LinkBarItem* hover_item = getItemFromCurrentPosition();
    SetHoverItem(hover_item);
    SetPressedItem(NULL);
    
    // hide the popup window while the clicked item is 'loading'.
    // Once the popup window is hidden, we need to call ::wxYield()
    // to get the windows to repaint in the background.  Unforuntately,
    // this would result in onPopupDestructing being called, so we
    // prevent that by setting m_popup_window to NULL.  At the end of
    // this method, restore the values and call
    // closePopupWindow/onPopupDestructing

    m_popup_window->Show(false);
    LinkBarPopupWindow* popup_window = m_popup_window;
    IFsPanelPtr popup_fspanel = m_popup_fspanel;
    m_popup_window = NULL;
    m_popup_fspanel.clear();
    ::wxYield();
    
    int open_flags = appOpenDefault;

    if (::wxGetMouseState().ControlDown())
    {
        // ctrl-click is the same as a middle-click
        open_flags = appOpenForceNewWindow | appOpenActivateNewWindow;
    }

    std::wstring bookmark_path = g_app->getBookmarkFs()->getBookmarkItemPath(item);
    m_last_clicked_path = bookmark_path;
    activateItem(bookmark_path, open_flags);

    m_popup_window = popup_window;
    m_popup_fspanel = popup_fspanel;
    closePopupWindow();
}

void LinkBar::onItemMiddleClicked(IFsItemPtr item)
{
    if (!m_popup_window)
        return;
    
    // set the hover item highlight and no pressed item highlight
    LinkBarItem* hover_item = getItemFromCurrentPosition();
    SetHoverItem(hover_item);
    SetPressedItem(NULL);
    
    // hide the popup window while the clicked item is 'loading'.
    // Once the popup window is hidden, we need to call ::wxYield()
    // to get the windows to repaint in the background.  Unforuntately,
    // this would result in onPopupDestructing being called, so we
    // prevent that by setting m_popup_window to NULL.  At the end of
    // this method, restore the values and call
    // closePopupWindow/onPopupDestructing

    m_popup_window->Show(false);
    LinkBarPopupWindow* popup_window = m_popup_window;
    IFsPanelPtr popup_fspanel = m_popup_fspanel;
    m_popup_window = NULL;
    m_popup_fspanel.clear();
    ::wxYield();
    
    /*
    IDbFolderFsItemPtr folder = item;
    if (folder)
    {
        // middle-click on a folder opens all link items in the folder
        std::vector<IFsItemPtr> vec = enum2vec(item->getChildren());
        openItems(vec);
    }
     else
    {
        // middle-click opens web links in new tabs
        DbDoc::actionActivate(item, appOpenForceNewWindow |
                                    appOpenActivateNewWindow);
    }
    */

    m_popup_window = popup_window;
    m_popup_fspanel = popup_fspanel;
    closePopupWindow();
}

bool LinkBar::isFolderItem(int id)
{
    if (id < ID_FirstLinkBarId)
        return false;
    
    int item_idx = id-ID_FirstLinkBarId;
    if ((size_t)item_idx >= m_items.size())
        return false;
    
    IFsItemPtr item = m_items[item_idx];

    return item->isFolder();
}


IFsItemPtr LinkBar::getItemFromId(int id)
{
    if (id < ID_FirstLinkBarId)
        return xcm::null;
    
    int item_idx = id-ID_FirstLinkBarId;
    if ((size_t)item_idx >= m_items.size())
        return xcm::null;
    
    return m_items[item_idx];
}


bool LinkBar::isPopupWindowOpen()
{
    return (m_popup_window != NULL ? true : false);
}

void LinkBar::onPopupDestructing()
{
    if (!m_popup_window)
        return;
    
    // make sure the drop down button is refreshed
    RefreshOverflowState();
    
    // set the hover item highlight and no pressed item highlight
    LinkBarItem* hover_item = getItemFromCurrentPosition();
    SetHoverItem(hover_item);
    SetPressedItem(NULL);
    
    m_popup_window->Show(false);
    if (!wxPendingDelete.Member(m_popup_window))
        wxPendingDelete.Append(m_popup_window);
    m_popup_window = NULL;
    
    m_popup_fspanel.clear();
    
    m_popup_timer.Stop();
    m_popup_during_drag = false;
    m_popup_id = 0;
}

void LinkBar::closePopupWindow()
{
    onPopupDestructing();
}
    
void LinkBar::showPopupWindow(int id,
                              bool popup_mode,
                              xd::IFileInfoEnumPtr items)
{
    if (m_popup_id == id)
    {
        // correct popup already shown
        return;
    }
    
    // close any existing window
    if (isPopupWindowOpen())
    {
        bool popup_during_drag = m_popup_during_drag;
        closePopupWindow();
        m_popup_during_drag = popup_during_drag;
    }
    
    IFsItemPtr folder;
    
    // show the linkbar "overflow" popup
    if (id == LinkBarDropDownId)
    {
    /*
        if (items.isNull())
            return;
        
        // assign the children to the folder manually
        DbFolderFsItem* folder_raw = new DbFolderFsItem;
        folder_raw->setPath(m_base_path);
        folder_raw->setChildrenOverride(items);
        folder_raw->setDatabase(g_app->getDatabase());
        folder = static_cast<IFsItem*>(folder_raw);
    */
        return;

    }
     else
    {
        int item_idx = id-ID_FirstLinkBarId;
        if ((size_t)item_idx >= m_items.size())
            return;
        
        // get the folder item from vector of IFsItems
        folder = m_items[item_idx];
    }
    
    m_popup_window = new LinkBarPopupWindow(this, wxBORDER_NONE);
    m_popup_fspanel = createFsPanelObject();
    m_popup_fspanel->setStyle(fsstyleTreeHideRoot | fsstyleTrackSelect);

    if (!m_popup_fspanel->create(m_popup_window, -1, wxPoint(1,1), m_max_popup_windows_size, 0))
    {
        wxFAIL_MSG(wxT("Could not create FsPanel window"));
        return;
    }


    m_popup_fspanel->setRootItem(folder);
    m_popup_fspanel->setView(fsviewTree);
    m_popup_fspanel->refresh();


    // resize the popup window as best we can to the size of its contents
    recalcPopupWindowSize();
    
    m_popup_fspanel->refresh();
    m_popup_fspanel->setDragDrop(true);


    m_popup_fspanel->sigItemSelected().disconnect();
    m_popup_fspanel->sigItemActivated().disconnect();
    m_popup_fspanel->sigItemSelected().connect(this, &LinkBar::onItemActivated);
    m_popup_fspanel->sigItemMiddleClicked().connect(this, &LinkBar::onItemMiddleClicked);
    m_popup_fspanel->sigMouseMove().disconnect();
    m_popup_fspanel->sigMouseMove().connect(this, &LinkBar::onPopupMouseMove);
    m_popup_fspanel->sigDragDrop().connect(this, &LinkBar::onFsDataFolderDrop);


    m_popup_window->sigDismiss.connect(this, &LinkBar::onPopupDestructing);
    m_popup_id = id;
    
    // reposition the popup window based on
    // its size and which tool was clicked
    repositionPopupWindow();
    
    if (popup_mode)
        m_popup_window->Popup();
         else
        m_popup_window->Show();
    
    m_popup_timer.Start(10, wxTIMER_CONTINUOUS);
}

void LinkBar::recalcPopupWindowSize()
{
    int max_height = m_max_popup_windows_size.GetHeight();
    int max_width  = m_max_popup_windows_size.GetWidth();
    int min_height = m_min_popup_windows_size.GetHeight();
    int min_width  = m_min_popup_windows_size.GetWidth();
    
    int new_width, new_height;
    m_popup_fspanel->getVirtualSize(&new_width, &new_height);
    
    if (new_height < min_height)
        new_height = min_height;
    if (new_height > max_height)
        new_height = max_height;
    if (new_width < min_width)
        new_width = min_width;
    if (new_width > max_width)
        new_width = max_width;
    
    new_height += wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y);
    new_width  += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
    
    IDocumentPtr doc = m_popup_fspanel;
    wxWindow* doc_wnd = doc->getDocumentWindow();
    wxSize s = doc_wnd->GetSize();
    
    // no change in height or width, we're done
    if (s.GetHeight() == new_height && s.GetWidth() == new_width)
        return;
    
    s.SetHeight(new_height);
    s.SetWidth(new_width);
    doc_wnd->SetClientSize(s);
    
    wxWindow* cont_wnd = doc_wnd->GetParent();
    cont_wnd->SetClientSize(s+cont_wnd->FromDIP(wxSize(2,2)));     // extra pixels for border
}

void LinkBar::repositionPopupWindow()
{
    wxPoint pt;
    if (m_popup_id == LinkBarDropDownId)
    {
        int x,y;
        GetClientSize(&x,&y);
        pt = this->ClientToScreen(wxPoint(x,y));
    }
     else
    {
        wxRect rect = GetToolRect(m_popup_id);
        pt = this->ClientToScreen(rect.GetBottomLeft());
    }
    
    wxSize sz = m_popup_window->GetSize();
    
    
    // make sure the popup doesn't go offscreen
    if (m_popup_id == LinkBarDropDownId)
    {
        // "overflow" popups are positioned from the
        // right side of the client area (see above)
        pt.x -= sz.x;
    }
     else
    {
        int screen_width = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
        if (pt.x+sz.x > screen_width)
        {
            wxRect tool_rect = GetToolRect(m_popup_id);
            pt.x += tool_rect.width;
            pt.x -= sz.x;
        }
    }
       
    m_popup_window->SetSize(pt.x, pt.y, sz.x, sz.y);
    
}

void LinkBar::onPopupMouseMove(wxCoord x, wxCoord y)
{
}

void LinkBar::onPopupTimer(wxTimerEvent& evt)
{    
    if (!isPopupWindowOpen())
        return;
    
    // drag is finished
    if (m_popup_during_drag && !::wxGetMouseState().LeftIsDown())
    {
        closePopupWindow();
        return;
    }
    
    // if the popup window is the linkbar "overflow" dropdown,
    // don't allow hover popup window switching
    if (m_popup_id == LinkBarDropDownId)
        return;
    
    LinkBarItem* item = getItemFromCurrentPosition();
    if (!item)
        return;
    
    int id = item->GetId();
    if (id >= ID_FirstLinkBarId && (size_t)id < ID_FirstLinkBarId+m_items.size())
    {
        // set the hover item highlight and no pressed item highlight
        SetHoverItem(item);
        SetPressedItem(NULL);
        
        // only do hover popup if we're hovering over a different
        // folder item from the one that is currently shown
        if (isFolderItem(item->GetId()) && item->GetId() != m_popup_id)
        {
            // we're over another folder, destroy existing popup
            // and open the new one instead
            if (m_popup_during_drag)
            {
                // show in non-popup mode, because we are dragging
                showPopupWindow(item->GetId(), false);
            }
             else
            {
                showPopupWindow(item->GetId(), true);
            }
            return;
        }
    }
}


void LinkBar::onBeginDrag(wxAuiToolBarEvent& evt)
{
    int id = evt.GetToolId();
    
    // ignore invalid (and separator) tool indexes
    if (id == -1)
    {
        evt.Skip();
        return;
    }
        
    if (id >= ID_FirstLinkBarId && (size_t)id < ID_FirstLinkBarId+m_items.size())
    {
        int item_idx = id-ID_FirstLinkBarId;
        IFsItemPtr item = m_items[item_idx];

        // only allow folder drags if the control key
        // is down -- also see onToolDropDownClick()
        if (item->isFolder() && !::wxGetMouseState().ControlDown())
        {
            evt.Skip();
            return;
        }
        
        // reset the hover stopwatch
        m_hover_stopwatch.Start();
        m_hover_stopwatch.Pause();

        // set the drag id
        m_drag_id = id;
        
        // remove hover and pressed item highlights if they exist
        SetHoverItem(NULL);
        SetPressedItem(NULL);

        int x,y;
        ::wxGetMousePosition(&x,&y);
        ScreenToClient(&x,&y);

        // get the initial insert index and draw the drop highlight
        // (this must be done before we call DoDragDrop() because that
        // function steals the main thread until the drop happens)
        m_drop_idx = getInsertIndex(wxPoint(x,y), true);
        drawDropHighlight();
        
        item->setItemData((long)id);
        xcm::IVectorImpl<IFsItemPtr>* items = new xcm::IVectorImpl<IFsItemPtr>;
        items->append(item);
        
        FsDataObject data;
        data.setSourceId(ID_Toolbar_Link);
        data.setFsItems(items);
        wxDropSource dragSource(data, this);
        wxDragResult result = dragSource.DoDragDrop(wxDrag_DefaultMove);
    }
}

void LinkBar::onToolButtonClick(wxCommandEvent& evt)
{
    int id = evt.GetId();
    if (id >= ID_FirstLinkBarId && (size_t)id < ID_FirstLinkBarId+m_items.size())
    {
        DbDoc* dbdoc = g_app->getDbDoc();
        if (!dbdoc)
        {
            evt.Skip();
            return;
        }
        
        int item_idx = id-ID_FirstLinkBarId;
        IFsItemPtr item = m_items[item_idx];

        // folder clicks are handled in onToolDropDownClick()
        if (item->isFolder())
        {
            evt.Skip();
            return;
        }
        
        int open_mask = appOpenDefault;
        if (::wxGetMouseState().ControlDown())
        {
            // ctrl-click is the same as a middle-click
            open_mask = appOpenForceNewWindow | appOpenActivateNewWindow;
        }

        std::wstring bookmark_path = g_app->getBookmarkFs()->getBookmarkItemPath(item);
        m_last_clicked_path = bookmark_path;

        activateItem(bookmark_path, open_mask);
    }
    
    evt.Skip();
}

void LinkBar::onToolDropDownClick(wxAuiToolBarEvent& evt)
{
    int id = evt.GetId();
    if (id >= ID_FirstLinkBarId && (size_t)id < ID_FirstLinkBarId+m_items.size())
    {
        DbDoc* dbdoc = g_app->getDbDoc();
        if (!dbdoc)
        {
            evt.Skip();
            return;
        }
        
        // we've left-clicked on the tool item that is already
        // showing the open popup window, so close the popup window
        if (isPopupWindowOpen() && id == m_popup_id)
        {
            closePopupWindow();
            evt.Skip();
            return;
        }

        // non-folder clicks are handled in onToolButtonClick()
        if (!isFolderItem(id))
        {
            evt.Skip();
            return;
        }

        // if we clicked on a folder and the control key is down
        // we're doing a drag operation -- also see onBeginDrag()
        if (isFolderItem(id) && ::wxGetMouseState().ControlDown())
        {
            evt.Skip();
            return;
        }
            
        showPopupWindow(id, true);
    }
     else
    {
        evt.Skip();
    }
}

void LinkBar::onRightClick(wxAuiToolBarEvent& evt)
{
    wxPoint pt = evt.GetClickPoint();
    
    // we right-clicked to the left of the link area,
    // don't show a right-click menu
    if (pointInExclude(pt))
    {
        evt.Skip();
        return;
    }

    // sticky the item we've right-clicked
    int tool_id = evt.GetToolId();
    if (tool_id != -1)
        SetToolSticky(tool_id, true);
    
    // determine if the item is a folder
    bool is_folder_clicked = false;
    if (tool_id != -1)
        is_folder_clicked = isFolderItem(tool_id);
    
    // convert the tool index to a link index
    int idx = GetToolIndex(tool_id);
    if (idx == -1)
        idx = getInsertIndex(pt);
    tool2LinkIndex(idx);
    
    // get the IFsItemPtr from the index
    IFsItemPtr item;
    if (idx >= 0 && idx < (int)m_items.size())
        item = m_items[idx];

    // create and populate the popup menu
    
    wxMenu menuPopup;
    if (item.isNull())
    {
        menuPopup.Append(ID_LinkBar_OpenAll, _("&Open All in Tabs"));
    }
     else
    {
        if (is_folder_clicked)
        {
            menuPopup.Append(ID_LinkBar_OpenAllFolder, _("&Open All in Tabs"));
        }
         else
        {
            // we'll make 'open' do something different later on --
            // for now, 'open' is the same as 'open in new tab'
            menuPopup.Append(ID_LinkBar_Open, _("&Open"));
            menuPopup.Append(ID_LinkBar_OpenTab, _("&Open in New Tab"));
        }
           
    }
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_LinkBar_NewBookmark, _("New &Bookmark..."));
    menuPopup.Append(ID_LinkBar_NewFolder, _("New &Folder..."));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_LinkBar_Delete, _("&Delete"));
    menuPopup.Append(ID_LinkBar_Rename, _("&Rename"));
    menuPopup.Enable(ID_LinkBar_Rename, item.isOk() ? true : false);
    menuPopup.Enable(ID_LinkBar_Delete, item.isOk() ? true : false);
    if (item.isOk())
    {
        menuPopup.AppendSeparator();
        menuPopup.Append(ID_LinkBar_Properties, _("Properti&es"));
    }
    
    // capture the event result
    CommandCapture* cc = new CommandCapture;
    PushEventHandler(cc);

#if wxCHECK_VERSION(2,9,3)
    PopupMenu(&menuPopup, m_actionPos);
#else
    PopupMenu(&menuPopup, m_action_pos);
#endif

    int command = cc->getLastCommandId();
    PopEventHandler(true);
    
    // unsticky the item we've right-clicked
    if (tool_id != -1)
        SetToolSticky(tool_id, false);
    
    // if we right-clicked on a folder, make sure the starting folder
    // for the LinkPropsDialog is that folders path
    wxString start_folder = "";
    if (is_folder_clicked)
    {
        start_folder += wxT("/");
        start_folder += item->getLabel();
    }
    
    // process the event
    switch (command)
    {
        case ID_LinkBar_OpenAll:
        {
            openItems(m_items);
            break;
        }
        case ID_LinkBar_OpenAllFolder:
        {
            if (item.isOk())
            {
                std::vector<IFsItemPtr> vec = enum2vec(item->getChildren());
                openItems(vec);
            }
            break;
        }
        case ID_LinkBar_Open:
        {
            if (item.isOk())
                DbDoc::actionActivate(item, appOpenDefault);
            break;
        }
        case ID_LinkBar_OpenTab:
        {
            if (item.isOk())
            {
                std::vector<IFsItemPtr> vec;
                vec.push_back(item);
                openItems(vec);
            }
            break;
        }
        case ID_LinkBar_NewBookmark:
        {
            LinkPropsDialog dlg(this);
            dlg.setFolder(start_folder);
            dlg.SetTitle(_("\"New Bookmark\" Properties"));
            dlg.SetSize(g_app->getMainWindow()->FromDIP(wxSize(370, 230)));
            dlg.SetMinSize(g_app->getMainWindow()->FromDIP(wxSize(370, 200)));
            dlg.CenterOnScreen();

            if (dlg.ShowModal() == wxID_OK)
            {
                // create the bookmark
                Bookmark b;
                b.location = towstr(dlg.getLocation());
                b.description = towstr(dlg.getDescription());
                b.tags = towstr(dlg.getTags());
                b.run_target = dlg.getRunTarget();
                
                g_app->getBookmarkFs()->saveBookmark(towstr(dlg.getPath()), b);

                // position the bookmark in the linkbar
                g_app->getBookmarkFs()->setFileVisualLocation(towstr(dlg.getPath()), idx);
                
                // repopulate and refresh the linkbar
                refresh();
            }
            
            break;
        }
        case ID_LinkBar_NewFolder:
        {
            wxString message = wxString::Format(_("Please enter the name of the folder:"));
            
            LinkPropsDialog dlg(this);
            dlg.setMode(LinkPropsDialog::ModeCreateFolder);
            dlg.setFolder(start_folder);
            dlg.setMessage(message);
            dlg.setName(_("New Folder"));
            dlg.SetTitle(_("New Folder"));
            dlg.SetSize(g_app->getMainWindow()->FromDIP(wxSize(370, 230)));
            dlg.SetMinSize(g_app->getMainWindow()->FromDIP(wxSize(370, 200)));
            dlg.CenterOnScreen();
            
            if (dlg.ShowModal() == wxID_OK)
            {
                g_app->getBookmarkFs()->createFolder(towstr(dlg.getPath()));
                
                // position the folder in the linkbar
                g_app->getBookmarkFs()->setFileVisualLocation(towstr(dlg.getPath()), idx);
                
                // repopulate and refresh the linkbar
                refresh();
            }
            
            break;
        }
        case ID_LinkBar_Properties:
        {
            wxString path = g_app->getBookmarkFs()->getBookmarkItemPath(item);

            if (is_folder_clicked)
            {
                // we're editing the properties of a folder
                
                wxString message = wxString::Format(_("Please enter the name of the folder:"));
                wxString title = wxString::Format(_("\"%s\" Properties"),
                                                  item->getLabel().c_str());
                                                  
                LinkPropsDialog dlg(this);
                dlg.setMode(LinkPropsDialog::ModeRename);
                dlg.setMessage(message);
                dlg.setName(item->getLabel());
                dlg.SetTitle(title);
                dlg.SetSize(g_app->getMainWindow()->FromDIP(wxSize(370, 230)));
                dlg.SetMinSize(g_app->getMainWindow()->FromDIP(wxSize(370, 200)));
                dlg.CenterOnScreen();
                
                if (dlg.ShowModal() == wxID_OK)
                {
                    xd::IDatabasePtr db = g_app->getDatabase();
                    if (db.isNull())
                        return;
                    
                    wxString new_path = dlg.getPath();
                    if (new_path.CmpNoCase(path) == 0)
                        return;
                    
                    bool result = db->moveFile(towstr(path), towstr(new_path));
                    if (!result)
                        return;
                    
                    // position the folder in the linkbar
                    g_app->getBookmarkFs()->setFileVisualLocation(towstr(new_path), idx);
                    
                    // repopulate and refresh the linkbar
                    refresh();
                }
            }
             else
            {
                Bookmark b;
                if (!g_app->getBookmarkFs()->loadBookmark(towstr(path), b))
                    return;

                wxString title = wxString::Format(_("\"%s\" Properties"),
                                                    item->getLabel().c_str());
                    
                LinkPropsDialog dlg(this);
                dlg.setName(item->getLabel());
                dlg.setLocation(b.location);
                dlg.setTags(b.tags);
                dlg.setDescription(b.description);
                dlg.setRunTarget(b.run_target);
                dlg.SetTitle(title);
                dlg.SetSize(g_app->getMainWindow()->FromDIP(wxSize(370, 230)));
                dlg.SetMinSize(g_app->getMainWindow()->FromDIP(wxSize(370, 200)));
                dlg.CenterOnScreen();
                    
                if (dlg.ShowModal() == wxID_OK)
                {
                    wxString new_path = dlg.getPath();

                    // save the properties
                    b.location = towstr(dlg.getLocation());
                    b.tags = towstr(dlg.getTags());
                    b.description = towstr(dlg.getDescription());
                    b.run_target = dlg.getRunTarget();

                    if (g_app->getBookmarkFs()->saveBookmark(towstr(new_path), b))
                    {
                        if (new_path != path)
                        {
                            // delete old bookmark
                            g_app->getBookmarkFs()->deleteItem(towstr(path));
                        }

                        // position the bookmark in the linkbar
                        g_app->getBookmarkFs()->setFileVisualLocation(towstr(new_path), idx);
                        
                        // repopulate and refresh the linkbar
                        refresh();
                    }
                }
            }

            
            break;
        }
        case ID_LinkBar_Rename:
        {
            // we're editing the properties of a folder
            wxString message;
            if (is_folder_clicked)
                message = _("Please enter the new name of the folder:");
                 else
                message = _("Please enter the new name of the bookmark:");

            wxString title = wxString::Format(_("\"%s\" Properties"),
                                                item->getLabel().c_str());
                                                  
            LinkPropsDialog dlg(this);
            dlg.setMode(LinkPropsDialog::ModeRename);
            dlg.setMessage(message);
            dlg.setName(item->getLabel());
            dlg.SetTitle(title);
            dlg.SetSize(g_app->getMainWindow()->FromDIP(wxSize(370, 230)));
            dlg.SetMinSize(g_app->getMainWindow()->FromDIP(wxSize(370, 200)));
            dlg.CenterOnScreen();
                
            if (dlg.ShowModal() == wxID_OK)
            {
                std::wstring old_path = g_app->getBookmarkFs()->getBookmarkItemPath(item);
                std::wstring new_path = towstr(dlg.getPath());

                if (g_app->getBookmarkFs()->moveItem(old_path, new_path))
                {
                    // position the folder in the linkbar
                    g_app->getBookmarkFs()->setFileVisualLocation(new_path, idx);
                    
                    // repopulate and refresh the linkbar
                    refresh();
                }
            }

            break;
        }
        case ID_LinkBar_Delete:
        {
            std::wstring bookmark_path = g_app->getBookmarkFs()->getBookmarkItemPath(item);
            if (bookmark_path.length() > 0)
            {
                g_app->getBookmarkFs()->deleteItem(towstr(item->getLabel()));
                refresh();
            }

            break;
        }
    }
}

void LinkBar::onMiddleClick(wxAuiToolBarEvent& evt)
{
    // invalid tool id, bail out
    int tool_id = evt.GetToolId();
    if (tool_id == -1)
        return;
    
    // determine if the item is a folder
    bool is_folder = isFolderItem(tool_id);
    
    // convert the tool index to a link index
    int idx = GetToolIndex(tool_id);
    tool2LinkIndex(idx);
    
    // get the IFsItemPtr from the index
    IFsItemPtr item;
    if (idx >= 0 && idx < (int)m_items.size())
        item = m_items[idx];

    if (item.isNull())
        return;
    
    if (is_folder)
    {
        // middle-click on a folder opens all link items in the folder
        std::vector<IFsItemPtr> vec = enum2vec(item->getChildren());
        openItems(vec);
    }    
     else
    {
        // middle-click on a regular item opens it in a new tab
        DbDoc::actionActivate(item, appOpenForceNewWindow |
                                    appOpenActivateNewWindow);
    }
}


void LinkBar::onOverflowClick(wxAuiToolBarEvent& evt)
{
    // the linkbar dropdown is already open
    if (isPopupWindowOpen() && m_popup_id == LinkBarDropDownId)
        return;
    
    xcm::IVectorImpl<xd::IFileInfoPtr>* vec;
    vec = new xcm::IVectorImpl<xd::IFileInfoPtr>;

    /*
    // only show items that don't fit in the dropdown
    size_t i, count = m_items.size();
    for (i = 0; i < count; ++i)
    {
        if (!GetToolFits(ID_FirstLinkBarId+i))
        {
            xd::IFileInfoPtr info = fsItemToFileInfo(m_items[i]);
            if (info.isOk())
                vec->append(info);
        }
    }
    */
    
    // no items to show
    if (vec->size() == 0)
        return;
    
    showPopupWindow(LinkBarDropDownId, true, vec);
}

static wxRect getToolRectByItem(LinkBarItem* item)
{
    if (!item)
        return wxRect();
    
    return item->GetSizerItem()->GetRect();
}

void LinkBar::drawDropHighlight()
{
    // redraw the background
    Refresh(false);
    Update();
    
    // invalid drop index, don't draw any drop highlight
    if (m_drop_idx < 0)
        return;
    
    // if there's a popup window open, don't draw any drop highlight
    if (isPopupWindowOpen())
        return;
    
    wxClientDC cdc(this);
    
    // there are no items in the linkbar
    if (GetToolCount() == 0)
    {
        int w,h;
        GetClientSize(&w,&h);
        
        // draw the drop highlight at the far left of the linkbar
        cdc.DrawLine(2, 4, 2, h-3);
        cdc.DrawLine(3, 4, 3, h-3);
        return;
    }
    
    // get the rectangle of the item we're using to draw the drop highlight
    
    LinkBarItem* item;
    if ((size_t)m_drop_idx >= GetToolCount())
    {
        item = FindToolByIndex(m_drop_idx-1);
        wxRect item_rect = getToolRectByItem(item);
    
        // draw the drop highlight to the right of the hover item
        cdc.DrawLine(item_rect.x + item_rect.width, item_rect.y + 1,
                     item_rect.x + item_rect.width, item_rect.y + item_rect.height - 1);
        cdc.DrawLine(item_rect.x + item_rect.width + 1, item_rect.y + 1,
                     item_rect.x + item_rect.width + 1, item_rect.y + item_rect.height - 1);
    }
     else
    {
        item = FindToolByIndex(m_drop_idx);
        wxRect item_rect = getToolRectByItem(item);
    
        // draw the drop highlight to the left of the hover item
        cdc.DrawLine(item_rect.x - 2, item_rect.y + 1,
                     item_rect.x - 2, item_rect.y + item_rect.height - 1);
        cdc.DrawLine(item_rect.x - 1, item_rect.y + 1,
                     item_rect.x - 1, item_rect.y + item_rect.height - 1);
    }
}

void LinkBar::refresh()
{
    int desired_icon_size = FromDIP(100) > 100 ? 24 : 16;

    this->Clear();    // remove items from toolbar
    m_items.clear();  // clear our items vector

#if APP_NEW_TOOLBARS != 1
    // add command buttons
    AddTool(ID_Project_New, ID2BMPSMALL(ID_Project_New), wxNullBitmap, false);
    AddSpacer(3);
    AddTool(ID_Project_OpenFile, ID2BMPSMALL(ID_Project_OpenFile), wxNullBitmap, false);
    AddSpacer(3);
    AddTool(ID_File_Save, ID2BMPSMALL(ID_File_Save), wxNullBitmap, false);
    AddSpacer(3);
    AddSeparator();
    AddSpacer(3);
    AddTool(ID_Data_Sort, ID2BMPSMALL(ID_Data_Sort), wxNullBitmap, false);
    AddSpacer(3);
    AddTool(ID_Data_CreateDynamicField, ID2BMPSMALL(ID_Data_CreateDynamicField), wxNullBitmap, false);
    AddSpacer(3);
    AddTool(ID_Data_GroupRecords, ID2BMPSMALL(ID_Data_GroupRecords), wxNullBitmap, false);
    AddSpacer(3);
    AddTool(ID_Data_Filter, ID2BMPSMALL(ID_Data_Filter), wxNullBitmap, false);
    AddSpacer(3);
    AddTool(ID_App_ToggleRelationshipSync, ID2BMPSMALL(ID_App_ToggleRelationshipSync), wxNullBitmap, false);
    AddSpacer(3);
    AddSeparator();
    AddSpacer(3);

    SetToolDropDown(ID_App_ToggleRelationshipSync, true);

    SetToolShortHelp(ID_Project_New, _("New"));
    SetToolShortHelp(ID_Project_OpenFile, _("Open"));
    SetToolShortHelp(ID_File_Save, _("Save"));
    SetToolShortHelp(ID_Data_Sort, _("Sort Data"));
    SetToolShortHelp(ID_Data_CreateDynamicField, _("Insert Calculated Field"));
    SetToolShortHelp(ID_Data_GroupRecords, _("Group Data"));
    SetToolShortHelp(ID_Data_Filter, _("Filter Data"));
    SetToolShortHelp(ID_App_ToggleRelationshipSync, _("Filter Related Records"));
 #endif

    
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        Realize();
        return;
    }

    IFsItemPtr root_item = g_app->getBookmarkFs()->getBookmarkFolderItem(L"/");
    
    if (root_item.isNull())
    {
        this->Realize();
        return;
    }
    
    IFsItemEnumPtr children = root_item->getChildren();
    size_t i, count = children->size();
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = children->getItem(i);
        
        int id = ID_FirstLinkBarId + i;
        wxBitmap bmp = rescaleBitmapTo(item->getBitmap(), desired_icon_size);
        wxString label = item->getLabel();
        label.Trim(false).Trim(true);
        
        // truncate long labels
        wxString tool_label = label;
        if (tool_label.Length() > 24)
        {
            tool_label.Remove(24);
            tool_label.Trim(true);
            tool_label += wxT("...");
        }
        
        AddTool(id, bmp, wxNullBitmap, false, 0);
        SetToolLabel(id, tool_label);
        
        /*
        IDbFolderFsItemPtr folder_item = item;
        if (folder_item.isNull())
        {
            // only show tooltips for non-folder items
            wxString tooltip;
            
            Bookmark b;
            bool bookmark_loaded = b.load(g_app->getBookmarkFs()->getBookmarkItemPath(item));

            // truncate long labels
            if (label.Length() > 80)
            {
                label.Remove(80);
                label += wxT("...");
            }
            
            // add label to tooltip
            if (label.Length() > 0)
                tooltip += label.Trim(false).Trim(true);
            
            // if we were able to load the bookmark file,
            // append the bookmark location to the tooltip
            if (bookmark_loaded)
            {
                // truncate long locations
                wxString loc = b.getLocation();
                if (loc.Length() > 80)
                {
                    loc.Remove(80);
                    loc += wxT("...");
                }
                
                // add location to tooltip
                if (loc.Length() > 0)
                {
                    tooltip += wxT("\n");
                    tooltip += loc.Trim(false).Trim(true);
                }
                
                // show some of the description
                wxString desc = b.getDescription();
                if (desc.Length() > 240)
                {
                    desc.Remove(240);
                    desc += wxT("...");
                }

                // add description to tooltip
                if (desc.Length() > 0)
                {
                    tooltip += wxT("\n");
                    
                    int split_idx = -1;
                    wxString desc_line;
                    
                    int desc_line_count = 0;
                    while (desc.Length() > 0 && desc_line_count < 16)
                    {
                        tooltip += wxT("\n");
                        
                        split_idx = desc.Find(wxT('\n'));
                        if (split_idx == -1 && desc.Length() < 80)
                        {
                            // the description doesn't have a line feed
                            // and is less than 60 characters long
                            tooltip += desc.Trim(false).Trim(true);
                            break;
                        }
                         else if (split_idx < 80 && split_idx != -1)
                        {
                            // there was a line feed in the first 80
                            // characters of the description line
                            tooltip += desc.Mid(0,split_idx)
                                           .Trim(false).Trim(true);
                            desc = desc.Mid(split_idx+1);
                        }
                         else
                        {
                            // there wasn't a line feed in the first 80
                            // characters of the description line
                            desc_line = desc.Mid(0,60);
                            split_idx = desc_line.Find(wxT(' '), true);
                            if (split_idx == -1)
                                split_idx = desc_line.Find(wxT('\t'), true);
                            if (split_idx == -1)
                                split_idx = desc_line.Length();
                            
                            tooltip += desc_line.Mid(0,split_idx)
                                                .Trim(false).Trim(true);
                            desc = desc.Mid(split_idx);
                        }
                        
                        split_idx = -1;
                        desc_line_count++;
                    }
                }
            }
            
            if (tooltip.Length() > 0)
                SetToolShortHelp(id, tooltip);
        }
        */

        m_items.push_back(item);
    }
    
    Realize();
    
    // remove hover and pressed item highlights if they exist
    SetHoverItem(NULL);
    SetPressedItem(NULL);
    
    // show/hide the dropdown as necessary
    showOrHideDropDown();
}

void LinkBar::showOrHideDropDown()
{
    if (GetToolFits(ID_FirstLinkBarId + (m_items.size()-1)))
    {
        // last item fits in linkbar, we don't need the toolbar dropdown
        if (GetOverflowVisible())
        {
            SetOverflowVisible(false);
            Realize();
        }
    }
     else
    {
        // last item goes past end of linkbar, we need the toolbar dropdown
        if (!GetOverflowVisible())
        {
            SetOverflowVisible(true);
            Realize();
        }
    }
}

void LinkBar::tool2LinkIndex(int& tool_idx)
{
    int first_idx = GetToolIndex(ID_FirstLinkBarId);
    if (first_idx == -1)
    {
        tool_idx = 0;
        return;
    }
    
    tool_idx = (tool_idx-first_idx);
}

void LinkBar::link2ToolIndex(int& link_idx)
{
    int first_idx = GetToolIndex(ID_FirstLinkBarId);
    if (first_idx == -1)
    {
        link_idx = (int)GetToolCount();
        return;
    }
    
    link_idx = (link_idx+first_idx);
}


// BE CAREFUL! -- This function returns the (tool-based) insert index,
// not the link-based insert index

int LinkBar::getInsertIndex(const wxPoint& pt, bool ignore_ypos)
{
    // check to make sure we have at least one item in the linkbar
    
    int tool_count = (int)GetToolCount();
    if (tool_count == 0)
        return 0;
    
    // check to make sure we have at least one link in the linkBar
    
    int first_idx = GetToolIndex(ID_FirstLinkBarId);
    if (first_idx == -1)
        return tool_count;

    int x = pt.x;
    int y = pt.y;
    
    // ignore vertical padding of items (always try and find
    // the item regardless of the mouse y-coordinate)
    if (ignore_ypos)
        y = GetClientSize().GetHeight()/2;
    
    // check to see if we're hovering over an item
    
    LinkBarItem* hover_item = FindToolByPosition(x, y);
    if (hover_item != NULL)
    {
        wxRect hover_item_rect = GetToolRect(hover_item->GetId());

        // get the index of the hover item in the linkbar
        int insert_idx = GetToolIndex(hover_item->GetId());
        
        // make sure the insert index is one greater if the hover point
        // is more than half way past the item rectange horizontally
        if (x > (hover_item_rect.x + (hover_item_rect.width)/2))
            insert_idx += 1;
        
        if (insert_idx < 0)
            insert_idx = 0;
        
        if (insert_idx > tool_count)
            insert_idx = tool_count;
        
        return insert_idx;
    }
    

    // check to see if we're hovering in between items
    
    int insert_idx = 0;

    // the chunk of code below will find the closest non-separator item
    // to the left or right of the hover position (within reason) -- we
    // have to find a non-separator item because separators all have the
    // same id, thus GetItemIndex() returns an invalid index

    // check to the left
    
    int left_x = x, left_offset_count = 0;
    LinkBarItem* left_item = FindToolByPosition(left_x, y);
    
    while (left_x > x-100)
    {
        // check to see if there's a non-separator item to the left
        left_x--;
        left_item = FindToolByPosition(left_x, y);
        
        if (left_item == NULL)
            continue;
        
        if (left_item != NULL && left_item->GetKind() == wxITEM_SEPARATOR)
        {
            // skip to the left of the separator
            wxRect r = getToolRectByItem(left_item);
            left_x = r.x;

            // make sure we know how many items to the left we've gone
            left_offset_count++;
            continue;
        }
        
        // left item is not null and is not a separator
        insert_idx = GetToolIndex(left_item->GetId());
        insert_idx += left_offset_count;
        insert_idx += 1;

        if (insert_idx < 0)
            insert_idx = 0;
        
        if (insert_idx > tool_count)
            insert_idx = tool_count;
        
        return insert_idx;
    }

    // check to the right
    
    int right_x = x, right_offset_count = 0;
    LinkBarItem* right_item = FindToolByPosition(right_x, y);
    
    while (right_x < x+100)
    {
        // check to see if there's a non-separator item to the left
        right_x++;
        right_item = FindToolByPosition(right_x, y);
        
        if (right_item == NULL)
            continue;
        
        if (right_item != NULL && right_item->GetKind() == wxITEM_SEPARATOR)
        {
            // skip to the right of the separator
            wxRect r = getToolRectByItem(right_item);
            right_x = r.x+r.width;

            // make sure we know how many items to the right we've gone
            right_offset_count++;
            continue;
        }
        
        // right item is not null and is not a separator
        insert_idx = GetToolIndex(right_item->GetId());
        insert_idx -= right_offset_count;

        if (insert_idx < 0)
            insert_idx = 0;
        
        if (insert_idx > tool_count)
            insert_idx = tool_count;
        
        return insert_idx;
    }

    return tool_count;
}

void LinkBar::onFsDataLeave()
{
    if (!isPopupWindowOpen())
    {
        // remove hover and pressed item highlights if they exist
        SetHoverItem(NULL);
        SetPressedItem(NULL);
    }
    
    Refresh(false);
    Update();
    
    m_drop_idx = -1;
    m_last_id = -1;
}

bool LinkBar::pointInExclude(const wxPoint& pt)
{
    if (m_exclude_id_begin == -1 && m_exclude_id_end == -1)
        return false;

    int exclude_start_x = -1, exclude_end_x = -1;
    int packing = GetToolPacking();
    wxRect r;
    
    // find out the boundaries of the excluded items
    if (m_exclude_id_begin != -1)
    {
        r = GetToolRect(m_exclude_id_begin);
        exclude_start_x = r.x - packing;
    }
    
    if (m_exclude_id_end != -1)
    {
        r = GetToolRect(m_exclude_id_end);
        exclude_end_x = r.x + r.width + packing;
    }
    
    // exclude all items up until the end item
    if (exclude_start_x == -1 && pt.x <= exclude_end_x)
        return true;
    
    // exclude all items after the begin item
    if (exclude_end_x == -1 && pt.x >= exclude_start_x)
        return true;
    
    // exclude all items in between the begin and end item
    if (exclude_start_x != -1 && exclude_end_x != -1 &&
        exclude_start_x >= pt.x && pt.x <= exclude_end_x)
        return true;

    return false;
}

static bool isNearItemEdge(const wxRect& rect, int x)
{
    int tolerance = 8;
    if (rect.width < 40)
        tolerance = (int)((double)rect.width * 0.2);
    if (x < rect.x+tolerance || x > rect.x+rect.width-tolerance)
        return true;
    return false;
}

void LinkBar::onFsDataDragOver(wxDragResult& def)
{
    int x,y;
    ::wxGetMousePosition(&x,&y);
    ScreenToClient(&x,&y);
    
    // don't allow drops in the exclude region
    if (pointInExclude(wxPoint(x,y)))
    {
        // draw the linkbar without a drop highlight
        if (m_drop_idx != -1)
        {
            // remove hover and pressed item highlights if they exist
            SetHoverItem(NULL);
            SetPressedItem(NULL);

            Refresh(false);
            Update();
        }
        
        m_drop_idx = -1;
        m_last_id = -1;
        
        def = wxDragNone;
        return;
    }
    
    // find the tool in the toolbar that we're hovering over
    LinkBarItem* item = getItemFromCurrentPosition(true);
    
    // make sure we are hovering over an item
    // (or the area to the right of the last item)
    if (!item)
    {
        // we already have a drop index, we're done
        if (m_drop_idx != -1)
            return;
        
        // get the drop index by finding the closest item to our location
        int drop_idx = getInsertIndex(wxPoint(x,y), true);
        if (m_drop_idx != drop_idx)
        {
            // remove hover and pressed item highlights if they exist
            SetHoverItem(NULL);
            SetPressedItem(NULL);

            m_drop_idx = drop_idx;
            drawDropHighlight();
        }
        return;
    }
    
    // ignore separators
    if (item->GetKind() == wxITEM_SEPARATOR)
    {
        if (m_drop_idx == -1)
            def = wxDragNone;
        return;
    }
    
    // get the rectangle of the tool on the toolbar
    wxRect item_rect = GetToolRect(item->GetId());
    
    // restart the hover stopwatch everytime we move to a new item
    // and the last item we were on was not a folder item
    if (item->GetId() != m_last_id && !isFolderItem(m_last_id))
        m_hover_stopwatch.Start();

    // store the id of the item we're on for comparison
    // above when we move to a new item
    m_last_id = item->GetId();

    // show the popup window if we've paused over the center of a folder item
    bool near_edge = isNearItemEdge(item_rect, x);
    bool hover_paused = (m_hover_stopwatch.Time() > 750 ? true : false);
    
    if (isFolderItem(item->GetId()) && !near_edge)
    {
        if (hover_paused)
        {
            // convert the tool index to a link index
            int idx = GetToolIndex(item->GetId());
            tool2LinkIndex(idx);
            
            // get the IFsItemPtr from the index
            IFsItemPtr fs_item;
            if (idx >= 0 && idx < (int)m_items.size())
                fs_item = m_items[idx];
            
            if (fs_item.isOk())
            { 
                // only show the popup window if we're hovering over
                // a folder item and it's not the folder we're dragging
                if (item->GetId() != m_drag_id && item->GetId() != m_popup_id)
                {
                    showPopupWindow(item->GetId(), false);
                    m_popup_during_drag = true;
                }
            }
        }

        // we want to show the user that they're going to drop the item
        // in the folder, not next to it, so remove the drop highlight from
        // the linkbar... the first half of the 'or' statement below handles
        // the case when we're dragging from the linkbar and the second half
        // handles the case when we're dragging from the project panel
        if ((item->GetId() != m_drag_id && m_drop_idx != -1) || m_drag_id == -1)
        {
            // set the hover item highlight and no pressed item highlight
            SetHoverItem(item);
            SetPressedItem(NULL);

            m_drop_idx = -1;
            Refresh(false);
            Update();
        }
    }
     else
    {
        // we're not hovering over a folder tool item, so if we still
        // have a popup window hanging around, destroy it
        if (isPopupWindowOpen())
            closePopupWindow();
        
        // we're hovering over an item, so get the insert index
        // based on the item's index
        int drop_idx = getInsertIndex(wxPoint(x,y), true);
        
        // draw the drop highlight
        if (m_drop_idx != drop_idx)
        {
            // remove hover and pressed item highlights if they exist
            SetHoverItem(NULL);
            SetPressedItem(NULL);

            m_drop_idx = drop_idx;
            drawDropHighlight();
        }
    }
}

// borrowed from dbdoc
wxBitmap getShortcutBitmap(const wxBitmap& input_bmp);


// this function handles all drag and drop to the linkbar
// from the project panel (or any existing object in the project,
// in the case of the dragging from the url combobox)
static void doProjectTreeDragDrop(IFsItemPtr item,
                                  wxString drop_folder_path,
                                  int link_drop_idx = -1)
{
    // get the full path of the item
    wxString src_path = DbDoc::getFsItemPath(item);

    // get the name (chop off folders)
    wxString src_name = src_path.AfterLast(wxT('/'));
    if (src_name.IsEmpty())
        return;

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    IBookmarkFsPtr bookmark_fs = g_app->getBookmarkFs();
    if (bookmark_fs.isNull())
        return;

    std::wstring destination_path = xd::appendPath(drop_folder_path.ToStdWstring(), src_name.ToStdWstring());

    if (bookmark_fs->createBookmark(destination_path, src_path.ToStdWstring(), L"", L"", getShortcutBitmap(item->getBitmap()).ConvertToImage()))
    {
        if (link_drop_idx != -1)
        {
            bookmark_fs->setFileVisualLocation(destination_path, link_drop_idx);
        }
    }
}

void LinkBar::onFsDataFolderDrop(IFsItemPtr target, wxDataObject* data, wxDragResult* result)
{
    FsDataObject* tree_data = (FsDataObject*)data;
    IFsItemEnumPtr items = tree_data->getFsItems();

    size_t i, count = items->size();

    std::wstring target_folder = g_app->getBookmarkFs()->getBookmarkItemPath(m_popup_fspanel->getRootItem());

    int x, y;
    ::wxGetMousePosition(&x, &y);
    IDocumentPtr doc = m_popup_fspanel;
    doc->getDocumentWindow()->ScreenToClient(&x, &y);
            
    IFsItemPtr target_item = m_popup_fspanel->hitTest(x,y);
    if (target_item.isOk())
    {

        int target_idx = m_popup_fspanel->getItemIndex(target_item);
        if (target_idx != -1)
        {
            for (i = 0; i < count; ++i)
            {
                IFsItemPtr item = items->getItem(i);
                std::wstring src_path = g_app->getBookmarkFs()->getBookmarkItemPath(item);
                std::wstring dest_path = target_folder + L"/" + towstr(item->getLabel());

                g_app->getBookmarkFs()->moveItem(src_path, dest_path);
                g_app->getBookmarkFs()->setFileVisualLocation(dest_path, target_idx + i);
            }
        }
    }
            
    refresh();
}


void LinkBar::onFsDataDrop(wxDragResult& def, FsDataObject* data)
{
    IFsItemEnumPtr items = data->getFsItems();
    if (items->size() == 0)
    {
        // no items to drop, bail out
        m_drop_idx = -1;
        m_drag_id = -1;
        m_last_id = -1;
        return;
    }
    
    // this chunk of code only applies if we're dropping items into a folder
    LinkBarItem* drop_folder = NULL;
    std::wstring drop_folder_path;
    IFsItemPtr drop_item;

    if (m_drop_idx == -1)
    {
        int x,y;
        ::wxGetMousePosition(&x,&y);
        ScreenToClient(&x,&y);

        // find the item we're dropping on
        drop_folder = getItemFromCurrentPosition(true);
        if (!drop_folder)
            return;
        
        drop_item = getItemFromId(drop_folder->GetId());
        if (drop_item.isNull())
            return;

        // get the path of the drop folder in the linkbar
        drop_folder_path = g_app->getBookmarkFs()->getBookmarkItemPath(drop_item);
    }
    
    if (data->getSourceId() == ID_Toolbar_Link)
    {

        // dragging from the linkbar to the linkbar
        
        // we can't drag more than one item at a time from the linkbar
        if (items->size() != 1)
            return;
     
        int drag_idx = GetToolIndex(m_drag_id);
        
        if (m_drop_idx != drag_idx &&
            m_drop_idx != drag_idx+1 &&
            drop_folder_path.empty())
        {
            // repositioning an item in the linkbar
        
            // if we drop the item to the right of itself, we have to compensate
            // for the fact that it's gone when we calculate the drop index
            if (m_drop_idx > drag_idx+1)
                m_drop_idx--;
            
            // convert the tool index to a link index
            int link_drop_idx = m_drop_idx;
            tool2LinkIndex(link_drop_idx);
            
            std::wstring path = g_app->getBookmarkFs()->getBookmarkItemPath(items->getItem(0));
            g_app->getBookmarkFs()->setFileVisualLocation(path, link_drop_idx);
        }
         else if (drop_folder_path.length() > 0)
        {
            // dragging an item into a folder in the linkbar
            
            // get the full path of the source item
            std::wstring src_path = g_app->getBookmarkFs()->getBookmarkItemPath(items->getItem(0));
            
            // move the item to the destination folder
            std::wstring dest_path = drop_folder_path;
            dest_path += L"/";
            dest_path += kl::afterLast(src_path, '/');

            g_app->getBookmarkFs()->moveItem(src_path, dest_path);

            int link_drop_idx = 0;
            IFsItemEnumPtr children = drop_item->getChildren();
            if (children.isOk())
                link_drop_idx = children->size();

            // make sure it goes to the bottom of the folder
            g_app->getBookmarkFs()->setFileVisualLocation(dest_path, link_drop_idx);
        }
    }
     else if (data->getSourceId() == ID_Frame_UrlCtrl)
    {
        // dragging from the url combobox to the linkbar

        // we can't drag more than one item at a time from the url combobox
        if (items->size() != 1)
            return;

        IDbObjectFsItemPtr drag_obj = items->getItem(0);

        // we're browsing an item that doesn't exist in the project,
        // create a bookmark (link) and add it to the linkbar
        // (or folder in the linkbar)

        IFsItemPtr item = items->getItem(0);
            
        std::wstring dest_path;

        wxString label = item->getLabel();
        trimUnwantedUrlChars(label);
            
        if (drop_folder_path.length() > 0)
        {
            // create the item in the destination folder in the linkbar
            dest_path = drop_folder_path;
            dest_path += L"/";
            dest_path += towstr(label);
        }
            else
        {
            // create the item in the main linkbar folder
            dest_path = L"/" + label;
        }


        // grab favicon -- since we're dragging from the url bar, the
        // current document should handily provide us with the right icon
        wxImage favicon;
        IDocumentSitePtr site = g_app->getMainFrame()->getActiveChild();
        IWebDocPtr webdoc;
        if (site)
            webdoc = site->getDocument();
                
        if (webdoc)
        {
            favicon = webdoc->getFavIcon();
            webdoc.clear();
        }
         else
        {
            kcl::BitmapComboControl* wnd = (kcl::BitmapComboControl*)g_app->getMainWindow()->FindWindowById(ID_Frame_UrlCtrl);
            if (wnd)
            {
                wxBitmap bmp = wnd->getBitmap();
                if (bmp.IsOk())
                {
                    favicon = bmp.ConvertToImage();
                }
            }

        }
                

        // create the bookmark at the specified location
        g_app->getBookmarkFs()->createBookmark(dest_path, towstr(drag_obj->getPath()), L"", L"", favicon);

        // if we're not dragging to a folder, set the
        // position of the item on the linkbar
        if (drop_folder_path.empty())
        {
            // convert the tool index to a link index
            int link_drop_idx = m_drop_idx;
            tool2LinkIndex(link_drop_idx);
            
            // position the item in the linkbar
            g_app->getBookmarkFs()->setFileVisualLocation(dest_path, link_drop_idx);
        }
    }
    else
    {
        // dragging from the project panel to the linkbar

        size_t i, count = items->size();
        for (i = 0; i < count; ++i)
        {
            IFsItemPtr item = items->getItem(i);
            if (item.isNull())
                continue;

            // convert the tool index to a link index
            int link_drop_idx = m_drop_idx;
            tool2LinkIndex(link_drop_idx);

            doProjectTreeDragDrop(item, drop_folder_path, link_drop_idx + i);
        }
    }


    refresh();
    
    // we're no longer dragging anything
    m_drop_idx = -1;
    m_drag_id = -1;
    m_last_id = -1;
}

// this function was ripped directly from
// AppController::onUpdateUI_RelationshipSync()
void LinkBar::onUpdateUI_RelationshipSync(wxUpdateUIEvent& evt)
{
/*
    if (g_app->getMainFrame().isNull())
        return;

    IDocumentSitePtr& doc_site = g_app->getMainFrame()->getActiveChild();
    if (doc_site.isNull())
    {
        evt.Enable(false);
        evt.Check(false);
        return;
    }
    
    ITableDocPtr tabledoc = doc_site->getDocument();
    evt.Enable(tabledoc.isOk() ? true : false);
    evt.Check(tabledoc.isOk() ? g_app->getAppController()->getRelationshipSync() : false);
*/
}

void LinkBar::OnLeaveWindow(wxMouseEvent& evt)
{
    RefreshOverflowState();
    
    if (!isPopupWindowOpen())
    {
        // remove hover and pressed item highlights if they exist
        SetHoverItem(NULL);
        SetPressedItem(NULL);
    }
    
#if wxCHECK_VERSION(2,9,3)
    m_tipItem = NULL;
#else
    m_tip_item = NULL;
#endif

}

void LinkBar::OnSize(wxSizeEvent& evt)
{
    // show/hide the dropdown as necessary
    showOrHideDropDown();

    wxAuiToolBar::OnSize(evt);
}
