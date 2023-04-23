/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-11-08
 *
 */


#include "appmain.h"
#include <wx/dnd.h>
#include "../kcl/button.h"
#include "fspanel_private.h"
#include "fsitems_private.h"
#include "fspaneltreeview.h"
#include "fspanellistview.h"


#ifdef WIN32
#include <shlobj.h>
#endif


#ifdef __WXGTK__
#define CFW_USE_GENERIC_TREECTRL
#define CFW_USE_GENERIC_LISTCTRL
#endif


#ifdef __WXMSW__
#include <windows.h>
#include <commctrl.h>
#endif



IFsPanelPtr createFsPanelObject()
{
    return static_cast<IFsPanel*>(new FsPanel);
}




/* XPM */
const static char* xpm_folder_closed[] = {
"16 16 6 1",
"  c None",
"! c #000000",
"# c #808080",
"$ c #C0C0C0",
"% c #FFFF00",
"& c #FFFFFF",
"                ",
"   #####        ",
"  #%$%$%#       ",
" #%$%$%$%###### ",
" #&&&&&&&&&&&&#!",
" #&%$%$%$%$%$%#!",
" #&$%$%$%$%$%$#!",
" #&%$%$%$%$%$%#!",
" #&$%$%$%$%$%$#!",
" #&%$%$%$%$%$%#!",
" #&$%$%$%$%$%$#!",
" #&%$%$%$%$%$%#!",
" ##############!",
"  !!!!!!!!!!!!!!",
"                ",
"                "};


/* XPM */
const static char* xpm_folder_open[] = {
"16 16 5 1",
"  c None",
"! c #000000",
"# c #808000",
"$ c #FFFF00",
"% c #FFFFFF",
"                ",
"                ",
"                ",
"  !!!           ",
" !%$%!!!!!!!    ",
" !$%$%$%$%$!    ",
" !%$%$%$%$%!    ",
" !$%$%!!!!!!!!!!",
" !%$%!#########!",
" !$%!#########! ",
" !%!#########!  ",
" !!#########!   ",
" !!!!!!!!!!!    ",
"                ",
"                ",
"                "};




// Drag and Drop functionality

class FsTreeDropTarget : public wxDropTarget
{
public:
    
    FsTreeDropTarget(FsPanel* fspanel) : wxDropTarget(new FsDataObject)
    {
        m_fspanel = fspanel;
        m_tree = fspanel->m_treeview;
    }

    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
    {
        if (!m_tree)
            return wxDragNone;
        
        if (def == wxDragNone)
            return def;

        if (!m_fspanel->isDragDropEnabled())
            return wxDragNone;
        
        int w, h;
        m_tree->GetClientSize(&w, &h);
        if (y < 30)
        {
            m_tree->doScroll(-1);
        }
         else if (y > h-30)
        {
            m_tree->doScroll(1);
        }
        
        wxPoint pt(x,y);
        int hit_flags = 0;
        
        wxTreeItemId id = m_tree->HitTest(pt, hit_flags);
        
        if (hit_flags & wxTREE_HITTEST_NOWHERE)
        {
            id = m_tree->GetRootItem();
        }
        
        if (id.IsOk())
        {
            IFsItemPtr highlight_item = m_tree->getItemFromId(id);
            if (!highlight_item)
                return wxDragNone;

            m_fspanel->sigItemHighlightRequest().fire(highlight_item);
            if (!highlight_item)
                return wxDragNone;

            m_tree->highlightItem(highlight_item->getTreeItemId());
            return wxDragMove;
        }
         else
        {
            m_tree->unhighlightItem();
        }

        return wxDragNone;
    }

    void OnLeave()
    {
        if (!m_tree)
            return;

        // on linux, sometimes spurious OnLeave() calls are made on the
        // drop target.  This code checks for and takes care of this
        
        wxPoint pt = ::wxGetMousePosition();
        pt = m_tree->ScreenToClient(pt);
        
        wxSize clisize = m_tree->GetClientSize();
        
        if (pt.x < 0 ||
            pt.y < 0 ||
            pt.x > clisize.x ||
            pt.y > clisize.y)
        {
            m_tree->unhighlightItem();
        }
    }

    bool OnDrop(wxCoord x, wxCoord y)
    {
        if (!m_tree)
            return false;

        m_target_item = m_tree->getHighlightItem();
        m_tree->unhighlightItem();
        return true;
    }

    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
    {
        if (!m_tree)
            return wxDragNone;

        if (!m_target_item.IsOk())
            return wxDragNone;

        FsItemData* target = (FsItemData*)m_tree->GetItemData(m_target_item);
        if (!target)
            return wxDragNone;

        if (!GetData())
            return wxDragNone;

        wxDragResult result = def;
        m_fspanel->sigDragDrop().fire(target->m_fsitem, GetDataObject(), &result);
        return result;
    }

private:

    FsPanel* m_fspanel;
    FsPanelTreeView* m_tree;
    wxTreeItemId m_highlight_item;
    wxTreeItemId m_target_item;
};


// this function only works on Windows

static void setTreeItemHeight(wxTreeCtrl* ctrl, int height)
{
#ifdef __WXMSW__
    HWND hwnd = (HWND)ctrl->GetHandle();
    SendMessage(hwnd, TVM_SETITEMHEIGHT, height, 0);
#endif
}

