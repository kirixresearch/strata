/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2007-03-06
 *
 */


#include "kcanvas_int.h"
#include "dnd.h"


namespace kcanvas
{


ComponentDataObject::ComponentDataObject()
  : wxCustomDataObject(wxDataFormat(component_data_format))
{
    m_get_called = false;
}

ComponentDataObject::~ComponentDataObject()
{
}

void ComponentDataObject::SetComponent(IComponentPtr comp)
{
    if (comp.p)
    {
        comp.p->ref();
    }

    SetData(sizeof(IComponent*), (const void*)&(comp.p));
}

IComponentPtr ComponentDataObject::GetComponent()
{
    // the first time GetComponent is called, get
    // the pointer from the array, and unref it;
    // subsequent calls will get our cached copy
    
    // note: component data object is a component object 
    // that's used to package up the components we're 
    // interested in by storing them as children; as
    // a result call clone with the deep parameter
    // set to false so children aren't cloned but instead
    // have their pointers copied
    
    if (!m_get_called)
    {
        m_get_called = true;

        m_comp.clear();

        uintptr_t* data = (uintptr_t*)GetData();
        IComponent* comp = (IComponent*)data[0];
        if (comp)
        {
            m_comp = comp->clone();
            comp->unref();
        }
    }

    if (m_comp.isNull())
        return xcm::null;

    return m_comp->clone();
}

void ComponentDataObject::Free()
{
    // TODO: called when data is freed; we should
    // unref the smart pointer here to ensure it
    // is deleted
}

CanvasDataObjectComposite::CanvasDataObjectComposite()
{
    m_component_type = wxString(component_data_format);
    m_bitmap_type = wxString(bitmap_data_format);
    m_text_type = wxString(text_data_format);

    ComponentDataObject* component_object = new ComponentDataObject;
    m_data_objects[m_component_type] = component_object;
    
    wxBitmapDataObject* bitmap_object = new wxBitmapDataObject;
    bitmap_object->SetBitmap(wxNullBitmap);
    m_data_objects[m_bitmap_type] = bitmap_object;
    
    wxTextDataObject* text_object = new wxTextDataObject;
    text_object->SetText(wxEmptyString);
    m_data_objects[m_text_type] = text_object;

    this->Add(component_object, true);
    this->Add(bitmap_object, false);
    this->Add(text_object, false);
}

CanvasDataObjectComposite::~CanvasDataObjectComposite()
{
}

void CanvasDataObjectComposite::AddDataObject(const wxString& format, wxDataObjectSimple* object)
{
    // save the object
    m_data_objects[format] = object;

    // add the object to the list of supported formats
    this->Add(object, false);
}

wxDataObjectSimple* CanvasDataObjectComposite::GetDataObject(const wxString& format)
{
    std::map<wxString, wxDataObjectSimple*>::iterator it;
    it = m_data_objects.find(format);
    if (it != m_data_objects.end())
    {
        return it->second;
    }
    
    return NULL;
}

CanvasDropTarget::CanvasDropTarget() : wxDropTarget(NULL)
{
    m_composite_data = new CanvasDataObjectComposite;
    SetDataObject(m_composite_data);
}

CanvasDropTarget::~CanvasDropTarget()
{
}

CanvasDataObjectComposite* CanvasDropTarget::GetDataObject()
{
    return m_composite_data;
}

wxDragResult CanvasDropTarget::OnEnter(wxCoord x, wxCoord y, wxDragResult result)
{
    return result;
}

wxDragResult CanvasDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult result)
{
    sigItemDragged(*m_composite_data, result);
    return result;
}

wxDragResult CanvasDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult result)
{
    if (!GetData())
        return wxDragNone;

    sigItemDropped(*m_composite_data, result);
    return result;
}

void CanvasDropTarget::OnLeave()
{
}


} // namespace kcanvas

