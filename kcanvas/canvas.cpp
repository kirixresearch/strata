/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2002-01-25
 *
 */


#include "kcanvas_int.h"
#include "canvas.h"
#include "util.h"


namespace kcanvas
{


const wxString ACTION_NONE = wxT("canvas.action.none");
const wxString ACTION_DISPATCH = wxT("canvas.action.dispatch");
const wxString ACTION_NAVIGATE = wxT("canvas.action.navigate");
const wxString ACTION_CREATE = wxT("canvas.action.create");
const wxString ACTION_RESIZE = wxT("canvas.action.resize");
const wxString ACTION_MOVE = wxT("canvas.action.move");
const wxString ACTION_SELECT = wxT("canvas.action.select");


// canvas implementation

Canvas::Canvas()
{
    // connect the default canvas handlers
    EventHandler* handler = new kcanvas::EventHandler;
    handler->sigEvent().connect(this, &Canvas::onCanvasEvent);

    addEventHandler(kcanvas::EVENT_KEY, handler);
    addEventHandler(kcanvas::EVENT_KEY_DOWN, handler);
    addEventHandler(kcanvas::EVENT_KEY_UP, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_LEFT_DCLICK, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_LEFT_DOWN, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_LEFT_UP, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_DCLICK, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_DOWN, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_RIGHT_UP, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_DCLICK, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_DOWN, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_MIDDLE_UP, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_MOTION, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_WHEEL, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_ENTER, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_LEAVE, handler);
    addEventHandler(kcanvas::EVENT_MOUSE_OVER, handler);
    addEventHandler(kcanvas::EVENT_INVALIDATE, handler);

    // initialize scale and zoom variables
    m_scale_x = CANVAS_SCREEN_DPI;
    m_scale_y = CANVAS_SCREEN_DPI;
    m_zoom_percent = ZOOM_SCALE;

    // view size
    m_view_x = 0;
    m_view_y = 0;

    // set the draw origin
    m_draw_origin_x = 0;
    m_draw_origin_y = 0;

    // initialize mouse event variables
    m_mouse_action = ACTION_NONE;   // name of current mouse action
    m_mouse_model_xstart = 0;       // starting x pos of the mouse in model coordinates
    m_mouse_model_ystart = 0;       // starting y pos of the mosue in model coordinates
    m_mouse_model_xlast = 0;        // last x pos of the mouse in model coordinates
    m_mouse_model_ylast = 0;        // last y pos of the mouse in model coordinates
    m_mouse_model_x = 0;            // x pos of the mouse in model coordinates
    m_mouse_model_y = 0;            // y pos of the mouse in model coordinates
    m_mouse_client_x = 0;           // x pos of the mouse in client coordinates
    m_mouse_client_y = 0;           // y pos of the mouse in client coordinates
    m_mouse_capture = 0;            // mouse capture flag

     // initialize tool variable
    m_tool = toolSelect;

    // create the model component that contains other components 
    // in the canvas
    kcanvas::IComponentPtr model_component;
    model_component = Component::create();
    model_component->setOrigin(0, 0);
    model_component->setSize(0, 0);
    setModelComponent(model_component);
}

Canvas::~Canvas()
{
}

ICanvasPtr Canvas::create()
{
    return static_cast<ICanvas*>(new Canvas);
}

void Canvas::destroy()
{
    // since a canvas isn't reference counted, we need some way
    // to destroy a canvas if it isn't destroyed by an owner
    // object; for example, CanvasControl is automatically
    // destroyed because it also derives from wxControl;
    // however CanvasPDF and CanvasPrintout aren't automatically
    // destroyed, so we need a way to allow for this
    delete this;
}

ICanvasPtr Canvas::clone()
{
    Canvas* c = new Canvas;
    c->copy(this);

    return static_cast<ICanvas*>(c);
}

void Canvas::copy(ICanvasPtr canvas)
{    
    if (canvas.isNull())
        return;

    // clear the cache
    clearCache();

    // make a deep copy of the model
    IComponentPtr input_model, copy_model;
    input_model = canvas->getModelComponent();
    copy_model = CopyComponentTree(input_model);

    // if the copy is valid, set the current
    // model to the copy, otherwise, keep the
    // current model we have
    if (!copy_model.isNull())
        m_model_component = copy_model;

    // copy the scale and zoom
    canvas->getScale(&m_scale_x, &m_scale_y);
    m_zoom_percent = canvas->getZoom();
    
    // copy the view size
    canvas->getViewSize(&m_view_x, &m_view_y);
}

bool Canvas::setScale(int x, int y)
{
    // scale must be positive
    if (x <= 0 || y <= 0)
        return false;

    m_scale_x = x;
    m_scale_y = y;
    return true;
}

void Canvas::getScale(int* x, int* y)
{
    *x = m_scale_x;
    *y = m_scale_y;
}

int Canvas::mtod_x(int x) const
{
    double x_device = x*((double)m_scale_x/(double)CANVAS_MODEL_DPI)*((double)m_zoom_percent/(double)ZOOM_SCALE);
    double diff = x_device - (int)x_device;
    if (diff >= 0.5)
        return (int)x_device + 1;
    else
        return (int)x_device;
}

int Canvas::mtod_y(int y) const
{
    double y_device = y*((double)m_scale_y/(double)CANVAS_MODEL_DPI)*((double)m_zoom_percent/(double)ZOOM_SCALE);
    double diff = y_device - (int)y_device;
    if (diff >= 0.5)
        return (int)y_device + 1;
    else
        return (int)y_device;
}

int Canvas::dtom_x(int x) const
{
    double x_model = x*((double)CANVAS_MODEL_DPI/((double)m_scale_x*((double)m_zoom_percent/(double)ZOOM_SCALE)));
    double diff = x_model - (int)x_model;
    if (diff >= 0.5)
        return (int)x_model + 1;
    else
        return (int)x_model;
}

int Canvas::dtom_y(int y) const
{
    double y_model = y*((double)CANVAS_MODEL_DPI/((double)m_scale_y*((double)m_zoom_percent/(double)ZOOM_SCALE)));
    double diff = y_model - (int)y_model;
    if (diff >= 0.5)
        return (int)y_model + 1;
    else
        return (int)y_model;
}

void Canvas::clientToModel(int x_client, int y_client,
                           int* x_model, int* y_model) const
{
    int x_view_origin, y_view_origin;
    getViewOrigin(&x_view_origin, &y_view_origin);

    *x_model = dtom_x(x_client) + x_view_origin;
    *y_model = dtom_y(y_client) + y_view_origin;
}

void Canvas::modelToClient(int x_model, int y_model,
                           int* x_client, int* y_client) const
{
    int x_view_origin, y_view_origin;
    getViewOrigin(&x_view_origin, &y_view_origin);

    *x_client = mtod_x(x_model - x_view_origin);
    *y_client = mtod_y(y_model - y_view_origin);
}

void Canvas::setSize(int x, int y)
{
    m_model_component->setSize(x, y);
}

void Canvas::getSize(int* x, int* y) const
{
    m_model_component->getSize(x, y);
}

int Canvas::getWidth() const
{
    return m_model_component->getWidth();
}

int Canvas::getHeight() const
{
    return m_model_component->getHeight();
}

void Canvas::setViewOrigin(int x, int y)
{
    // the view origin is relative to the model origin; so we 
    // achieve the effect of moving the view origin by moving 
    // the model origin in the opposite direction
    m_model_component->setOrigin(-x, -y);
}

void Canvas::getViewOrigin(int* x, int* y) const
{
    // the view origin is relative to the model origin; so the
    // view origin is actually the negative of the model origin
    m_model_component->getOrigin(x, y);
    *x = -(*x);
    *y = -(*y);
}

void Canvas::setViewSize(int x, int y)
{
    if (x <= 0)
        x = 0;
        
    if (y <= 0)
        y = 0;

    m_view_x = x;
    m_view_y = y;
}

void Canvas::getViewSize(int* x, int* y) const
{
    *x = m_view_x;
    *y = m_view_y;
}

int Canvas::getViewWidth() const
{
    int width, height;
    getViewSize(&width, &height);
    return width;
}

int Canvas::getViewHeight() const
{
    int width, height;
    getViewSize(&width, &height);
    return height;
}

bool Canvas::scrollTo(int x, int y)
{
    return moveView(x, y);
}
    
bool Canvas::scrollBy(int x_diff, int y_diff)
{
    int x, y;
    getViewOrigin(&x, &y);
    return moveView(x + x_diff, y + y_diff);
}

void Canvas::setZoom(int percent)
{
    // if the new zoom is the same as the old one, 
    // we're done
    if (m_zoom_percent == percent)
        return;

    // clamp the zoom
    if (percent > ZOOM_MAX)
        percent = ZOOM_MAX;

    if (percent < ZOOM_MIN)
        percent = ZOOM_MIN;

    // set zoom
    m_zoom_percent = percent;

    // when we zoom, some components that were
    // visible may no longer be visible and vice
    // versa, so clear out the list of viewable
    // components so the viewable components are
    // recalculated in the render function
    clearRenderCache();

    // dispatch a canvas zoom event
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_CANVAS_ZOOM, this);
    dispatchEvent(notify_evt);
}