// this function only works on Windows

static int getTreeItemHeight(wxTreeCtrl* ctrl)
{
#ifdef __WXMSW__
    HWND hwnd = (HWND)ctrl->GetHandle();
    int height = SendMessage(hwnd, TVM_GETITEMHEIGHT, 0, 0);
    return height;
#endif

    return 0;
}

enum
{
    ID_TreeView = wxID_HIGHEST + 1,
    ID_ListView,
    ID_LocationChoice,
    ID_UpLevelButton,
    ID_ChangeViewButton,
    ID_ChangeView_LargeIcon,
    ID_ChangeView_SmallIcon,
    ID_ChangeView_List,
    ID_ChangeView_Details,
    ID_DoExpandFolder
};


BEGIN_EVENT_TABLE(FsPanel, wxWindow)
    EVT_SIZE(FsPanel::onSize)
    EVT_MENU(ID_DoExpandFolder, FsPanel::onDoExpandFolder)
    EVT_CHOICE(ID_LocationChoice, FsPanel::onLocationChoice)
    EVT_BUTTON(ID_UpLevelButton, FsPanel::onUpLevelClicked)
    EVT_BUTTON(ID_ChangeViewButton, FsPanel::onChangeViewClicked)
    EVT_MENU(ID_ChangeView_LargeIcon, FsPanel::onChangeView)
    EVT_MENU(ID_ChangeView_SmallIcon, FsPanel::onChangeView)
    EVT_MENU(ID_ChangeView_List, FsPanel::onChangeView)
    EVT_MENU(ID_ChangeView_Details, FsPanel::onChangeView)
    EVT_ERASE_BACKGROUND(FsPanel::onEraseBackground)
    EVT_CHILD_FOCUS(FsPanel::onChildFocus)
    
    // treeview events
    EVT_TREE_ITEM_RIGHT_CLICK(ID_TreeView, FsPanel::onFsTreeItemRightClicked)
    EVT_TREE_ITEM_MIDDLE_CLICK(ID_TreeView, FsPanel::onFsTreeItemMiddleClicked)
    EVT_TREE_ITEM_ACTIVATED(ID_TreeView, FsPanel::onFsTreeItemActivated)
    EVT_TREE_ITEM_EXPANDING(ID_TreeView, FsPanel::onFsTreeItemExpanding)
    EVT_TREE_ITEM_COLLAPSING(ID_TreeView, FsPanel::onFsTreeItemCollapsing)
    EVT_TREE_BEGIN_LABEL_EDIT(ID_TreeView, FsPanel::onFsTreeItemBeginLabelEdit)
    EVT_TREE_END_LABEL_EDIT(ID_TreeView, FsPanel::onFsTreeItemEndLabelEdit)
    EVT_TREE_KEY_DOWN(ID_TreeView, FsPanel::onFsTreeKeyDown)
    EVT_TREE_BEGIN_DRAG(ID_TreeView, FsPanel::onFsTreeBeginDrag)
    EVT_TREE_SEL_CHANGED(ID_TreeView, FsPanel::onFsTreeSelectionChanged)

    // listview events
    EVT_LIST_ITEM_RIGHT_CLICK(ID_ListView, FsPanel::onFsListItemRightClicked)
    EVT_LIST_ITEM_MIDDLE_CLICK(ID_ListView, FsPanel::onFsListItemMiddleClicked)
    EVT_LIST_ITEM_ACTIVATED(ID_ListView, FsPanel::onFsListItemActivated)
    EVT_LIST_BEGIN_LABEL_EDIT(ID_ListView, FsPanel::onFsListItemBeginLabelEdit)
    EVT_LIST_END_LABEL_EDIT(ID_ListView, FsPanel::onFsListItemEndLabelEdit)
    EVT_LIST_KEY_DOWN(ID_ListView, FsPanel::onFsListKeyDown)
    EVT_LIST_BEGIN_DRAG(ID_ListView, FsPanel::onFsListBeginDrag)
END_EVENT_TABLE()


FsPanel::FsPanel()
{
    m_main_sizer = NULL;
    m_location_bar = NULL;
    m_treeview = NULL;
    m_listview = NULL;
    
    m_location_choice = NULL;
    m_up_level_button = NULL;
    m_change_view_button = NULL;

    m_dragdrop_enabled = true;
    m_show_location_bar = false;
    m_multi_select = true;
    m_last_view = fsviewUndefined;
    m_view = fsviewUndefined;
    
    m_treeview_middledown_action_id = wxTreeItemId();
    
    m_style = 0;
    
    m_root.clear();
}


FsPanel::~FsPanel()
{
    sigDestructing().fire();
    destroyTreeView();
    destroyListView();
}


