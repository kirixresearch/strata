/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-11-08
 *
 */


#ifndef __APP_FSPANEL_H
#define __APP_FSPANEL_H


#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>

#ifdef CFW_USE_GENERIC_TREECTRL
#include <wx/generic/treectlg.h>
#include <wx/generic/listctrl.h>
typedef wxGenericTreeCtrl CfwTreeCtrl;
typedef wxGenericListCtrl CfwListCtrl;
#else
typedef wxTreeCtrl CfwTreeCtrl;
typedef wxListCtrl CfwListCtrl;
#endif


xcm_interface IFsPanel;
xcm_interface IFsItem;

XCM_DECLARE_SMARTPTR(IFsPanel)
XCM_DECLARE_SMARTPTR(IFsItem)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IFsItemPtr>, IFsItemEnumPtr)


enum
{
    fsviewUndefined = 0,
    fsviewTree = 1000,
    fsviewList = 1001,
    fsviewLargeIcon = 1002,
    fsviewSmallIcon = 1003,
    fsviewDetails = 1004
};


enum
{
    fsbmpSmall = 1,
    fsbmpSmallExpanded = 2,
    fsbmpLarge = 3
};

enum
{
    fsstyleTreeHideRoot = 1 << 1,
    fsstyleTrackSelect =  1 << 2
};


const wxString FS_DATA_OBJECT_FORMAT = wxT("application/vnd.kx.fspanel");

xcm_interface IFsItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IFsItem")

public:

    virtual bool isDeferred() = 0;
    virtual bool isFolder() = 0;
    virtual bool hasChildren() = 0;

    virtual wxTreeItemId getTreeItemId() = 0;
    virtual wxString getLabel() = 0;
    virtual wxBitmap getBitmap(unsigned int bitmap_type = fsbmpSmall) = 0;
    virtual long getItemData() = 0;
    virtual wxString getItemStringData() = 0;
    virtual IFsItemEnumPtr getChildren() = 0;

    virtual void setTreeItemId(const wxTreeItemId& treeitem_id) = 0;
    virtual void setLabel(const wxString& label) = 0;
    virtual void setBitmap(const wxBitmap& bitmap, unsigned int bitmap_type = fsbmpSmall) = 0;
    virtual void setItemData(long data) = 0;
    virtual void setItemStringData(const wxString& data) = 0;

    virtual void onRenamed(wxString label) = 0;
    virtual void onRightClicked() = 0;
    virtual void onMiddleClicked() = 0;
    virtual void onActivated() = 0;
    virtual void onCommandEvent(wxCommandEvent& evt) = 0;
};


xcm_interface IFsPanel : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IFsPanel")

