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
#include <wx/imaglist.h>
#include <kl/xcm.h>
#include <set>
#include "framework.h"
#include "fspanel.h"
#include "fspaneltreeview.h"
#include "fsitems_private.h"
#include "fspanel_private.h"


#ifdef __WXMSW__

#include <windows.h>
#include <commctrl.h>

#ifndef SendMessage
#ifdef _UNICODE
#define SendMessage SendMessageW
#else
#define SendMessage SendMessageA
#endif
#endif

#endif




static wxString _getStringPath(CfwTreeCtrl* tree, wxTreeItemId& id)
{
    wxString result;

    // get base
    result = tree->GetItemText(id);
    wxTreeItemId parent = tree->GetItemParent(id);
    while (parent.IsOk())
    {
        if (parent == tree->GetRootItem())
            break;
        result += wxT("||");
        result += tree->GetItemText(parent);
        parent = tree->GetItemParent(parent);
    }

    return result;
}


static void _populateFolderStatus(CfwTreeCtrl* tree,
                                  wxTreeItemId& id,
                                  std::set<wxString>& expanded)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = tree->GetFirstChild(id, cookie);
    while (child.IsOk())
    {
        if (tree->ItemHasChildren(child))
        {
            if (tree->IsExpanded(child))
            {
                expanded.insert(_getStringPath(tree, child));
            }

            _populateFolderStatus(tree, child, expanded);
        }

        child = tree->GetNextChild(id, cookie);
    }
}


static void _expandFolders(CfwTreeCtrl* tree,
                           wxTreeItemId& id,
                           std::set<wxString>& expanded)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = tree->GetFirstChild(id, cookie);
    while (child.IsOk())
    {
        if (tree->ItemHasChildren(child))
        {
            if (expanded.find(_getStringPath(tree, child)) != expanded.end())
            {
                tree->Expand(child);
            }

            _expandFolders(tree, child, expanded);
        }

        child = tree->GetNextChild(id, cookie);
    }
}




BEGIN_EVENT_TABLE(FsPanelTreeView, CfwTreeCtrl)
END_EVENT_TABLE()


FsPanelTreeView::FsPanelTreeView(wxWindow* parent, wxWindowID id,
                                 const wxPoint& pos, const wxSize& size,
                                 long flags)
                               : CfwTreeCtrl(parent, id, pos, size, flags)
{
    m_last_scroll = 0;
    m_highlight_item.m_pItem = 0;
    m_delete_item.m_pItem = 0;

    m_image_list = new wxImageList;
    m_image_list->Create(16, 16, true, 10);
    
    SetImageList(m_image_list);

    m_root.clear();
}


FsPanelTreeView::~FsPanelTreeView()
{
    if (m_image_list)
        delete m_image_list;
}


void FsPanelTreeView::highlightItem(const wxTreeItemId& id)
{
    if (m_highlight_item == id)
        return;
        
    if (m_highlight_item)
    {
        SetItemDropHighlight(m_highlight_item, false);
    }
    
    SetItemDropHighlight(id, true);
    
    //Refresh(FALSE);
    //Update();
    
    m_highlight_item = id;
}


void FsPanelTreeView::unhighlightItem()
{
    if (m_highlight_item)
    {
        SetItemDropHighlight(m_highlight_item, false);
    }
    
    m_highlight_item.m_pItem = 0;
}


wxTreeItemId FsPanelTreeView::getHighlightItem()
{
    return m_highlight_item;
}

// getNextVisibleItem() and getPrevVisibleItem() exist because wxTreeCtrl->GetPrevVisibleItem()
// doesn't work during drag and drop anymore.  To solve this, we implemented similar
// functionality ourselves.   See doScroll() below.

static wxTreeItemId getNextVisibleItem(CfwTreeCtrl* tree, wxTreeItemId item)
{
    wxTreeItemIdValue cookie;

    if (tree->ItemHasChildren(item) && tree->IsExpanded(item))
        return tree->GetFirstChild(item, cookie);
         else
        return tree->GetNextSibling(item);
}

