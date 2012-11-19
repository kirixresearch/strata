/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   David Z. Williams
 * Created:  2005-11-10
 *
 */


#ifndef __CFW_FSPANELLISTVIEW_H
#define __CFW_FSPANELLISTVIEW_H



class FsPanelListView : public CfwListCtrl
{

public:

    FsPanelListView(wxWindow* parent,
                    wxWindowID id,
                    const wxPoint& pos,
                    const wxSize& size,
                    long flags);
    ~FsPanelListView();

    void insertItem(IFsItemPtr item);
    IFsItemEnumPtr getSelectedItems();
    IFsItemPtr getParentItem();
    
    void setImageListLarge(bool val = true);
    void setParentItem(IFsItemPtr item);
    void setRootItem(IFsItemPtr item);
    
    void populate();
    void refresh();

private:

    int getImageListIdx(const wxBitmap& bitmap,
                        const wxBitmap& large_bitmap);
    void deleteAllItems();

private:
    
    IFsItemPtr m_root;
    IFsItemPtr m_parent;        // parent item of the items that are shown

    wxImageList* m_imagelist;
    wxImageList* m_imagelist_large;
    
    DECLARE_EVENT_TABLE()
};



#endif