void FsPanel::createLocationBar()
{
    // if we've already created a treeview, don't do anything
    if (m_location_bar)
        return;

    wxStaticText* static_lookin = new wxStaticText(this, -1, _("Look in:"));

    m_location_choice = new wxChoice(this,
                                     ID_LocationChoice,
                                     wxDefaultPosition,
                                     wxDefaultSize);

    wxBitmap bmp_folder_open = wxBitmap(xpm_folder_open);
    wxBitmap bmp_folder_closed = wxBitmap(xpm_folder_closed);

    m_up_level_button = new kcl::Button(this,
                                        ID_UpLevelButton,
                                        wxDefaultPosition,
                                        wxSize(23,22),
                                        wxT(""),
                                        bmp_folder_open);

    m_change_view_button = new kcl::Button(this,
                                           ID_ChangeViewButton,
                                           wxDefaultPosition,
                                           wxSize(23,22),
                                           wxT(""),
                                           bmp_folder_closed);

    // create the location bar
    m_location_bar = new wxBoxSizer(wxHORIZONTAL);
    m_location_bar->Add(static_lookin, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5);
    m_location_bar->Add(m_location_choice, 1, wxALIGN_CENTER | wxRIGHT, 5);
    m_location_bar->Add(m_up_level_button, 0, wxALIGN_CENTER | wxRIGHT, 2);
    m_location_bar->Add(m_change_view_button, 0, wxALIGN_CENTER | wxRIGHT, 15);
}

void FsPanel::createTreeView()
{
    // if we've already created a treeview, don't do anything
    if (m_treeview)
        return;

    unsigned int style = wxTR_HAS_BUTTONS |
                         wxNO_BORDER |
                         wxTR_EDIT_LABELS |
                         wxTR_NO_LINES;
           
    if (m_multi_select)
        style |= wxTR_MULTIPLE;

    if (m_style & fsstyleTreeHideRoot)
        style |= wxTR_HIDE_ROOT;

#ifdef CFW_USE_GENERIC_TREECTRL
    style |= wxTR_NO_LINES;
    style |= wxTR_HAS_VARIABLE_ROW_HEIGHT;
#endif


    m_treeview = new FsPanelTreeView(this,
                                     ID_TreeView,
                                     wxPoint(0,0),
                                     wxSize(1,1),
                                     style);
    m_treeview->Connect(wxEVT_MOTION, wxMouseEventHandler(FsPanel::onMouseMove), NULL, this);
    m_treeview->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(FsPanel::onMouseLeftUp), NULL, this);
    m_treeview->Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(FsPanel::onMouseMiddleDown), NULL, this);
    m_treeview->Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(FsPanel::onMouseMiddleUp), NULL, this);
    
#ifdef __WXMSW__
    int treeitem_height = getTreeItemHeight(m_treeview);
    setTreeItemHeight(m_treeview, treeitem_height+2);
    
    if (m_style & fsstyleTrackSelect)
    {
        HWND h = (HWND)m_treeview->GetHWND();
        LONG l = GetWindowLongA(h, GWL_STYLE);
        l |= 0x200; /*TVS_TRACKSELECT*/
        SetWindowLongA(h, GWL_STYLE, l);
    }
#endif
    
    m_treeview->SetDropTarget(new FsTreeDropTarget(this));
    m_treeview->SetQuickBestSize(false);
    
#ifdef CFW_USE_GENERIC_TREECTRL
    m_treeview->SetIndent(16);
#endif

}


void FsPanel::createListView()
{
    // if we've already created a listview, don't do anything
    if (m_listview)
        return;

    unsigned int style = wxNO_BORDER | wxLC_EDIT_LABELS;

    switch (m_view)
    {
        default:
        case fsviewList:        style |= wxLC_LIST;          break;
        case fsviewLargeIcon:   style |= wxLC_ICON;          break;
        case fsviewSmallIcon:   style |= wxLC_SMALL_ICON;    break;
        case fsviewDetails:     style |= wxLC_REPORT;        break;
    }
            
    if (!m_multi_select)
        style |= wxLC_SINGLE_SEL;

    m_listview = new FsPanelListView(this,
                                     ID_ListView,
                                     wxPoint(0,0),
                                     wxSize(1,1),
                                     style);
}


void FsPanel::destroyTreeView()
{
    // if there is no treeview, don't do anything
    if (!m_treeview)
        return;

    m_treeview->Destroy();
    m_treeview = NULL;
}


void FsPanel::destroyListView()
{
    // if there is no listview, don't do anything
    if (!m_listview)
        return;

    m_listview->Destroy();
    m_listview = NULL;
}


void FsPanel::handleLayout()
{    
    if (m_view == fsviewUndefined)
        return;

    if (m_view == fsviewTree)
    {
        if (m_location_choice->IsShown())
            m_location_bar->Show(false);
        
        if (m_listview->IsShown())
            m_listview->Show(false);
            
        if (!m_treeview->IsShown())
            m_treeview->Show(true);
    }
     else
    {
        if (m_show_location_bar && !m_location_choice->IsShown())
            m_location_bar->Show(true);
        
        if (!m_listview->IsShown())
            m_listview->Show(true);
            
        if (m_treeview->IsShown())
            m_treeview->Show(false);
    }
    
    Layout();
}


void FsPanel::destroy()
{
    Destroy();
}

