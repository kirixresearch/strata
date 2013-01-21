/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-06-30
 *
 */


#ifndef __KXINTER_TREECONTROLLER_H
#define __KXINTER_TREECONTROLLER_H


class TreeController : public wxEvtHandler,
                       public xcm::signal_sink
{

public:

    TreeController();
    void refreshItem(cfw::ITreeItemPtr item);
    void refreshAllItems();
    void populate();
    cfw::ITreePanelPtr getTreePanel();
    
private:

    void onItemRemove(wxCommandEvent& event);
    void onItemRename(wxCommandEvent& event);
    void onItemDelete(wxCommandEvent& event);
    void onItemProperties(wxCommandEvent& event);
    void onNewConnection(wxCommandEvent& event);
    void onNewFolder(wxCommandEvent& event);
    void onSetAsActiveDatabase(wxCommandEvent& event);
    void onOpen(wxCommandEvent& event);
    void onCreateTable(wxCommandEvent& event);
    void onEditStructure(wxCommandEvent& event);
    void onSetFileLayout(wxCommandEvent& event);
    void onConvert(wxCommandEvent& event);

private:

    void onItemActivated(cfw::ITreeItemPtr item);
    void onItemRightClicked(cfw::ITreeItemPtr item);
    void onItemEndLabelEdit(cfw::ITreeItemPtr item, wxString label, bool cancelled, bool* allow);
    void onDragDrop(cfw::ITreeItemPtr target, wxDataObject* data, wxDragResult* result);

    DECLARE_EVENT_TABLE()

private:

    cfw::ITreePanelPtr m_tree;
    cfw::ITreeItemPtr m_rightclick_item;
};


#endif



