/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-11-08
 *
 */


#ifndef __APP_FSPANEL_PRIVATE_H
#define __APP_FSPANEL_PRIVATE_H


namespace kcl
{
    class Button;
}




class FsPanelTreeView;
class FsPanelListView;
class FsPanelLocationBar;


// this class is used to store item data for CfwTreeCtrls
// and CfwListCtrls... it is derived from wxTreeItemData
// for compatibility with the CfwTreeCtrl, but will work
// with list controls as well

class FsItemData : public wxTreeItemData
{
public:

    IFsItemPtr m_fsitem;
    bool m_deferred;
};


class FsPanel : public wxWindow,
                public IFsPanel,
                public IDocument
{
friend class FsTreeDropTarget;

    XCM_CLASS_NAME_NOREFCOUNT("cfw.FsPanel")
    XCM_BEGIN_INTERFACE_MAP(FsPanel)
        XCM_INTERFACE_ENTRY(IDocument)
        XCM_INTERFACE_ENTRY(IFsPanel)
    XCM_END_INTERFACE_MAP()


    XCM_IMPLEMENT_SIGNAL2(sigMouseLeftUp, wxCoord /*x*/, wxCoord /*y*/)
    XCM_IMPLEMENT_SIGNAL2(sigMouseMove, wxCoord /*x*/, wxCoord /*y*/)
    XCM_IMPLEMENT_SIGNAL2(sigItemBeginLabelEdit, IFsItemPtr, bool* /*allow*/)
    XCM_IMPLEMENT_SIGNAL4(sigItemEndLabelEdit, IFsItemPtr, wxString, bool /*cancelled*/, bool* /*allow*/)
    XCM_IMPLEMENT_SIGNAL1(sigItemMiddleClicked, IFsItemPtr)
    XCM_IMPLEMENT_SIGNAL1(sigItemRightClicked, IFsItemPtr)
    XCM_IMPLEMENT_SIGNAL1(sigItemActivated, IFsItemPtr)
    XCM_IMPLEMENT_SIGNAL1(sigItemSelected, IFsItemPtr)
    XCM_IMPLEMENT_SIGNAL1(sigItemHighlightRequest, IFsItemPtr&)
    XCM_IMPLEMENT_SIGNAL1(sigKeyDown, const wxKeyEvent&)
    XCM_IMPLEMENT_SIGNAL3(sigDragDrop, IFsItemPtr, wxDataObject*, wxDragResult*)
    XCM_IMPLEMENT_SIGNAL0(sigDestructing)

public:

    FsPanel();
    ~FsPanel();

private:

    void setStyle(int style_flags) { m_style = style_flags; }

    bool create(wxWindow* parent,
                wxWindowID id,
                const wxPoint& position,
                const wxSize& size,
                int flags);
    void destroy();
    
    // IDocument interface implementation

    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // IFsPanel interface implementation

    void setView(int val);
    void setMultiSelect(bool val);
    void showLocationBar(bool show = true);

    void expand(IFsItemPtr item);
    void collapse(IFsItemPtr item);
    bool isItemExpanded(IFsItemPtr item);

    IFsItemPtr getRootItem();
    void setRootItem(IFsItemPtr item);

    IFsItemPtr getItemParent(IFsItemPtr item);
    IFsItemEnumPtr getItemChildren(IFsItemPtr item);
    IFsItemEnumPtr getSelectedItems();
    IFsItemPtr getSelectedItem();
    IFsItemPtr appendItem(IFsItemPtr parent, const wxString& text, const wxBitmap& bitmap);
    IFsItemPtr insertItem(IFsItemPtr parent, IFsItemPtr previous, const wxString& text, const wxBitmap& bitmap);
    IFsItemPtr hitTest(int x, int y);
    int getItemIndex(IFsItemPtr item);
    
    void editLabel(IFsItemPtr item);
    void remove(IFsItemPtr item);

    void selectItem(IFsItemPtr item);
    void unselect();
    void unselectAll();

    void setItemBitmap(IFsItemPtr item, const wxBitmap& bitmap, unsigned bitmap_type);
    void setItemText(IFsItemPtr item, const wxString& text);
    void setItemBold(IFsItemPtr item, bool bold);

    void refresh();
    void refreshAll();
    void refreshItem(IFsItemPtr item);

    void getVirtualSize(int* width, int* height);

    int popupMenu(wxMenu* menu);
    
    void setDragDrop(bool enable_dragdrop);
    bool isDragDropEnabled();

private:

    IFsItemPtr getItemFromId(wxTreeItemId& id);

    void changeLocation(IFsItemPtr item);
    void refreshLocationBar();
        
    void populateLocationChoiceDeferred(int idx);
    void populateLocationChoiceInternal(int idx, IFsItemEnumPtr items);

    void createLocationBar();
    void createTreeView();
    void createListView();

    void destroyTreeView();
    void destroyListView();

    void handleLayout();

private:
    
    // event handlers
    
    void onSize(wxSizeEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);
    void onDoExpandFolder(wxCommandEvent& evt);
    void onLocationChoice(wxCommandEvent& evt);
    void onUpLevelClicked(wxCommandEvent& evt);
    void onChangeViewClicked(wxCommandEvent& evt);
    void onChangeView(wxCommandEvent& evt);
    void onMouseMove(wxMouseEvent& evt);
    void onMouseLeftUp(wxMouseEvent& evt);
    void onMouseMiddleDown(wxMouseEvent& evt);
    void onMouseMiddleUp(wxMouseEvent& evt);
    void onChildFocus(wxChildFocusEvent& evt);
    
    // treeview evts
    void onFsTreeItemActivated(wxTreeEvent& evt);
    void onFsTreeItemRightClicked(wxTreeEvent& evt);
    void onFsTreeItemMiddleClicked(wxTreeEvent& evt);
    void onFsTreeItemBeginLabelEdit(wxTreeEvent& evt);
    void onFsTreeItemEndLabelEdit(wxTreeEvent& evt);
    void onFsTreeItemExpanding(wxTreeEvent& evt);
    void onFsTreeItemCollapsing(wxTreeEvent& evt);
    void onFsTreeSelectionChanged(wxTreeEvent& evt);
    void onFsTreeKeyDown(wxTreeEvent& evt);
    void onFsTreeBeginDrag(wxTreeEvent& evt);
    
    // listview evts
    void onFsListItemActivated(wxListEvent& evt);
    void onFsListItemRightClicked(wxListEvent& evt);
    void onFsListItemMiddleClicked(wxListEvent& evt);
    void onFsListItemBeginLabelEdit(wxListEvent& evt);
    void onFsListItemEndLabelEdit(wxListEvent& evt);
    void onFsListKeyDown(wxListEvent& evt);
    void onFsListBeginDrag(wxListEvent& evt);
    
private:

    wxBoxSizer* m_main_sizer;

    wxBoxSizer* m_location_bar;
    FsPanelTreeView* m_treeview;
    FsPanelListView* m_listview;

    // for the location bar
    wxChoice* m_location_choice;
    kcl::Button* m_up_level_button;
    kcl::Button* m_change_view_button;
    
    IFsItemPtr m_root;

    bool m_dragdrop_enabled;
    bool m_show_location_bar;
    bool m_multi_select;
    int m_last_view;
    int m_view;
    int m_style;
    
    wxTreeItemId m_treeview_middledown_action_id;
    
    wxStopWatch m_focus_stopwatch;
    
    DECLARE_EVENT_TABLE()
};



#endif