bool FsPanel::create(wxWindow* parent,
                     wxWindowID id,
                     const wxPoint& position,
                     const wxSize& size,
                     int flags)
{
    // create document's window
    bool result = Create(parent,
                         -1,
                         position,
                         size,
                          wxNO_FULL_REPAINT_ON_RESIZE |
                          wxCLIP_CHILDREN |
                          flags);

    if (!result)
        return false;
    
    // set the background color to white... this will help eliminate flicker
    SetBackgroundColour(*wxWHITE);
    
    createLocationBar();
    createTreeView();
    createListView();
    
    m_location_bar->Show(false);
    m_listview->Show(false);
    m_treeview->Show(false);
    
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->Add(m_location_bar, 0, wxEXPAND);
    m_main_sizer->Add(m_treeview, 1, wxEXPAND);
    m_main_sizer->Add(m_listview, 1, wxEXPAND);
    SetSizer(m_main_sizer);
    
    // this function calls Layout() for us
    handleLayout();
    return true;
}
                        
bool FsPanel::initDoc(IFramePtr frame,
                      IDocumentSitePtr doc_site,
                      wxWindow* docsite_wnd,
                      wxWindow* panesite_wnd)
{
    // create document's window
    bool result = create(docsite_wnd,
                         -1,
                         wxDefaultPosition,
                         docsite_wnd->GetClientSize(),
                         0);

    if (result && doc_site.isOk())
    {
        doc_site->setCaption(_("Filesystem Panel"));
        wxSize min_site_size = doc_site->getContainerWindow()->GetSize();
        doc_site->setMinSize(50, 100);
    }

    return result;
}

wxWindow* FsPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void FsPanel::setDocumentFocus()
{
    switch(m_view)
    {
        default:
            break;
            
        case fsviewTree:
            m_treeview->SetFocus();
            break;

        case fsviewList:
        case fsviewLargeIcon:
        case fsviewSmallIcon:
        case fsviewDetails:
            m_listview->SetFocus();
            break;
    }
}

void FsPanel::setView(int val)
{
    if (val == m_last_view)
        return;
        
    m_last_view = m_view;
    m_view = val;
    
    if (m_view == fsviewTree)
    {
        if (!m_treeview)
            return;
    }

    if (!m_listview)
        return;
        
    switch (m_view)
    {
        case fsviewLargeIcon:
        {
            m_listview->SetWindowStyleFlag(wxLC_ICON);
            m_listview->setImageListLarge(true);
            break;
        }
        
        case fsviewSmallIcon:
        {
            m_listview->SetWindowStyleFlag(wxLC_SMALL_ICON);
            m_listview->setImageListLarge(false);
            break;
        }
        
        case fsviewDetails:
        {
            m_listview->SetWindowStyleFlag(wxLC_REPORT);
            m_listview->setImageListLarge(false);
            break;
        }
        
        case fsviewList:
        {
            m_listview->SetWindowStyleFlag(wxLC_LIST);
            m_listview->setImageListLarge(false);
            break;
        }
    }
}

void FsPanel::setMultiSelect(bool val)
{
    m_multi_select = val;
    
    if (m_treeview)
    {
        long flags = m_treeview->GetWindowStyle();
        if (val == true)
        {
            m_treeview->SetWindowStyle(flags | wxTR_MULTIPLE);
        }
         else
        {
            // track select requires multiple selection
            if (m_style & fsstyleTrackSelect)
                return;
            
            m_treeview->SetWindowStyle(flags & ~wxTR_MULTIPLE);
        }
    }
}

void FsPanel::showLocationBar(bool show)
{
    m_show_location_bar = show;
}

bool FsPanel::isItemExpanded(IFsItemPtr item)
{
    if (item.isNull())
        return false;

    if (m_view == fsviewTree)
        return m_treeview->IsExpanded(item->getTreeItemId());

    return false;
}

IFsItemPtr FsPanel::getItemFromId(wxTreeItemId& id)
{
    if (m_view == fsviewTree)
        return m_treeview->getItemFromId(id);

    return xcm::null;
}

IFsItemPtr FsPanel::getItemParent(IFsItemPtr item)
{
    if (item.isNull())
        return xcm::null;

    if (m_view == fsviewTree)
        return m_treeview->getItemParent(item);

    return xcm::null;
}

IFsItemEnumPtr FsPanel::getItemChildren(IFsItemPtr item)
{
    if (item.isNull())
        return xcm::null;

    if (m_view == fsviewTree)
        return m_treeview->getItemChildren(item);

    return xcm::null;
}

IFsItemEnumPtr FsPanel::getSelectedItems()
{
    if (m_view == fsviewTree)
        return m_treeview->getSelectedItems();

    if (m_view == fsviewList)
        return m_listview->getSelectedItems();

    return xcm::null;
}

IFsItemPtr FsPanel::getSelectedItem()
{
    if (m_view == fsviewTree)
        return m_treeview->getSelectedItem();

    return xcm::null;
}

void FsPanel::editLabel(IFsItemPtr item)
{
    if (item.isNull())
        return;

    if (m_view == fsviewTree)
        m_treeview->EditLabel(item->getTreeItemId());
}

void FsPanel::remove(IFsItemPtr item)
{
    if (item.isNull())
        return;

    if (m_view == fsviewTree)
        m_treeview->remove(item);
}

void FsPanel::expand(IFsItemPtr item)
{
    if (item.isNull())
        return;

    if (m_view == fsviewTree)
        m_treeview->expand(item);
}

