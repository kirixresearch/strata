/*!
 *
 * Copyright (c) 2024, fopen GmbH.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2024-03-14
 *
 */


#include "appmain.h"

#include "bookmarkfscombo.h"
#include <wx/wx.h>
#include <wx/combo.h>
#include <wx/treectrl.h>
#include <wx/popupwin.h>


BookmarkFsComboCtrl::BookmarkFsComboCtrl(wxWindow* parent,
                                         wxWindowID id,
                                         const wxString& value,
                                         const wxPoint& pos,
                                         const wxSize& size,
                                         long style,
                                         const wxValidator& validator,
                                         const wxString& name)
                                         : wxComboCtrl(parent, id, value, pos, size, style, validator, name)
{
    m_path = "/";

    BookmarkFsComboCtrlPopup* popup = new BookmarkFsComboCtrlPopup();
    this->SetPopupControl(popup);
}


bool BookmarkFsComboCtrl::Create(wxWindow* parent,
                                 wxWindowID id,
                                 const wxString& value,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style,
                                 const wxValidator& validator,
                                 const wxString& name)
{
    if (!wxComboCtrl::Create(parent, id, value, pos, size, style, validator, name))
        return false;
    
    BookmarkFsComboCtrlPopup* popup = new BookmarkFsComboCtrlPopup();
    SetPopupControl(popup);
    return true;
}

void BookmarkFsComboCtrl::setBookmarkFs(IBookmarkFsPtr bookmark_fs)
{
    m_bookmark_fs = bookmark_fs;
    static_cast<BookmarkFsComboCtrlPopup*>(GetPopupControl())->populateTree(bookmark_fs);
}


void BookmarkFsComboCtrl::OnButtonClick()
{
    wxComboCtrl::OnButtonClick();
}



class BookmarkTreeItemData : public wxTreeItemData
{
public:
    BookmarkTreeItemData(const wxString& data) : m_str(data) { }
    virtual ~BookmarkTreeItemData() { }
    wxString getStringData() const { return m_str; }

private:
    wxString m_str;
};


BookmarkFsComboCtrlPopup::BookmarkFsComboCtrlPopup()
{
}

BookmarkFsComboCtrlPopup::~BookmarkFsComboCtrlPopup()
{
}


void BookmarkFsComboCtrlPopup::Init()
{
    m_treectrl = nullptr;
}

bool BookmarkFsComboCtrlPopup::Create(wxWindow* parent)
{
    m_treectrl = new wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);

    int imageWidth = m_treectrl->FromDIP(16);
    int imageHeight = m_treectrl->FromDIP(16);
    wxImageList* imageList = new wxImageList(imageWidth, imageHeight);
    imageList->Add(GETBMPSMALL(gf_folder_closed));
    m_treectrl->AssignImageList(imageList);

    // bind the tree selection changed event to our handler
    m_treectrl->Bind(wxEVT_TREE_SEL_CHANGED, &BookmarkFsComboCtrlPopup::onTreeSelectionChanged, this);

    return true;
}

wxWindow* BookmarkFsComboCtrlPopup::GetControl()
{
    return m_treectrl;
}

void BookmarkFsComboCtrlPopup::onTreeSelectionChanged(wxTreeEvent& event)
{
    wxTreeItemId selectedId = event.GetItem();
    if (selectedId.IsOk())
    {
        m_selected_value = m_treectrl->GetItemText(selectedId);
        static_cast<BookmarkFsComboCtrl*>(m_combo)->setPath(static_cast<BookmarkTreeItemData*>(m_treectrl->GetItemData(selectedId))->getStringData());
        Dismiss(); // close the popup after selection
    }
}

void BookmarkFsComboCtrlPopup::SetStringValue(const wxString& value)
{
    m_selected_value = value;
}

wxString BookmarkFsComboCtrlPopup::GetStringValue() const
{
    return m_selected_value;
}

static void populateTreeWithFolders(wxTreeCtrl* tree, const wxTreeItemId& parentId, const wxString& parent_folder, IFsItemPtr item)
{
    wxTreeItemId treeitem_id = tree->AppendItem(parentId, item->getLabel(), -1, -1);
    tree->SetItemImage(treeitem_id, 0, wxTreeItemIcon_Normal);

    wxString this_folder = xd::appendPath(parent_folder.ToStdWstring(), item->getLabel().ToStdWstring());

    BookmarkTreeItemData* data = new BookmarkTreeItemData(this_folder);
    tree->SetItemData(treeitem_id, data);


    // iterate through the children of this item and add them to the tree
    if (item->hasChildren())
    {
        IFsItemEnumPtr childrenEnum = item->getChildren();

        for (size_t i = 0; i < childrenEnum->size(); ++i)
        {
            IFsItemPtr child = childrenEnum->getItem(i);
            populateTreeWithFolders(tree, treeitem_id, this_folder, child);
        }

    }
}

void BookmarkFsComboCtrlPopup::populateTree(IBookmarkFsPtr bookmark_fs)
{
    wxString root_label = _("Bookmarks Toolbar");

    m_treectrl->DeleteAllItems();
    wxTreeItemId rootId = m_treectrl->AddRoot(root_label);
    m_treectrl->SetItemImage(rootId, 0, wxTreeItemIcon_Normal);
    
    BookmarkTreeItemData* data = new BookmarkTreeItemData("/");
    m_treectrl->SetItemData(rootId, data);

    IFsItemPtr rootItem = bookmark_fs->getBookmarkFolderItem(L"/");
    IFsItemEnumPtr childrenEnum = rootItem->getChildren();

    for (size_t i = 0; i < childrenEnum->size(); ++i)
    {
        IFsItemPtr child = childrenEnum->getItem(i);

        if (child->isFolder())
        {
            populateTreeWithFolders(m_treectrl, rootId, "/", child);
        }
    }


    m_treectrl->Expand(rootId);

    m_treectrl->SelectItem(rootId);

    wxTreeEvent evt;
    evt.SetItem(rootId);
    onTreeSelectionChanged(evt);
    static_cast<BookmarkFsComboCtrl*>(m_combo)->setPath("/");
    static_cast<BookmarkFsComboCtrl*>(m_combo)->SetValue(root_label);
}

void BookmarkFsComboCtrlPopup::OnTreeItemActivated(wxTreeEvent& event)
{
    Dismiss();
}