static wxTreeItemId getPrevVisibleItem(CfwTreeCtrl* tree, wxTreeItemId item)
{
    wxTreeItemId t;
    t = tree->GetPrevSibling(item);
    if (!t.IsOk())
        return tree->GetItemParent(item);
         else
        item = t;

    while (item.IsOk() && tree->ItemHasChildren(item) && tree->IsExpanded(item))
        item = tree->GetLastChild(item);

    return item;
}

void FsPanelTreeView::doScroll(int direction)
{
    #ifndef CFW_USE_GENERIC_TREECTRL
    wxPoint pt = ScreenToClient(wxGetMousePosition());
    wxTreeItemId curitem = HitTest(pt);
    if (!curitem)
        return;

    EnsureVisible(curitem);

    if (direction > 0)
    {
        wxTreeItemId id = getNextVisibleItem(this, curitem);
        if (id.IsOk())
        {
            if (this->GetRootItem() == id && this->HasFlag(wxTR_HIDE_ROOT))
                return;

            EnsureVisible(id);
        }
    }
     else if (direction < 0)
    {
        wxTreeItemId id = getPrevVisibleItem(this, curitem);
        if (id.IsOk())
        {
            if (this->GetRootItem() == id && this->HasFlag(wxTR_HIDE_ROOT))
                return;

            EnsureVisible(id);
        }
    }
    #endif
}


int FsPanelTreeView::getImageListIdx(const wxBitmap& bitmap)
{
    if (!bitmap.Ok())
        return -1;

    std::vector<wxBitmap>::iterator it;
    int idx = 0;

    for (it = m_bitmaps.begin(); it != m_bitmaps.end(); ++it)
    {
        if (it->IsSameAs(bitmap))
        {
            return idx;
        }

        ++idx;
    }
    
    if (m_bitmaps.size() > 200)
        return -1;

    m_image_list->Add(bitmap);
    m_bitmaps.push_back(bitmap);

    return idx;
}


void FsPanelTreeView::setRootItem(IFsItemPtr item)
{
    m_root = item;
}


void FsPanelTreeView::selectItem(IFsItemPtr item)
{
    wxTreeItemId id = item->getTreeItemId();
    
    if ((GetWindowStyle() & wxTR_HIDE_ROOT) && GetRootItem() == id)
    {
        wxFAIL_MSG(wxT("can't select root item"));
        return;
    }

    
    SelectItem(id);


#ifdef __WXMSW__

    // this function will set the focus to this tree item
    
    // I'm not sure as to the extent that this is necessary anymore
    // vista tree's add some spin on this issue with the g_unlockedItem
    // global in treectrl.cpp

    HWND tree_hwnd = (HWND)GetHWND();

    if (!::IsWindowVisible(tree_hwnd))
        return;

    HTREEITEM oldfocus_item = (HTREEITEM)TreeView_GetSelection(tree_hwnd);
    HTREEITEM newfocus_item = (HTREEITEM)id.m_pItem;

    if (newfocus_item != oldfocus_item)
    {
        bool was_selected = IsSelected(id);

        if (oldfocus_item)
        {
            // find out if the old focus item was selected
            TV_ITEM tvi;
            tvi.mask = TVIF_STATE | TVIF_HANDLE;
            tvi.stateMask = TVIS_SELECTED;
            tvi.hItem = oldfocus_item;

            TreeView_GetItem(tree_hwnd, &tvi);
            
            if (tvi.state & TVIS_SELECTED)
            {
                TreeView_SelectItem(tree_hwnd, 0);

                tvi.stateMask = TVIS_SELECTED;
                tvi.state = TVIS_SELECTED;
                TreeView_SetItem(tree_hwnd, &tvi);
            }
        }

        TreeView_SelectItem(tree_hwnd, newfocus_item);

        if (!was_selected)
        {
            UnselectItem(id);
        }
    }
#endif
}