void FsPanel::collapse(IFsItemPtr item)
{
    if (item.isNull())
        return;

    if (m_view == fsviewTree)
        m_treeview->collapse(item);
}

IFsItemPtr FsPanel::appendItem(IFsItemPtr parent,
                               const wxString& text,
                               const wxBitmap& bitmap)
{
    if (parent.isNull())
        return xcm::null;

    if (m_view == fsviewTree)
    {
        return m_treeview->appendItem(parent,
                                      text,
                                      bitmap);
    }

    return xcm::null;
}

IFsItemPtr FsPanel::insertItem(IFsItemPtr parent,
                               IFsItemPtr previous_item,
                               const wxString& text,
                               const wxBitmap& bitmap)
{
    if (parent.isNull())
        return xcm::null;

    if (m_view == fsviewTree)
    {
        return m_treeview->insertItem(parent,
                                      previous_item,
                                      text,
                                      bitmap);
    }

    return xcm::null;
}

IFsItemPtr FsPanel::hitTest(int x, int y)
{
    if (m_view == fsviewTree)
    {
        int flags = 0;
        wxTreeItemId item = m_treeview->HitTest(wxPoint(x, y), flags);
        return m_treeview->getItemFromId(item);
    }
     else
    {
        wxFAIL_MSG(wxT("hitText() not implemented for non-tree fspanels yet"));
    }
    
    return xcm::null;
}

int FsPanel::getItemIndex(IFsItemPtr _item)
{
    if (m_view == fsviewTree)
    {
        wxTreeItemId item = _item->getTreeItemId();
        if (!item.IsOk())
            return -1;
        
        int i = 0;
        
        wxTreeItemId prev;
        while (1)
        {
            prev = m_treeview->GetPrevSibling(item);
            if (!prev.IsOk())
                return i;
            item = prev;
            ++i;
        }
        
        // never arrives here
        return -1;
    }
     else
    {
        wxFAIL_MSG(wxT("hitText() not implemented for non-tree fspanels yet"));
    }
    
    return -1;
}

    
void FsPanel::setItemBitmap(IFsItemPtr item, const wxBitmap& bmp, unsigned int bitmap_type)
{
    if (item.isNull())
        return;

    if (m_view == fsviewTree)
        m_treeview->setItemBitmap(item, bmp, bitmap_type);
}

void FsPanel::setItemText(IFsItemPtr item, const wxString& text)
{
    if (item.isNull())
        return;

    if (m_view == fsviewTree)
        m_treeview->SetItemText(item->getTreeItemId(), text);
}

void FsPanel::setItemBold(IFsItemPtr item, bool bold)
{
    if (item.isNull())
        return;

    if (m_view == fsviewTree)
        m_treeview->SetItemBold(item->getTreeItemId(), bold);
}

void FsPanel::selectItem(IFsItemPtr item)
{
    if (item.isNull())
        return;
    
    if (m_view == fsviewTree)
        m_treeview->selectItem(item);
}

void FsPanel::unselect()
{
    if (m_view == fsviewTree)
        m_treeview->Unselect();
}

void FsPanel::unselectAll()
{
    if (m_view == fsviewTree)
        m_treeview->UnselectAll();
}

void FsPanel::refreshItem(IFsItemPtr item)
{
    if (m_view == fsviewUndefined)
        return;
        
    if (item.isNull())
        return;

    if (m_view == fsviewTree)
        m_treeview->refreshItem(item);
}

void FsPanel::refreshAll()
{
    if (m_view == fsviewUndefined)
        return;
        
    if (m_view == fsviewTree)
        m_treeview->refreshAll();
}

void FsPanel::refresh()
{
    if (m_view == fsviewUndefined)
        return;
        
    if (m_view == fsviewTree)
    {
        m_treeview->refresh();
    }
     else
    {
        if (m_show_location_bar)
            refreshLocationBar();
            
        m_listview->refresh();
    }

    handleLayout();
}

void FsPanel::refreshLocationBar()
{
    if (m_root.isNull())
        return;

    int count = m_location_choice->GetCount();
    
    if (count > 0)
    {
        FsItemData* data = (FsItemData*)m_location_choice->GetClientData(0);
    
        // if we've switched root items, we need
        // to repopulate the choice control

        if (m_root != data->m_fsitem)
            m_location_choice->Clear();
    }
    

    // if the location choice is empty, get things started

    if (count == 0)
    {
        m_location_choice->Append(m_root->getLabel());
        
        // create our intial FsItemData
        FsItemData* data = new FsItemData;
        data->m_deferred = true;
        data->m_fsitem = m_root;
        
        m_location_choice->SetClientData(0, data);
        populateLocationChoiceDeferred(0);
        m_location_choice->SetSelection(0);
    }
}