int Canvas::getZoom() const
{
    return m_zoom_percent;
}

bool Canvas::setModelComponent(IComponentPtr component)
{
    // note: sets the root-level component of the canvas,
    // which is the model component; TODO: perhaps we
    // should rename the model component to document
    // component, or something along those lines
    
    if (component.isNull())
        return false;

    // if the component is the same as the model component,
    // we're done
    if (component == m_model_component)
        return true;

    // clear the component cache
    clearCache();

    // canvas components
    std::vector<IComponentPtr> canvas_components;
    std::vector<IComponentPtr>::iterator it, it_end;

    // reset the canvas and parent members in the previous model
    // component and all it's children
    getAllSubComponents(canvas_components, m_model_component);
    canvas_components.push_back(m_model_component);

    it_end = canvas_components.end();
    for (it = canvas_components.begin(); it != it_end; ++it)
    {
        if (it->isNull())
            continue;

        Component* c = static_cast<Component*>(it->p);
        c->m_canvas = NULL;
        c->m_parent = NULL;
    }

    // set the new model component
    m_model_component = component;

    // set the canvas member in the new model component
    // and all it's children
    canvas_components.clear();
    getAllSubComponents(canvas_components, m_model_component);
    canvas_components.push_back(m_model_component);

    it_end = canvas_components.end();
    for (it = canvas_components.begin(); it != it_end; ++it)
    {
        if (it->isNull())
            continue;

        Component* c = static_cast<Component*>(it->p);
        c->m_canvas = NULL;  // initialize; will be set when populating the cache
        c->m_parent = NULL;  // initialize; will be set when populating the cache
    }

    // exception: set the model's canvas member
    Component* c = static_cast<Component*>(m_model_component.p);
    c->m_canvas = this;

    // dispatch canvas changed event and return true
    dispatchCanvasChangedEvent();
    return true;
}

IComponentPtr Canvas::getModelComponent()
{
    return m_model_component;
}

bool Canvas::add(IComponentPtr component)
{
    // add the component; if we can't add it, return false
    if (!m_model_component->add(component))
        return false;

    // clear the component cache
    clearCache();

    // dispatch canvas changed event
    dispatchCanvasChangedEvent();
    return true;
}

bool Canvas::remove(IComponentPtr component)
{
    // remove the component; if we can't remove it, return false
    if (!m_model_component->remove(component))
        return false;

    // clear the canvas component cache
    clearCache();

    // dispatch canvas changed event
    dispatchCanvasChangedEvent();
    return true;
}

void Canvas::removeAll()
{
    // remove all components
    m_model_component->removeAll();

    // clear the canvas component cache
    clearCache();
    
    // dispatch canvas changed event
    dispatchCanvasChangedEvent();
}

void Canvas::getComponents(std::vector<IComponentPtr>& components) const
{
    m_model_component->getComponents(components);
}

void Canvas::addProperty(const wxString& prop_name, const PropertyValue& value)
{
    m_properties.add(prop_name, value);
}

void Canvas::addProperties(const Properties& properties)
{
    m_properties.add(properties);
}

void Canvas::removeProperty(const wxString& prop_name)
{
    m_properties.remove(prop_name);
}

bool Canvas::setProperty(const wxString& prop_name, const PropertyValue& value)
{
    return m_properties.set(prop_name, value);
}

bool Canvas::setProperties(const Properties& properties)
{
    return m_properties.set(properties);
}

bool Canvas::getProperty(const wxString& prop_name, PropertyValue& value) const
{
    return m_properties.get(prop_name, value);
}

bool Canvas::getProperties(Properties& properties) const
{
    return m_properties.get(properties);
}

Properties& Canvas::getPropertiesRef()
{
    return m_properties;
}

IComponentPtr Canvas::getComponentById(const wxString& id)
{
    // note: this function returns the component with
    // the specified id

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = m_canvas_components.end();

    for (it = m_canvas_components.begin(); it != it_end; ++it)
    {
        if ((*it)->getId() == id)
            return *it;
    }

    return xcm::null;
}

IComponentPtr Canvas::getComponentByPos(int x, int y)
{
    // note: this function finds the top-most component
    // underneath a given point

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();
    
    // get all the components underneath the given point;
    // if no components are under the point, return null
    std::vector<IComponentPtr> components;
    getComponentsByPos(components, x, y);
    if (components.empty())
        return xcm::null;

    // if we found a component under the point, return the
    // last component on the list, which is the top-most
    // component
    return components.back();
}

void Canvas::getComponentsByPos(std::vector<IComponentPtr>& components,
                                int x,
                                int y)
{
    // note: this function finds all the components that are underneath
    // a given point; this isn't necessarily the parent-child hiearchy
    // of the top component under the given point since parent's can
    // contain children that are outside their own boundaries

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // first, search the viewable components
    bool viewable = false;
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = m_viewable_components.end();
    
    for (it = m_viewable_components.begin(); it != it_end; ++it)
    {
        // see if the point (x,y) is contained in the component; note: 
        // this depends on the component; some components, such as squares 
        // fill all the space alloted to them while other components, such 
        // as lines, do not; if contains() is overidden to reflect this, 
        // we need to account for it
        int _x, _y;
        getAbsolutePosition(*it, &_x, &_y);
        if (!(*it)->contains(x - _x, y - _y))
            continue;
            
        // if we've found a component, set the viewable flag and add it
        // to the list of components we've found
        viewable = true;
        components.push_back(*it);
    }

    // if we found the components in the viewable components, we're done
    if (viewable)
        return;

    // get the components that intersect a small rectangle
    wxRect rect = wxRect(x, y, 1, 1);
    getComponentsByRect(components, rect);
}

void Canvas::getComponentsByRect(std::vector<IComponentPtr>& components,
                                 const wxRect& rect)
{
    // note: this function finds all canvas components that
    // intersect an input rectangle that's in absolute model
    // coordinates

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    // if rect is empty, return all the canvas components
    if (rect.IsEmpty())
    {
        components = m_canvas_components;
        return;
    }

    // traverse through the canvas components and find those that 
    // intersect the input rectangle
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = m_canvas_components.end();

    for (it = m_canvas_components.begin(); it != it_end; ++it)
    {
        // get the absolute position and size of the canvas
        // component
        wxRect r;
        getAbsolutePosition(*it, &r.x, &r.y);
        getAbsoluteSize(*it, &r.width, &r.height);
    
        // find the intersection between the component's absolute
        // position and size and the input rectangle
        r.Intersect(rect);

        // TODO: need to test for intersection based on component 
        // function, to account for different shaped components, 
        // similar to how we're doing it in getComponentsByPos()
        // for points; for example, square components fill all the 
        // space alloted to them while line components do not; we 
        // want to make sure this function only picks up components 
        // that truly intersect the input rectangle

        // if the insersection results in an empty rectangle, we're
        // done with this component; continue onto the next
        if (r.IsEmpty())
            continue;

        // otherwise, add it to the list
        components.push_back(*it);
    }
}

void Canvas::getComponentsByType(std::vector<IComponentPtr>& components, const wxString& type)
{
    // note: this function returns a list of all components in the 
    // canvas of a particular type

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = m_canvas_components.end();

    for (it = m_canvas_components.begin(); it != it_end; ++it)
    {
        if ((*it)->getType() == type)
            components.push_back(*it);
    }
}

