/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2003-08-21
 *
 */


#ifndef H_KCANVAS_COMPONENT_H
#define H_KCANVAS_COMPONENT_H


#include "property.h"
#include "event.h"


namespace kcanvas
{


// Canvas Base Component Classes

class Canvas;

class Component : public IComponent,
                  public IEventTarget,
                  public xcm::signal_sink
{
friend class Canvas;

    XCM_CLASS_NAME("kcanvas.Component")
    XCM_BEGIN_INTERFACE_MAP(Component)
        XCM_INTERFACE_ENTRY(IComponent)
        XCM_INTERFACE_ENTRY(IEventTarget)
    XCM_END_INTERFACE_MAP()

public:

    Component();
    virtual ~Component();

    static IComponentPtr create();
    static void initProperties(Properties& properties);
 
     // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);

    wxString getId();
    wxString getType();

    bool add(IComponentPtr component);
    bool remove(IComponentPtr component);
    void removeAll();
    void getComponents(std::vector<IComponentPtr>& components) const;

    void addProperty(const wxString& prop_name, const PropertyValue& value);
    void addProperties(const Properties& properties);
    void removeProperty(const wxString& prop_name);

    bool setProperty(const wxString& prop_name, const PropertyValue& value);
    bool setProperties(const Properties& properties);
    bool getProperty(const wxString& prop_name, PropertyValue& value) const;
    bool getProperties(Properties& properties) const;
    Properties& getPropertiesRef();

    void setOrigin(int x, int y);
    void getOrigin(int* x, int* y) const;

    void setSize(int w, int h);
    void getSize(int* w, int* h) const;

    void setWidth(int width);
    int getWidth() const;

    void setHeight(int height);
    int getHeight() const;

    void visible(bool visible);
    bool isVisible() const;

    bool contains(int x, int y);
    bool intersects(const wxRect& rect);
    void extends(wxRect& rect);
    void render(const wxRect& rect = wxRect());

    // IEventTarget
    bool addEventHandler(const wxString& name,
                         IEventHandlerPtr handler,
                         bool capture_phase = false);
    bool removeEventHandler(const wxString& name,
                            IEventHandlerPtr handler,
                            bool capture_phase = false);
    void removeAllEventHandlers();
    void dispatchEvent(IEventPtr evt);

public:

    void setCanvas(ICanvasPtr canvas);
    ICanvasPtr getCanvas();

    void setParent(IComponentPtr component);
    IComponentPtr getParent();

protected:

    // function for firing an event that signals that a portion
    // of the component needs to be redrawn
    void invalidate(const wxRect& rect = wxRect());

    // function for adding event handler
    template<class T>
    void addEventHandler(const wxString& name,
                         void (T::*func)(IEventPtr),
                         bool capture_phase = false)
    {
        T* t = (T*)this;
        EventHandler* handler = new EventHandler;
        handler->sigEvent().connect(t, func);
        addEventHandler(name, handler, capture_phase);
    }

private:

    // stock render event handler
    void onRender(IEventPtr);

protected:

    // child components
    std::vector<IComponentPtr> m_components;

    // properties
    Properties m_properties;
    
private:

    // m_canvas: pointer to canvas, set to the canvas to which the component
    // belongs if the component is in the hierarchy of components contained
    // by the canvas; set to NULL otherwise

    // note: we use Canvas* instead of ICanvasPtr to avoid circular references,
    // so that both the canvas and the component can destroy properly
    Canvas* m_canvas;

    // m_parent: pointer to component's immediate parent; set to the immediate 
    // parent if the component is in the hierarchy of components contained in 
    // the canvas and NULL otherwise;
    
    // note: a component can be contained by multiple components, but can only
    // have one parent that's in the hierarchy of components belonging to the 
    // canvas; components in the canvas hierarchy need to have this parent 
    // relationship so functions like focus know which component in the layout 
    // is being referred to without having to resort to additional parameters 
    // such as position to distinguish between the same component being in two 
    // locations; however, it's convenient to use components as containers for 
    // other components for use-cases such as undo/redo, where it's necessary 
    // to store histories of hierarchies;

    // note: we use Component* instead of IComponentPtr so that the chain of
    // components can destroy properly; if we use IComponentPtr for the parent, 
    // the parent won't destroy until the child destroys, which is dependent on 
    // the parent being destroyed, creating a locking, circular reference
    Component* m_parent;

    // event handlers
    EventTarget m_event_target;

    // m_absolute_x, m_absolute_y, m_absolute_w, m_absolute_h: the canvas uses
    // the following variables when laying out the components; when the canvas 
    // is determining the layout, it dispatches a layout event to each component
    // in the hierarchy of sub-components; after it dispatches each event, it 
    // gets the position and size of the component and stores these in absolute 
    // model coordinates so it can determine if the layout subsequently changes 
    // in response to other layout events; when the absolute position and size 
    // of the whole hiearchy of components stays fixed after iterating through 
    // all the sub-components, the layout is complete
    int m_absolute_x;
    int m_absolute_y;
    int m_absolute_w;
    int m_absolute_h;

    // unique component id; generated first time getId()
    // function is called
    wxString m_id;
};


}; // namespace kcanvas


#endif