void FsPanel::changeLocation(IFsItemPtr item)
{
    // if the item we clicked on is not a directory item,
    // we can't change the location
    
    IDirectoryFsItemPtr dir_item = item;
    if (!dir_item)
        return;
        
    if (m_view == fsviewList ||
        m_view == fsviewLargeIcon ||
        m_view == fsviewSmallIcon ||
        m_view == fsviewDetails)
    {
        if (m_listview)
        {
            m_listview->Freeze();
            m_listview->setParentItem(item);
            m_listview->populate();
            m_listview->Thaw();
        }
        
        if (m_location_choice)
        {
            int idx;
            int count = m_location_choice->GetCount();
            
            wxString dir_path = dir_item->getPath();
            wxString choice_path;
            
            for (idx = 0; idx < count; ++idx)
            {
                FsItemData* data = (FsItemData*)m_location_choice->GetClientData(idx);
                
                // only look at directory items
                IDirectoryFsItemPtr choice_dir_item = data->m_fsitem;
                if (!choice_dir_item)
                    continue;
                    
                choice_path = choice_dir_item->getPath();
                if (!choice_path.CmpNoCase(dir_path))
                    break;
            }
            
            m_location_choice->SetSelection(idx);
            populateLocationChoiceDeferred(idx);
        }
    }
}

void FsPanel::populateLocationChoiceDeferred(int idx)
{
    FsItemData* data = (FsItemData*)m_location_choice->GetClientData(idx);
    if (!data->m_deferred)
        return;

    data->m_deferred = false;

    if (data->m_fsitem)
    {
        wxBusyCursor bc;
        
        IFsItemEnumPtr items = data->m_fsitem->getChildren();
        populateLocationChoiceInternal(idx, items);
    }
}

void FsPanel::populateLocationChoiceInternal(int idx, IFsItemEnumPtr items)
{
    int i;
    int count = items->size();

    for (i = 0; i < count; i++)
    {
        IFsItemPtr item = items->getItem(i);
        
        IDirectoryFsItemPtr dir_item = item;
        if (!dir_item)
            continue;
            
        m_location_choice->Insert(item->getLabel(), ++idx);
        
        FsItemData* data = new FsItemData;
        data->m_deferred = true;
        data->m_fsitem = item;
        
        m_location_choice->SetClientData(idx, data);
    }
}

IFsItemPtr FsPanel::getRootItem()
{
    return m_root;
}

void FsPanel::setRootItem(IFsItemPtr item)
{
    m_root = item;

    if (item)
    {
        wxTreeItemId null_item;
        item->setTreeItemId(null_item);
    }
    
    if (m_treeview)
        m_treeview->setRootItem(item);
        
    if (m_listview)
        m_listview->setRootItem(item);
}

void FsPanel::getVirtualSize(int* width, int* height)
{
    if (m_view == fsviewUndefined)
    {
        *width = 0;
        *height = 0;
    }
     else if (m_view == fsviewTree)
    {
        m_treeview->InvalidateBestSize();
        m_treeview->GetBestSize(width, height);
    }
     else
    {
        // this should just return the client size
        m_listview->GetVirtualSize(width, height);
    }
}

int FsPanel::popupMenu(wxMenu* menu)
{
    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);

    CommandCapture* cc = new CommandCapture;
    PushEventHandler(cc);
    PopupMenu(menu, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);

    return command;
}

void FsPanel::setDragDrop(bool enable_dragdrop)
{
    m_dragdrop_enabled = enable_dragdrop;
}

bool FsPanel::isDragDropEnabled()
{
    return m_dragdrop_enabled;
}


// event handlers

void FsPanel::onSize(wxSizeEvent& evt)
{
    Layout();
}

void FsPanel::onEraseBackground(wxEraseEvent& evt)
{
}

void FsPanel::onChildFocus(wxChildFocusEvent& evt)
{
    m_focus_stopwatch.Start();
}


void FsPanel::onLocationChoice(wxCommandEvent& evt)
{
    int idx = evt.GetInt();
    FsItemData* data = (FsItemData*)m_location_choice->GetClientData(idx);

    populateLocationChoiceDeferred(idx);
    changeLocation(data->m_fsitem);
}


void FsPanel::onUpLevelClicked(wxCommandEvent& evt)
{

}


void FsPanel::onChangeViewClicked(wxCommandEvent& evt)
{
    wxMenu menuPopup;
    menuPopup.Append(ID_ChangeView_LargeIcon, _("Large Icons"));
    menuPopup.Append(ID_ChangeView_SmallIcon, _("Small Icons"));
    menuPopup.Append(ID_ChangeView_List, _("List"));
    //menuPopup.Append(ID_ChangeView_Details, _("Details"));

    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);
    PopupMenu(&menuPopup, pt_mouse);
}

void FsPanel::onChangeView(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
        case ID_ChangeView_LargeIcon:   setView(fsviewLargeIcon);   break;
        case ID_ChangeView_SmallIcon:   setView(fsviewSmallIcon);   break;
        case ID_ChangeView_Details:     setView(fsviewDetails);     break;
        case ID_ChangeView_List:        setView(fsviewList);        break;
    }
}

void FsPanel::onMouseMove(wxMouseEvent& evt)
{
    // let tree run its default handler for mouse move --
    // this is necessary in track select mode
    evt.Skip();
    
    if (!m_treeview)
        return;
    
    // make sure our middle click action id is zeroed out if the
    // middle button is not down
    if (m_treeview_middledown_action_id.IsOk())
    {
        if (!::wxGetMouseState().MiddleIsDown())
            m_treeview_middledown_action_id = wxTreeItemId();
    }
    
    // translate from FsPanelTreeView coordiates to FsPanel coordinates
    wxCoord x = evt.GetX();
    wxCoord y = evt.GetY();
    m_treeview->ClientToScreen(&x, &y);
    ScreenToClient(&x, &y);
    sigMouseMove().fire(x,y);
}