void Canvas::getComponentPos(IComponentPtr component, int* x, int* y)
{
    // note: this function gets the position of a component in
    // absolute model coordinates

    // if the cache is empty, populate it
    if (isCacheEmpty())
        populateCache();

    getAbsolutePosition(component, x, y);
}

bool Canvas::select(IComponentPtr component)
{
    // note: this function adds the specified component
    // to the list of selected components; if the component
    // is added to the list of selected components (i.e.,
    // newly selected), the function returns true;
    // otherwise, it returns false

    // if the component is null, we're done
    if (component.isNull())
        return false;

    // if the component is already selected, we're done
    if (isSelected(component))
        return false;

    // dispatch a pre-select event to indicate we're 
    // about to select a component      
    INotifyEventPtr notify_pre_select_evt;
    notify_pre_select_evt = NotifyEvent::create(EVENT_PRE_SELECT, component);
    dispatchEvent(notify_pre_select_evt);

    // if the event was vetoed, we're done
    if (!notify_pre_select_evt->isAllowed())
        return false;

    // add the component to the list of selected
    // components        
    m_selected_components.push_back(component);

    // dispatch a select event to indicate that we've
    // selected a component
    INotifyEventPtr notify_select_evt;
    notify_select_evt = NotifyEvent::create(EVENT_SELECT, component);
    dispatchEvent(notify_select_evt);

    return true;
}

bool Canvas::removeSelection(IComponentPtr component)
{
    // note: this function removes the specified component
    // from the list of specified components; if the
    // component exists as a selected and is then removed,
    // the function returns true; otherwise, it returns
    // false

    // if the component is null, we're done
    if (component.isNull())
        return false;

    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = m_selected_components.end();
    
    for (it = m_selected_components.begin(); it != it_end; ++it)
    {
        m_selected_components.erase(it);
        return true;
    }
    
    return false;
}

void Canvas::removeAllSelections()
{
    // note: this function clears the list of selected components

    m_selected_components.clear();
}

void Canvas::getSelections(std::vector<IComponentPtr>& components) const
{
    // note: this function gets the list of selected components

    components = m_selected_components;
}

bool Canvas::isSelected(IComponentPtr component) const
{
    // note: this function returns true if the specified
    // component is selected; otherwise, it returns false

    // if the component is null, we're done
    if (component.isNull())
        return false;

    std::vector<IComponentPtr>::const_iterator it, it_end;
    it_end = m_selected_components.end();
    
    for (it = m_selected_components.begin(); it != it_end; ++it)
    {
        if ((*it) == component)
            return true;
    }
    
    return false;
}

bool Canvas::hasSelections() const
{
    // note: this function returns true if at least one
    // component is selected; if no components are
    // selected, it returns false

    if (m_selected_components.empty())
        return false;
        
    return true;
}

bool Canvas::setFocus(IComponentPtr component)
{
    // note: sets the focus to the specified component

    // if the component is null, return false
    if (component.isNull())
        return false;

    // if the focus hasn't changed, we're done
    if (component == m_focus_component)
        return true;

    // TODO: only allow non-null components?; only allow
    // components that are in the canvas (i.e., have a
    // parent)?; need to decide

    // dispatch a kill focus event to the old focus component;
    // note: do this before setting the new focus component so
    // getFocus() in EVENT_FOCUS_KILL handlers will still return
    // the old focus
    INotifyEventPtr notify_kill_focus_evt;
    notify_kill_focus_evt = NotifyEvent::create(EVENT_FOCUS_KILL, m_focus_component);
    dispatchEvent(notify_kill_focus_evt);

    // set the new focus component
    m_focus_component = component;

    // dispatch a set focus event to the new focus component;
    // note: do this after setting the new focus component so
    // getFocus() in EVENT_FOCUS_SET handlers will return the
    // new focus
    INotifyEventPtr notify_set_focus_evt;
    notify_set_focus_evt = NotifyEvent::create(EVENT_FOCUS_SET, m_focus_component);
    dispatchEvent(notify_set_focus_evt);
    
    return true;
}

IComponentPtr Canvas::getFocus() const
{
    // note: returns the component that currently
    // has the focus

    return m_focus_component;
}

bool Canvas::hasFocus(IComponentPtr component) const
{
    // note: returns true if the specified component
    // currently has the focus; returns false otherwise

    if (component.isNull())
        return false;

    if (m_focus_component != component)
        return false;

    return true;
}

void Canvas::layout()
{
    // note: this function 1) sets the position and size of all the components 
    // in the canvas, and 2) limits the view position to the model

    // first, set the position and size of all the components in the canvas;
    // to do this, 1) dispatch a pre-layout event so handlers have a chance
    // to stop the layout or do any last minute processing, such as component-
    // assembly, before the layout happens; 2) determine the parent-child
    // relationships of the canvas components by populating the canvas component
    // cache; 3) iterate through the components, a) dispatching layout events to
    // each component, b) reading the positions and size of the components 
    // after the layout event and comparing with the previous position and size
    // of the components, c) repeating this process until the position and size
    // of a component before dispatching an event is the same as the position
    // and size of it after the event, for each component in the hierarchy, or
    // until a maximum number of iterations is reached (to handle the case of
    // an "unstable" layout)

    // dispatch a pre-layout event
    INotifyEventPtr notify_pre_evt;
    notify_pre_evt = NotifyEvent::create(EVENT_PRE_LAYOUT, this);
    dispatchEvent(notify_pre_evt);
    
    if (!notify_pre_evt->isAllowed())
        return;

    // populate the cache; here we do this without regard to
    // whether or not the cache is already populated; that way,
    // layout() will refresh the layout no matter what
    populateCache();

    // iterate through the canvas components up to the
    // maximum number of allowed layout iterations
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = m_canvas_components.end();
    
    int count = 0;
    bool layout_finished = false;
    
    while (!layout_finished)
    {
        // get the model size
        int model_width, model_height;
        getSize(&model_width, &model_height);
    
        layout_finished = true;
        for (it = m_canvas_components.begin(); it != it_end; ++it)
        {
            // save the old position and size of the component
            int old_absolute_x, old_absolute_y, old_absolute_w, old_absolute_h;
            getAbsolutePosition(*it, &old_absolute_x, &old_absolute_y);
            getAbsoluteSize(*it, &old_absolute_w, &old_absolute_h);
        
            // dispatch a layout notify event to the component
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_LAYOUT, *it);
            dispatchEvent(notify_evt);
        
            // set the new position and size of the component
            // from the component properties
            Component* comp_child = static_cast<Component*>(it->p);
            Component* comp_parent = comp_child->m_parent;
            
            int x_child, y_child, w_child, h_child;
            (*it)->getOrigin(&x_child, &y_child);
            (*it)->getSize(&w_child, &h_child);
            
            comp_child->m_absolute_x = comp_parent->m_absolute_x + x_child;
            comp_child->m_absolute_y = comp_parent->m_absolute_y + y_child;
            comp_child->m_absolute_w = w_child;
            comp_child->m_absolute_h = h_child;

            // if the layout flag is currently set to true to indicate
            // we're done laying out the components, then compare the 
            // current position and size with the previous position and 
            // size; if anything is different, set the layout to false to 
            // indicate we're not done; if the layout flag is set to
            // false, we already know we need to layout the components
            // again, so we're done
            if (layout_finished)
            {
                if (old_absolute_x != comp_child->m_absolute_x)
                    layout_finished = false;
                    
                if (old_absolute_y != comp_child->m_absolute_y)
                    layout_finished = false;
                    
                if (old_absolute_w != comp_child->m_absolute_w)
                    layout_finished = false;
                    
                if (old_absolute_h != comp_child->m_absolute_h)
                    layout_finished = false;
            }
        }

        // if the model size has changed, set the layout_finished
        // flag to false
        if (model_width != getWidth() || model_height != getHeight())
            layout_finished = false;

        // don't allow more than a certain number of iterations
        if (count >= CANVAS_LAYOUT_ITERATION_MAX)
            layout_finished = true;

        count++;
    }

    // dispatch a notify event with the canvas as the target to 
    // signal the end of the component layout sequence
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_LAYOUT, this);
    dispatchEvent(notify_evt);

    // limit the view position to the model    
    int x_view_origin, y_view_origin;
    getViewOrigin(&x_view_origin, &y_view_origin);
    
    int x_view_size, y_view_size;
    getViewSize(&x_view_size, &y_view_size);

    int x_model_size, y_model_size;
    m_model_component->getSize(&x_model_size, &y_model_size);

    if (x_view_origin > x_model_size - x_view_size)
        x_view_origin = x_model_size - x_view_size;
    if (y_view_origin > y_model_size - y_view_size)
        y_view_origin = y_model_size - y_view_size;
    if (x_view_origin < 0)
        x_view_origin = 0;
    if (y_view_origin < 0)
        y_view_origin = 0;
    
    setViewOrigin(x_view_origin, y_view_origin);
    
    // TODO: if the view has been clamped, we really should layout
    // the components again since they may be depending on view
    // info
}