public:

    virtual void setStyle(int style_flags) = 0;

    // this may be used alternatively to the cfw document mechanism
    virtual bool create(wxWindow* parent,
                        wxWindowID id,
                        const wxPoint& position,
                        const wxSize& size,
                        int flags) = 0;

    virtual void destroy() = 0;

    virtual void setView(int val) = 0;
    virtual void setMultiSelect(bool val) = 0;
    virtual void showLocationBar(bool show) = 0;

    virtual void expand(IFsItemPtr item) = 0;
    virtual void collapse(IFsItemPtr item) = 0;
    virtual bool isItemExpanded(IFsItemPtr item) = 0;

    virtual IFsItemPtr getRootItem() = 0;
    virtual void setRootItem(IFsItemPtr item) = 0;

    virtual IFsItemPtr getItemParent(IFsItemPtr item) = 0;
    virtual IFsItemEnumPtr getItemChildren(IFsItemPtr item) = 0;
    virtual IFsItemEnumPtr getSelectedItems() = 0;
    virtual IFsItemPtr getSelectedItem() = 0;
    virtual IFsItemPtr appendItem(IFsItemPtr parent, const wxString& text, const wxBitmap& bitmap) = 0;
    virtual IFsItemPtr insertItem(IFsItemPtr parent, IFsItemPtr previous, const wxString& text, const wxBitmap& bitmap) = 0;
    virtual IFsItemPtr hitTest(int x, int y) = 0;
    virtual int getItemIndex(IFsItemPtr item) = 0;
    
    virtual void editLabel(IFsItemPtr item) = 0;
    virtual void remove(IFsItemPtr item) = 0;

    virtual void selectItem(IFsItemPtr item) = 0;
    virtual void unselect() = 0;
    virtual void unselectAll() = 0;

    virtual void setItemBitmap(IFsItemPtr item, const wxBitmap& bitmap, unsigned int bitmap_type = fsbmpSmall) = 0;
    virtual void setItemText(IFsItemPtr item, const wxString& text) = 0;
    virtual void setItemBold(IFsItemPtr item, bool bold) = 0;

    virtual void refresh() = 0;
    virtual void refreshAll() = 0;
    virtual void refreshItem(IFsItemPtr item) = 0;

    virtual void getVirtualSize(int* width, int* height) = 0;

    virtual int popupMenu(wxMenu* menu) = 0;

    virtual void setDragDrop(bool enable_dragdrop) = 0;
    virtual bool isDragDropEnabled() = 0;

    XCM_DECLARE_SIGNAL2(sigMouseLeftUp, wxCoord /*x*/, wxCoord /*y*/)
    XCM_DECLARE_SIGNAL2(sigMouseMove, wxCoord /*x*/, wxCoord /*y*/)
    XCM_DECLARE_SIGNAL2(sigItemBeginLabelEdit, IFsItemPtr, bool* /*allow*/)
    XCM_DECLARE_SIGNAL4(sigItemEndLabelEdit, IFsItemPtr, wxString, bool /*cancelled*/, bool* /*allow*/)
    XCM_DECLARE_SIGNAL1(sigItemMiddleClicked, IFsItemPtr)
    XCM_DECLARE_SIGNAL1(sigItemRightClicked, IFsItemPtr)
    XCM_DECLARE_SIGNAL1(sigItemActivated, IFsItemPtr)
    XCM_DECLARE_SIGNAL1(sigItemSelected, IFsItemPtr)
    XCM_DECLARE_SIGNAL1(sigItemHighlightRequest, IFsItemPtr&)
    XCM_DECLARE_SIGNAL1(sigKeyDown, const wxKeyEvent&)
    XCM_DECLARE_SIGNAL3(sigDragDrop, IFsItemPtr, wxDataObject*, wxDragResult*)
    XCM_DECLARE_SIGNAL0(sigDestructing)
};




class FsItemBase : public wxEvtHandler,
                   public IFsItem
{

XCM_CLASS_NAME("cfw.FsItemBase")
XCM_BEGIN_INTERFACE_MAP(FsItemBase)
    XCM_INTERFACE_ENTRY(IFsItem)
XCM_END_INTERFACE_MAP()

public:

    FsItemBase()
    {
        m_base_item_string = wxT("");
        m_base_item_long = 0;
        m_base_has_children = true;
        m_base_is_deferred = false;
    }

    bool isDeferred()
    {
        return m_base_is_deferred;
    }

    bool hasChildren()
    {
        return m_base_has_children;
    }

    wxTreeItemId getTreeItemId()
    {
        return m_base_treeitem_id;
    }

    bool isFolder()
    {
        return false;
    }

    wxBitmap getBitmap(unsigned int bitmap_type)
    {
        switch (bitmap_type)
        {
            case fsbmpSmall:         return m_base_bitmap;
            case fsbmpSmallExpanded: return m_base_expanded_bitmap;
            case fsbmpLarge:         return m_base_large_bitmap;
            default:
                return wxNullBitmap;
        }
    }

    wxString getLabel()
    {
        return m_base_label;
    }

    IFsItemEnumPtr getChildren()
    {
        xcm::IVectorImpl<IFsItemPtr>* v = new xcm::IVectorImpl<IFsItemPtr>;
        return v;
    }

    wxString getItemStringData()
    {
        return m_base_item_string;
    }

    long getItemData()
    {
        return m_base_item_long;
    }

    void setDeferred(bool new_val)
    {
        m_base_is_deferred = new_val;
    }

    void setTreeItemId(const wxTreeItemId& treeitem_id)
    {
        m_base_treeitem_id = treeitem_id;
    }

    void setHasChildren(bool new_val)
    {
        m_base_has_children = new_val;
    }

    void setLabel(const wxString& label)
    {
        m_base_label = label;
    }

    void setBitmap(const wxBitmap& bitmap, unsigned int bitmap_type = fsbmpSmall)
    {
        switch (bitmap_type)
        {
            case fsbmpSmall:         m_base_bitmap = bitmap; break;
            case fsbmpSmallExpanded: m_base_expanded_bitmap = bitmap; break;
            case fsbmpLarge:         m_base_large_bitmap = bitmap; break;
        }
    }

    void setItemData(long l)
    {
        m_base_item_long = l;
    }

    void setItemStringData(const wxString& s)
    {
        m_base_item_string = s;
    }

    // event handlers

    void onRenamed(wxString label)
    {
        m_base_label = label;
    }

    void onRightClicked()
    {
    }

    void onMiddleClicked()
    {
    }

    void onActivated()
    {
    }

    void onCommandEvent(wxCommandEvent& evt)
    {
        // forward this info to the treemodel
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, evt.GetId());
        e.SetEventObject(this);
        ProcessEvent(e);
    }