void FsPanel::onMouseMiddleDown(wxMouseEvent& evt)
{
#ifdef CFW_USE_GENERIC_TREECTRL
    // do nothing (EVT_TREE_ITEM_MIDDLE_CLICK works with the generic treectrl)
#else
    // treectrl only for now
    if (!m_treeview)
        return;

    int flags = 0;
    m_treeview_middledown_action_id = m_treeview->HitTest(evt.GetPosition(), flags);
#endif
}

void FsPanel::onMouseMiddleUp(wxMouseEvent& evt)
{
#ifdef CFW_USE_GENERIC_TREECTRL
    // do nothing (EVT_TREE_ITEM_MIDDLE_CLICK works with the generic treectrl)
#else
    // treectrl only for now
    if (!m_treeview)
        return;

    if (!m_treeview_middledown_action_id.IsOk())
        return;
    
    int flags = 0;
    wxTreeItemId action_id = m_treeview->HitTest(evt.GetPosition(), flags);

    // middle-down and middle-up were on the same item, so fire an event
    if (action_id == m_treeview_middledown_action_id)
    {
        // fire a middle-click signal here (perhaps in the future
        // we'll fire a _REAL_ wxTreeEvent instead -- for now, this works)
        FsItemData* data = (FsItemData*)m_treeview->GetItemData(action_id);
        data->m_fsitem->onMiddleClicked();

        sigItemMiddleClicked().fire(data->m_fsitem);
    }
        
    // zero out our action id
    m_treeview_middledown_action_id = wxTreeItemId();
#endif
}




// treeview event handlers

void FsPanel::onFsTreeItemRightClicked(wxTreeEvent& evt)
{
    FsItemData* data = (FsItemData*)m_treeview->GetItemData(evt.GetItem());
    data->m_fsitem->onRightClicked();

    sigItemRightClicked().fire(data->m_fsitem);
}

// EVT_TREE_ITEM_MIDDLE_CLICK only seems to be implemented for the 
// generic tree control, so we're out of luck for now on Windows
void FsPanel::onFsTreeItemMiddleClicked(wxTreeEvent& evt)
{
    FsItemData* data = (FsItemData*)m_treeview->GetItemData(evt.GetItem());
    data->m_fsitem->onMiddleClicked();

    sigItemMiddleClicked().fire(data->m_fsitem);
}

void FsPanel::onFsTreeItemActivated(wxTreeEvent& evt)
{
    wxTreeItemId item = evt.GetItem();
    
    if (!item.IsOk())
    {
        // somebody's bug / assertion failure leads to a bad/empty tree item
        // being passed here.  The steps to duplicate are:
        // 1) open a report
        // 2) drag in a file from the tree
        // 3) type in "ASDF"
        // 4) type in <ENTER>
        // 5) boom.
        return;
    }
        
    FsItemData* data = (FsItemData*)m_treeview->GetItemData(item);
    if (data)
    {
        data->m_fsitem->onActivated();
        sigItemActivated().fire(data->m_fsitem);
    }
     else
    {
        IFsItemEnumPtr sel_items = getSelectedItems();

        if (sel_items->size() == 1)
        {
            IFsItemPtr item = sel_items->getItem(0);
            item->onActivated();
            sigItemActivated().fire(item);
        }
    }
}


void FsPanel::onFsTreeItemBeginLabelEdit(wxTreeEvent& evt)
{
#ifdef __WXMSW__
    // this code will prevent the scenario where a tree item
    // label is edited where the user only really wanted
    // to set the focus to the tree control
    if (m_focus_stopwatch.Time() < 1000)
    {
        evt.Veto();
        return;
    }
#endif
    
        
    // this next section is really a band-aid for the fact
    // that the wx tree control starts an item rename when
    // multiple tree items are selected
    IFsItemEnumPtr sel_items = getSelectedItems();
    if (sel_items->size() > 1)
    {
        unselectAll();

        wxTreeItemId id = evt.GetItem();
        IFsItemPtr item = getItemFromId(id);

        if (item.isOk())
        {
            selectItem(item);
        }

        evt.Veto();
        return;
    }
    // end band-aid


    FsItemData* data = (FsItemData*)m_treeview->GetItemData(evt.GetItem());

    bool allow = true;
    sigItemBeginLabelEdit().fire(data->m_fsitem, &allow);
    if (!allow)
    {
        evt.Veto();
    }
}


void FsPanel::onFsTreeItemEndLabelEdit(wxTreeEvent& evt)
{
    FsItemData* data = (FsItemData*)m_treeview->GetItemData(evt.GetItem());
    IFsItemPtr edit_item = data->m_fsitem;

#ifdef CFW_USE_GENERIC_TREECTRL
    // FIXME: This code needs to be fixed on linux
    /*
    if (event.GetItem() == m_delete_item)
    {
        // see remove() - this item is being deleted, so don't notify
        // the caller
        m_delete_item.Unset();
        return;
    }
    */
#endif
    

    bool allow = true;
    sigItemEndLabelEdit().fire(data->m_fsitem,
                               evt.GetLabel(),
                               evt.IsEditCancelled(),
                               &allow);

    if (!allow)
    {
        evt.Veto();
        return;
    }

    if (edit_item)
    {
        edit_item->onRenamed(evt.GetLabel());
    }
}