void Canvas::render(const wxRect& rect)
{
    // note: rect is in absolute model coordinates, not view coordinates

    // get the viewport rectangle so we can use it is our default 
    // update rectangle if no additional update rectangle is specified
    int x, y, w, h;
    getViewOrigin(&x, &y);
    getViewSize(&w, &h);

    // if the width or height of the view is zero, we're done
    if (w <= 0 || h <= 0)
        return;

    // create a view rectangle from the input rectangle; however, if we're
    // not printing, expand the view rectangle to the viewport
    wxRect view_rect = rect;
    if (!isPrinting())
        view_rect = wxRect(x, y, w, h);

    // get the viewable components; if we already have them in the cache,
    // we don't need to do anything; however if the cache is empty, populate
    // it with the components that fall in the view rectangle; note: we need
    // to populate the viewable components with all the components that fall
    // in the view, even if we only need to render components in a smaller
    // update rectangle, because the viewable components are searched by mouse
    // related functions to speed up user interaction with canvas components
    if (m_viewable_components.empty())
        getComponentsByRect(m_viewable_components, view_rect);

    // make sure the update rectangle isn't larger than the viewport
    wxRect update_rect = rect.Intersect(view_rect);

    // if an update rectangle isn't specified, use the viewport 
    // rectangle for the update rectangle
    if (rect.IsEmpty())
        update_rect = view_rect;

    // if we don't have a valid update rectangle, we're done
    if (update_rect.IsEmpty())
        return;

    // set the draw origin to zero, so we can render everything relative 
    // to the absolute coordinate system; however, before we can do this, 
    // save the origin so we can restore it at the end of the function,
    // since we change the draw origin to the origin of each component we 
    // draw, and we want to leave the draw origin the same as it was when 
    // this function was called
    int old_draw_origin_x, old_draw_origin_y;
    getDrawOrigin(&old_draw_origin_x, &old_draw_origin_y);

    // set the draw origin to (0,0) to model coordinates
    setDrawOrigin(0,0);
    
    // dispatch a pre-render event    
    INotifyEventPtr notify_pre_evt;
    notify_pre_evt = NotifyEvent::create(EVENT_PRE_RENDER, this);
    notify_pre_evt->addProperty(EVENT_PROP_X, update_rect.x);
    notify_pre_evt->addProperty(EVENT_PROP_Y, update_rect.y);
    notify_pre_evt->addProperty(EVENT_PROP_W, update_rect.width);
    notify_pre_evt->addProperty(EVENT_PROP_H, update_rect.height);
    dispatchEvent(notify_pre_evt);

    // restore the draw origin
    setDrawOrigin(old_draw_origin_x, old_draw_origin_y);

    if (!notify_pre_evt->isAllowed())
        return;

    // iterate through viewable components and draw them
    std::vector<IComponentPtr>::const_iterator it, it_end;
    it_end = m_viewable_components.end();

    for (it = m_viewable_components.begin(); it != it_end; ++it)
    {
        // add a top-level clipping rectangle
        setDrawOrigin(0,0);
        removeAllClippingRectangles();
        addClippingRectangle(update_rect.x,
                             update_rect.y,
                             update_rect.width,
                             update_rect.height);
        setDrawOrigin(old_draw_origin_x, old_draw_origin_y);
    
        // drawable area
        wxRect rect_comp;
        getAbsolutePosition(*it, &rect_comp.x, &rect_comp.y);
        getAbsoluteSize(*it, &rect_comp.width, &rect_comp.height);

        // if the rectangle intersects the component, render the area
        if (update_rect.Intersects(rect_comp))
        {
            // set the draw origin to the origin of the component we're 
            // drawing and translate the render rectangle into the
            // local coordinates of the component
            setDrawOrigin(rect_comp.x, rect_comp.y);
            update_rect.Offset(-rect_comp.x, -rect_comp.y);

            // TODO: dispatching an event here gets us into a little trouble 
            // with screen repaints triggered by the canvas update during printing;
            // for now, we're not allowing screen redraws when the printer is the 
            // dc, but we may want to think of a more comprehensive solution

            // create a notify event; the event parameters correspond to the
            // render rectangle in the coordinates of the component we're
            // rendering
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_RENDER, *it);
            notify_evt->addProperty(EVENT_PROP_X, update_rect.x);
            notify_evt->addProperty(EVENT_PROP_Y, update_rect.y);
            notify_evt->addProperty(EVENT_PROP_W, update_rect.width);
            notify_evt->addProperty(EVENT_PROP_H, update_rect.height);
            dispatchEvent(notify_evt);

            // set the render rectangle back to what it was
            update_rect.Offset(rect_comp.x, rect_comp.y);
        }
    }
    
    // remove all clipping rectangles
    removeAllClippingRectangles();

    // restore the drawing origin, so that the draw origin 
    // is the same as when we started
    setDrawOrigin(old_draw_origin_x, old_draw_origin_y);

    // dispatch a notify event with the canvas as the target to 
    // signal the end of the component render sequence
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_RENDER, this);
    notify_evt->addProperty(EVENT_PROP_X, update_rect.x);
    notify_evt->addProperty(EVENT_PROP_Y, update_rect.y);
    notify_evt->addProperty(EVENT_PROP_W, update_rect.width);
    notify_evt->addProperty(EVENT_PROP_H, update_rect.height);
    dispatchEvent(notify_evt);
}

bool Canvas::isPrinting() const
{
    return false;
}

bool Canvas::setDrawOrigin(int x, int y)
{
    int view_x = 0;
    int view_y = 0;
    getViewOrigin(&view_x, &view_y);

    m_draw_origin_x = x - view_x;
    m_draw_origin_y = y - view_y;
    
    return true;
}

void Canvas::getDrawOrigin(int* x, int* y) const
{
    int view_x = 0;
    int view_y = 0;
    getViewOrigin(&view_x, &view_y);

    *x = m_draw_origin_x + view_x;
    *y = m_draw_origin_y + view_y;
}

void Canvas::moveDrawOrigin(int x, int y)
{
    int draw_origin_x;
    int draw_origin_y;

    getDrawOrigin(&draw_origin_x, &draw_origin_y);
    setDrawOrigin(draw_origin_x + x, draw_origin_y + y);
}

bool Canvas::setPen(const Pen& pen)
{
    return false;
}

bool Canvas::setPen(const Properties& properties)
{
    return false;
}

bool Canvas::setBrush(const Brush& brush)
{
    return false;
}

bool Canvas::setBrush(const Properties& properties)
{
    return false;
}

bool Canvas::setFont(const Font& font)
{
    return false;
}

bool Canvas::setTextBackground(const Color& color)
{
    return false;
}

bool Canvas::setTextForeground(const Color& color)
{
    return false;
}

void Canvas::drawImage(const wxImage& image,
                        int x, int y, int w, int h)
{
}

void Canvas::drawCheckBox(int x, int y, int w, int h, bool checked)
{
}

void Canvas::drawCircle(int x, int y, int r)
{
}

void Canvas::drawEllipse(int x, int y, int w, int h)
{
}

void Canvas::drawLine(int x1, int y1, int x2, int y2)
{
}

void Canvas::drawPoint(int x, int y)
{
}

void Canvas::drawPolygon(int n, wxPoint points[], int x, int y)
{
}

void Canvas::drawGradientFill(const wxRect& rect,
                              const Color& start_color,
                              const Color& end_color,
                              const wxDirection& direction)
{
}