private:

    wxTreeItemId m_base_treeitem_id;
    wxString m_base_label;
 
    wxString m_base_item_string;
    long m_base_item_long;
    bool m_base_has_children;
    bool m_base_is_deferred;
    
    wxBitmap m_base_bitmap;
    wxBitmap m_base_expanded_bitmap;
    wxBitmap m_base_large_bitmap;
};


enum FsDataObjectMetadata
{
    IDX_FSITEM_COUNT = 0,
    IDX_SOURCE_ID = 1
};
const int FSDATAOBJECT_METADATA_COUNT = 2;


class FsDataObject : public wxCustomDataObject
{
public:

    FsDataObject() : wxCustomDataObject(wxDataFormat(FS_DATA_OBJECT_FORMAT))
    {
        unsigned long i = 0;
        SetData(sizeof(long), (void*)&i);
        
        m_source_id = 0;
    }

    ~FsDataObject()
    {
    }


    void* GetData() const
    {
        return wxCustomDataObject::GetData();
    }

    void setSourceId(wxWindowID id)
    {
        m_source_id = id;
    }
    
    void setFsItems(IFsItemEnumPtr items)
    {
        size_t item_count = items->size();
        size_t i, data_size = (FSDATAOBJECT_METADATA_COUNT + item_count) * sizeof(long);

        unsigned long* data = new unsigned long[data_size];
        data[IDX_FSITEM_COUNT] = item_count;
        data[IDX_SOURCE_ID] = m_source_id;

        for (i = 0; i < item_count; ++i)
        {
            IFsItemPtr item = items->getItem(i);
            data[i + FSDATAOBJECT_METADATA_COUNT] = (unsigned long)item.p;
        }

        SetData(data_size, data);

        delete[] data;
    }

    wxWindowID getSourceId()
    {
        unsigned long* data = (unsigned long*)GetData();
        wxWindowID source_id = (wxWindowID)(data[IDX_SOURCE_ID]);
        return source_id;
    }
    
    IFsItemEnumPtr getFsItems()
    {
        xcm::IVectorImpl<IFsItemPtr>* v = new xcm::IVectorImpl<IFsItemPtr>;

        unsigned long* data = (unsigned long*)GetData();

        size_t i, item_count = *data;

        for (i = 0; i < item_count; ++i)
        {
            IFsItemPtr t;
            t.p = (IFsItem*)data[i + FSDATAOBJECT_METADATA_COUNT];
            t.p->ref();

            v->append(t);
        }

        return v;
    }

private:

    wxWindowID m_source_id; // pointer to insert into our "clipboard" data
};




class FsDataDropTarget : public wxDropTarget
{
public:
    
    FsDataDropTarget() : wxDropTarget(new FsDataObject),
                         m_format(FS_DATA_OBJECT_FORMAT)
    {
    }

    wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def)
    {
        sigDragEnter(def);
        return def;
    }

    void OnLeave()
    {
        sigDragLeave();
    }

    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
    {
        #ifndef WIN32
        def = wxDropTarget::OnDragOver(x, y, def);
        #endif
        
        sigDragOver(def);
        return def;
    }

    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
    {
        if (!GetData())
            return wxDragNone;

        FsDataObject* data = (FsDataObject*)GetDataObject();
        if (!data)
            return wxDragNone;

        sigDragDrop.fire(def, data);

        return def;
    }

public: // signals

    xcm::signal1<wxDragResult&> sigDragEnter;
    xcm::signal0 sigDragLeave;
    xcm::signal1<wxDragResult&> sigDragOver;
    xcm::signal2<wxDragResult&, FsDataObject*> sigDragDrop;

private:

    wxDataFormat m_format;
};


IFsPanelPtr createFsPanelObject();


#endif