void FsPanel::onFsTreeBeginDrag(wxTreeEvent& evt)
{
    // if drag&drop is disabled, don't do anything
    if (!isDragDropEnabled())
        return;

    FsDataObject data;
    data.setFsItems(getSelectedItems());
    wxDropSource dragSource(data, this);
    wxDragResult result = dragSource.DoDragDrop(TRUE);
}


void FsPanel::onDoExpandFolder(wxCommandEvent& evt)
{
}

void FsPanel::onFsTreeItemExpanding(wxTreeEvent& evt)
{
    // if we are expanding a deferred folder, populate its children first
    wxTreeItemId id = evt.GetItem();

    FsItemData* data = (FsItemData*)m_treeview->GetItemData(id);
    if (data->m_deferred)
    {
        m_treeview->Freeze();
        m_treeview->populateDeferred(id); 
        m_treeview->Thaw();
    }
}


void FsPanel::onFsTreeItemCollapsing(wxTreeEvent& evt)
{
    // prevent collapsing of the root item
    if (evt.GetItem() == m_treeview->GetRootItem())
    {
        evt.Veto();
        return;
    }

    evt.Skip();
}


void FsPanel::onFsTreeKeyDown(wxTreeEvent& evt)
{
    bool handled = false;
    sigKeyDown().fire(evt.GetKeyEvent(), &handled);

    if (!handled)
    {
        evt.Skip();
    }
}



void FsPanel::onMouseLeftUp(wxMouseEvent& evt)
{
    wxCoord x = evt.GetX();
    wxCoord y = evt.GetY();
    sigMouseLeftUp().fire(x,y);
        
    if (m_style & fsstyleTrackSelect)
    {
        m_treeview->UnselectAll();
        
        int hit_flags = 0;
        wxTreeItemId id = m_treeview->HitTest(wxPoint(x,y), hit_flags);
        if (id.IsOk() &&
                 (hit_flags & wxTREE_HITTEST_ONITEMLABEL ||
                  hit_flags & wxTREE_HITTEST_ONITEMICON))
        {
            wxTreeEvent e;
            e.SetItem(id);
            onFsTreeItemActivated(e);
        }
    }
}


void FsPanel::onFsTreeSelectionChanged(wxTreeEvent& evt)
{
    wxTreeItemId id = evt.GetItem();

    if (!id.IsOk())
        return;

    FsItemData* data = (FsItemData*)m_treeview->GetItemData(id);
    if (data)
    {
        sigItemSelected().fire(data->m_fsitem);
    }
}


// listview event handlers

void FsPanel::onFsListItemRightClicked(wxListEvent& evt)
{
    FsItemData* data = (FsItemData*)m_listview->GetItemData(evt.GetIndex());
    data->m_fsitem->onRightClicked();

    sigItemRightClicked().fire(data->m_fsitem);
}

void FsPanel::onFsListItemMiddleClicked(wxListEvent& evt)
{
    FsItemData* data = (FsItemData*)m_listview->GetItemData(evt.GetIndex());
    data->m_fsitem->onMiddleClicked();

    sigItemMiddleClicked().fire(data->m_fsitem);
}

void FsPanel::onFsListItemActivated(wxListEvent& evt)
{
    FsItemData* data = (FsItemData*)m_listview->GetItemData(evt.GetIndex());
    data->m_fsitem->onActivated();

    sigItemActivated().fire(data->m_fsitem);
    
    changeLocation(data->m_fsitem);
}



void FsPanel::onFsListItemBeginLabelEdit(wxListEvent& evt)
{
    FsItemData* data = (FsItemData*)m_listview->GetItemData(evt.GetIndex());

    bool allow = true;
    sigItemBeginLabelEdit().fire(data->m_fsitem, &allow);
    
    if (!allow)
        evt.Veto();
}


void FsPanel::onFsListItemEndLabelEdit(wxListEvent& evt)
{
    FsItemData* data = (FsItemData*)m_listview->GetItemData(evt.GetIndex());
    IFsItemPtr edit_item = data->m_fsitem;

    bool allow = true;
    sigItemEndLabelEdit().fire(data->m_fsitem,
                               evt.GetLabel(),
                               evt.IsEditCancelled(),
                               &allow);

    if (!allow)
    {
        evt.Veto();
        return;
    }

    if (edit_item)
    {
        edit_item->onRenamed(evt.GetLabel());
    }
}


void FsPanel::onFsListBeginDrag(wxListEvent& evt)
{
    FsDataObject data;
    data.setFsItems(getSelectedItems());
    wxDropSource dragSource(data, this);
    wxDragResult result = dragSource.DoDragDrop(TRUE);
}


void FsPanel::onFsListKeyDown(wxListEvent& evt)
{
    wxKeyEvent e;
    e.m_keyCode = evt.GetKeyCode();

    bool handled = false;
    sigKeyDown().fire(e, &handled);
    if (!handled)
        evt.Skip();
}


