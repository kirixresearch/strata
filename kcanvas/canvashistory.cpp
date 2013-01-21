/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2008-03-05
 *
 */


#include "kcanvas_int.h"
#include "canvashistory.h"
#include "component.h"
#include "util.h"


namespace kcanvas
{


// component snapshot

class ComponentSnapshot
{
public:

    // m_component is a pointer to the component that
    // we're taking a snapshot of and m_component_snapshot
    // is a copy of that component

    IComponentPtr m_component;
    IComponentPtr m_component_snapshot;
};


class CanvasSnapshot
{
public:

    CanvasSnapshot()
    {
    }
    
    ~CanvasSnapshot()
    {
    }

public:
    
    // a list of of components and their state at the 
    // time the history snapshot is taken
    std::vector<ComponentSnapshot> m_canvas_snapshot;

    // a string for storing a description of a
    // particular snapshot
    wxString m_description;
};


CanvasHistory::CanvasHistory()
{
    m_snapshot_idx = 0;
    m_commit_ref = 0;
}

CanvasHistory::~CanvasHistory()
{
}

ICanvasHistoryPtr CanvasHistory::create(IComponentPtr component)
{
    CanvasHistory* history = new CanvasHistory();
    history->m_root_component = component;
    return static_cast<ICanvasHistory*>(history);
}

bool CanvasHistory::canUndo() const
{
    // if we don't have a root component, we're done
    if (m_root_component.isNull())
        return false;

    // if the snapshot index is less than or equal 
    // to zero, we don't have any snapshot we can load, 
    // so return false
    if (m_snapshot_idx <= 0)
        return false;

    // we have both a history and the ability to 
    // load it, so return true
    return true;
}

bool CanvasHistory::canRedo() const
{
    // if we don't have a root component, we're done
    if (m_root_component.isNull())
        return false;

    // if the snapshot index is on or after the last 
    // element of thehistory, we don't have any snapshots
    // we can load, so return false
    if (m_snapshot_idx >= ((int)m_history.size() - 1))
        return false;

    // we have both a snapshot and the ability to 
    // load it, so return true
    return true;
}

void CanvasHistory::undo()
{
    // if we can't undo, we're done
    if (!canUndo())
        return;

    // if we're undoing for the very first time, we have to
    // save the current state of the root component which hasn't 
    // yet been committed, so we can get back to it with redo
    if (m_snapshot_idx >= (int)m_history.size())
    {
        // save the old history size;
        int previous_history_size = m_history.size();

        // begin tracking the changes
        beginTrackingChanges();

        // if the history size has changed (we're tracking for
        // the first time, we have to skip over the history we
        // just added so we can load the snapshot right before
        if ((int)m_history.size() > previous_history_size)
            --m_snapshot_idx;
    }

    // set the commit reference count to zero, which
    // is equivalent to forcing the last changes we
    // started tracking to save
    m_commit_ref = 0;
        
    // decrement the snapshot index and load the history
    --m_snapshot_idx;
    loadSnapshot(m_snapshot_idx);
}

void CanvasHistory::redo()
{
    // if we can't redo, we're done
    if (!canRedo())
        return;

    // set the commit reference count to zero, which
    // is equivalent to forcing the last changes we
    // started tracking to save
    m_commit_ref = 0;
        
    // increment the snapshot index and load the history
    ++m_snapshot_idx;
    loadSnapshot(m_snapshot_idx);
}

void CanvasHistory::track()
{
    // begin tracking changes; if the tracked changes
    // are committed, they are stored in the history
    // for undo/redo; if they are rolled back, they
    // are rejected, and do not enter into the history
    // for undo/redo
    beginTrackingChanges();
}

void CanvasHistory::commit()
{
    // commit the changes that we have been tracking
    // to the history
    endTrackingChanges(true);
}

void CanvasHistory::rollback()
{
    // reject the changes we have been tracking and
    // do not include them in the history
    endTrackingChanges(false);
}

void CanvasHistory::clear()
{
    // clear the history
    m_snapshot_idx = 0;
    m_commit_ref = 0;
    m_history.clear();
}


void CanvasHistory::beginTrackingChanges(const wxString& description)
{
    // note: this function begins tracking the changes for
    // saving a snapshot; it must be matched with a call to
    // endTrackingChanges(); calling beginTrackingChanges()
    // increments a reference count and while calling
    // endTrackingChanges() decrements the reference count;
    // when the reference count returns back to zero, the
    // history is saved; the reason for this is to avoid 
    // several operations trying to save a history for the same 
    // change; for example, if a text component is being edited 
    // and a cut operation is performed on the text, both the 
    // edit and the cut operation may try to save the state of 
    // the canvas, and without reference counting, the cut operation 
    // would create a duplicate history of a state that's already 
    // being commited

    // if we don't have a root component, we're done
    if (m_root_component.isNull())
        return;

    // increment the commit reference count
    ++m_commit_ref;

    // if the commit reference count is greater than 
    // one, we're done
    if (m_commit_ref > 1)
        return;

    // sanity check; make sure the snapshot idx isn't < 0 so
    // we don't try to resize the history to a negative value
    wxASSERT_MSG(m_snapshot_idx >= 0, wxT("Canvas history index out of range."));

    if (m_snapshot_idx < 0)
        m_snapshot_idx = 0;

    // before we commit a change, clear any snapshots
    // that are at or after the current snapshot index
    m_history.resize(m_snapshot_idx);

    // save the history
    saveSnapshot(description);

    // increment the canvas snapshot index
    ++m_snapshot_idx;
}

void CanvasHistory::endTrackingChanges(bool accept)
{
    // note: see beginTrackingChanges() for more information
    // about how changes are tracked and when they are
    // actually saved
    
    // if accept is true, the changes are stored to the
    // history; if accept is false, the changes are not
    // stored; this allows us begin tracking changes, then
    // accept or reject the changes depending on the outcome 
    // of the changes (i.e., commit() or rollback()); for 
    // example, when editing a component, we want to commit 
    // the history right before the edit is performed so that 
    // if it's undone, the position of the canvas, the cursor, 
    // etc, will be the way it was right before the change;
    // however, we don't know if the edit is going to be 
    // accepted or rejected so that we don't know if we 
    // really want to store the history or not

    // if we don't have a root component, we're done
    if (m_root_component.isNull())
        return;

    // decrement the commit reference count
    --m_commit_ref;
    
    // if the reference count isn't zero, we're done
    if (m_commit_ref > 0)
        return;
    
    // if the reference count is negative, we're trying to 
    // stop tracking the changes something that hasn't been 
    // initiated with a beginTrackingChanges(); set the  
    // reference count to zero, and we're done
    if (m_commit_ref < 0)
    {
        m_commit_ref = 0;
        return;
    }

    // we have a reference count of zero; if we're accepting 
    // the changes, leave the snapshot we archived, and we're 
    // done
    if (accept)
        return;
        
    // otherwise, we don't want to keep the changes, so remove 
    // the last snapshot we added and decrement the snapshot 
    // count
    m_history.pop_back();
    --m_snapshot_idx;
}

void CanvasHistory::saveSnapshot(const wxString& description)
{
    // if we don't have a root component, we're done
    if (m_root_component.isNull())
        return;

    // create a snapshot
    CanvasSnapshot snapshot;

    // save a snapshot of the root component and its
    // child components
    std::vector<IComponentPtr> components;
    
    components.push_back(m_root_component);
    getUniqueSubComponents(components, m_root_component);
    
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = components.end();
    
    for (it = components.begin(); it != it_end; ++it)
    {
        ComponentSnapshot h;
        h.m_component = *it;
        h.m_component_snapshot = (*it)->clone();
        snapshot.m_canvas_snapshot.push_back(h);
    }

    // save the snapshot description
    snapshot.m_description = description;

    // add the snapshot to the canvas history
    m_history.push_back(snapshot);
}

void CanvasHistory::loadSnapshot(int idx)
{
    // if we don't have a root component, we're done
    if (m_root_component.isNull())
        return;

    // if the index to load is outside the range of 
    // snapshots we have available, so we're done
    if (idx < 0 || idx >= (int)m_history.size())
        return;

    // get the snapshot
    CanvasSnapshot* snapshot = &m_history[idx];
    
    // load the saved component properties
    std::vector<ComponentSnapshot>::iterator it, it_end;
    it_end = snapshot->m_canvas_snapshot.end();
    
    for (it = snapshot->m_canvas_snapshot.begin();
         it != it_end; ++it)
    {
        it->m_component->copy(it->m_component_snapshot);
    }
}


}; // namespace kcanvas

