/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-11
 *
 */


#ifndef H_KCANVAS_COMPONENTGRAPH_H
#define H_KCANVAS_COMPONENTGRAPH_H


namespace kcanvas
{


// Canvas Graph Component Classes

class CompGraph : public Component
{
    XCM_CLASS_NAME("kcanvas.CompGraph")
    XCM_BEGIN_INTERFACE_MAP(CompGraph)
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompGraph();
    virtual ~CompGraph();

    static IComponentPtr create();
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    void render(const wxRect& rect = wxRect());
};


} // namespace kcanvas


#endif

