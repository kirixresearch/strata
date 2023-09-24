/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2007-03-06
 *
 */


#ifndef H_KCANVAS_DND_H
#define H_KCANVAS_DND_H


namespace kcanvas
{


// Canvas Drag and Drop Classes

static const wxChar* component_data_format = wxT("application/component_data_format");
static const wxChar* bitmap_data_format = wxT("application/bitmap_data_format");
static const wxChar* text_data_format = wxT("application/text_data_format");

// note: component data object is a component object that's
// used to package up the components we're interested
// in by storing them as children
class ComponentDataObject : public wxCustomDataObject
{
public:

    ComponentDataObject();
    virtual ~ComponentDataObject();

    void SetComponent(IComponentPtr comp);
    IComponentPtr GetComponent();
    void Free();
   
private:
 
    IComponentPtr m_comp;
    bool m_get_called;
};

class CanvasDataObjectComposite : public wxDataObjectComposite
{
public:

    CanvasDataObjectComposite();
    virtual ~CanvasDataObjectComposite();
    
    void AddDataObject(const wxString& format, wxDataObjectSimple* object);
    wxDataObjectSimple* GetDataObject(const wxString& format);

private:

    std::map<wxString, wxDataObjectSimple*> m_data_objects;

    wxString m_component_type;
    wxString m_bitmap_type;
    wxString m_text_type;
};


class CanvasDropTarget : public wxDropTarget
{
public:

    CanvasDropTarget();
    virtual ~CanvasDropTarget();

    CanvasDataObjectComposite* GetDataObject();

private:
    
    wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult result);
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult result);
    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult result);
    void OnLeave();

public:

    xcm::signal2<CanvasDataObjectComposite&, wxDragResult&> sigItemDragged;
    xcm::signal2<CanvasDataObjectComposite&, wxDragResult&> sigItemDropped;

private:

    CanvasDataObjectComposite* m_composite_data;
};


} // namespace kcanvas


#endif

