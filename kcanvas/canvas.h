/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2002-01-25
 *
 */


#ifndef H_KCANVAS_CANVAS_H
#define H_KCANVAS_CANVAS_H


#include "event.h"


namespace kcanvas
{


// Canvas Base Classes

class Canvas : public ICanvas,
               public IEventTarget,
               public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("kcanvas.Canvas")
    XCM_BEGIN_INTERFACE_MAP(Canvas)
        XCM_INTERFACE_ENTRY(ICanvas)
        XCM_INTERFACE_ENTRY(IEventTarget)
    XCM_END_INTERFACE_MAP()

public:

    Canvas();
    virtual ~Canvas();

    static ICanvasPtr create();
    void destroy();

    // canvas clone/copy functions
    ICanvasPtr clone();
    void copy(ICanvasPtr canvas);

    // scale and conversion functions
    bool setScale(int x, int y);
    void getScale(int* x, int* y);
    
    int mtod_x(int x) const;
    int mtod_y(int y) const;
    int dtom_x(int x) const;
    int dtom_y(int y) const;

    void clientToModel(int x_client, int y_client,
                       int* x_model, int* y_model) const;
                       
    void modelToClient(int x_model, int y_model,
                       int* x_client, int* y_client) const;

    // canvas model size
    void setSize(int x, int y);
    void getSize(int* x, int* y) const;

    int getWidth() const;
    int getHeight() const;

    // canvas view functions    
    void setViewOrigin(int x, int y);
    void getViewOrigin(int* x, int* y) const;

    void setViewSize(int x, int y);
    void getViewSize(int* x, int* y) const;

    int getViewWidth() const;
    int getViewHeight() const;

    bool scrollTo(int x, int y);
    bool scrollBy(int x_diff, int y_diff);
    
    void setZoom(int percent);
    int getZoom() const;

    // canvas model component
    bool setModelComponent(IComponentPtr component);
    IComponentPtr getModelComponent();

    // canvas component add and remove functions
    bool add(IComponentPtr component);
    bool remove(IComponentPtr component);
    void removeAll();
    void getComponents(std::vector<IComponentPtr>& components) const;

    // canvas properties
    void addProperty(const wxString& prop_name, const PropertyValue& value);
    void addProperties(const Properties& properties);
    void removeProperty(const wxString& prop_name);

    bool setProperty(const wxString& prop_name, const PropertyValue& value);
    bool setProperties(const Properties& properties);
    bool getProperty(const wxString& prop_name, PropertyValue& value) const;
    bool getProperties(Properties& properties) const;
    Properties& getPropertiesRef();

    // canvas component location functions
    IComponentPtr getComponentById(const wxString& id);
    IComponentPtr getComponentByPos(int x, int y);
    void getComponentsByPos(std::vector<IComponentPtr>& components, int x, int y);
    void getComponentsByRect(std::vector<IComponentPtr>& components, const wxRect& rect = wxRect());
    void getComponentsByType(std::vector<IComponentPtr>& components, const wxString& type);
    void getComponentPos(IComponentPtr component, int* x, int* y);

    // selection functions
    bool select(IComponentPtr component);
    bool removeSelection(IComponentPtr component);
    void removeAllSelections();

    void getSelections(std::vector<IComponentPtr>& components) const;
    bool isSelected(IComponentPtr component) const;
    bool hasSelections() const;

    // set/get focus functions
    bool setFocus(IComponentPtr component);
    IComponentPtr getFocus() const;
    bool hasFocus(IComponentPtr component) const;

    // functions for performing the layout and rendering
    // as well as determining whether or not we're printing
    void layout();
    void render(const wxRect& rect = wxRect());
    bool isPrinting() const;

    // drawing origin functions
    bool setDrawOrigin(int x, int y);
    void getDrawOrigin(int* x, int* y) const;
    void moveDrawOrigin(int x, int y);

    // draw related functions
    bool setPen(const Pen& pen);
    bool setPen(const Properties& properties);

    bool setBrush(const Brush& brush);
    bool setBrush(const Properties& properties);

    bool setFont(const Font& font);
    bool setTextBackground(const Color& color);
    bool setTextForeground(const Color& color);

    void drawImage(const wxImage& image,
                    int x, int y, int w, int h);

    void drawCheckBox(int x, int y, int w, int h, bool checked = true);
    void drawCircle(int x, int y, int r);
    void drawEllipse(int x, int y, int w, int h);
    void drawLine(int x1, int  y1, int x2, int y2);
    void drawPoint(int x, int y);
    void drawPolygon(int n, wxPoint points[], int x = 0, int y = 0);
    void drawGradientFill(const wxRect& rect,
                          const Color& start_color,
                          const Color& end_color,
                          const wxDirection& direction = wxSOUTH);
    void drawRectangle(int x, int y, int w, int h);
    void drawText(const wxString& text, int x, int y);
    