void Canvas::drawRectangle(int x, int y, int w, int h)
{
}

void Canvas::drawText(const wxString& text, int x, int y)
{
}

bool Canvas::getTextExtent(const wxString& text,
                           int* x,
                           int* y,
                           int* descent,
                           int* external) const
{
    return false;
}

bool Canvas::getPartialTextExtents(const wxString& text,
                                   wxArrayInt& widths) const
{
    return false;
}

int Canvas::getCharHeight() const
{
    return 0;
}

int Canvas::getCharWidth() const
{
    return 0;
}

void Canvas::addClippingRectangle(int x, int y, int w, int h)
{
}

void Canvas::removeClippingRectangle()
{
}

void Canvas::removeAllClippingRectangles()
{
}

bool Canvas::addEventHandler(const wxString& name,
                             IEventHandlerPtr handler,
                             bool capture_phase)
{
    return m_event_target.addEventHandler(name, handler, capture_phase);
}

bool Canvas::removeEventHandler(const wxString& name,
                                IEventHandlerPtr handler,
                                bool capture_phase)
{
    return m_event_target.removeEventHandler(name, handler, capture_phase);
}

void Canvas::removeAllEventHandlers()
{
    m_event_target.removeAllEventHandlers();
}

void Canvas::dispatchEvent(IEventPtr evt)
{
    // if the event is bad, we're done
    if (evt.isNull())
        return;

    // we want to handle posted (async) events at the very end 
    // of the event loop; by doing this, we guarantee that an event 
    // that's posted in the course of handling another another event 
    // will never get processed before that particular event is done 
    // getting handled; to do this, we need to keep track of the 
    // very first event that started the sequence and not process 
    // posted events for other events besides this
    if (m_initial_event.isNull())
        m_initial_event = evt;

    // if the event is an asynchronous event, add it to the list 
    // of posted events
    if (evt->isAsync())
        m_posted_events.push_back(evt);
    
    // if the event is not asynchronous, handle it; if it's 
    // asynchronous, we'll handle it at the end of the event 
    // loop
    if (!evt->isAsync())  
        handleEvent(evt);

    // if we're not on the initial event, we're done, because we 
    // want to handle posted events at the very end of the event 
    // loop; see above
    if (m_initial_event != evt)
        return;

    // handle any posted events; copy the events since new
    // events may be posted while handling the events
    std::vector<IEventPtr> posted_events = m_posted_events;
    m_posted_events.clear();
    
    std::vector<IEventPtr>::iterator it, it_end;
    it_end = posted_events.end();

    for (it = posted_events.begin(); it != it_end; ++it)
    {
        handleEvent(*it);
    }
        
    // reset the initial event to null
    m_initial_event = xcm::null;
}

void Canvas::dispatchCanvasChangedEvent()
{
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_CANVAS_CHANGED, this);
    dispatchEvent(notify_evt);
}

void Canvas::getParentComponents(std::vector<IComponentPtr>& components,
                                 IComponentPtr child)
{
    // note: this function gets the parents of the given child component,
    // starting with the top-most parent, and traversing down to the
    // child component's immediate parent
    
    // if we don't have an initial child component, we're done
    if (child.isNull())
        return;

    // get the hierarchy of parents in reverse order, including
    // the child for now
    IComponentPtr comp = child;
    while (!comp.isNull())
    {
        components.push_back(comp);
        comp = getParent(comp);
    }
    
    // reverse the hiearchy so we start with the top-most parent,
    // and remove the last element, since it's the child
    std::reverse(components.begin(), components.end());
    components.pop_back();
}

void Canvas::getChildComponents(std::vector<IComponentPtr>& components,
                                IComponentPtr parent)
{
    // note: this function gets all the child components of the given
    // parent component, setting the parent member of each child component 
    // to the parent component of the child

    // if we don't have an initial parent component, we're done
    if (parent.isNull())
        return;

    // create a raw component pointer to the parent component,
    // which is what we'll set the parent member to in the
    // child component
    Component* ptr_parent;
    ptr_parent = static_cast<Component*>(parent.p);

    // get the child components of the initial component
    std::vector<IComponentPtr> c;
    parent->getComponents(c);
    
    // iterate through the child components
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = c.end();
    
    for (it = c.begin(); it != it_end; ++it)
    {
        // if for some reason, a child is null, move on
        if (it->isNull())
            continue;

        // get the raw child component
        Component* ptr_child;
        ptr_child = static_cast<Component*>(it->p);

        // set the canvas member to the same as the parent's canvas
        ptr_child->m_canvas = ptr_parent->m_canvas;

        // if the child isn't visible, don't include it in the list;
        // TODO: we want to exclude visible components from the layout;
        // however, this function might be general enough that we don't
        // want to do it here
        if (!(*it)->isVisible())
            continue;

        // set the parent member of the child component
        // to the parent component
        ptr_child->m_parent = ptr_parent;

        // add each child component onto the list of components
        // and in turn, get each of their children
        components.push_back(*it);
        getChildComponents(components, *it);
    }
}

void Canvas::getAbsolutePosition(IComponentPtr component, int* x, int* y)
{
    // note: this function gets the position of a component in
    // absolute model coordinates; this position is determined
    // in the layout and won't be correct unless layout() is
    // called first

    if (component.isNull())
        return;

    Component* comp_ptr = static_cast<Component*>(component.p);
    *x = comp_ptr->m_absolute_x;
    *y = comp_ptr->m_absolute_y;
}

void Canvas::getAbsoluteSize(IComponentPtr component, int* w, int* h)
{
    // note: this function gets the size of a component in
    // absolute model coordinates; this size is determined
    // in the layout and won't be correct unless layout() is
    // called first; because size measures a length rather
    // than a position, the absolute size after calling
    // layout() is the same as the size returned by the 
    // component; this function is included for convenience 
    // in laying out the components

    if (component.isNull())
        return;

    Component* comp_ptr = static_cast<Component*>(component.p);
    *w = comp_ptr->m_absolute_w;
    *h = comp_ptr->m_absolute_h;
}

IComponentPtr Canvas::getParent(IComponentPtr component)
{
    if (component.isNull())
        return xcm::null;

    Component* comp_ptr = static_cast<Component*>(component.p);
    return static_cast<IComponent*>(comp_ptr->m_parent);
}

IComponentPtr Canvas::findComponentByPos(const wxString& type, int* x, int* y)
{
    // note: this function finds the component of a particular type
    // nearest to the point given by (x,y); the function returns
    // the component that we found and sets the point (x,y) to the
    // actual position of the component in absolute model coordintates

    // component that we found
    IComponentPtr comp_find;

    // find all the components of a particular type
    std::vector<IComponentPtr> components;
    getComponentsByType(components, type);
    
    // if we don't have any of the particular component, 
    // we're done
    if (components.empty())
        return xcm::null;
    
    // if we only have one of the particular component,
    // set the position, and return the component
    if (components.size() == 1)
    {
        IComponentPtr comp_find = components.back();
        getAbsolutePosition(comp_find, x, y);
        return comp_find;
    }
    
    // iterate through the components of the particular type
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = components.end();
    
    for (it = components.begin(); it != it_end; ++it)
    {
        Component* comp_ptr = static_cast<Component*>(it->p);
    
        int x1 = comp_ptr->m_absolute_x;
        int y1 = comp_ptr->m_absolute_y;
        int x2 = comp_ptr->m_absolute_x + comp_ptr->m_absolute_w;
        int y2 = comp_ptr->m_absolute_y + comp_ptr->m_absolute_h;
    
        // if the component contains the given point, we're done
        if (*x >= x1 && *x <= x2 && *y >= y1 && *y <= y2)
        {
            comp_find = *it;
            break;
        }
        
        // if we don't have a component that's closest to the
        // initial point, set it to the first one we encounter
        if (comp_find.isNull())
            comp_find = *it;
        
        // find the distance between the previous component we've
        // found and the new one; if the distance from the input
        // point is closer to the new component we've found, set
        // the found component to the new one
        int prev_x, prev_y, new_x, new_y;
        getAbsolutePosition(comp_find, &prev_x, &prev_y);
        getAbsolutePosition(*it, &new_x, &new_y);

        int prev_distance = abs(prev_x - *x) + abs(prev_y - *y);
        int new_distance = abs(new_x - *x) + abs(new_y - *y);

        if (new_distance < prev_distance)
            comp_find = *it;
    }

    // return the found component
    getAbsolutePosition(comp_find, x, y);
    return comp_find;
}

