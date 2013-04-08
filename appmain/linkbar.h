/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-05-26
 *
 */


#ifndef __APP_LINKBAR_H
#define __APP_LINKBAR_H


typedef wxAuiToolBarItem LinkBarItem;


// forward declarations
class LinkBarPopupWindow;
class LinkBarDropTarget;


class LinkBar : public wxAuiToolBar,
                public xcm::signal_sink
{
friend class LinkBarDropTarget;

public:
    
    LinkBar(wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize);
    
    void refresh();
    bool isFolderItem(int id);
    IFsItemPtr getItemFromId(int id);
    bool isPopupWindowOpen();
    void showPopupWindow(int id, bool popup_mode,
                         tango::IFileInfoEnumPtr items = xcm::null);
    void closePopupWindow();
    LinkBarItem* getItemFromCurrentPosition(bool ignore_ypos = false);
    
    wxString getLastClickedPath()
    {
        wxString res = m_last_clicked_path;
        m_last_clicked_path = L"";
        return res;
    }

private:

    void onPopupTimer(wxTimerEvent& evt);
    void onBeginDrag(wxAuiToolBarEvent& evt);
    void onRightClick(wxAuiToolBarEvent& evt);
    void onMiddleClick(wxAuiToolBarEvent& evt);
    void onOverflowClick(wxAuiToolBarEvent& evt);
    void onToolButtonClick(wxCommandEvent& evt);
    void onToolDropDownClick(wxAuiToolBarEvent& evt);
    void onUpdateUI_RelationshipSync(wxUpdateUIEvent& evt);
    
    // wxAuiToolBar override
    void OnLeaveWindow(wxMouseEvent& evt);
    void OnSize(wxSizeEvent& evt);
    
    void doHoverPopupOpen(int id);
    void drawDropHighlight();
    
    void tool2LinkIndex(int& tool_idx);
    void link2ToolIndex(int& link_idx);
    int  getInsertIndex(const wxPoint& pt,
                        bool ignore_ypos = false);   // returns a tool index
    
    void repositionPopupWindow();
    void recalcPopupWindowSize();
    void showOrHideDropDown();
    void forceRedraw();
    
    // signal handlers
    void onItemSelected(IFsItemPtr item);
    void onItemActivated(IFsItemPtr item);
    void onItemMiddleClicked(IFsItemPtr item);
    void onPopupMouseMove(wxCoord x, wxCoord y);
    void onPopupDestructing();
    
    // helper function
    bool pointInExclude(const wxPoint& pt);
    
    // drag and drop signal handlers
    void onFsDataLeave();
    void onFsDataDragOver(wxDragResult& def);
    void onFsDataDrop(wxDragResult& def, FsDataObject* data);
    void onFsDataFolderDrop(IFsItemPtr target, wxDataObject* data, wxDragResult* result);
    
private:

    LinkBarPopupWindow* m_popup_window;
    IFsPanelPtr m_popup_fspanel;
    wxTimer m_popup_timer;
    bool m_popup_during_drag;
    int m_popup_id;
    wxString m_base_path;
    std::vector<IFsItemPtr> m_items;
    wxString m_last_clicked_path;

    // drag and drop
    int m_exclude_id_begin;         // beginning of the range of items where data can't be dropped
    int m_exclude_id_end;           // end of the range of items where data can't be dropped
    int m_drop_idx;                 // index where the dragged item will be dropped
    int m_drag_id;                  // id of the toolbar item we're dragging
    int m_last_id;                  // id of the last toolbar item we hovered over
    wxStopWatch m_hover_stopwatch;  // stopwatch to determine how long we've been hovering over the same item
    
    DECLARE_EVENT_TABLE()
};


#endif