IFsItemPtr FsPanelTreeView::appendItem(IFsItemPtr parent,
                                       const wxString& text,
                                       const wxBitmap& bitmap)
{
    int image_list_idx = getImageListIdx(bitmap);

    // add the item to the tree
    wxTreeItemId id = AppendItem(parent->getTreeItemId(),
                                 text,
                                 image_list_idx);

    GenericFsItem* item = new GenericFsItem;
    item->setTreeItemId(id);
    item->setLabel(text);
    item->setBitmap(bitmap, fsbmpSmall);

    // set the tree control's FsItemData
    FsItemData* data;
    data = new FsItemData;
    data->m_fsitem = item;
    data->m_deferred = false;

    SetItemData(id, data);
    return static_cast<IFsItem*>(item);
}


IFsItemPtr FsPanelTreeView::insertItem(IFsItemPtr parent,
                                       IFsItemPtr previous_item,
                                       const wxString& text,
                                       const wxBitmap& bitmap)
{
    int image_list_idx = getImageListIdx(bitmap);
    wxTreeItemId previous_id;
    
    if (previous_item)
    {
        previous_id = previous_item->getTreeItemId();
    }

    // add the item to the tree
    wxTreeItemId id;
    
    if (previous_id.IsOk())
    {
        id = InsertItem(parent->getTreeItemId(),
                        previous_id,
                        text,
                        image_list_idx);
    }
     else
    {
        id = PrependItem(parent->getTreeItemId(),
                         text,
                         image_list_idx);
    }

    GenericFsItem* item = new GenericFsItem;
    item->setTreeItemId(id);
    item->setLabel(text);
    item->setBitmap(bitmap, fsbmpSmall);

    // set the tree control's FsItemData
    FsItemData* data;
    data = new FsItemData;
    data->m_fsitem = item;
    data->m_deferred = false;

    SetItemData(id, data);
    return static_cast<IFsItem*>(item);
}


void FsPanelTreeView::remove(IFsItemPtr item)
{
    // when deletion of an item which has an active edit control occurs,
    // wx's generic tree control will fire an end edit signal which we
    // do not want to forward to the user

    m_delete_item = item->getTreeItemId();
    if (m_delete_item.IsOk())
    {
        Delete(m_delete_item);
    }
}


void FsPanelTreeView::populate(IFsItemEnumPtr items,
                               const wxTreeItemId& parent_id)
{
    if (items.isNull() || items->size() == 0)
        return;

    FsItemData* data;
    wxTreeItemId id;
    int normal_image_idx;
    int expanded_image_idx;

    int count = items->size();
    int i;

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);

        // get the corresponding imagelist
        // index from bitmap of the FsItem

        wxBitmap normal_bitmap = item->getBitmap(fsbmpSmall);
        wxBitmap expanded_bitmap = item->getBitmap(fsbmpSmallExpanded);
        
        normal_image_idx = getImageListIdx(normal_bitmap);
        expanded_image_idx = getImageListIdx(expanded_bitmap);

        if (!parent_id)
        {
            // if no root exists, add the root
            id = AddRoot(item->getLabel(),
                         normal_image_idx);
        }
         else
        {
            // add the item to the tree
            id = AppendItem(parent_id,
                            item->getLabel(),
                            normal_image_idx);
        }

        if (expanded_image_idx != -1)
        {
            SetItemImage(id, expanded_image_idx, wxTreeItemIcon_Expanded);
        }
        
        // set the FsItem's wxTreeItemId
        item->setTreeItemId(id);

        // set the tree control's FsItemData
        data = new FsItemData;
        data->m_fsitem = item;
        data->m_deferred = false;
        SetItemData(id, data);


        if (parent_id && item->isDeferred())
        {
            // deferred item
            data->m_deferred = true;

            if (item->hasChildren())
            {
                // add a place-holder item
                AppendItem(id, wxT(""), 0);
            }
        }
         else
        {
            // recursively populate the tree with the
            // children of the current FsItem
            populate(item->getChildren(), id);
        }
    }
}