bool Canvas::moveView(int x, int y)
{
    // note: moves the view; however, the canvas
    // has to be rendered for the new view position
    // to show; returns true if the view moved, and
    // false otherwise

    // only do something in the view changes
    int x_old, y_old;
    getViewOrigin(&x_old, &y_old);
    if (x == x_old && y == y_old)
        return false;

    // set the origin
    setViewOrigin(x, y);

    // when we move the view, some components that
    // were visible may no longer be visible and vice
    // versa, so clear out the list of viewable
    // components so the viewable components are
    // recalculated in the render function
    clearRenderCache();

    // dispatch a canvas scrolled event
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_CANVAS_SCROLL, this);
    dispatchEvent(notify_evt);
    
    // the view moved; return true
    return true;
}

void Canvas::invalidate(IComponentPtr comp)
{
    if (comp.isNull())
        return;

    Component* c = static_cast<Component*>(comp.p);
    c->invalidate();
}

void Canvas::invalidateSelection()
{
    // note: this function invalidates all the selected components;
    // TODO: for efficiency, we might want to only invalidate the
    // rectangular areas around the edges that contain the selection
    // outline

    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = m_selected_components.end();

    for (it = m_selected_components.begin(); it != it_end; ++it)
    {
        invalidate(it->p);
    }
}

bool Canvas::isCacheEmpty() const
{
    if (m_canvas_components.empty())
        return true;
        
    return false;
}

void Canvas::populateCache()
{
    // make sure the cache is empty
    clearCache();

    // populate the canvas components; the other caches will
    // get their elements from the canvas components on an
    // as-needed basis
    getChildComponents(m_canvas_components, m_model_component);
}

void Canvas::clearCache()
{
    // clear all lists of components dependent on the list
    // of canvas components
    m_canvas_components.clear();
    m_viewable_components.clear();
}

void Canvas::clearRenderCache()
{
    // clear the list of viewable components
    m_viewable_components.clear();
}

void Canvas::preProcessEvent(std::vector<IComponentPtr>& target_parents,
                             IEventPtr evt)
{
    // note: this function changes the target of the key and mouse events
    // from the canvas to the appropriate component; it then finds and
    // returns the hierarchy of parent components for the target, starting
    // with the top-most parent and traversing to the parent of the target;
    // also handles additional mouse event items such as mouse capture and
    // event timer on/off

    // the target of the event; used throughout
    IComponentPtr target_component;

    // we want the focus component to receive key events, so if we 
    // have a key event, set the target to the focus component
    // and return the hierarchy of the target components parents
    IKeyEventPtr key_evt = evt;
    if (!key_evt.isNull())
    {
        // set the target
        target_component = getFocus();
        evt->setTarget(target_component);
        
        // set the position of the event to local coordinates
        int comp_x, comp_y;
        getAbsolutePosition(target_component, &comp_x, &comp_y);
        key_evt->setX(key_evt->getX() - comp_x);
        key_evt->setY(key_evt->getY() - comp_y);
        
        // get the target parent hierarchy
        getParentComponents(target_parents, target_component);
        return;
    }

    // dispatch a mouse over event to the new target; note: the
    // mouse over event is fired when the mouse cursor moves
    // relative the model, regardless of whether it actually
    // moves relative to the view; for example, if the canvas
    // is zoomed using the keys, the mouse position doesn't
    // change relative to the view, but it does change relative
    // to the model; this event is useful for updating the cursor
    
    // TODO: we might want to extend this event to a mouse hover
    // event by firing the event initially when the mouse moves
    // over a new model location, then at regular intervals
    // thereafter; this would be useful for functionality such
    // as tooltips
    if (evt->getName() == EVENT_MOUSE_MOTION ||
        evt->getName() == EVENT_MOUSE_ENTER ||
        evt->getName() == EVENT_CANVAS_ZOOM ||
        evt->getName() == EVENT_CANVAS_SCROLL)
    {
        // use the last known client position as our starting
        // point; if the event is a canvas zoom or scroll event,
        // the mouse is still in the same client position, but
        // a different model position; so calculate the model
        // position
        int mouse_over_client_x = m_mouse_client_x;
        int mouse_over_client_y = m_mouse_client_y;
        
        int mouse_over_model_x, mouse_over_model_y;
        clientToModel(mouse_over_client_x, mouse_over_client_y,
                      &mouse_over_model_x, &mouse_over_model_y);

        // if the event is a mouse motion or enter event,
        // the mouse event itself has the correct model
        // and client positions, so use these
        IMouseEventPtr mouse_evt = evt;
        if (!mouse_evt.isNull())
        {
            mouse_over_model_x = mouse_evt->getX();
            mouse_over_model_y = mouse_evt->getY();
            mouse_over_client_x = mouse_evt->getClientX();
            mouse_over_client_y = mouse_evt->getClientY();
        }

        // create a mouse over event; set the target to the canvas 
        // for now; since we've set the correct model and client 
        // position, the appropriate target will be when the event
        // is processed
        IMouseEventPtr mouse_over_evt;
        mouse_over_evt = MouseEvent::create(EVENT_MOUSE_OVER, this);
        mouse_over_evt->setX(mouse_over_model_x);
        mouse_over_evt->setY(mouse_over_model_y);
        mouse_over_evt->setClientX(m_mouse_client_x);
        mouse_over_evt->setClientY(m_mouse_client_y);
        dispatchEvent(mouse_over_evt);
    }

    // at this point, we have a canvas target and if we have a mouse
    // event, we need to find the target based on the position of
    // the mouse; if we don't have a mouse, event, we don't need
    // to do anything else
    IMouseEventPtr mouse_evt = evt;
    if (mouse_evt.isNull())
        return;

    // initialize mouse position variables
    m_mouse_model_xlast = m_mouse_model_x;
    m_mouse_model_ylast = m_mouse_model_y;
    m_mouse_model_x = mouse_evt->getX();
    m_mouse_model_y = mouse_evt->getY();
    m_mouse_client_x = mouse_evt->getClientX();
    m_mouse_client_y = mouse_evt->getClientY();

    // set the target component to the component
    // at the mouse position
    target_component = getComponentByPos(m_mouse_model_x, m_mouse_model_y);

    // if we have a down event, prepare mouse action state
    if (evt->getName() == EVENT_MOUSE_LEFT_DOWN ||
        evt->getName() == EVENT_MOUSE_RIGHT_DOWN ||
        evt->getName() == EVENT_MOUSE_MIDDLE_DOWN)
    {
        // set the focus to the new target
        setFocus(target_component);

        // on mouse down events, set the capture state to
        // true, and on mouse up events, set it to false;
        // if capture is true, events will be routed to the
        // focus component; TODO: may want to add an official
        // mechanism on the target interface for this
        m_mouse_capture = true;
    }

    // if we don't have a selection tool, we're done
    if (m_tool != toolSelect)
        return;

    // we want to dispatch all events, not just those allowed by
    // startAction(), endAction() and isAction(); so, handle the
    // action state manually; if an action is ongoing and it's
    // not a dispatch action, we're done
    if (m_mouse_action != ACTION_NONE && m_mouse_action != ACTION_DISPATCH)
        return;

    // normally, we want to send all mouse events to whatever
    // component we happen to be over; the exception is if the
    // mouse is being captured, in which case, we want the focus
    // component to capture the mouse so that it continues to 
    // receive mouse events; TODO: may want to add an official
    // mechanism on the target interface for this
    if (m_mouse_capture)
        target_component = getFocus();

    // release the mouse capture after we've ensured the
    // mouse up will still go to the focus component
    if (evt->getName() == EVENT_MOUSE_LEFT_UP ||
        evt->getName() == EVENT_MOUSE_RIGHT_UP ||
        evt->getName() == EVENT_MOUSE_MIDDLE_UP)
    {
        m_mouse_capture = false;
    }

    // if the target mouse component is different then the current 
    // mouse target, dispatch the mouse enter and mouse leave events;
    // m_mouse_target is used to track the target to see if it changes 
    // between the last mouse event and the current mosue event
    if (target_component != m_mouse_target)
    {
        // TODO: when a mouse enter/leave event is fired on the canvas,
        // the event is coming here, triggering a secondary dispatch
        // of the same event in some cases

        // dispatch a mouse leave event to the old target
        if (!m_mouse_target.isNull())
        {
            int comp_leave_x, comp_leave_y;
            getAbsolutePosition(m_mouse_target, &comp_leave_x, &comp_leave_y);
            
            IMouseEventPtr mouse_leave_evt;
            mouse_leave_evt = MouseEvent::create(EVENT_MOUSE_LEAVE, m_mouse_target);
            mouse_leave_evt->setX(m_mouse_model_x - comp_leave_x);
            mouse_leave_evt->setY(m_mouse_model_y - comp_leave_y);
            mouse_leave_evt->setClientX(m_mouse_client_x);
            mouse_leave_evt->setClientY(m_mouse_client_y);
            dispatchEvent(mouse_leave_evt);
        }

        // dispatch a mouse enter event to the new target
        if (!target_component.isNull())
        {
            int comp_enter_x, comp_enter_y;
            getAbsolutePosition(target_component, &comp_enter_x, &comp_enter_y);

            IMouseEventPtr mouse_enter_evt;
            mouse_enter_evt = MouseEvent::create(EVENT_MOUSE_ENTER, target_component);
            mouse_enter_evt->setX(m_mouse_model_x - comp_enter_x);
            mouse_enter_evt->setY(m_mouse_model_y - comp_enter_y);
            mouse_enter_evt->setClientX(m_mouse_client_x);
            mouse_enter_evt->setClientY(m_mouse_client_y);
            dispatchEvent(mouse_enter_evt);
        }

        // set the new mouse target
        m_mouse_target = target_component;
    }

    // set the target
    evt->setTarget(target_component);
    
    // set the position of the event to local coordinates
    int comp_x, comp_y;
    getAbsolutePosition(target_component, &comp_x, &comp_y);
    mouse_evt->setX(mouse_evt->getX() - comp_x);
    mouse_evt->setY(mouse_evt->getY() - comp_y);
    
    // get the parent hierarchy
    getParentComponents(target_parents, target_component);
}

