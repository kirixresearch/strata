/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   David Z. Williams
 * Created:  2005-11-10
 *
 */


#include <wx/wx.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <xcm/xcm.h>
#include "framework.h"
#include "fspanel.h"
#include "fspanellistview.h"
#include "fspanel_private.h"
#include "fsitems_private.h"


namespace cfw
{


BEGIN_EVENT_TABLE(FsPanelListView, CfwListCtrl)
END_EVENT_TABLE()


FsPanelListView::FsPanelListView(wxWindow* parent, wxWindowID id,
                                 const wxPoint& pos, const wxSize& size,
                                 long flags)
                               : CfwListCtrl(parent, id, pos, size, flags)
{
    m_imagelist = new wxImageList;
    m_imagelist->Create(16, 16, true, 10);
    SetImageList(m_imagelist, wxIMAGE_LIST_SMALL);

    m_imagelist_large = new wxImageList;
    m_imagelist_large->Create(32, 32, true, 10);

    m_root.clear();
    m_parent.clear();
}


FsPanelListView::~FsPanelListView()
{
    if (m_imagelist)
        delete m_imagelist;

    if (m_imagelist_large)
        delete m_imagelist_large;
}


IFsItemPtr FsPanelListView::getParentItem()
{
    return m_parent;
}


void FsPanelListView::setParentItem(IFsItemPtr item)
{
    m_parent = item;
}


void FsPanelListView::setRootItem(IFsItemPtr item)
{
    m_root = item;
}


void FsPanelListView::setImageListLarge(bool val)
{
    if (val)
    {
        SetImageList(m_imagelist_large, wxIMAGE_LIST_NORMAL);
        return;
    }
    
    SetImageList(m_imagelist, wxIMAGE_LIST_SMALL);
}


void FsPanelListView::refresh()
{
    // make sure the list is shown before we freeze it...
    // this eliminates flicker on the first show
    Show();

    // don't show the list while we populate it
    Freeze();

    // delete all list items
    deleteAllItems();

    // populate the list
    populate();

    // now we can show the list
    Thaw();
}


int FsPanelListView::getImageListIdx(const wxBitmap& bitmap,
                                     const wxBitmap& large_bitmap)
{
    if (!bitmap.Ok())
        return 0;

    int idx, count;
    wxBitmap bmp;
    wxImage img;
    
    wxBitmap large_bmp = large_bitmap;
    
    if (!large_bitmap.Ok() ||
        large_bitmap.GetWidth() != 32)
    {
        // scale the small bitmap to a big one
        img = bitmap.ConvertToImage();
        img.Rescale(32, 32);
        
        wxBitmap temp(img);
        large_bmp = temp;
    }
    
    
    count = m_imagelist->GetImageCount();
    
    for (idx = 0; idx < count; ++idx)
    {
        bmp = m_imagelist->GetBitmap(idx);
        
        if (bmp.IsSameAs(bitmap))
            return idx;
    }
    
    m_imagelist->Add(bitmap);
    m_imagelist_large->Add(large_bmp);

    return idx;
}


void FsPanelListView::insertItem(IFsItemPtr item)
{
    int imagelist_idx = getImageListIdx(item->getBitmap(fsbmpSmall),
                                        item->getBitmap(fsbmpLarge));
                                         
    // add the item to the list
    long id = InsertItem(GetItemCount(),
                         item->getLabel(),
                         imagelist_idx);

    FsItemData* data = new FsItemData;
    data->m_fsitem = item;
    data->m_deferred = false;

    SetItemData(id, (long)data);
}


void FsPanelListView::deleteAllItems()
{
    int count = GetItemCount();
    int i;

    for (i = 0; i < count; ++i)
    {
        FsItemData* data;
        data = (FsItemData*)GetItemData(i);

        // we need to make sure we delete any FsItemData that
        // was associated with the list item
        if (data)
            delete data;
    }

    DeleteAllItems();
}


void FsPanelListView::populate()
{
    if (m_root.isNull())
        return;

    // if m_parent is uninitialized, start things off by
    // making it the m_root item
    if (m_parent.isNull())
        setParentItem(m_root);

    // clear out the list
    deleteAllItems();

    // populate the list
    IFsItemEnumPtr items = m_parent->getChildren();
    size_t i, count = items->size();

    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        insertItem(item);
    }
}


IFsItemEnumPtr FsPanelListView::getSelectedItems()
{
    xcm::IVectorImpl<IFsItemPtr>* vec = new xcm::IVectorImpl<IFsItemPtr>;

    long item = -1;
    
    int i;
    int count = GetSelectedItemCount();
    
    for (i = 0; i < count; ++i)
    {
        item = GetNextItem(item,
                           wxLIST_NEXT_ALL,
                           wxLIST_STATE_SELECTED);

        FsItemData* data = (FsItemData*)GetItemData(item);
        vec->append(data->m_fsitem);
    }
    
    return vec;
}


};  // namespace cfw


