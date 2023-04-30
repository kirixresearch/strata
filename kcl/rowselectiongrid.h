/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2006-11-07
 *
 */


#ifndef H_KCL_ROWSELECTIONGRID_H
#define H_KCL_ROWSELECTIONGRID_H


#include "grid.h"
#include "util.h"


namespace kcl
{


class GridDataDropTarget;

class RowSelectionGrid : public kcl::Grid,
                         public xcm::signal_sink
{
public:

    enum RowSelectionGridOption
    {
        refreshAfterDataDropped = 1 << 0,
        refreshAfterPreGhostRowInsert = 2 << 0,
        refreshAfterPreInvalidAreaInsert = 3 << 0,
        refreshAll = (refreshAfterDataDropped  |
                      refreshAfterPreGhostRowInsert |
                      refreshAfterPreInvalidAreaInsert),
        refreshNone = 0
    };

public:

    RowSelectionGrid(wxWindow* window,
                     wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = kcl::DEFAULT_BORDER,
                     bool drag_source = true,
                     bool drop_target = true);

    ~RowSelectionGrid();
    
    void setRowSelectionGridFlags(int flags);
    void setAllowDeletes(bool allow_deletes = true);
    void setAllowKeyboardInserts(bool allow_keyboard_inserts = true);
    void setAllowInvalidAreaInserts(bool allow_invalidarea_inserts = true);

    void insertSelectedRows(bool refresh_rows = true);
    void deleteSelectedRows(bool refresh_rows = true);
    
    void setAcceptsFocus(bool b) { m_accepts_focus = b; }
    
    size_t getSelectedRowCount();
    std::vector<int> getSelectedRows();
    
    //  event handlers
    void onSize(wxSizeEvent& evt);
    void onKeyDown(kcl::GridEvent& evt);
    void onEndEdit(kcl::GridEvent& evt);
    void onBeginDrag(kcl::GridEvent& evt);
    void onCursorMove(kcl::GridEvent& evt);
    void onInvalidAreaClick(kcl::GridEvent& evt);
    void onSelectionChanged(kcl::GridEvent& evt);
    void onPreGhostRowInsert(kcl::GridEvent& evt);
    void onPreInvalidAreaInsert(kcl::GridEvent& evt);
    void onDataDropped(kcl::GridDataDropTarget* drop_target);

    bool AcceptsFocus() const { return m_accepts_focus; }
    
public: // signals

    xcm::signal1< std::vector<int> > sigInsertingRows;
    xcm::signal1< std::vector<int> > sigInsertedRows;
    xcm::signal2< std::vector<int>, bool* > sigDeletingRows;
    xcm::signal1< std::vector<int> > sigDeletedRows;
    xcm::signal1<kcl::GridDataDropTarget*> sigDataDropped;

private:

    int m_row_selection_grid_flags;
    bool m_drag_source;
    bool m_drop_target;
    bool m_was_editing;
    bool m_allow_deletes;
    bool m_allow_keyboard_inserts;
    bool m_allow_invalidarea_inserts;
    bool m_accepts_focus;
    
    DECLARE_EVENT_TABLE()
};


};  // namespace kcl


#endif  // __KCL_ROWSELECTIONGRID_H