void Canvas::handleEvent(IEventPtr evt)
{
    // note: events are processed similar to the DOM, where event 
    // handlers are called in three phases: the capture phase, the 
    // at-target phase, and the bubble phase; in this sequence, the 
    // event handlers are first called on the top-level parent that
    // contains the target, which in this case is the canvas; then 
    // the event handlers are called on the rest of the parents of 
    // the target until the target itself is reached; when the target
    // is reached, the event phase changes to the at-target phase, 
    // and the event handlers are called on the target; then, the 
    // event phase changes again to the bubbling phase, and the event
    // handlers are called in the reverse order on the parents of
    // the target, all the way back up to the canvas


    // following are used throughout
    std::vector<IComponentPtr> target_parents;
    IEventTargetPtr target;
    ICanvasPtr canvas;

    // pre-process the event; reserve space for a target
    // hierarchy of 10 parent components; set the target
    // to whatever the preProcessEvent() function may
    // have set it to
    target_parents.reserve(10);
    
    // get the target; if the target isn't the canvas, get the
    // parent hierarchy; otherwise find out the target based
    // on the kind of event
    canvas = evt->getTarget();
    if (canvas.isNull())
        getParentComponents(target_parents, evt->getTarget());
         else
        preProcessEvent(target_parents, evt);

    target = evt->getTarget();

    // if we don't have an event target, we're done
    if (target.isNull())
        return;

    // if the target of the event is still the canvas, set 
    // the phase to the "at-target" phase and dispatch the 
    // event to the handlers and we're done
    canvas = target;
    if (!canvas.isNull())
    {
        target.clear();
        canvas.clear();

        evt->setPhase(EVENT_AT_TARGET);
        m_event_target.dispatchEvent(evt);
        return;
    }

    // save the draw origin
    int old_draw_x, old_draw_y;
    getDrawOrigin(&old_draw_x, &old_draw_y);

    // set the draw origin to the origin of the target, which is 
    // a component since we already handled the case of a canvas 
    // target; the reason we set the draw origin to the target is 
    // so that any canvas drawing events the target may perform
    // during the event are in the local component coordinates
    int comp_x, comp_y;
    getAbsolutePosition(evt->getTarget(), &comp_x, &comp_y);
    setDrawOrigin(comp_x, comp_y);

    // dispatch the event to the capturing phase listeners on
    // the canvas
    evt->setPhase(EVENT_CAPTURING_PHASE);
    m_event_target.dispatchEvent(evt);
    
    // dispatch the event to the capturing phase listeners
    // on the target's parents
    evt->setPhase(EVENT_CAPTURING_PHASE);
    std::vector<IComponentPtr>::iterator it, it_end;
    it_end = target_parents.end();
    for (it = target_parents.begin(); it != it_end; ++it)
    {
        IEventTargetPtr parent = *it;
        parent->dispatchEvent(evt);
    }
    
    // dispatch the event to the at-target phase listeners
    // on the target
    evt->setPhase(EVENT_AT_TARGET);
    target->dispatchEvent(evt);

    // dispatch the event to the bubbling phase listeners
    // on the target's parents
    evt->setPhase(EVENT_BUBBLING_PHASE);
    std::vector<IComponentPtr>::reverse_iterator itr, itr_end;
    itr_end = target_parents.rend();
    for (itr = target_parents.rbegin(); itr != itr_end; ++itr)
    {
        IEventTargetPtr parent = *itr;
        parent->dispatchEvent(evt);
    }

    // dispatch the event to the bubbling phase listeners on
    // the canvas
    evt->setPhase(EVENT_BUBBLING_PHASE);
    m_event_target.dispatchEvent(evt);

    // restore the draw origin
    setDrawOrigin(old_draw_x, old_draw_y);
}

void Canvas::onCanvasEvent(IEventPtr evt)
{
    wxString name = evt->getName();

    // mouse events
    if (name == EVENT_MOUSE_LEFT_DCLICK || 
        name == EVENT_MOUSE_LEFT_DOWN || 
        name == EVENT_MOUSE_LEFT_UP || 
        name == EVENT_MOUSE_RIGHT_DCLICK || 
        name == EVENT_MOUSE_RIGHT_DOWN || 
        name == EVENT_MOUSE_RIGHT_UP || 
        name == EVENT_MOUSE_MIDDLE_DCLICK || 
        name == EVENT_MOUSE_MIDDLE_DOWN || 
        name == EVENT_MOUSE_MIDDLE_UP || 
        name == EVENT_MOUSE_MOTION || 
        name == EVENT_MOUSE_OVER || 
        name == EVENT_MOUSE_WHEEL)
    {
        onCanvasMouseEvent(evt);
        return;
    }
    
    // key events
    if (name == EVENT_KEY_DOWN ||
        name == EVENT_KEY_UP ||
        name == EVENT_KEY)
    {
        onCanvasKeyEvent(evt);
        return;
    }

    // invalidate event
    if (name == EVENT_INVALIDATE)
    {
        onCanvasInvalidateEvent(evt);
        return;
    }
}

void Canvas::onCanvasKeyEvent(IEventPtr evt)
{
}

void Canvas::onCanvasMouseEvent(IEventPtr evt)
{
    // if the event has been handled by any of the previous 
    // event handlers, set the mouse action flag; this prevents 
    // the default handlers downstream from processing the event
    m_mouse_action = ACTION_NONE;
    if (evt->isHandled())
        m_mouse_action = ACTION_DISPATCH;

    // if the middle mouse wheel moves, scroll the view
    if (evt->getName() == EVENT_MOUSE_WHEEL)
    {
        IMouseEventPtr mouse_evt = evt;
        int wheel_move = mouse_evt->getRotation();
        if (!mouse_evt->isCtrlDown())
        {
            // if the control key isn't down, scroll the canvas
            int move_view_amount = getViewHeight()*SCROLL_MOUSE_PERC/100;
            int move = -1*wheel_move*wxMax(dtom_y(5), move_view_amount);
            scrollBy(0, move);
        }
        else
        {
            // if the control key is down, zoom the canvas
            int zoom = getZoom() - wheel_move*ZOOM_CHANGE;
            setZoom(zoom);
        }

        return;
    }

    mouseNavigateCanvas(evt);
    mouseCreateComponent(evt);
    mouseResizeComponent(evt);
    mouseMoveComponent(evt);
    mouseSelectComponent(evt);
}

