/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2008-03-05
 *
 */


#ifndef __KCANVAS_CANVASHISTORY_H
#define __KCANVAS_CANVASHISTORY_H


namespace kcanvas
{


// Canvas History Classes

// note: the canvas history class tracks changes to a
// component and all of it's descendants; it can be used 
// for tracking individual components not currently stored 
// in the canvas, or it can be used for tracking the 
// canvas itself by tracking changes in the canvas model 
// component, which can be obtained via a function call 
// on the canvas

// changes are tracked, starting with a call to
// track(), and are either committed to the history
// using commit() or are rejected() using rollback();
// calls to track are reference counted and as a
// result, must be paired with a corresponding call to 
// commit() or rollback(); if multiple calls to track()
// are made, followed by multiple calls to commit()
// or rollback(), all the changes from the first track()
// call to the last commit()/rollback() call will be
// saved/rejected as a single operation; this prevents 
// related operations that are tracking changes on the 
// same component from saving multiple histories of the 
// same state changes

// once a component's history is saved, previous states 
// of the component can be loaded using undo(); if
// previous states are loaded, later states can be 
// reloaded using redo() note: undo/redo only loads the 
// state of the component hiearchy being tracked; the 
// canvas must be updated with layout(), render(), and 
// repaint() to actually view these changes; this allows 
// more flexibility with where the CanvasHistory object 
// is used and prevents unwanted updates to the canvas


class CanvasSnapshot;
class CanvasHistory : public ICanvasHistory
{
    XCM_CLASS_NAME("kcanvas.CanvasHistory")
    XCM_BEGIN_INTERFACE_MAP(CanvasHistory)
        XCM_INTERFACE_ENTRY(ICanvasHistory)
    XCM_END_INTERFACE_MAP()

public:

    CanvasHistory();
    virtual ~CanvasHistory();

    static ICanvasHistoryPtr create(IComponentPtr component);

public:

    // ICanvasHistory interface

    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void track();
    void commit();
    void rollback();
    void clear();

private:

    // helper functions tracking, committing, and
    // rolling back the root component and its
    // child components
    void beginTrackingChanges(const wxString& description = wxT(""));
    void endTrackingChanges(bool accept = true);

    // functions to save and load a snapshot of the
    // root component
    void saveSnapshot(const wxString& description = wxT(""));
    void loadSnapshot(int idx);

private:

    // history
    std::vector<CanvasSnapshot> m_history;

    // root component to archive; this component,
    // along with all of it's children will be
    // archived each time changes are tracked and
    // committed
    IComponentPtr m_root_component;
    
    // snapshot index and m_commit_ref
    int m_snapshot_idx;
    int m_commit_ref;
};


}; // namespace kcanvas


#endif

