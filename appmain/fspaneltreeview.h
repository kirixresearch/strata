/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-11-08
 *
 */


#ifndef H_APP_FSPANELTREEVIEW_H
#define H_APP_FSPANELTREEVIEW_H


class FsPanelTreeView : public CfwTreeCtrl
{

public:

    FsPanelTreeView(wxWindow* parent,
                    wxWindowID id,
                    const wxPoint& pos,
                    const wxSize& size,
                    long flags);
    ~FsPanelTreeView();

    void highlightItem(const wxTreeItemId& id);
    void unhighlightItem();
    wxTreeItemId getHighlightItem();

    void doScroll(int direction);

    void refresh();
    void refreshAll();
    void refreshItem(IFsItemPtr item);

    IFsItemPtr appendItem(IFsItemPtr parent,
                          const wxString& text,
                          const wxBitmap& bitmap);
    IFsItemPtr insertItem(IFsItemPtr parent,
                          IFsItemPtr previous_item,
                          const wxString& text,
                          const wxBitmap& bitmap);
    void remove(IFsItemPtr item);

    void populate(IFsItemEnumPtr items,
                  const wxTreeItemId& parent_id);
    void populateDeferred(wxTreeItemId& id);

    void selectItem(IFsItemPtr item);
    void setRootItem(IFsItemPtr item);

    void expand(IFsItemPtr item);
    void collapse(IFsItemPtr item);

    void setItemBitmap(IFsItemPtr item, const wxBitmap& bmp, unsigned bitmap_type);

    IFsItemPtr getItemParent(IFsItemPtr item);
    IFsItemPtr getItemFromId(wxTreeItemId& id);
    IFsItemPtr getSelectedItem();
    IFsItemEnumPtr getSelectedItems();
    IFsItemEnumPtr getItemChildren(IFsItemPtr item);

private:

    int getImageListIdx(const wxBitmap& bitmap);

private:

    IFsItemPtr m_root;

    wxTreeItemId m_highlight_item;
    clock_t m_last_scroll;
    
    wxImageList* m_image_list;
    std::vector<wxBitmap> m_bitmaps;

    wxTreeItemId m_delete_item;

    DECLARE_EVENT_TABLE()
};



#endif