void Canvas::onCanvasInvalidateEvent(IEventPtr evt)
{
    // get the component from the event
    INotifyEventPtr notify_evt = evt;
    IComponentPtr comp = evt->getTarget();

    // if we don't have a component, we're done
    if (comp.isNull())
        return;

    // if the component isn't visible, we're done
    if (!comp->isVisible())
        return;

    // don't render anything if the component isn't
    // in the canvas; we can easily determine this
    // by examining the parent member of the component:
    // if the component doesn't have a parent, it's 
    // not in the canvas
    IComponentPtr parent = getParent(comp);
    if (parent.isNull())
        return;

    // get the area to repaint
    wxRect rect;
    PropertyValue value;

    notify_evt->getProperty(EVENT_PROP_X, value);
    rect.x = value.getInteger();

    notify_evt->getProperty(EVENT_PROP_Y, value);
    rect.y = value.getInteger();

    notify_evt->getProperty(EVENT_PROP_W, value);
    rect.width = value.getInteger();

    notify_evt->getProperty(EVENT_PROP_H, value);
    rect.height = value.getInteger();

    // if the invalid width or rect is empty, we're done
    if (rect.IsEmpty())
        return;

    // translate the invalid area to absolute model coordinates
    int x, y;
    getAbsolutePosition(comp, &x, &y);
    rect.SetX(x + rect.GetX());
    rect.SetY(y + rect.GetY());

    // if the invalid area doesn't intersect the view,
    // we're done
    wxRect view_rect;
    getViewOrigin(&view_rect.x, &view_rect.y);
    getViewSize(&view_rect.width, &view_rect.height);
    
    if (!view_rect.Intersects(rect))
        return;
    
    // render the invalid area
    render(rect);
}

void Canvas::mouseNavigateCanvas(IEventPtr evt)
{
    IMouseEventPtr mouse_evt = evt;

    // if the tool isn't a movement tool, and the event isn't a mouse
    // wheel event, we aren't processing the event, so return
    if (m_tool != toolNavigate)
        return;

    if (m_mouse_action == ACTION_NONE)
    {
    }

    if (startAction(mouse_evt, ACTION_NAVIGATE))
    {
    }
    
    if (endAction(mouse_evt, ACTION_NAVIGATE))
    {
    }
    
    if (isAction(mouse_evt, ACTION_NAVIGATE))
    {
        scrollBy(-1*(m_mouse_model_x - m_mouse_model_xlast), 
                 -1*(m_mouse_model_y - m_mouse_model_ylast));
    }
}

void Canvas::mouseCreateComponent(IEventPtr evt)
{
    IMouseEventPtr mouse_evt = evt;

    // if we have don't have a create tool, we're done
    // return false
    if (m_tool != toolCreate)
        return;

    if (m_mouse_action == ACTION_NONE)
    {
    }

    if (startAction(mouse_evt, ACTION_CREATE))
    {
    }
    
    if (endAction(mouse_evt, ACTION_CREATE))
    {
    }
    
    if (isAction(mouse_evt, ACTION_CREATE))
    {
    }
}

void Canvas::mouseResizeComponent(IEventPtr evt)
{
    IMouseEventPtr mouse_evt = evt;

    // if we don't have a selection tool, we're done
    if (m_tool != toolSelect)
        return;

    // TODO: dispatch the canvas changed event

    if (startAction(mouse_evt, ACTION_RESIZE))
    {
        // if we're not on the edge of a component, reset the action
        resetAction();
        return;
    }
    
    if (endAction(mouse_evt, ACTION_RESIZE))
    {
    }
    
    if (isAction(mouse_evt, ACTION_RESIZE))
    {
    }
}

void Canvas::mouseMoveComponent(IEventPtr evt)
{
    // TODO: need to figure out how to do drag
    // and drop now that canvas no longer derives
    // from wxControl
/*
    IMouseEventPtr mouse_evt = evt;

    // if we don't have a selection tool, we're done
    if (m_tool != toolSelect)
        return;

    // if there aren't any selections, we're done
    if (!hasSelections())
        return;

    // if the target isn't selected, we're done        
    IComponentPtr comp = evt->getTarget();
    if (!isSelected(comp))
        return;

    if (m_mouse_action == ACTION_NONE)
    {
    }

    if (startAction(mouse_evt, ACTION_MOVE))
    {
        // TODO: for now, turn off component moving
        resetAction();
        return;
    
        // get the action components
        std::vector<IComponentPtr> action_components;
        getSelections(action_components);

        // create a component to package up the action components
        ComponentDataObject drag_object;
        IComponentPtr drag_component = Component::create();
        
        // iterate through the action components and add them to 
        // the drop component
        std::vector<IComponentPtr>::iterator it, it_end;
        it_end = action_components.end();

        for (it = action_components.begin(); it != it_end; ++it)
        {
            drag_component->add(*it);
        }

        // set the drag object component
        drag_object.SetComponent(drag_component);

        // set the drag & drop data
        wxDropSource source(this);
        source.SetData(drag_object);
        source.DoDragDrop(true);

        // reset the action state; set the mouse-up state manually
        resetAction();
    }
*/
}

void Canvas::mouseSelectComponent(IEventPtr evt)
{
    // if we don't have a selection tool, we're done
    if (m_tool != toolSelect)
        return;

    if (m_mouse_action == ACTION_NONE)
    {
    }

    // handle case of single click
    if (evt->getName() == EVENT_MOUSE_LEFT_DOWN ||
        evt->getName() == EVENT_MOUSE_RIGHT_DOWN ||
        evt->getName() == EVENT_MOUSE_MIDDLE_DOWN)
    {
        // if the component we're clicking on is not
        // selected, and the control isn't down, remove
        // previous selections
        IMouseEventPtr mouse_evt = evt;
        if (!isSelected(evt->getTarget()) && 
            !mouse_evt->isCtrlDown())
        {
            invalidateSelection();
            removeAllSelections();
        }

        // try to select the component; if we're successful, 
        // invalidate the component
        if (select(evt->getTarget()))
            invalidateSelection();
    }

    // handle case of selection areas
    if (startAction(evt, ACTION_SELECT))
    {
    }

    if (endAction(evt, ACTION_SELECT))
    {
    }

    if (isAction(evt, ACTION_SELECT))
    {
    }
}


bool Canvas::startAction(IEventPtr evt, wxString action)
{
    // if the event is a left-mouse button down and we
    // don't already have an action, set the action and 
    // return true
    if (evt->getName() == EVENT_MOUSE_LEFT_DOWN &&
        m_mouse_action == ACTION_NONE)
    {
        // set the mouse action
        m_mouse_action = action;

        // set the mouse start position        
        m_mouse_model_xstart = m_mouse_model_x;
        m_mouse_model_ystart = m_mouse_model_y;

        return true;
    }
    
    // otherwise, return false
    return false;
}

bool Canvas::endAction(IEventPtr evt, wxString action)
{
    // if the event is a left-mouse button up and we
    // already have an action that matches the input 
    // action, reset the action and return true
    if (evt->getName() == EVENT_MOUSE_LEFT_UP &&
        m_mouse_action == action)
    {
        m_mouse_action = ACTION_NONE;
        return true;
    }
    
    // otherwise, return false
    return false;
}

bool Canvas::isAction(IEventPtr evt, wxString action)
{
    // if the event is a mouse move event and we
    // already have an action that matches the input 
    // action, return true
    if (evt->getName() == EVENT_MOUSE_MOTION &&
        m_mouse_action == action)
    {
        return true;
    }

    // otherwise, return false
    return false;
}

void Canvas::resetAction()
{
    // reset the mouse action state; this is used to bail out
    // of an action if some condition in the start action isn't 
    // true, so other handlers have a chance to process the event
    m_mouse_action = ACTION_NONE;
}


} // namespace kcanvas