void FsPanelTreeView::populateDeferred(wxTreeItemId& id)
{
    FsItemData* data = (FsItemData*)GetItemData(id);
    if (!data->m_deferred)
        return;

    data->m_deferred = false;

    // delete place holder item
    wxTreeItemIdValue l;
    wxTreeItemId place_holder_item = GetFirstChild(id, l);
    if (place_holder_item.IsOk())
    {
        m_delete_item = place_holder_item; // see remove()
        Delete(place_holder_item);
    }

    if (data->m_fsitem)
    {
        wxBusyCursor bc;
        populate(data->m_fsitem->getChildren(), id);
    }
}


void FsPanelTreeView::expand(IFsItemPtr item)
{
    wxTreeItemId id = item->getTreeItemId();

    // don't try to expand the root item if it's hidden
    bool do_expand = true;
    if (id == GetRootItem() && GetWindowStyle() & wxTR_HIDE_ROOT)
        do_expand = false;

    FsItemData* data = (FsItemData*)GetItemData(id);
    if (data && data->m_deferred)
    {
        Freeze();
        populateDeferred(id);
        
        if (do_expand)
            Expand(id);
            
        Thaw();
    }
     else
    {
        if (do_expand)
            Expand(id);
    }
}


void FsPanelTreeView::collapse(IFsItemPtr item)
{
    Collapse(item->getTreeItemId());
}


void FsPanelTreeView::setItemBitmap(IFsItemPtr item, const wxBitmap& bmp, unsigned bitmap_type)
{
    wxTreeItemId id = item->getTreeItemId();
    int idx = getImageListIdx(bmp);
    SetItemImage(id, idx);
}


void FsPanelTreeView::refresh()
{   
    if (m_root.isOk())
    {
        if (m_root->getTreeItemId().IsOk())
        {
            refreshAll();
            return;
        }
    }
    
    // make sure the tree is shown before we freeze it...
    // this eliminates flicker on the first show
    Show();

    // don't show the tree while we populate it
    Freeze();

    // delete all tree items
    DeleteAllItems();
    
    // recursively populate the tree
    if (m_root)
    {
        wxTreeItemId id;

        xcm::IVectorImpl<IFsItemPtr>* vec;
        vec = new xcm::IVectorImpl<IFsItemPtr>;
        vec->append(m_root);

        populate(vec, id);
    }

    // if there is a root item for this tree, expand it
    if (!(GetWindowStyle() & wxTR_HIDE_ROOT))
    {
        wxTreeItemId root = GetRootItem();
        if (root.IsOk())
            Expand(root);
    }

    // now we can show the tree
    Thaw();
}


void FsPanelTreeView::refreshAll()
{
    if (m_root.isNull())
        return;

    int item = GetScrollPos(wxVERTICAL);
    if (item > 0)
    {
        // this allows us to scroll back to
        // the original position without a
        // serious amount of flicker.  The control
        // will flash once.  This is the best
        // alternative I could find.
        Show(false);
    }
     else
    {
        Freeze();
    }
    
    
    refreshItem(m_root);
    
    
    if (item > 0)
    {
        // see above comment
        SetScrollPos(wxVERTICAL, item, true);
        Show(true);
    }
     else
    {
        Thaw();
    }
}