    bool getTextExtent(const wxString& text,
                       int* x,
                       int* y,
                       int* descent = NULL,
                       int* external = NULL) const;
                       
    bool getPartialTextExtents(const wxString& text,
                               wxArrayInt& widths) const;
                               
    int getCharHeight() const;
    int getCharWidth() const;

    void addClippingRectangle(int x, int y, int w, int h);
    void removeClippingRectangle();
    void removeAllClippingRectangles();

    // IEventTarget interface
    bool addEventHandler(const wxString& name,
                         IEventHandlerPtr handler,
                         bool capture_phase = false);
    bool removeEventHandler(const wxString& name,
                            IEventHandlerPtr handler,
                            bool capture_phase = false);
    void removeAllEventHandlers();
    void dispatchEvent(IEventPtr evt);    

protected:

    // helper function for dispatching a canvas changed event
    void dispatchCanvasChangedEvent();

    // utility functions for getting parent/child components
    void getParentComponents(std::vector<IComponentPtr>& components,
                             IComponentPtr child);
    void getChildComponents(std::vector<IComponentPtr>& components,
                            IComponentPtr parent);

    // finds a particular component type that's nearest to a point
    // utility functions for getting the absolute position and size
    // of a component and finding them in the canvas
    void getAbsolutePosition(IComponentPtr component, int* x, int* y);
    void getAbsoluteSize(IComponentPtr component, int* w, int* h);
    IComponentPtr getParent(IComponentPtr component);
    IComponentPtr findComponentByPos(const wxString& type, int* x, int* y);

    // viewport function (in model coordinates)
    bool moveView(int x, int y);

    // helper function for invalidating components
    void invalidate(IComponentPtr comp);
    void invalidateSelection();

    // cache functions for clearing/populating the list of
    // canvas components
    bool isCacheEmpty() const;
    void populateCache();
    void clearCache();
    void clearRenderCache();

private:

    // primary event handler
    void preProcessEvent(std::vector<IComponentPtr>& target_parents, IEventPtr evt);    
    void handleEvent(IEventPtr evt);

    // default canvas event handlers
    void onCanvasEvent(IEventPtr evt);
    void onCanvasKeyEvent(IEventPtr evt);
    void onCanvasMouseEvent(IEventPtr evt);
    void onCanvasInvalidateEvent(IEventPtr evt);

    // mouse event handler helpers
    void mouseNavigateCanvas(IEventPtr evt);
    void mouseCreateComponent(IEventPtr evt);
    void mouseResizeComponent(IEventPtr evt);
    void mouseMoveComponent(IEventPtr evt);
    void mouseSelectComponent(IEventPtr evt);

    bool startAction(IEventPtr evt, wxString action);
    bool endAction(IEventPtr evt, wxString action);
    bool isAction(IEventPtr evt, wxString action);
    void resetAction();

protected:

    // properties
    Properties m_properties;

    // draw origin
    int m_draw_origin_x;
    int m_draw_origin_y;

private:

    // event queue for posted events
    std::vector<IEventPtr> m_posted_events;

    // canvas components; list of all components in the canvas
    std::vector<IComponentPtr> m_canvas_components;

    // viewable components; list of visible components in the canvas
    std::vector<IComponentPtr> m_viewable_components;

    // TODO: selected components and action components need to clear
    // out when components are added/removed from the canvas hierarchy

    // selected components; list of selected components in the canvas
    std::vector<IComponentPtr> m_selected_components;

    // focus component; this is the component that will receive
    // keyboard events
    IComponentPtr m_focus_component;

    // model component; top level component that contains other
    // components in the canvas
    IComponentPtr m_model_component;

    // event target to manage event handlers
    EventTarget m_event_target;
    
    // initial event in a sequence of dispatched events; used for
    // making sure posted events aren't processed until after all
    // synchronous events are handled
    IEventPtr m_initial_event;

    // scale and zoom variables
    int m_scale_x;
    int m_scale_y;
    int m_zoom_percent;

    // view size
    int m_view_x;
    int m_view_y;
    
    // mouse event variables
    IComponentPtr m_mouse_target;   // the current mouse target
    wxString m_mouse_action;        // name of current mouse action
    int m_mouse_model_xstart;       // starting x pos of the mouse in screen coordinates
    int m_mouse_model_ystart;       // starting y pos of the mosue in screen coordinates
    int m_mouse_model_xlast;        // last x pos of the mouse in model coordinates
    int m_mouse_model_ylast;        // last y pos of the mouse in model coordinates
    int m_mouse_model_x;            // x pos of the mouse in model coordinates
    int m_mouse_model_y;            // y pos of the mouse in model coordinates
    int m_mouse_client_x;           // x pos of the mouse in client coordinates
    int m_mouse_client_y;           // y pos of the mouse in client coordinates
    bool m_mouse_capture;           // mouse capture flag

    // tool id
    int m_tool;    
};


} // namespace kcanvas


#endif