void FsPanelTreeView::refreshItem(IFsItemPtr item)
{
    if (item.isNull())
        return;

    wxTreeItemId id = item->getTreeItemId();

    FsItemData* data = (FsItemData*)GetItemData(id);
    if (!data)
        return;
        
    if (data->m_deferred)
    {
        // deferred items don't need to have their children
        // repopulated, because they haven't been opened yet.
        // They do, hwoever, need their label updated;
        // See 
        if (GetEditControl() == NULL)
        {
            SetItemText(id, item->getLabel());
        }
        return;
    }

    // get folder status
    std::set<wxString> expanded_folders;
    _populateFolderStatus(this, id, expanded_folders);


    // don't show the tree while we populate it
    Freeze();

    // update the item's label
    SetItemText(id, item->getLabel());


    // remove all children of the item
    std::vector<wxTreeItemId> to_remove;

    wxTreeItemIdValue cookie;
    wxTreeItemId child = GetFirstChild(id, cookie);
    while (child.IsOk())
    {
        to_remove.push_back(child);
        child = GetNextChild(id, cookie);
    }

    std::vector<wxTreeItemId>::iterator it;
    for (it = to_remove.begin(); it != to_remove.end(); ++it)
    {
        m_delete_item = *it; // see remove()
        Delete(*it);
    }

    populate(item->getChildren(), id);

    _expandFolders(this, id, expanded_folders);

    // if there is a root item for this tree, expand it
    if (!(GetWindowStyle() & wxTR_HIDE_ROOT))
        Expand(GetRootItem());
    
    // now we can show the tree
    Thaw();
}


IFsItemPtr FsPanelTreeView::getItemParent(IFsItemPtr item)
{
    wxTreeItemId item_id = item->getTreeItemId();
    if (!item_id.IsOk())
        return xcm::null;
        
    wxTreeItemId parent_id = GetItemParent(item_id);
    if (!parent_id.IsOk())
        return xcm::null;

    FsItemData* data = (FsItemData*)GetItemData(parent_id);
    return data->m_fsitem;
}


IFsItemPtr FsPanelTreeView::getItemFromId(wxTreeItemId& id)
{
    if (!id.IsOk())
        return xcm::null;
        
    FsItemData* data = (FsItemData*)GetItemData(id);
    if (!data)
        return xcm::null;

    return data->m_fsitem;
}


IFsItemPtr FsPanelTreeView::getSelectedItem()
{
    if (GetWindowStyle() & wxTR_MULTIPLE)
    {
        wxArrayTreeItemIds selections;
        GetSelections(selections);

        if (selections.Count() <= 0)
            return xcm::null;

        FsItemData* data;
        data = (FsItemData*)GetItemData(selections.Item(0));

        if (data)
            return data->m_fsitem;
    }
     else
    {
        wxTreeItemId id = GetSelection();
        FsItemData* data = (FsItemData*)GetItemData(id);

        if (data)
            return data->m_fsitem;
    }

    return xcm::null;
}


IFsItemEnumPtr FsPanelTreeView::getSelectedItems()
{
    xcm::IVectorImpl<IFsItemPtr>* vec = new xcm::IVectorImpl<IFsItemPtr>;

    if (GetWindowStyle() & wxTR_MULTIPLE)
    {
        wxArrayTreeItemIds selections;
        GetSelections(selections);

        int count = selections.Count();
        int i;

        for (i = 0; i < count; ++i)
        {
            FsItemData* data;
            data = (FsItemData*)GetItemData(selections.Item(i));

            if (data)
                vec->append(data->m_fsitem);
        }
    }
     else
    {
        wxTreeItemId id = GetSelection();
        FsItemData* data = (FsItemData*)GetItemData(id);

        if (data)
            vec->append(data->m_fsitem);
    }

    return vec;
}


IFsItemEnumPtr FsPanelTreeView::getItemChildren(IFsItemPtr item)
{
    xcm::IVectorImpl<IFsItemPtr>* vec = new xcm::IVectorImpl<IFsItemPtr>;

    FsItemData* data;
    wxTreeItemId parent = item->getTreeItemId();

    // check to make sure a deferred item is populated
    data = (FsItemData*)GetItemData(parent);
    if (data && data->m_deferred)
    {
        populateDeferred(parent);
    }

    // get the item's children
    wxTreeItemIdValue cookie;
    wxTreeItemId id = GetFirstChild(parent, cookie);
    while (id.IsOk())
    {
        data = (FsItemData*)GetItemData(id);

        if (data)
        {
            if (data->m_fsitem)
            {
                vec->append(data->m_fsitem);
            }
        }

        id = GetNextChild(parent, cookie);
    }

    return vec;
}




